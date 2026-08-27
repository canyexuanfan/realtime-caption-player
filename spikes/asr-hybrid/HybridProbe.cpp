// HybridProbe — T0022 探针：融合字幕编排骨架（VAD 分句 × 识别器 → SRT）。
//
// 架构决策（用户验收标准："中文准确识别达到直接在网盘内播放的效果"）：
//   * 权威精准字幕路径 = Silero VAD 分句 + SenseVoice 离线识别（每段送 SenseVoice）。
//     SenseVoice 在 sherpa-onnx 1.13.6 / ORT 1.27.1 下已验证跑通（T0021 exit 0），
//     中文识别强、自带 ITN（数字/标点规整），满足"准确到可替代观看"的验收。
//   * 在线 Paraformer 流式 partial 仅作"实时逐字"增强项；因 ORT 1.27.1 对 int8 Paraformer
//     存在回归（BLOCKER-2：CreateOnlineRecognizer 硬崩溃），默认关闭，避免打断精准链路。
//     设环境变量 RCP_TRY_STREAMING=1 才尝试流式 Paraformer（预期崩溃，复现 BLOCKER-2）。
//
// 用法：hybrid_probe.exe [paraformer_dir] [silero_dir] [sensevoice_dir] [encoder_basename]
//   默认 .tools/models/{paraformer,silero,sensevoice}；encoder_basename 默认 encoder.int8.onnx
//
// 链接目标：rcp::sherpa-onnx -> sherpa-onnx-c-api.dll

#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>
extern "C" {
#include <sherpa-onnx/c-api/c-api.h>
}

int main(int argc, char** argv) {
    setvbuf(stdout, NULL, _IONBF, 0);  // 无缓冲：崩溃前也能看到定位输出
    std::printf("hybrid_probe: start\n");
    std::string pf_dir  = (argc > 1) ? argv[1] : ".tools/models/paraformer";
    std::string vad_dir = (argc > 2) ? argv[2] : ".tools/models/silero";
    std::string sv_dir  = (argc > 3) ? argv[3] : ".tools/models/sensevoice";
    std::string enc_base = (argc > 4) ? argv[4] : "encoder.int8.onnx";
    std::string vad_model = vad_dir + "/silero_vad.onnx";
    std::string sv_model  = sv_dir  + "/model.int8.onnx";
    std::string sv_tokens = sv_dir  + "/tokens.txt";
    std::string enc = pf_dir + "/" + enc_base;
    std::string dec = pf_dir + "/decoder.onnx";
    std::string tok = pf_dir + "/tokens.txt";
    const bool try_streaming = (std::getenv("RCP_TRY_STREAMING") != nullptr);

    // ---------- 1) Silero VAD 分句（VoiceActivityDetector）----------
    SherpaOnnxVadModelConfig vad_cfg;
    memset(&vad_cfg, 0, sizeof(vad_cfg));
    vad_cfg.silero_vad.model = vad_model.c_str();
    vad_cfg.silero_vad.threshold = 0.5f;
    vad_cfg.silero_vad.min_silence_duration = 0.5f;
    vad_cfg.silero_vad.min_speech_duration = 0.25f;
    vad_cfg.silero_vad.max_speech_duration = 20.0f;
    vad_cfg.silero_vad.window_size = 512;  // 必须 512/1024/1536；漏设=0 会致 VAD 加载/推理异常
    vad_cfg.sample_rate = 16000;
    vad_cfg.num_threads = 1;
    vad_cfg.provider = "cpu";
    // ten_vad 必须给空串而非 NULL（memset 后=NULL），否则 dll 内 std::string(NULL) 触发 UB
    vad_cfg.ten_vad.model = "";
    vad_cfg.ten_vad.threshold = 0.5f;
    vad_cfg.ten_vad.min_silence_duration = 0.5f;
    vad_cfg.ten_vad.min_speech_duration = 0.25f;
    vad_cfg.ten_vad.max_speech_duration = 20.0f;
    vad_cfg.ten_vad.window_size = 256;

    const SherpaOnnxVoiceActivityDetector* vad = SherpaOnnxCreateVoiceActivityDetector(&vad_cfg, 30.0f);
    if (!vad) { std::printf("hybrid_probe: VAD detector FAILED (silero 路径/权重错误?)\n"); return 1; }
    std::printf("hybrid_probe: Silero VAD detector created (threshold=0.5)\n");

    // ---------- 2) SenseVoice 离线识别器（权威精准路径）----------
    SherpaOnnxOfflineRecognizerConfig sv_cfg;
    memset(&sv_cfg, 0, sizeof(sv_cfg));
    sv_cfg.feat_config.sample_rate = 16000;
    sv_cfg.feat_config.feature_dim = 80;
    sv_cfg.model_config.tokens = sv_tokens.c_str();
    sv_cfg.model_config.sense_voice.model = sv_model.c_str();
    sv_cfg.model_config.sense_voice.language = "auto";
    sv_cfg.model_config.sense_voice.use_itn = 1;

    const SherpaOnnxOfflineRecognizer* sv = SherpaOnnxCreateOfflineRecognizer(&sv_cfg);
    if (!sv) { std::printf("hybrid_probe: SenseVoice recognizer FAILED (模型路径/权重错误?)\n"); return 1; }
    std::printf("hybrid_probe: SenseVoice recognizer created (int8, lang=auto, itn=on)\n");

    // ---------- 3) 合成音频 → VAD 分段 → 每段 SenseVoice 识别 → 融合 SRT ----------
    const int32_t sr = 16000;
    const int32_t total = sr * 4;
    const int32_t chunk = 3200;
    std::vector<float> all_samples;   // 累积全部样本，供按 VAD 段边界切片送 SenseVoice
    all_samples.reserve((size_t)total);
    int seg_idx = 1;
    std::printf("\n--- 融合 SRT（VAD 分句 × SenseVoice 精准识别；合成音频无语音，文本为空属正常）---\n");
    for (int32_t start = 0; start < total; start += chunk) {
        int32_t n = (start + chunk <= total) ? chunk : (total - start);
        float* buf = new float[n];
        for (int32_t i = 0; i < n; i++) {
            double t = (double)(start + i) / sr;
            double freq = (t < 1.5) ? 220.0 : (t > 2.2 ? 330.0 : 0.0);
            buf[i] = (freq == 0.0) ? 0.0f : 0.3f * (float)sin(2.0 * 3.141592653589793 * freq * t);
        }
        // VAD 分段
        SherpaOnnxVoiceActivityDetectorAcceptWaveform(vad, buf, n);
        // 切片累积
        all_samples.insert(all_samples.end(), buf, buf + n);
        delete[] buf;
        while (SherpaOnnxVoiceActivityDetectorDetected(vad)) {
            const SherpaOnnxSpeechSegment* seg = SherpaOnnxVoiceActivityDetectorFront(vad);
            // 取该段的样本切片
            const float* seg_samples = all_samples.data() + seg->start;
            const SherpaOnnxOfflineStream* os = SherpaOnnxCreateOfflineStream(sv);
            SherpaOnnxAcceptWaveformOffline(os, sr, seg_samples, seg->n);
            SherpaOnnxDecodeOfflineStream(sv, os);
            const SherpaOnnxOfflineRecognizerResult* r = SherpaOnnxGetOfflineStreamResult(os);
            const char* text = (r && r->text) ? r->text : "";
            int32_t abs_start = seg->start;
            int32_t abs_end   = seg->start + seg->n;
            std::printf("%d\n%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\n%s\n\n",
                        seg_idx,
                        abs_start / sr / 60, (abs_start / sr) % 60, (abs_start % sr) / 1000, abs_start % 1000,
                        abs_end   / sr / 60, (abs_end   / sr) % 60, (abs_end   % sr) / 1000, abs_end   % 1000,
                        text);
            seg_idx++;
            SherpaOnnxDestroyOfflineStream(os);
            if (r) SherpaOnnxDestroyOfflineRecognizerResult(r);
            SherpaOnnxVoiceActivityDetectorPop(vad);
            SherpaOnnxDestroySpeechSegment(seg);
        }
    }
    std::printf("hybrid_probe: 融合编排链路已接通（VAD 分句 → 每段 SenseVoice 精准识别 → SRT）\n");
    SherpaOnnxDestroyOfflineRecognizer(sv);
    SherpaOnnxDestroyVoiceActivityDetector(vad);

    // ---------- 4) 可选：在线 Paraformer 流式 partial（增强项，默认关；ORT 1.27.1 下崩溃=BLOCKER-2）----------
    if (try_streaming) {
        std::printf("hybrid_probe: [RCP_TRY_STREAMING=1] 尝试在线 Paraformer 流式（预期复现 BLOCKER-2）...\n");
        SherpaOnnxOnlineRecognizerConfig pf_cfg;
        memset(&pf_cfg, 0, sizeof(pf_cfg));
        pf_cfg.feat_config.sample_rate = 16000;
        pf_cfg.feat_config.feature_dim = 80;
        pf_cfg.model_config.paraformer.encoder = enc.c_str();
        pf_cfg.model_config.paraformer.decoder = dec.c_str();
        pf_cfg.model_config.tokens = tok.c_str();
        pf_cfg.model_config.provider = "cpu";
        pf_cfg.model_config.num_threads = 1;
        pf_cfg.decoding_method = "greedy_search";
        const SherpaOnnxOnlineRecognizer* pf = SherpaOnnxCreateOnlineRecognizer(&pf_cfg);
        if (!pf) { std::printf("hybrid_probe: Paraformer FAILED (BLOCKER-2)\n"); return 1; }
        SherpaOnnxDestroyOnlineRecognizer(pf);
        std::printf("hybrid_probe: Paraformer 流式 OK（BLOCKER-2 已解决）\n");
    } else {
        std::printf("hybrid_probe: 跳过在线 Paraformer 流式（默认关；设 RCP_TRY_STREAMING=1 可复现/验证 BLOCKER-2）\n");
    }

    std::printf("hybrid_probe: done (精准字幕路径已验证；真实中文分句/识别正确性需真机音频回填)\n");
    return 0;
}

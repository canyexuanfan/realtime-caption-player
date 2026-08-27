// tools/deps_smoke/main.cpp
// 依赖可链接冒烟：验证 FFmpeg / libmpv / sherpa-onnx 三套头文件可见、库可链接。
// 不实际播放或推理（运行需 GUI/音频设备，沙箱无法执行）；链接通过即证明依赖就位。
#include <cstdio>

// 这些是 C 库头文件。在 C++ 翻译单元里必须包在 extern "C" 中，
// 否则符号会被 C++ 名字修饰，链接时报 LNK2019(找不到 mpv_*/avformat_* 等)。
extern "C" {
#include <mpv/client.h>
#include <libavformat/avformat.h>
#include <sherpa-onnx/c-api/c-api.h>
}

int main() {
    // libmpv：创建句柄（不销毁，仅验证符号可链接）
    mpv_handle* ctx = mpv_create();
    (void)ctx;

    // FFmpeg：取版本号，验证 libavformat 符号
    (void)avformat_version();

    // sherpa-onnx：调用真实导出的 C API 函数（无需模型），强制链接器
    // 拉入 sherpa-onnx-c-api.dll 导入库，验证符号可链接。
    const char* sherpa_ver = SherpaOnnxGetVersionStr();
    (void)sherpa_ver;

    SherpaOnnxOnlineRecognizerConfig cfg{};
    (void)cfg;

    std::printf("deps_smoke: link OK (mpv+ffmpeg+sherpa-onnx)\n");
    return 0;
}

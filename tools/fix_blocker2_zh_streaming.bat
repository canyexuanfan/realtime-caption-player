@echo off
chcp 65001 >nul
REM =====================================================================
REM  BLOCKER-2 一键修复：把实时逐字流式引擎换成 Zipformer2-CTC（ORT-1.27 兼容）
REM  原 2023-02 双语 Paraformer 模型图与 ORT 流式路径不兼容（已确认降版/换fp32均无效），
REM  改用现代 Zipformer2-CTC 中文流式模型，与 SenseVoice（精准终稿+ITN）双引擎互补。
REM  用法：在仓库根目录双击本文件（需已装 VS2022 BuildTools + 本仓库 venv 的 cmake/ninja）
REM =====================================================================
setlocal
cd /d "%~dp0"

set REPO=%CD%
set MODEL_DIR=%REPO%\.tools\models\zipformer-ctc
set BUILD=%REPO%\out\build\spikes2
set SHERPA_BIN=%REPO%\.tools\sherpa-onnx\bin
set SHERPA_LIB=%REPO%\.tools\sherpa-onnx\lib

REM ---- 1) 下载现代中文流式模型（Zipformer2-CTC，单文件 model.int8.onnx + tokens.txt）----
REM 生产级（推荐，质量高）：zipformer-ctc-zh-int8-2025-06-30
REM 备选小体积验证：zipformer-ctc-small-2024-03-18
set MODEL_TAR=sherpa-onnx-streaming-zipformer-ctc-zh-int8-2025-06-30.tar.bz2
set MODEL_URL=https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/%MODEL_TAR%
if not exist "%MODEL_DIR%" mkdir "%MODEL_DIR%"
echo [1/5] 下载 %MODEL_TAR% （-k 绕过代理证书，-C 断点续传）...
curl -kL -C - --max-time 600 -o "%MODEL_DIR%\%MODEL_TAR%" "%MODEL_URL%"
if errorlevel 1 ( echo 下载失败，请检查网络/代理后重试 & pause & exit /b 1 )

echo [2/5] 解压模型...
tar xjf "%MODEL_DIR%\%MODEL_TAR%" -C "%MODEL_DIR%"
if errorlevel 1 ( echo 解压失败 & pause & exit /b 1 )
REM 找到解压出的目录，把 model.int8.onnx / tokens.txt 提到 zipformer-ctc 根（探针默认读 model_basename=model.int8.onnx）
for /d %%d in ("%MODEL_DIR%\sherpa-onnx-streaming-zipformer-ctc-zh-int8-2025-06-30") do (
  if exist "%%d\model.int8.onnx" copy /Y "%%d\model.int8.onnx" "%MODEL_DIR%\model.int8.onnx" >nul
  if exist "%%d\tokens.txt"      copy /Y "%%d\tokens.txt"      "%MODEL_DIR%\tokens.txt" >nul
)

REM ---- 3) 把 bundled 4 个 dll 拷到探针 exe 旁（避免系统过时 ORT 1.10 被错误加载；BLOCKER-1 修复）----
echo [3/5] 拷贝 bundled dll 到探针构建目录...
if exist "%SHERPA_LIB%\sherpa-onnx-c-api.dll" (
  copy /Y "%SHERPA_LIB%\sherpa-onnx-c-api.dll"        "%BUILD%\spikes\asr-hybrid\" >nul
  copy /Y "%SHERPA_LIB%\sherpa-onnx-cxx-api.dll"      "%BUILD%\spikes\asr-hybrid\" >nul
  copy /Y "%SHERPA_LIB%\onnxruntime.dll"              "%BUILD%\spikes\asr-hybrid\" >nul
  copy /Y "%SHERPA_LIB%\onnxruntime_providers_shared.dll" "%BUILD%\spikes\asr-hybrid\" >nul
) else (
  echo 警告：未找到 %SHERPA_LIB% 下的 dll，探针可能仍会加载系统过时 ORT
)

REM ---- 4) 编译三个探针（Ninja generator，MSVC）----
echo [4/5] 编译探针（online/sensevoice/hybrid）...
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
set PATH=%PATH%;%REPO%\.venv\Scripts;%REPO%\.tools\python\envs\default\Scripts
where ninja >nul 2>&1 || set PATH=%PATH%;C:\Users\<你的用户名>\.workbuddy\binaries\python\envs\default\Scripts
ninja -C "%BUILD%" online_probe sensevoice_probe hybrid_probe
if errorlevel 1 ( echo 编译失败 & pause & exit /b 1 )

REM ---- 5) 真机验证 ----
echo [5/5] 运行验证（合成音频仅验证管线；真实中文需真机音频）...
echo ===== ONLINE (Zipformer2-CTC 实时逐字) =====
"%BUILD%\spikes\asr-hybrid\online_probe.exe" .tools/models/zipformer-ctc
echo ===== HYBRID (实时逐字 + SenseVoice 精准终稿) =====
"%BUILD%\spikes\asr-hybrid\hybrid_probe.exe" .tools/models/zipformer-ctc .tools/models/silero .tools/models/sensevoice
echo.
echo 若 online_probe/hybrid_probe 不再崩溃并打印 recognizer created / [partial]，则 BLOCKER-2 已修复。
pause

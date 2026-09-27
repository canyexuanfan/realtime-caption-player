# tools/gen_seek_e2e_media.ps1 - generate long test media with speech at SeekSec
# for the caption pipeline seek E2E (phase B acceptance).
# Output: out/e2e-verify/seek-e2e.mp4 (silence [0,SeekSec) + speech ~10s + tail 5s).
param(
    [string]$RepoRoot = (Split-Path -Parent $PSScriptRoot),
    [int]$SeekSec = 1800
)
$ErrorActionPreference = 'Continue'
$OutDir = Join-Path $RepoRoot 'out\e2e-verify'
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$Ffmpeg = Join-Path $RepoRoot '.tools\ffmpeg\bin\ffmpeg.exe'

Add-Type -AssemblyName System.Speech
$s = New-Object System.Speech.Synthesis.SpeechSynthesizer
$zh = $s.GetInstalledVoices() | Where-Object { $_.VoiceInfo.Culture.Name -like 'zh*' } | Select-Object -First 1
if (-not $zh) { 'NO_ZH_VOICE' | Out-File -Encoding utf8 (Join-Path $OutDir 'seek-e2e-log.txt'); exit 2 }
$s.SelectVoice($zh.VoiceInfo.Name)
$s.Rate = 0

$speechWav = Join-Path $OutDir 'seek_speech.wav'
$s.SetOutputToWaveFile($speechWav)
$s.Speak(('在第{0}分钟的位置，我们验证字幕能够跟随播放头定位。实时语音识别一切正常。' -f [int]($SeekSec / 60)))
$s.SetOutputToNull()
$s.Dispose()

$silence = Join-Path $OutDir 'seek_silence.wav'
& $Ffmpeg -y -loglevel error -f lavfi -i 'anullsrc=r=16000:cl=mono' -t $SeekSec $silence
if ($LASTEXITCODE -ne 0) { "FFMPEG-SILENCE-FAILED" | Out-File -Encoding utf8 (Join-Path $OutDir 'seek-e2e-log.txt'); exit 3 }
$tail = Join-Path $OutDir 'seek_tail.wav'
& $Ffmpeg -y -loglevel error -f lavfi -i 'anullsrc=r=16000:cl=mono' -t 5 $tail
if ($LASTEXITCODE -ne 0) { "FFMPEG-TAIL-FAILED" | Out-File -Encoding utf8 (Join-Path $OutDir 'seek-e2e-log.txt'); exit 4 }

$outMp4 = Join-Path $OutDir 'seek-e2e.mp4'
& $Ffmpeg -y -loglevel error -i $silence -i $speechWav -i $tail `
    -filter_complex '[0:a][1:a][2:a]concat=n=3:v=0:a=1,aformat=sample_rates=16000:channel_layouts=mono' `
    -c:a aac -b:a 64k $outMp4
if ($LASTEXITCODE -ne 0) { "FFMPEG-CONCAT-FAILED" | Out-File -Encoding utf8 (Join-Path $OutDir 'seek-e2e-log.txt'); exit 5 }

('DONE spoken=30min-speech') | Out-File -Encoding utf8 (Join-Path $OutDir 'seek-e2e-log.txt')
Write-Output ('DONE ' + $outMp4)

# Sintetiza frases con las voces neuronales/OneCore de Windows (WinRT) a WAV.
# Uso: powershell -ExecutionPolicy Bypass -File tts_lines.ps1 <lineas.tsv> <carpeta_salida>
#   lineas.tsv (UTF-8): nombre<TAB>voz (Pablo/Laura/Helena)<TAB>tono (p.ej. -10%)<TAB>velocidad (p.ej. 1.1)<TAB>texto
# Salida: <carpeta_salida>/<nombre>.wav (voz seca; el procesado de radio/grito lo hace tts_process.py).
param([string]$Tsv, [string]$OutDir)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Runtime.WindowsRuntime
$null = [Windows.Media.SpeechSynthesis.SpeechSynthesizer, Windows.Media.SpeechSynthesis, ContentType = WindowsRuntime]
$null = [Windows.Storage.Streams.DataReader, Windows.Storage.Streams, ContentType = WindowsRuntime]

$asTask = ([System.WindowsRuntimeSystemExtensions].GetMethods() | Where-Object {
        $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and
        $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' })[0]
function Await($op, [Type]$t) {
    $task = $asTask.MakeGenericMethod($t).Invoke($null, @($op))
    $task.Wait() | Out-Null
    $task.Result
}

$synth = New-Object Windows.Media.SpeechSynthesis.SpeechSynthesizer
$voices = @{}
foreach ($v in [Windows.Media.SpeechSynthesis.SpeechSynthesizer]::AllVoices) {
    if ($v.Language -like 'es-*') { $voices[$v.DisplayName.Split(' ')[1]] = $v }
}
New-Item -ItemType Directory -Force $OutDir | Out-Null

foreach ($line in [System.IO.File]::ReadAllLines($Tsv, [System.Text.Encoding]::UTF8)) {
    if ($line.Trim() -eq '' -or $line.StartsWith('#')) { continue }
    $f = $line.Split("`t")
    $name = $f[0]; $voice = $f[1]; $pitch = $f[2]; $rate = $f[3]; $text = [System.Security.SecurityElement]::Escape($f[4])
    $synth.Voice = $voices[$voice]
    $ssml = "<speak version='1.0' xmlns='http://www.w3.org/2001/10/synthesis' xml:lang='es-ES'>" +
    "<prosody pitch='$pitch' rate='$rate'>$text</prosody></speak>"
    $stream = Await ($synth.SynthesizeSsmlToStreamAsync($ssml)) ([Windows.Media.SpeechSynthesis.SpeechSynthesisStream])
    $reader = New-Object Windows.Storage.Streams.DataReader($stream.GetInputStreamAt(0))
    $size = [uint32]$stream.Size
    $null = Await ($reader.LoadAsync($size)) ([uint32])
    $bytes = New-Object byte[] $size
    $reader.ReadBytes($bytes)
    [System.IO.File]::WriteAllBytes((Join-Path $OutDir "$name.wav"), $bytes)
    Write-Output "$name ($size bytes)"
}

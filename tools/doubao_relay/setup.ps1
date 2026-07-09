$ErrorActionPreference = 'Stop'

$DefaultInstallRoot = if ($env:SMARTHOMEHUB_RELAY_HOME) {
    $env:SMARTHOMEHUB_RELAY_HOME
} elseif (Test-Path -LiteralPath 'D:\A-Soft\DevTools') {
    'D:\A-Soft\DevTools\SmartHomeHubVoiceRelay'
} else {
    Join-Path $env:LOCALAPPDATA 'SmartHomeHubVoiceRelay'
}
$InstallRoot = $DefaultInstallRoot
$Venv = Join-Path $InstallRoot '.venv'
$PreferredPython = if ($env:SMARTHOMEHUB_PYTHON) {
    $env:SMARTHOMEHUB_PYTHON
} else {
    'D:\A-Soft\DevTools\Python313\python.exe'
}
$PythonCommand = if (Test-Path -LiteralPath $PreferredPython) {
    $PreferredPython
} else {
    Get-Command python -ErrorAction Stop
}
$PythonLabel = if ($PythonCommand -is [string]) { $PythonCommand } else { $PythonCommand.Definition }

Write-Host 'Estimated program/cache use: under 100 MB.'
Write-Host "Relay install root: $InstallRoot"
Write-Host "Using Python: $PythonLabel"
New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
& $PythonCommand -m venv $Venv
& (Join-Path $Venv 'Scripts\python.exe') -m pip install --upgrade pip
& (Join-Path $Venv 'Scripts\python.exe') -m pip install -r "$PSScriptRoot\requirements.txt"

$VenvPython = Join-Path $Venv 'Scripts\python.exe'
Write-Host "Python: $((Resolve-Path $VenvPython).Path)"
& $VenvPython -c "import websockets; print('websockets=' + websockets.__version__)"

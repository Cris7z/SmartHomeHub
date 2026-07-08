$ErrorActionPreference = 'Stop'

$InstallRoot = 'D:\A-Soft\DevTools\SmartHomeHubVoiceRelay'
$Venv = Join-Path $InstallRoot '.venv'
$Python = 'D:\A-Soft\DevTools\Python313\python.exe'

Write-Host 'Estimated program/cache use: under 100 MB on D:'
New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
& $Python -m venv $Venv
& (Join-Path $Venv 'Scripts\python.exe') -m pip install --upgrade pip
& (Join-Path $Venv 'Scripts\python.exe') -m pip install -r "$PSScriptRoot\requirements.txt"

$VenvPython = Join-Path $Venv 'Scripts\python.exe'
Write-Host "Python: $((Resolve-Path $VenvPython).Path)"
& $VenvPython -c "import websockets; print('websockets=' + websockets.__version__)"

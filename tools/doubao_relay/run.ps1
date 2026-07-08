$ErrorActionPreference = 'Stop'

$EnvFile = Join-Path $PSScriptRoot '.env'
$Python = 'D:\A-Soft\DevTools\SmartHomeHubVoiceRelay\.venv\Scripts\python.exe'

if (-not (Test-Path -LiteralPath $EnvFile)) {
    throw 'Missing tools\doubao_relay\.env. Create it from .env.example first.'
}
if (-not (Test-Path -LiteralPath $Python)) {
    throw 'Missing relay virtual environment. Run tools\doubao_relay\setup.ps1 first.'
}

Get-Content -LiteralPath $EnvFile | ForEach-Object {
    $Line = $_.Trim()
    if (-not $Line -or $Line.StartsWith('#') -or -not $Line.Contains('=')) {
        return
    }
    $Key, $Value = $Line.Split('=', 2)
    [Environment]::SetEnvironmentVariable($Key.Trim(), $Value.Trim(), 'Process')
}

Write-Host 'LAN IPv4 addresses:'
Get-NetIPAddress -AddressFamily IPv4 |
    Where-Object { $_.IPAddress -notlike '127.*' -and $_.PrefixOrigin -ne 'WellKnown' } |
    ForEach-Object { Write-Host "  ws://$($_.IPAddress):8765/voice" }

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
Push-Location $RepoRoot
try {
    & $Python -m tools.doubao_relay.server
} finally {
    Pop-Location
}

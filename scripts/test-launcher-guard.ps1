$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$guardScript=Join-Path $PSScriptRoot 'launcher-guard.ps1'
. $guardScript
if(Get-Process mafia2 -ErrorAction SilentlyContinue){throw 'Close Mafia normally before this isolated guard fixture'}
$directory=Join-Path $root ('work/launcher-guard-tests/'+[Guid]::NewGuid().ToString('N'))
[void][IO.Directory]::CreateDirectory($directory)
$child=Join-Path $directory 'child.ps1'
[IO.File]::WriteAllText($child,@'
param([string]$GuardScript,[string]$Expected)
$ErrorActionPreference='Stop'
. $GuardScript
$guard=$null
try {
    try {$guard=Enter-CoopLaunchGuard}
    catch {
        if($Expected -eq 'busy' -and $_.Exception.Data['CoopFailureCode'] -eq 'session_busy') {
            Write-Output 'PASS competing worker rejected with session_busy'
            exit 0
        }
        throw
    }
    if($Expected -ne 'acquired'){throw 'Competing process acquired an owned launch guard'}
    Write-Output 'PASS worker acquired guard after previous owner released it'
} finally {Exit-CoopLaunchGuard $guard}
'@,(New-Object Text.UTF8Encoding($true)))
$powershell=Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
$guard=$null
try {
    $guard=Enter-CoopLaunchGuard
    & $powershell -NoLogo -NoProfile -File $child -GuardScript $guardScript -Expected busy
    if($LASTEXITCODE -ne 0){throw 'Cross-process exclusion failed'}
} finally {Exit-CoopLaunchGuard $guard}
& $powershell -NoLogo -NoProfile -File $child -GuardScript $guardScript -Expected acquired
if($LASTEXITCODE -ne 0){throw 'Guard remained locked after normal owner release'}
Write-Output "Fixtures=$directory"

param([Parameter(Mandatory=$true)][string]$GameDirectory, [switch]$WaitForExit)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$exe = Join-Path $GameDirectory 'pc/mafia2.exe'
$expected = '16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f'
if ((Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant() -ne $expected) {
    throw 'Unknown game executable. Audit the new build before probing; no game started.'
}
if (Get-Process mafia2 -ErrorAction SilentlyContinue) { throw 'Close existing Mafia II before starting this probe.' }
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$local = Join-Path $root "work/probe-sessions/$stamp"
New-Item -ItemType Directory -Path $local -Force | Out-Null
# Private data stays in ignored work; absence is recorded, never fabricated.
$candidates = @(
    (Join-Path $env:LOCALAPPDATA '2K Games/Mafia II'),
    (Join-Path $env:APPDATA '2K Games/Mafia II'),
    (Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'My Games/Mafia II')
)
$saveState = @()
for ($i=0; $i -lt $candidates.Count; $i++) {
    $present = Test-Path -LiteralPath $candidates[$i]
    $saveState += [pscustomobject]@{Path=$candidates[$i];Present=$present}
    if ($present) { Copy-Item -LiteralPath $candidates[$i] -Destination (Join-Path $local "save-backup-$i") -Recurse }
}
$saveState | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $local 'save-locations.json')
Get-ChildItem -LiteralPath (Join-Path $GameDirectory 'pc') -File |
    Where-Object Extension -in '.exe','.dll','.asi','.ini','.cfg' |
    Get-FileHash | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $local 'baseline-hashes.json')
$launcher = Join-Path $root 'build/probe/probe_launcher.exe'
$dll = Join-Path $root 'build/probe/coop_probe.dll'
$log = Join-Path $local 'probe.log'
$extra = @()
if ($WaitForExit) { $extra += '--wait' }
& $launcher $exe $dll $log @extra
if ($LASTEXITCODE -ne 0) { throw "Probe failed; private evidence in $local" }
if (!(Test-Path -LiteralPath $log)) { throw 'Launcher succeeded without probe log' }
Get-Content -LiteralPath $log
Write-Output "Private evidence: $local"
Write-Output 'DLL loading only. Campaign, game callbacks and co-op are NOT verified.'

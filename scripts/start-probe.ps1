param([Parameter(Mandatory=$true)][string]$GameDirectory, [switch]$WaitForExit,
      [switch]$Observer, [string]$BuildDirectory = 'build/probe', [switch]$SecondScreen,
      [ValidateSet('off','host','guest')][string]$NetworkRole='off',
      [string]$Address='127.0.0.1', [ValidateRange(1,65535)][int]$Port=28960,
      [ValidatePattern('^\d{8}-\d{6}-\d{3}$')][string]$TestProfileFrom,
      [switch]$ContinueMainStory)
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
if($ContinueMainStory) {
    if(!$Observer -or !$TestProfileFrom){throw 'Continue test requires observer and an explicit earlier test profile'}
    [IO.File]::WriteAllText((Join-Path $local 'story-test.ini'),"[test]`r`ncontinue_main_story=1`r`n",[Text.Encoding]::ASCII)
}
if($NetworkRole -ne 'off') {
    if(!$Observer) {throw 'Network telemetry requires observer build'}
    $parsedAddress=$null
    if($Address -notmatch '^(0|[1-9]\d{0,2})(\.(0|[1-9]\d{0,2})){3}$' -or
       ![Net.IPAddress]::TryParse($Address,[ref]$parsedAddress) -or
       $parsedAddress.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork) {
        throw 'A literal IPv4 address is required'
    }
    [IO.File]::WriteAllText((Join-Path $local 'coop-session.ini'),
        "[network]`r`nrole=$NetworkRole`r`naddress=$Address`r`nport=$Port`r`n",[Text.Encoding]::ASCII)
}
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
function Get-OriginalProfileHashes {
    $result = @{}
    for ($i=0; $i -lt $candidates.Count; $i++) {
        if (Test-Path -LiteralPath $candidates[$i]) {
            Get-ChildItem -LiteralPath $candidates[$i] -File -Recurse | ForEach-Object {
                $key = "$i/" + $_.FullName.Substring($candidates[$i].Length+1)
                $result[$key] = (Get-FileHash -LiteralPath $_.FullName).Hash
            }
        }
    }
    return $result
}
$profileBefore = Get-OriginalProfileHashes
$profileBefore | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $local 'original-profile-before.json')
if ($Observer) {
    $profileRoot = Join-Path $local 'profile-localappdata'
    $profileParent = Join-Path $profileRoot '2K Games'
    New-Item -ItemType Directory -Path $profileParent -Force | Out-Null
    $profileSource=if($TestProfileFrom) {
        Join-Path $root "work/probe-sessions/$TestProfileFrom/profile-localappdata/2K Games/Mafia II"
    } else {$candidates[0]}
    if($TestProfileFrom -and !(Test-Path -LiteralPath $profileSource)){throw 'Earlier test profile not found'}
    if (Test-Path -LiteralPath $profileSource) {
        Copy-Item -LiteralPath $profileSource -Destination (Join-Path $profileParent 'Mafia II') -Recurse
    }
    if ($SecondScreen) {
        $videoPath = Join-Path $profileParent 'Mafia II/Saves/videoconfig.cfg'
        $video = (Get-Content -LiteralPath $videoPath -Raw).Trim() -split '\s+'
        if ($video.Count -ne 8 -or @($video | Where-Object {$_ -notmatch '^-?\d+$'}).Count) {
            throw 'Unknown video configuration; no game launched'
        }
        $video[0]='0'; $video[1]='0'; $video[2]='1280'; $video[3]='720'; $video[4]='0'
        [IO.File]::WriteAllText($videoPath, ($video -join ' '), [Text.Encoding]::ASCII)
    }
}
if ($SecondScreen -and !$Observer) { throw 'Second-screen tests require isolated observer profile' }
Get-ChildItem -LiteralPath (Join-Path $GameDirectory 'pc') -File |
    Where-Object Extension -in '.exe','.dll','.asi','.ini','.cfg' |
    Get-FileHash | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $local 'baseline-hashes.json')
$launcher = Join-Path (Join-Path $root $BuildDirectory) 'probe_launcher.exe'
$dllName = if ($Observer) { 'coop_observer.dll' } else { 'coop_probe.dll' }
$dll = Join-Path (Join-Path $root $BuildDirectory) $dllName
$log = Join-Path $local 'probe.log'
$extra = @('--user')
if ($WaitForExit) { $extra += '--wait' }
if ($Observer) { $extra += '--early' }
if ($SecondScreen) { $extra += '--monitor2' }
& $launcher $exe $dll $log @extra
$launchExit = $LASTEXITCODE
$profileAfter = Get-OriginalProfileHashes
$profileAfter | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $local 'original-profile-after.json')
$profileChanges = @(@($profileBefore.Keys) + @($profileAfter.Keys) | Sort-Object -Unique | Where-Object {
    $profileBefore[$_] -ne $profileAfter[$_]
})
[pscustomobject]@{WaitedForExit=[bool]$WaitForExit; ChangedFiles=$profileChanges.Count} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $local 'original-profile-comparison.json')
if ($profileChanges.Count) { throw "Original profile changed; inspect private backups in $local. No automatic restore performed." }
if ($launchExit -ne 0) { throw "Probe failed; private evidence in $local" }
if (!(Test-Path -LiteralPath $log)) { throw 'Launcher succeeded without probe log' }
Get-Content -LiteralPath $log -Tail 25
Write-Output "Private evidence: $local"
Write-Output 'Diagnostic code only. A log entry must prove each callback; no co-op support is implied.'

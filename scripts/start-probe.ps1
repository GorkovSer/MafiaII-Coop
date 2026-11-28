param([Parameter(Mandatory=$true)][string]$GameDirectory, [switch]$WaitForExit,
      [switch]$Observer, [switch]$BackgroundVisualTest, [string]$BuildDirectory = 'build/probe', [switch]$SecondScreen, [switch]$Windowed,
      [ValidateSet('off','host','guest')][string]$NetworkRole='off',
      [string]$Address='127.0.0.1', [ValidateRange(1,65535)][int]$Port=28960,
      [ValidatePattern('^\d{8}-\d{6}-\d{3}$')][string]$TestProfileFrom,
      [string]$SessionInfoPath,
      [switch]$ContinueMainStory, [switch]$TestModelClone, [switch]$TestHuman,
      [switch]$TestScriptMove, [switch]$TestVisualAvatar, [switch]$VisualNetwork,
      [ValidatePattern('^[A-Za-z0-9_.-]{1,64}$')][string]$VisualScene='classic-alley-mvp-v1',
      [ValidatePattern('^\d{8}-\d{6}-\d{3}$')][string]$ConcurrentHostSession)
$ErrorActionPreference = 'Stop'
$NetworkRole=$NetworkRole.ToLowerInvariant()
$root = Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'launch-result.ps1')
. (Join-Path $PSScriptRoot 'launcher-files.ps1')
$exe = Join-Path $GameDirectory 'pc/mafia2.exe'
$expected = '16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f'
if ((Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant() -ne $expected) {
    throw 'Unknown game executable. Audit the new build before probing; no game started.'
}
$games=@(Get-Process mafia2 -ErrorAction SilentlyContinue)
if($ConcurrentHostSession) {
    if(!$Observer -or !$VisualNetwork -or !$TestProfileFrom -or $NetworkRole -ne 'guest' -or
       $games.Count -ne 1 -or $games[0].Path -ne $exe){throw 'Second-client diagnostic requires one owned isolated visual host'}
    $hostRoot=Join-Path $root "work/probe-sessions/$ConcurrentHostSession"
    $hostLog=Get-Content -LiteralPath (Join-Path $hostRoot 'probe.log') -Raw
    $logAge=((Get-Item -LiteralPath (Join-Path $hostRoot 'probe.log')).CreationTimeUtc-$games[0].StartTime.ToUniversalTime()).TotalSeconds
    $hostConfig=Get-Content -LiteralPath (Join-Path $hostRoot 'coop-session.ini') -Raw
    if($logAge -lt 0 -or $logAge -gt 30 -or $hostLog -notmatch "(?m)^process_id=$($games[0].Id)\r?$" -or
       $hostLog -notmatch '(?m)^profile_local_appdata_redirected=1\r?$' -or
       $hostConfig -notmatch '(?m)^role=host\r?$' -or $hostConfig -notmatch '(?m)^visual_only=1\r?$') {
        throw 'Existing game is not the isolated visual host named by the diagnostic'
    }
} elseif($games.Count){throw 'Close existing Mafia II before starting this probe.'}
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$local = Join-Path $root "work/probe-sessions/$stamp"
if(Test-Path -LiteralPath $local){throw 'Fresh launch session required; no files overwritten'}
if($VisualNetwork -and (!$Observer -or !$TestProfileFrom -or $NetworkRole -eq 'off' -or
    $TestHuman -or $TestScriptMove -or $TestModelClone -or $ContinueMainStory -or $TestVisualAvatar)) {
    throw 'Visual network requires an isolated observer profile, host/guest role and no native diagnostics'
}
if($TestVisualAvatar -and (!$Observer -or !$TestProfileFrom -or $NetworkRole -ne 'off' -or
    $TestHuman -or $TestScriptMove -or $TestModelClone -or $ContinueMainStory)) {
    throw 'Visual avatar test requires an isolated observer profile, networking off and no other native tests'
}
if($BackgroundVisualTest -and (!$Observer -or !$VisualNetwork -or !$TestProfileFrom)){throw 'Background diagnostic requires isolated visual networking'}
New-Item -ItemType Directory -Path $local -Force | Out-Null
if($SessionInfoPath) {
    Write-CoopRunJson $SessionInfoPath @{schema=1;session_id=$stamp}
}
if($BackgroundVisualTest){[IO.File]::WriteAllText((Join-Path $local 'background-test.ini'),"[test]`r`nbackground_visual_client=1`r`n",[Text.Encoding]::ASCII)}
if($ConcurrentHostSession){
    [IO.File]::WriteAllText((Join-Path $local 'instance-test.ini'),"[test]`r`nsecond_visual_client=1`r`n",[Text.Encoding]::ASCII)
}
if($TestVisualAvatar) {
    [IO.File]::WriteAllText((Join-Path $local 'visual-test.ini'),"[test]`r`nvisual_once=1`r`n",[Text.Encoding]::ASCII)
}
if($TestScriptMove -and (!$TestHuman -or $NetworkRole -ne 'off')) {
    throw 'Script movement acceptance requires the isolated Human test with networking off'
}
if($TestHuman) {
    if(!$Observer -or !$TestProfileFrom -or $TestModelClone){throw 'Human test requires observer, isolated profile and no simultaneous model-only test'}
    $scriptMoveValue=[int][bool]$TestScriptMove
    [IO.File]::WriteAllText((Join-Path $local 'human-test.ini'),"[test]`r`nhuman_once=1`r`nscript_move_once=$scriptMoveValue`r`n",[Text.Encoding]::ASCII)
}
if($TestModelClone) {
    if(!$Observer -or !$TestProfileFrom){throw 'Model clone test requires observer and an explicit isolated test profile'}
    [IO.File]::WriteAllText((Join-Path $local 'model-test.ini'),"[test]`r`nclone_once=1`r`n",[Text.Encoding]::ASCII)
}
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
    $visualSettings=''
    if($VisualNetwork) {
        # This is explicit operator agreement on a scene label, NOT automatic
        # mission/save detection. Its capability cannot authorize physical actors.
        $sceneHasher=[Security.Cryptography.SHA256]::Create()
        try {$sceneDigest=$sceneHasher.ComputeHash([Text.Encoding]::UTF8.GetBytes("MafiaII-Coop visual-only scene approval/v1`n$VisualScene"))}
        finally {$sceneHasher.Dispose()}
        $sceneKey=([BitConverter]::ToString($sceneDigest)).Replace('-','').ToLowerInvariant()
        $visualSettings="visual_only=1`r`nscene_key=$sceneKey`r`n"
    }
    [IO.File]::WriteAllText((Join-Path $local 'coop-session.ini'),
        "[network]`r`nrole=$NetworkRole`r`naddress=$Address`r`nport=$Port`r`n$visualSettings",[Text.Encoding]::ASCII)
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
    if ($SecondScreen -or $Windowed) {
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
if ($Windowed -and !$Observer) { throw 'Windowed tests require isolated observer profile' }
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
Write-CoopSessionPhase $SessionInfoPath 'game_starting' $stamp
& $launcher $exe $dll $log @extra | ForEach-Object {
    if($_ -match '^probe_loaded pid=\d+ log_written=1$') {
        Write-CoopSessionPhase $SessionInfoPath 'game_running' $stamp
    }
    $_
} | Tee-Object -FilePath (Join-Path $local 'launcher-output.txt')
$launchExit = $LASTEXITCODE
Write-CoopSessionPhase $SessionInfoPath 'verifying_profile' $stamp
$profileAfter = Get-OriginalProfileHashes
$profileAfter | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $local 'original-profile-after.json')
$profileChanges = @(@($profileBefore.Keys) + @($profileAfter.Keys) | Sort-Object -Unique | Where-Object {
    $profileBefore[$_] -ne $profileAfter[$_]
})
[pscustomobject]@{WaitedForExit=[bool]$WaitForExit; ChangedFiles=$profileChanges.Count} |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $local 'original-profile-comparison.json')
$launcherText = Get-Content -LiteralPath (Join-Path $local 'launcher-output.txt') -Raw
Get-CoopLaunchResult -LauncherOutput $launcherText -LauncherExit $launchExit `
    -WaitedForExit ([bool]$WaitForExit) -OriginalProfileChanges $profileChanges.Count |
    ConvertTo-Json | Set-Content -LiteralPath (Join-Path $local 'launch-result.json')
if ($profileChanges.Count) { throw (New-CoopSessionFailure 'original_profile_changed' "Original profile changed; inspect private backups in $local. No automatic restore performed.") }
if ($launchExit -ne 0) { throw (New-CoopSessionFailure 'game_launch_or_exit_failed' "Probe failed; private evidence in $local") }
if (!(Test-Path -LiteralPath $log)) { throw 'Launcher succeeded without probe log' }
Get-Content -LiteralPath $log -Tail 25
Write-Output "Private evidence: $local"
Write-Output 'Diagnostic code only. A log entry must prove each callback; no co-op support is implied.'

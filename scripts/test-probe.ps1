param([string]$BuildDirectory = 'build/probe')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'launch-result.ps1')
foreach($invalid in @('', "child_exit=0`nchild_exit=7", 'child_exit=unknown')) {
    $unknown=Get-CoopLaunchResult -LauncherOutput $invalid -LauncherExit 0 -WaitedForExit $true -OriginalProfileChanges 0
    if($null -ne $unknown.child_exit_code) {throw 'Ambiguous launcher result incorrectly accepted'}
}
$out = Join-Path $root $BuildDirectory
& (Join-Path $out 'test_entity_registry.exe')
if ($LASTEXITCODE -ne 0) { throw 'Production entity registry fixture failed' }
& (Join-Path $out 'test_command_queue.exe')
if ($LASTEXITCODE -ne 0) { throw 'Production command queue fixture failed' }
& (Join-Path $out 'test_move_reentry.exe')
if ($LASTEXITCODE -ne 0) { throw 'Move reentry fixture failed' }
& (Join-Path $out 'test_script_move_state.exe')
if ($LASTEXITCODE -ne 0) { throw 'Script movement state fixture failed' }
& (Join-Path $out 'test_script_move_reentry.exe')
if ($LASTEXITCODE -ne 0) { throw 'Script movement reentry fixture failed' }
& (Join-Path $out 'test_script_progress.exe')
if ($LASTEXITCODE -ne 0) { throw 'Production script progress fixture failed' }
& (Join-Path $out 'test_world_sync.exe')
if ($LASTEXITCODE -ne 0) { throw 'World agreement fixture failed' }
& (Join-Path $out 'test_remote_actor.exe')
if ($LASTEXITCODE -ne 0) { throw 'Remote actor fixture failed' }
& (Join-Path $out 'test_replication_foundation.exe')
if ($LASTEXITCODE -ne 0) { throw 'Replication foundation fixture failed' }
& (Join-Path $out 'telemetry_host.exe')
if ($LASTEXITCODE -ne 0) { throw 'Production telemetry lifecycle fixture failed' }
$dir = Join-Path $root ('work/loader-test/' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $dir -Force | Out-Null
$launcher = Join-Path $out 'probe_launcher.exe'
$hostExe = Join-Path $out 'probe_host.exe'
$dll = Join-Path $out 'coop_probe.dll'
$log = Join-Path $dir 'probe.log'
$result = (& $launcher $hostExe $dll $log --wait) -join "`n"
if ($LASTEXITCODE -ne 0) { throw 'Real x86 DLL loading failed' }
$fields = Get-Content -LiteralPath $log | ConvertFrom-StringData
if ($fields.pointer_bits -ne '32' -or $fields.pe_machine -ne '014c' -or
    $fields.status -ne 'loaded_inside_process' -or $fields.initial_game_patches -ne '0') {
    throw 'Unexpected in-process evidence'
}
if ($result -notmatch "pid=$($fields.process_id) ") { throw 'PID evidence mismatch' }
if ($result -notmatch 'child_exit=0') { throw 'Test host did not exit normally' }
$fixtureResult=Get-CoopLaunchResult -LauncherOutput $result -LauncherExit 0 -WaitedForExit $true -OriginalProfileChanges 0
if($fixtureResult.child_exit_code -ne 0 -or !$fixtureResult.waited_for_exit) {throw 'Native fixture exit evidence missing'}
if ($fields.test_host_entry_seen -ne '1') { throw 'Normal load should follow test host entry' }
Write-Output 'PASS real x86 load, explicit export invocation, matching PID and log'
Write-Output 'PASS native process handle reports normal child exit'
$before = (Get-FileHash -LiteralPath $log).Hash
& $launcher $hostExe $dll $log 2>$null
if ($LASTEXITCODE -ne 4 -or (Get-FileHash -LiteralPath $log).Hash -ne $before) {
    throw 'Existing evidence was not protected'
}
Write-Output 'PASS refusal to overwrite existing log'
& $launcher $hostExe (Join-Path $dir 'missing.dll') (Join-Path $dir 'missing.log') 2>$null
if ($LASTEXITCODE -ne 5 -or (Test-Path -LiteralPath (Join-Path $dir 'missing.log'))) {
    throw 'Missing DLL should fail before target launch'
}
Write-Output 'PASS missing DLL rejected before target launch'
$earlyLog = Join-Path $dir 'early.log'
$early = (& $launcher $hostExe $dll $earlyLog --early --wait) -join "`n"
if ($LASTEXITCODE -ne 0 -or $early -notmatch 'child_exit=0') { throw 'Early load failed' }
$earlyFields = Get-Content -LiteralPath $earlyLog | ConvertFrom-StringData
if ($earlyFields.test_host_entry_seen -ne '0') { throw 'Early load ran after executable entry' }
Write-Output 'PASS early DLL/export executes before host entry, then host exits normally'
$observerLog = Join-Path $dir 'observer-negative.log'
& $launcher $hostExe (Join-Path $out 'coop_observer.dll') $observerLog --early 2>$null
if ($LASTEXITCODE -ne 9 -or
    (Get-Content -LiteralPath $observerLog -Raw) -notmatch 'refused_unknown_image') {
    throw 'Observer must reject non-audited executable before game hooks'
}
Write-Output 'PASS observer refuses unaudited executable without installing a hook'
$profileDir = Join-Path $dir 'profile-test'
$profileRoot = Join-Path $profileDir 'profile-localappdata'
New-Item -ItemType Directory -Path $profileRoot -Force | Out-Null
$oldRoot = $env:COOP_PROFILE_TEST_ROOT
try {
    $env:COOP_PROFILE_TEST_ROOT = $profileRoot
    $profileArgs = @((Join-Path $out 'profile_host.exe'), (Join-Path $out 'coop_profile_probe.dll'),
                    (Join-Path $profileDir 'profile.log'), '--early', '--wait')
    $profileResult = (& $launcher @profileArgs) -join "`n"
    if ($LASTEXITCODE -ne 0 -or $profileResult -notmatch 'child_exit=0') { throw 'Profile host failed' }
    if (!(Test-Path -LiteralPath (Join-Path $profileRoot 'profile-test.marker'))) { throw 'Isolated write missing' }
    if ((Get-Content -LiteralPath (Join-Path $profileDir 'profile.log') -Raw) -notmatch 'profile_local_appdata_redirected=1') {
        throw 'Profile redirect not observed'
    }
    Write-Output 'PASS real host queries isolated LocalAppData and writes only there'
    Write-Output 'PASS unrelated CSIDL_WINDOWS query retains the real Windows directory'
} finally {
    $env:COOP_PROFILE_TEST_ROOT = $oldRoot
}
$userLog=Join-Path $dir 'user.log'
$userResult=(& $launcher $hostExe $dll $userLog --early --user --wait) -join "`n"
if($LASTEXITCODE -ne 0 -or $userResult -notmatch 'child_exit=0') {throw 'Limited-token test host failed'}
Write-Output 'PASS same-account user-token launch, early DLL and normal process exit'

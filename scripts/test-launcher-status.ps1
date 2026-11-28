$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'launcher-status.ps1')
$fixture=Join-Path $root ('work/launcher-status-tests/'+[Guid]::NewGuid().ToString('N'))
$run=Join-Path $fixture 'run'
$session=Join-Path $fixture 'work/probe-sessions/20261128-000000-001'
New-Item -ItemType Directory -Path $run,$session -Force | Out-Null
$state=New-CoopLiveState
Read-CoopLiveState $fixture $run $state
if((Get-CoopLiveCode $state) -ne 'starting'){throw 'Invented startup state'}
'{"schema":1,"session_id":"20261128-000000-001"}' | Set-Content (Join-Path $run session-info.json)
$log=Join-Path $session probe.log
[IO.File]::WriteAllText($log,'')
function Feed([string]$text,[string]$expected) {
    [IO.File]::AppendAllText($log,$text)
    Read-CoopLiveState $fixture $run $state
    if((Get-CoopLiveCode $state) -ne $expected){throw "Unexpected state, expected $expected got $(Get-CoopLiveCode $state)"}
}
Feed "network=listening thread=1`n" waiting_guest
Feed "network=ready_host thread=1`n" connected_loading
Feed "observer=load_finished value=1`n" waiting_scene
Feed 'visual_test=network_bound value=1' waiting_scene
Feed "`n" avatar_visible
Feed "observer=game_paused value=1`n" paused
Feed "observer=game_unpaused value=1`nvisual_test=network_bound value=2`n" avatar_visible
Feed "network=disconnected thread=1`nnetwork=retry_wait thread=1`n" reconnecting
Feed "network=ready_guest thread=1`n" waiting_scene
Feed "visual_test=network_bound value=3`n" avatar_visible
Feed "network=remote_expired thread=1`n" waiting_scene
Feed "network=retry_refused thread=1`n" refused
Feed "network=ready_guest thread=1`nvisual_test=scene_approval_ended value=32`nvisual_test=network_bound value=4`n" scene_ended
# A partial line cannot be acted on, and unrelated files are never discovered.
if(!$state.scene_ended){throw 'Scene withdrawal lost'}
$state=New-CoopLiveState
'{"schema":1,"session_id":"../../outside"}' | Set-Content (Join-Path $run session-info.json)
$refused=$false
try {Read-CoopLiveState $fixture $run $state} catch {$refused=$true}
if(!$refused){throw 'Invalid session link accepted'}
$good=@{launch_result=@{waited_for_exit=$true;child_exit_code=0;launcher_exit=0;original_profile_changed_files=0};
    visual_events=@{creation_registry_confirmed=1};model_lifetimes_balanced_after_exit=$true}
if(!(Get-CoopExitSummary $good).avatar_created){throw 'Verified avatar evidence lost'}
foreach($field in @('child_exit_code','launcher_exit','original_profile_changed_files','waited_for_exit')) {
    $bad=$good | ConvertTo-Json -Depth 5 | ConvertFrom-Json
    $bad.launch_result.$field=$null
    $refused=$false;try {Get-CoopExitSummary $bad | Out-Null} catch {$refused=$true}
    if(!$refused){throw "Missing $field accepted as success"}
}
$good.model_lifetimes_balanced_after_exit=$false
$refused=$false;try {Get-CoopExitSummary $good | Out-Null} catch {$refused=$true}
if(!$refused){throw 'Leaked avatar evidence accepted'}
$good.visual_events=@{}
if((Get-CoopExitSummary $good).avatar_created){throw 'No-avatar exit invented a visual success'}
Write-Output 'PASS launch/loading/avatar/pause/expiry/reconnect/refusal/reload, partial lines, link validation and exit evidence'
Write-Output "Fixtures=$fixture"

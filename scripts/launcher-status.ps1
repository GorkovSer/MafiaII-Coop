# Incremental, bounded reader of the one session explicitly linked by our worker.
# No process control, network probes, or scanning unrelated session directories.
function New-CoopLiveState {
    return @{session_id=$null;offset=[long]0;carry='';network='starting';loaded=$false;
        paused=$false;scene_ended=$false;avatar=$false}
}
function Read-CoopLiveState([string]$Root,[string]$RunDirectory,[hashtable]$State) {
    $link=Join-Path $RunDirectory 'session-info.json'
    if(!$State.session_id -and (Test-Path -LiteralPath $link)) {
        if((Get-Item -LiteralPath $link).Length -gt 1024){throw 'Session link exceeds limit'}
        $info=Get-Content -LiteralPath $link -Raw -Encoding UTF8 | ConvertFrom-Json
        if($info.schema -ne 1 -or $info.session_id -notmatch '^\d{8}-\d{6}-\d{3}$') {throw 'Invalid session link'}
        $State.session_id=[string]$info.session_id
    }
    if(!$State.session_id){return}
    $log=Join-Path $Root "work/probe-sessions/$($State.session_id)/probe.log"
    if(!(Test-Path -LiteralPath $log)){return}
    $stream=[IO.File]::Open($log,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
    try {
        if($stream.Length -lt $State.offset){throw 'Session log was truncated'}
        [void]$stream.Seek($State.offset,[IO.SeekOrigin]::Begin)
        $bytes=New-Object byte[] 65536
        $count=$stream.Read($bytes,0,$bytes.Length)
        $State.offset+=$count
    } finally {$stream.Dispose()}
    $text=$State.carry+[Text.Encoding]::ASCII.GetString($bytes,0,$count)
    $end=$text.LastIndexOf("`n")
    if($end -lt 0){$State.carry=$text;if($text.Length -gt 4096){throw 'Session line exceeds limit'};return}
    $State.carry=$text.Substring($end+1)
    if($State.carry.Length -gt 4096){throw 'Session line exceeds limit'}
    foreach($line in ($text.Substring(0,$end) -split "`n")) {
        if($line -match '^network=(\w+) ') {
            $event=$Matches[1]
            if($event -in @('listening','connecting','ready_host','ready_guest','disconnected',
                'timeout','retry_wait','retry_connecting','retry_unavailable','retry_refused','start_failed','stopped')) {
                $State.network=$event
            } elseif($event -like 'reject_*') {$State.network='refused'}
            if($event -in @('disconnected','timeout','remote_inactive','remote_expired','stopped') -or
                $event -like 'reject_*') {$State.avatar=$false}
        }
        if($line -match '^observer=load_started ') {$State.loaded=$false;$State.avatar=$false}
        if($line -match '^observer=load_finished ' -or $line -like 'player_sample *') {$State.loaded=$true}
        if($line -match '^observer=game_paused ') {$State.paused=$true;$State.avatar=$false}
        if($line -match '^observer=game_unpaused ') {$State.paused=$false}
        if($line -match '^visual_test=scene_approval_ended ') {$State.scene_ended=$true;$State.avatar=$false}
        if($line -match '^visual_test=(release_requested|scope_withdrawn|scene_invalidated) ') {$State.avatar=$false}
        if($line -match '^visual_test=network_bound ') {$State.avatar=$true}
    }
}
function Get-CoopLiveCode([hashtable]$State) {
    if($State.scene_ended){return 'scene_ended'}
    if($State.network -in @('refused','retry_refused','start_failed')){return 'refused'}
    if($State.paused){return 'paused'}
    if($State.network -in @('disconnected','timeout','retry_wait','retry_connecting','retry_unavailable')){return 'reconnecting'}
    if($State.avatar){return 'avatar_visible'}
    if($State.network -in @('ready_host','ready_guest')){if($State.loaded){return 'waiting_scene'};return 'connected_loading'}
    if($State.network -eq 'listening'){return 'waiting_guest'}
    if($State.network -eq 'connecting'){return 'connecting'}
    if($State.network -eq 'stopped'){return 'stopped'}
    return 'starting'
}
function Get-CoopExitSummary($Evidence) {
    $exit=$Evidence.launch_result
    if($exit.waited_for_exit -ne $true -or
        $null -eq $exit.child_exit_code -or $exit.child_exit_code -ne 0 -or
        $null -eq $exit.launcher_exit -or $exit.launcher_exit -ne 0 -or
        $null -eq $exit.original_profile_changed_files -or $exit.original_profile_changed_files -ne 0) {
        throw 'Game exit/profile verification failed; see session-report.json'
    }
    $created=$Evidence.visual_events.creation_registry_confirmed -gt 0
    if($created -and $Evidence.model_lifetimes_balanced_after_exit -ne $true) {
        throw 'Unbalanced avatar lifetime evidence; see session-report.json'
    }
    return @{child_exit_code=0;launcher_exit=0;original_profile_changed_files=0;avatar_created=$created}
}

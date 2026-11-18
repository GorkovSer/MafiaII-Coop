param(
    [Parameter(Mandatory=$true)][ValidatePattern('^\d{8}-\d{6}-\d{3}$')][string]$SessionId,
    [Parameter(Mandatory=$true)][ValidateRange(1,2147483647)][int]$GamePid,
    [Parameter(Mandatory=$true)][string]$BuildDirectory,
    [ValidateRange(5,45)][int]$MenuTimeoutSeconds=30,
    [ValidateRange(5,60)][int]$LoadTimeoutSeconds=60
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$session=Join-Path $root "work/probe-sessions/$SessionId"
$log=Join-Path $session 'probe.log'
$helper=Join-Path (Join-Path $root $BuildDirectory) 'coop_input.exe'
function Read-Probe {
    if((Get-Item -LiteralPath $log).Length -gt 1MB){throw 'Unexpected log size; stop and inspect manually'}
    return [IO.File]::ReadAllText($log)
}
$initial=Read-Probe
if($initial -notmatch "(?m)^process_id=$GamePid\r?$" -or
   $initial -notmatch '(?m)^profile_local_appdata_redirected=1\r?$' -or
   (Get-Content -LiteralPath (Join-Path $session 'launcher-output.txt') -Raw) -notmatch "(?m)^probe_loaded pid=$GamePid log_written=1\r?$") {
    throw 'Not the owned isolated observer launch; no input sent'
}
$loading='(?m)^observer=(load_started|mission_before_open|mission_open|load_finished) '
if($initial -match $loading -or $initial -match '(?m)^player_sample '){throw 'Session already entered loading/gameplay; no input sent'}
$identity=& $helper inspect $GamePid
if($LASTEXITCODE -ne 0 -or $identity -notmatch '^pid=\d+ creation=(\d+) image_verified=1$'){
    throw 'Native process validation failed; no input sent'
}
$creation=$Matches[1]
$watch=[Diagnostics.Stopwatch]::StartNew()
$keys=0
$loadingAt=$null
$readyAt=$null
while($watch.Elapsed.TotalSeconds -lt $MenuTimeoutSeconds){
    $text=Read-Probe
    if($text -match $loading){$loadingAt=$watch.ElapsedMilliseconds; break}
    # Each invocation revalidates EXE/creation/live process/foreground and releases Enter.
    $keyResult=& $helper key $GamePid $creation enter 100
    if($LASTEXITCODE -ne 0){throw "Native input stopped; reobserve window before retrying: $keyResult"}
    $keys++
    # Poll during the inter-key gap; never keep sending Enter after the loading event.
    for($i=0;$i -lt 4;$i++){
        Start-Sleep -Milliseconds 50
        if((Read-Probe) -match $loading){$loadingAt=$watch.ElapsedMilliseconds; break}
    }
    if($null -ne $loadingAt){break}
}
if($null -eq $loadingAt){throw 'Menu deadline reached; Enter stopped. Observe the actual screen; do not blindly rerun.'}
Write-Output "enter_stopped_on_loading=1 keys=$keys elapsed_ms=$loadingAt"
$loadDeadline=$watch.Elapsed.TotalSeconds+$LoadTimeoutSeconds
while($watch.Elapsed.TotalSeconds -lt $loadDeadline){
    $text=Read-Probe
    if($text -match '(?m)^observer=load_finished ' -and $text -match '(?m)^player_sample '){
        $readyAt=$watch.ElapsedMilliseconds; break
    }
    if($text -match '(?m)^observer=(game_done|mission_close|shutdown) '){throw 'Game ended before player telemetry; input remains stopped'}
    Start-Sleep -Milliseconds 100
}
if($null -eq $readyAt){throw 'Player telemetry deadline reached; input remains stopped. Observe the game.'}
$result=[pscustomobject]@{schema=1; keys_sent=$keys; loading_detected_ms=$loadingAt;
    player_sample_detected_ms=$readyAt; input_stopped_on_loading=$true; visual_confirmation_required=$true}
$result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $session 'ui-entry-result.json')
$result
Write-Output 'Player telemetry detected. Capture the scene once to confirm the loaded save visually.'

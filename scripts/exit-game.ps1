# One native invocation validates, sends Alt+F4, then immediately repeats Enter.
param(
    [Parameter(Mandatory=$true)][ValidatePattern('^\d{8}-\d{6}-\d{3}$')][string]$SessionId,
    [Parameter(Mandatory=$true)][ValidateRange(1,2147483647)][int]$GamePid,
    [Parameter(Mandatory=$true)][string]$BuildDirectory
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$session=Join-Path $root "work/probe-sessions/$SessionId"
$log=Get-Content -LiteralPath (Join-Path $session 'probe.log') -Raw
if($log -notmatch "(?m)^process_id=$GamePid\r?$" -or
   $log -notmatch '(?m)^profile_local_appdata_redirected=1\r?$' -or
   (Get-Content -LiteralPath (Join-Path $session 'launcher-output.txt') -Raw) -notmatch "(?m)^probe_loaded pid=$GamePid log_written=1\r?$"){
    throw 'Not the owned isolated observer launch; no input sent'
}
$helper=Join-Path (Join-Path $root $BuildDirectory) 'coop_input.exe'
$identity=& $helper inspect $GamePid
if($LASTEXITCODE -ne 0 -or $identity -notmatch '^pid=\d+ creation=(\d+) image_verified=1$'){
    throw 'Native process validation failed; no input sent'
}
$creation=$Matches[1]
$result=& $helper quit $GamePid $creation
if($LASTEXITCODE -ne 0){throw "Native quit stopped; observe before retrying (no force-close): $result"}
if($result -notmatch '^quit_enter_keys=(\d+) elapsed_ms=(\d+) process_exited=1 exit_code=0$'){
    throw 'Unexpected quit result; verify the native launcher result'
}
$evidence=[pscustomobject]@{schema=1; keys_sent=[int]$Matches[1];
    chord_to_process_exit_ms=[long]$Matches[2]; process_exit_code=0; launcher_profile_check_required=$true}
$evidence | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $session 'ui-exit-result.json')
$evidence

param([Parameter(Mandatory=$true)][string]$RequestPath)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'launcher-diagnostics.ps1')
. (Join-Path $PSScriptRoot 'launcher-status.ps1')
. (Join-Path $PSScriptRoot 'launcher-guard.ps1')
. (Join-Path $PSScriptRoot 'launcher-files.ps1')
$runDirectory=Split-Path ([IO.Path]::GetFullPath($RequestPath)) -Parent
$logPath=Join-Path $runDirectory 'output.txt'
$resultPath=Join-Path $runDirectory 'result.json'
$exitCode=1
$guard=$null
$phase='validating_request'
$sessionInfoPath=Join-Path $runDirectory 'session-info.json'
$result=@{schema=1;status='failed';code='worker_interrupted';phase=$phase;message='Worker did not complete.'}
try {
    $request=Read-CoopRunJson $RequestPath
    if($request.schema -ne 1 -or $request.role -notin @('host','guest')) {throw 'Invalid launcher request'}
    # Hold across preparation, game lifetime, report generation and profile checks.
    # A second preview directory must not race the first worker's startup checks.
    $phase='acquiring_session'
    Write-CoopSessionPhase $sessionInfoPath $phase
    $guard=Enter-CoopLaunchGuard
    $options=@{Role=[string]$request.role;Address=[string]$request.address;
        GameDirectory=[string]$request.game_directory;Port=28960;
        BuildDirectory=[string]$request.build_directory;
        SessionInfoPath=$sessionInfoPath}
    # The existing entry point retains all executable, manifest, address,
    # profile-isolation and one-game checks. No diagnostic bypass flags here.
    $global:LASTEXITCODE=0
    $phase='preparing'
    Write-CoopSessionPhase $sessionInfoPath $phase
    & (Join-Path $PSScriptRoot 'start-visual-session.ps1') @options *> $logPath
    if($LASTEXITCODE -ne 0) {throw "Session process exited with code $LASTEXITCODE"}
    $phase='verifying_result'
    Write-CoopSessionPhase $sessionInfoPath $phase
    $info=Read-CoopRunJson $sessionInfoPath
    if($info.schema -ne 1 -or $info.session_id -notmatch '\A\d{8}-\d{6}-\d{3}\z'){throw 'Invalid session link'}
    $sessionDirectory=Join-Path $root "work/probe-sessions/$($info.session_id)"
    $summary=& (Join-Path $PSScriptRoot 'visual-session-report.ps1') -SessionDirectory $sessionDirectory
    $summary | Set-Content -LiteralPath (Join-Path $runDirectory 'session-report.json') -Encoding UTF8
    $evidence=$summary | ConvertFrom-Json
    $verified=Get-CoopExitSummary $evidence
    $result=@{schema=1;status='finished';message='Clean game exit; original profile unchanged.';
        session_id=$info.session_id;child_exit_code=0;launcher_exit=0;original_profile_changed_files=0;
        avatar_created=$verified.avatar_created}
    $exitCode=0
    Write-CoopSessionPhase $sessionInfoPath 'finished' $info.session_id
} catch {
    $message=$_.Exception.Message
    $failureCode=[string]$_.Exception.Data['CoopFailureCode']
    if(!$failureCode) {
        $failureCode=switch($phase) {
            'validating_request' {'invalid_request'}
            'acquiring_session' {'session_lock_failed'}
            'verifying_result' {'report_failed'}
            default {'session_failed'}
        }
    }
    # Establish the result before optional logging: logging can itself fail.
    $result=@{schema=1;status='failed';code=$failureCode;phase=$phase;message=$message;failure_report=$null}
    $report=Write-CoopFailureReport -Root $root -Failure $_ -Kind 'session-start-or-exit-failure' -SessionDirectory $runDirectory
    $result.failure_report=$report
    try {$message | Out-File -LiteralPath $logPath -Append -Encoding UTF8} catch {}
    Write-CoopSessionPhase $sessionInfoPath 'failed'
} finally {
    try {
        Write-CoopRunJson $resultPath $result
    } finally {Exit-CoopLaunchGuard $guard}
}
exit $exitCode

param([Parameter(Mandatory=$true)][string]$RequestPath)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'launcher-diagnostics.ps1')
. (Join-Path $PSScriptRoot 'launcher-status.ps1')
$runDirectory=Split-Path ([IO.Path]::GetFullPath($RequestPath)) -Parent
$logPath=Join-Path $runDirectory 'output.txt'
$resultPath=Join-Path $runDirectory 'result.json'
$exitCode=1
try {
    $request=Get-Content -LiteralPath $RequestPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if($request.schema -ne 1 -or $request.role -notin @('host','guest')) {throw 'Invalid launcher request'}
    $options=@{Role=[string]$request.role;Address=[string]$request.address;
        GameDirectory=[string]$request.game_directory;Port=28960;
        BuildDirectory=[string]$request.build_directory;
        SessionInfoPath=(Join-Path $runDirectory 'session-info.json')}
    # The existing entry point retains all executable, manifest, address,
    # profile-isolation and one-game checks. No diagnostic bypass flags here.
    $global:LASTEXITCODE=0
    & (Join-Path $PSScriptRoot 'start-visual-session.ps1') @options *> $logPath
    if($LASTEXITCODE -ne 0) {throw "Session process exited with code $LASTEXITCODE"}
    $info=Get-Content -LiteralPath (Join-Path $runDirectory 'session-info.json') -Raw | ConvertFrom-Json
    if($info.schema -ne 1 -or $info.session_id -notmatch '^\d{8}-\d{6}-\d{3}$'){throw 'Invalid session link'}
    $sessionDirectory=Join-Path $root "work/probe-sessions/$($info.session_id)"
    $summary=& (Join-Path $PSScriptRoot 'visual-session-report.ps1') -SessionDirectory $sessionDirectory
    $summary | Set-Content -LiteralPath (Join-Path $runDirectory 'session-report.json') -Encoding UTF8
    $evidence=$summary | ConvertFrom-Json
    $verified=Get-CoopExitSummary $evidence
    $result=@{schema=1;status='finished';message='Clean game exit; original profile unchanged.';
        session_id=$info.session_id;child_exit_code=0;launcher_exit=0;original_profile_changed_files=0;
        avatar_created=$verified.avatar_created}
    $exitCode=0
} catch {
    $message=$_.Exception.Message
    $report=Write-CoopFailureReport -Root $root -Failure $_ -Kind 'session-start-or-exit-failure' -SessionDirectory $runDirectory
    $message | Out-File -LiteralPath $logPath -Append -Encoding UTF8
    $result=@{schema=1;status='failed';message=$message;failure_report=$report}
} finally {
    $result | ConvertTo-Json | Set-Content -LiteralPath ($resultPath+'.tmp') -Encoding UTF8
    Move-Item -LiteralPath ($resultPath+'.tmp') -Destination $resultPath
}
exit $exitCode

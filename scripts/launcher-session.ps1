param([Parameter(Mandatory=$true)][string]$RequestPath)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'launcher-diagnostics.ps1')
$runDirectory=Split-Path ([IO.Path]::GetFullPath($RequestPath)) -Parent
$logPath=Join-Path $runDirectory 'output.txt'
$resultPath=Join-Path $runDirectory 'result.json'
$exitCode=1
try {
    $request=Get-Content -LiteralPath $RequestPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if($request.schema -ne 1 -or $request.role -notin @('host','guest')) {throw 'Invalid launcher request'}
    $options=@{Role=[string]$request.role;Address=[string]$request.address;
        GameDirectory=[string]$request.game_directory;Port=28960;
        BuildDirectory=[string]$request.build_directory}
    # The existing entry point retains all executable, manifest, address,
    # profile-isolation and one-game checks. No diagnostic bypass flags here.
    $global:LASTEXITCODE=0
    & (Join-Path $PSScriptRoot 'start-visual-session.ps1') @options *> $logPath
    if($LASTEXITCODE -ne 0) {throw "Session process exited with code $LASTEXITCODE"}
    $result=@{schema=1;status='finished';message='Session process finished. See the game session report for exit/profile evidence.'}
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

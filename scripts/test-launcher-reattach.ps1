$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'launcher-files.ps1')
$root=Split-Path $PSScriptRoot -Parent
$fixture=Join-Path $root ('work/launcher-reattach-tests/'+[Guid]::NewGuid().ToString('N'))
$runId='20261129-000000-001-'+[Guid]::NewGuid().ToString('N').Substring(0,8)
$run=Join-Path $fixture ('work/launcher-runs/'+$runId)
[void][IO.Directory]::CreateDirectory($run)
if($null -ne (Open-CoopActiveRun $fixture)){throw 'Invented absent worker'}
$requestPath=Join-Path $run 'request.json'
Write-CoopRunJson $requestPath @{schema=1;role='host';address='127.0.0.1';game_directory='fixture'}
$child=Join-Path $fixture 'worker.ps1'
$ready=Join-Path $fixture 'ready'
$stop=Join-Path $fixture 'stop'
[IO.File]::WriteAllText($child,@'
param([string]$Ready,[string]$Stop)
[IO.File]::WriteAllText($Ready,'ready')
$deadline=[DateTime]::UtcNow.AddSeconds(30)
while(![IO.File]::Exists($Stop) -and [DateTime]::UtcNow -lt $deadline){[Threading.Thread]::Sleep(25)}
exit 17
'@,(New-Object Text.UTF8Encoding($true)))
$info=New-Object Diagnostics.ProcessStartInfo
$info.FileName=Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
$info.UseShellExecute=$false;$info.CreateNoWindow=$true
$info.Arguments='-NoLogo -NoProfile -File "'+$child+'" -Ready "'+$ready+'" -Stop "'+$stop+'"'
$worker=$null;$attached=$null
try {
    $worker=[Diagnostics.Process]::Start($info)
    $deadline=[DateTime]::UtcNow.AddSeconds(10)
    while(![IO.File]::Exists($ready) -and !$worker.HasExited -and [DateTime]::UtcNow -lt $deadline){[Threading.Thread]::Sleep(25)}
    if(![IO.File]::Exists($ready)){throw 'Fixture worker did not initialize'}
    Save-CoopActiveRun $fixture $run $worker
    $attached=Open-CoopActiveRun $fixture
    if(!$attached -or $attached.process.Id -ne $worker.Id -or $attached.directory -ne $run -or
        $attached.request.role -ne 'host'){throw 'Exact worker reattachment failed'}
    Write-Output 'PASS exact PID/start time/path and request reattachment'
    $pointer=Join-Path $fixture 'work/active-launcher-run.json'
    $saved=Read-CoopRunJson $pointer
    $changed=Read-CoopRunJson $pointer
    $changed.worker_start_ticks=([long]$changed.worker_start_ticks+1).ToString()
    Write-CoopRunJson $pointer $changed
    if($null -ne (Open-CoopActiveRun $fixture)){throw 'Reused PID identity accepted'}
    $changed.run_id='../outside'
    Write-CoopRunJson $pointer $changed
    $rejected=$false
    try {Open-CoopActiveRun $fixture | Out-Null} catch {$rejected=$true}
    if(!$rejected){throw 'Traversal run identifier accepted'}
    Write-CoopRunJson $pointer $saved
    Write-CoopRunJson $requestPath @{schema=1;role='unexpected'}
    $rejected=$false
    try {Open-CoopActiveRun $fixture | Out-Null} catch {$rejected=$true}
    if(!$rejected){throw 'Invalid attached request accepted'}
    Write-CoopRunJson $requestPath @{schema=1;role='host'}
    Write-Output 'PASS stale identity, traversal and invalid request refusal'
    [IO.File]::WriteAllText($stop,'stop')
    if(!$worker.WaitForExit(10000)){throw 'Fixture worker failed to exit normally'}
    if(!$attached.process.WaitForExit(10000) -or $attached.process.ExitCode -ne 17){throw 'Attached process exit code lost'}
    if($null -ne (Open-CoopActiveRun $fixture)){throw 'Exited worker attached'}
    Write-Output 'PASS retained exit code and expired reference refusal'
} finally {
    # Request normal fixture exit only; never terminate an arbitrary process.
    [IO.File]::WriteAllText($stop,'stop')
    if($attached){$attached.process.Dispose()}
    if($worker){[void]$worker.WaitForExit(10000);$worker.Dispose()}
}
Write-Output "Fixtures=$fixture"

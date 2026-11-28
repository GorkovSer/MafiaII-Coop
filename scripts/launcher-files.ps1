# Small run metadata only. Readers share delete access so atomic replacement works.
function Read-CoopRunJson([string]$Path,[int]$LimitBytes=8192) {
    if($LimitBytes -lt 1 -or $LimitBytes -gt 4194304){throw 'Invalid run metadata size limit'}
    $stream=[IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,
        ([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
    try {
        if($stream.Length -gt $LimitBytes){throw 'Run metadata exceeds its size limit'}
        # Bound the actual read too: another writer can grow the file after Length.
        $bytes=New-Object byte[] ($LimitBytes+1)
        $count=0
        while($count -lt $bytes.Length) {
            $read=$stream.Read($bytes,$count,$bytes.Length-$count)
            if(!$read){break}
            $count+=$read
        }
        if($count -gt $LimitBytes){throw 'Run metadata exceeds its size limit'}
        $offset=0
        if($count -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191){$offset=3}
        $utf8=New-Object Text.UTF8Encoding($false,$true)
        $json=$utf8.GetString($bytes,$offset,$count-$offset)
        if([string]::IsNullOrWhiteSpace($json)){throw 'Run metadata is empty'}
        if(!$json.TrimStart().StartsWith('{')){throw 'Run metadata must be a JSON object'}
        $value=ConvertFrom-Json -InputObject $json -ErrorAction Stop
        if($null -eq $value -or $value -isnot [Management.Automation.PSCustomObject]) {
            throw 'Run metadata must be a JSON object'
        }
        return $value
    } finally {$stream.Dispose()}
}
function Write-CoopRunJson([string]$Path,$Value) {
    $temporary=$Path+'.'+[Guid]::NewGuid().ToString('N')+'.tmp'
    try {
        $json=$Value | ConvertTo-Json -Depth 6
        [IO.File]::WriteAllText($temporary,$json,(New-Object Text.UTF8Encoding($false)))
        # Windows PowerShell converts ordinary $null to an empty string here.
        # Pass a real null backup filename to .NET rather than an invalid path.
        if([IO.File]::Exists($Path)){[IO.File]::Replace($temporary,$Path,[NullString]::Value)}
        else {[IO.File]::Move($temporary,$Path)}
    } finally {
        try {if([IO.File]::Exists($temporary)){[IO.File]::Delete($temporary)}} catch {}
    }
}
function Write-CoopSessionPhase([string]$SessionInfoPath,[string]$Phase,[string]$SessionId) {
    if(!$SessionInfoPath){return}
    # A progress-file problem must not interrupt the game or profile verification.
    try {
        $path=Join-Path (Split-Path $SessionInfoPath -Parent) 'session-state.json'
        Write-CoopRunJson $path @{schema=1;phase=$Phase;session_id=$SessionId;
            updated_utc=[DateTime]::UtcNow.ToString('o')}
    } catch {}
}
function New-CoopSessionFailure([string]$Code,[string]$Message,[Exception]$Cause) {
    $failure=if($Cause){New-Object InvalidOperationException($Message,$Cause)}
        else {New-Object InvalidOperationException($Message)}
    $failure.Data['CoopFailureCode']=$Code
    return $failure
}
function Save-CoopActiveRun([string]$Root,[string]$RunDirectory,$Process) {
    $runId=Split-Path $RunDirectory -Leaf
    if($runId -notmatch '\A\d{8}-\d{6}-\d{3}-[a-f0-9]{8}\z'){throw 'Unexpected launcher run identifier'}
    Write-CoopRunJson (Join-Path $Root 'work/active-launcher-run.json') @{
        schema=1;run_id=$runId;worker_pid=$Process.Id;
        worker_start_ticks=$Process.StartTime.ToUniversalTime().Ticks.ToString([Globalization.CultureInfo]::InvariantCulture)
    }
}
function Open-CoopActiveRun([string]$Root) {
    $path=Join-Path $Root 'work/active-launcher-run.json'
    if(!(Test-Path -LiteralPath $path)){return $null}
    $active=Read-CoopRunJson $path
    if($active.schema -ne 1 -or $active.run_id -notmatch '\A\d{8}-\d{6}-\d{3}-[a-f0-9]{8}\z' -or
        [string]$active.worker_start_ticks -notmatch '\A\d{1,19}\z' -or
        [string]$active.worker_pid -notmatch '\A[1-9]\d{0,9}\z'){throw 'Invalid active launcher reference'}
    $process=$null
    try {
        $process=Get-Process -Id ([int]$active.worker_pid) -ErrorAction SilentlyContinue
        if(!$process){return $null}
        $actual=$process.StartTime.ToUniversalTime().Ticks.ToString([Globalization.CultureInfo]::InvariantCulture)
        if($actual -cne [string]$active.worker_start_ticks -or $process.HasExited) {
            $process.Dispose();return $null
        }
        $allowed=@((Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'),
            (Join-Path $env:SystemRoot 'SysWOW64/WindowsPowerShell/v1.0/powershell.exe'))
        if($allowed -notcontains $process.Path){$process.Dispose();return $null}
        # Retain the handle so ExitCode remains available after an attached worker exits.
        $process.EnableRaisingEvents=$true
        $directory=Join-Path $Root ('work/launcher-runs/'+$active.run_id)
        $request=Read-CoopRunJson (Join-Path $directory 'request.json')
        if($request.schema -ne 1 -or $request.role -notin @('host','guest')){throw 'Invalid active run request'}
        return [pscustomobject]@{process=$process;directory=$directory;request=$request}
    } catch {if($process){$process.Dispose()};throw}
}

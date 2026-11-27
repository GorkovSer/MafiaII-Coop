# Private failure reports, not native engine minidumps. Reporting must never
# replace the original failure or prevent normal worker/profile cleanup.
function Write-CoopFailureReport {
    param([Parameter(Mandatory=$true)][string]$Root,
          [Parameter(Mandatory=$true)][Management.Automation.ErrorRecord]$Failure,
          [Parameter(Mandatory=$true)][string]$Kind,
          [string]$SessionDirectory)
    try {
        $directory=Join-Path $Root 'work/crashlogs'
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        $name=(Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-'+[Guid]::NewGuid().ToString('N')+'.json'
        $path=Join-Path $directory $name
        $report=[ordered]@{schema=1;kind=$Kind;time_utc=[DateTime]::UtcNow.ToString('o');
            message=$Failure.Exception.Message;exception=$Failure.Exception.ToString();
            error_id=$Failure.FullyQualifiedErrorId;script_stack=$Failure.ScriptStackTrace;
            launcher_run_directory=$SessionDirectory;native_dump=$false}
        [IO.File]::WriteAllText($path,($report | ConvertTo-Json -Depth 4),[Text.Encoding]::UTF8)
        return $path
    } catch {return $null}
}

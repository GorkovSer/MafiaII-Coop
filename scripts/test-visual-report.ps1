$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$cases=Join-Path $root ('work/report-tests/'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $cases -Force | Out-Null
function Report([string]$name,[string]$text) {
    $dir=Join-Path $cases $name
    New-Item -ItemType Directory -Path $dir | Out-Null
    [IO.File]::WriteAllText((Join-Path $dir 'probe.log'),$text)
    return (& (Join-Path $PSScriptRoot 'visual-session-report.ps1') -SessionDirectory $dir | ConvertFrom-Json)
}
$empty=Report empty ''
if(@($empty.network_timeline).Count -ne 0 -or $empty.network_events_without_tick -ne 0) {throw 'Empty log reported invented events'}
$old=Report legacy "network=timeout thread=1 sent=3 received=2`r`n"
if($old.network_events.timeout -ne 1 -or $old.network_events_without_tick -ne 1 -or
    $null -ne $old.network_timeline[0].tick_ms -or $null -ne $old.network_timeline[0].utc_ms) {throw 'Legacy timestamps were invented'}
$new=Report timed "network=ready_guest thread=1 sent=0 received=0 ms=500 utc_ms=1795850000000`r`nnetwork=timeout thread=1 sent=3 received=2 ms=10500 utc_ms=1795850010000`r`n"
if($new.network_events_without_tick -ne 0 -or $new.network_timeline[1].tick_ms -ne 10500 -or
    $new.network_timeline[1].utc_ms -ne 1795850010000 -or $new.network_timeline[1].received -ne 2 -or
    $new.network_timeline[0].event -ne 'ready_guest' -or $null -ne $new.launch_result) {throw 'Timed events were parsed incorrectly or running exit invented'}
$lines=1..300 | ForEach-Object {"network=retry_connecting thread=1 sent=0 received=0 ms=$_ utc_ms=$($_+1795850000000)"}
$bounded=Report bounded ($lines -join "`r`n")
if($bounded.network_events.retry_connecting -ne 300 -or $bounded.network_timeline_dropped -ne 44 -or
    @($bounded.network_timeline).Count -ne 256 -or $bounded.network_timeline[0].tick_ms -ne 45 -or
    $bounded.network_timeline[-1].tick_ms -ne 300) {throw 'Bounded timeline lost counts or ordering'}
Write-Output 'PASS empty/legacy/timed/bounded network timeline; no invented exit or timestamp evidence'
Write-Output "Fixtures=$cases"

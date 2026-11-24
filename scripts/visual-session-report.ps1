param([Parameter(Mandatory=$true)][string]$SessionDirectory,
      [ValidateRange(0,60)][int]$WaitForExitSeconds=0)
$ErrorActionPreference='Stop'
# Read-only evidence summary. A running session has no exit/profile verdict yet.
$directory=(Resolve-Path -LiteralPath $SessionDirectory).Path
$resultPath=Join-Path $directory 'launch-result.json'
$exitResult=$null
$wait=[Diagnostics.Stopwatch]::StartNew()
do {
    if(Test-Path -LiteralPath $resultPath) {
        try {$exitResult=Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json}
        catch {if($WaitForExitSeconds -eq 0) {throw}}
        if($null -ne $exitResult -and $exitResult.waited_for_exit -eq $true) {break}
    }
    if($wait.Elapsed.TotalSeconds -ge $WaitForExitSeconds) {break}
    Start-Sleep -Milliseconds 50
} while($true)
$log=Join-Path $directory 'probe.log'
if((Get-Item -LiteralPath $log).Length -gt 8MB) {throw 'Probe log exceeds the bounded report size'}
$text=[IO.File]::ReadAllText($log)
$events=[regex]::Matches($text,'(?m)^visual_test=(\w+) value=(\d+) ms=(\d+) thread=\d+\r?$')
$counts=@{}
$ownership=@{}
foreach($event in $events) {
    $name=$event.Groups[1].Value; $id=$event.Groups[2].Value
    if(!$counts.ContainsKey($name)) {$counts[$name]=0}
    $counts[$name]++
    if($name -in @('creation_registry_confirmed','release_requested','release_returned','removal_registry_confirmed')) {
        if(!$ownership.ContainsKey($id)) {$ownership[$id]=@{created=0;requested=0;returned=0;absent=0}}
        $field=switch($name) {
            creation_registry_confirmed {'created'}
            release_requested {'requested'}
            release_returned {'returned'}
            removal_registry_confirmed {'absent'}
        }
        $ownership[$id][$field]++
    }
}
$samples=[regex]::Matches($text,'(?m)^visual_sample ms=(\d+) phase=\d+ xyz=([^\r\n]+)\r?$')
$span=if($samples.Count -ge 2) {
    [long]$samples[$samples.Count-1].Groups[1].Value-[long]$samples[0].Groups[1].Value
} else {$null}
$balanced=$null
if($null -ne $exitResult -and $exitResult.waited_for_exit -eq $true) {
    $balanced=$ownership.Count -gt 0
    foreach($entry in $ownership.Values) {
        if($entry.created -ne $entry.requested -or $entry.created -ne $entry.returned -or
           $entry.created -ne $entry.absent) {$balanced=$false}
    }
}
$network=[regex]::Matches($text,'(?m)^network=(\w+) [^\r\n]*')
$networkState=if($network.Count) {$network[$network.Count-1].Groups[1].Value} else {$null}
$networkCounts=@{}
foreach($event in $network) {
    $name=$event.Groups[1].Value
    if(!$networkCounts.ContainsKey($name)) {$networkCounts[$name]=0}
    $networkCounts[$name]++
}
[pscustomobject]@{
    schema=1
    evidence_only=$true
    visual_samples=$samples.Count
    visual_sample_span_ms=$span
    visual_events=$counts
    model_lifetimes=$ownership
    model_lifetimes_balanced_after_exit=$balanced
    last_network_event=$networkState
    network_events=$networkCounts
    launch_result=$exitResult
    exit_evidence_wait_timed_out=($WaitForExitSeconds -gt 0 -and
        ($null -eq $exitResult -or $exitResult.waited_for_exit -ne $true))
    requires_visual_observation=$true
    proves_two_game_clients=$false
} | ConvertTo-Json -Depth 6

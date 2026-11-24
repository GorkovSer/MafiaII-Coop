param([string]$BuildDirectory='build/visual-network-20261124')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$exe=Join-Path (Join-Path $root $BuildDirectory) 'visual_bridge_host.exe'
$caseRoot=Join-Path $root ('work/visual-network-tests/'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $caseRoot -Force | Out-Null
$port=Get-Random -Minimum 32000 -Maximum 45000
$key=('0'*63)+'1'
function New-VisualCase([string]$name,[string]$role,[string]$sceneKey,[int]$flag=1) {
    $dir=Join-Path $caseRoot $name
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
    $log=Join-Path $dir 'probe.log'; [IO.File]::WriteAllText($log,'')
    [IO.File]::WriteAllText((Join-Path $dir 'coop-session.ini'),
        "[network]`r`nrole=$role`r`naddress=127.0.0.1`r`nport=$port`r`nvisual_only=$flag`r`nscene_key=$sceneKey`r`n")
    return $log
}
$eventsLog=New-VisualCase 'event-policy' 'host' $key
& (Join-Path (Join-Path $root $BuildDirectory) 'visual_events_host.exe') $eventsLog
if($LASTEXITCODE -ne 0) {throw 'Visual event policy fixture failed'}
foreach($bad in @('',('0'*63),('g'*64),('0'*64))) {
    $badLog=New-VisualCase ('bad-'+[Guid]::NewGuid().ToString('N')) 'host' $bad
    & $exe $badLog 0 2
    if($LASTEXITCODE -ne 0) {throw 'Invalid scene key was not refused'}
}
$badLog=New-VisualCase 'bad-flag' 'host' $key 2
& $exe $badLog 0 2
if($LASTEXITCODE -ne 0) {throw 'Invalid visual mode flag was not refused'}
foreach($match in @($true,$false)) {
    $prefix=if($match){'matching'}else{'mismatching'}
    $guestKey=if($match){$key}else{('0'*63)+'2'}
    $logs=@((New-VisualCase "$prefix-host" 'host' $key),(New-VisualCase "$prefix-guest" 'guest' $guestKey))
    $expected=if($match){1}else{0}
    $processes=@()
    for($i=0;$i -lt 2;$i++) {
        $processes+=Start-Process -FilePath $exe -ArgumentList @(('"'+$logs[$i]+'"'),'5000',"$expected") `
            -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $caseRoot "$prefix-$i.txt") `
            -RedirectStandardError (Join-Path $caseRoot "$prefix-$i.err")
    }
    foreach($process in $processes) {$process.WaitForExit(); if($process.ExitCode -ne 0){throw "Visual scope fixture failed: $prefix"}}
    for($i=0;$i -lt 2;$i++) {Get-Content (Join-Path $caseRoot "$prefix-$i.txt")}
    $port++
}
Write-Output 'PASS visual scene match/mismatch, withdrawal, physical authority isolation and stopped-zero'
Write-Output 'Synthetic loopback fixtures only; no Mafia process or two-PC acceptance.'

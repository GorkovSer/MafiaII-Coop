param([string]$BuildDirectory='build/network')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$exe=Join-Path (Join-Path $root $BuildDirectory) 'session_peer.exe'
& (Join-Path (Join-Path $root $BuildDirectory) 'test_wire.exe')
if($LASTEXITCODE -ne 0) {throw 'Wire tests failed'}
& (Join-Path (Join-Path $root $BuildDirectory) 'test_remote_snapshot.exe')
if($LASTEXITCODE -ne 0) {throw 'Receiver validity tests failed'}
& (Join-Path (Join-Path $root $BuildDirectory) 'test_world_sync.exe')
if($LASTEXITCODE -ne 0) {throw 'World agreement tests failed'}
$dir=Join-Path $root ('work/network-tests/'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $dir -Force | Out-Null
function Start-Peer($name,$role,$port,$duration,$mode=0) {
    $process=Start-Process -FilePath $exe -ArgumentList @($role,'127.0.0.1',"$port","$duration","$mode") -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $dir "$name.txt") -RedirectStandardError (Join-Path $dir "$name.err")
    return $process
}
function Finish-Peer($process,$name) {
    if(!$process.WaitForExit(15000)) {throw "Fixture still running: $name"}
    if($process.ExitCode -ne 0) {throw "Fixture failed: $name"}
    return Get-Content (Join-Path $dir "$name.txt") -Raw
}
$port=Get-Random -Minimum 30000 -Maximum 50000
$hostPeer=Start-Peer good-host host $port 5000
Start-Sleep -Milliseconds 200
$guestPeer=Start-Peer good-guest guest $port 3500
$guestLog=Finish-Peer $guestPeer good-guest
$hostLog=Finish-Peer $hostPeer good-host
foreach($log in @($hostLog,$guestLog)) {
    if($log -notmatch 'connections=1 ' -or $log -notmatch 'remote_fixture_seen=1 ' -or
       $log -notmatch 'event=remote_inactive' -or $log -notmatch 'received=[1-9]\d* ') {throw 'Bidirectional fixture failed'}
}
Write-Output 'PASS two processes handshake, assign IDs, exchange state and clear inactive state'
$port++
$hostPeer=Start-Peer world-host host $port 6500 5
Start-Sleep -Milliseconds 200
$guestPeer=Start-Peer world-guest guest $port 6000 5
$guestLog=Finish-Peer $guestPeer world-guest
$hostLog=Finish-Peer $hostPeer world-host
foreach($log in @($hostLog,$guestLog)) {
    if($log -notmatch 'world_seen=1 world_withdrawn=1 world_recovered=1' -or
       $log -notmatch 'rejected=0 ') {throw 'World barrier fixture failed'}
}
Write-Output 'PASS synthetic world agreement, mismatch withdrawal and recovery (not gameplay)'
$port++
$hostPeer=Start-Peer silence-host host $port 6000
Start-Sleep -Milliseconds 200
$guestPeer=Start-Peer silence-guest guest $port 4500 4
$guestLog=Finish-Peer $guestPeer silence-guest
$hostLog=Finish-Peer $hostPeer silence-host
if($hostLog -notmatch 'event=remote_expired' -or $hostLog -notmatch 'remote_expired_while_ready=1') {
    throw 'Missing receiver expiry while ENet peer remained ready'
}
Write-Output 'PASS incoming state expires while peer still services ENet control traffic'
foreach($mode in @(1,2,3)) {
    $port++
    $hostPeer=Start-Peer "reject-host-$mode" host $port 2500
    Start-Sleep -Milliseconds 200
    $guestPeer=Start-Peer "reject-guest-$mode" guest $port 1500 $mode
    $guestLog=Finish-Peer $guestPeer "reject-guest-$mode"
    $hostLog=Finish-Peer $hostPeer "reject-host-$mode"
    if($hostLog -notmatch 'event=reject_compatibility' -or $guestLog -notmatch 'connections=0 ' -or
       $guestLog -notmatch 'received=0 ') {throw 'Incompatible peer accepted'}
}
Write-Output 'PASS mismatched game build, module version and capability set rejected before state exchange'
$port++
$hostPeer=Start-Peer reconnect-host host $port 6000
Start-Sleep -Milliseconds 200
$first=Start-Peer reconnect-first guest $port 1500
$firstLog=Finish-Peer $first reconnect-first
Start-Sleep -Milliseconds 200
$second=Start-Peer reconnect-second guest $port 1500
$secondLog=Finish-Peer $second reconnect-second
$hostLog=Finish-Peer $hostPeer reconnect-host
if($hostLog -notmatch 'connections=2 ' -or $firstLog -notmatch 'connections=1 ' -or $secondLog -notmatch 'connections=1 ') {
    throw 'Sequential guest reconnect failed'
}
Write-Output 'PASS disconnected guest can reconnect with a fresh peer and reset receive state'
foreach($invalid in @('invalid-hostname','01.2.3.4')) {
    & $exe guest $invalid 40000 10 > (Join-Path $dir 'invalid.txt')
    if($LASTEXITCODE -ne 3) {throw 'Invalid or ambiguous IPv4 endpoint accepted'}
}
Write-Output 'PASS non-IPv4 and ambiguous numeric endpoint rejected without DNS lookup'
$port++
$bridge=Join-Path (Join-Path $root $BuildDirectory) 'network_bridge_host.exe'
$peers=@()
foreach($role in @('host','guest')) {
    $private=Join-Path $dir "bridge-$role"
    New-Item -ItemType Directory -Path $private -Force | Out-Null
    $log=Join-Path $private 'probe.log'
    [IO.File]::WriteAllText($log,'')
    [IO.File]::WriteAllText((Join-Path $private 'coop-session.ini'),"[network]`r`nrole=$role`r`naddress=127.0.0.1`r`nport=$port`r`n")
    $peers+=Start-Process -FilePath $bridge -ArgumentList @(('"'+$log+'"'),'5000') -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $dir "bridge-$role.txt") -RedirectStandardError (Join-Path $dir "bridge-$role.err")
    Start-Sleep -Milliseconds 200
}
for($i=0;$i -lt 2;$i++) {
    $role=@('host','guest')[$i]
    $log=Finish-Peer $peers[$i] "bridge-$role"
    if($log -notmatch 'ready=1 saw_active=1 saw_inactive=1 received=[1-9]\d*' -or
        $log -notmatch 'stopped_snapshot_zero=1') {throw 'Worker/mailbox fixture failed'}
}
Write-Output 'PASS network worker maintains session while producer stalls and expires stale position'
Write-Output 'These are loopback transport fixtures, not two-PC Radmin or gameplay acceptance.'

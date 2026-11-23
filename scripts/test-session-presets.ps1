$ErrorActionPreference='Stop'
$wrapper=Join-Path $PSScriptRoot 'start-diagnostic-session.ps1'
$base=@{GameDirectory='C:/NoGameForPresetTest';TestProfileFrom='20000101-000000-000';PrepareOnly=$true}
$cases=@(
    @{Preset='LoopbackHost';Role='host';Address=$null;Expected='127.0.0.1'},
    @{Preset='LoopbackGuest';Role='guest';Address=$null;Expected='127.0.0.1'},
    @{Preset='radminhost';Role='host';Address='26.1.2.3';Expected='26.1.2.3'},
    @{Preset='RadminGuest';Role='guest';Address='26.4.5.6';Expected='26.4.5.6'}
)
foreach($case in $cases) {
    $config=& $wrapper @base -Preset $case.Preset -Address $case.Address
    if($config.NetworkRole -ne $case.Role -or $config.Address -ne $case.Expected -or
        $config.Port -ne 28960 -or !$config.Observer -or !$config.SecondScreen -or
        !$config.WaitForExit -or $config.TestProfileFrom -ne $base.TestProfileFrom) {
        throw ('Unexpected preset configuration: '+$case.Preset)
    }
}
foreach($address in @($null,'example.com','224.1.2.3','0.0.0.1','255.255.255.255','::1')) {
    $refused=$false
    try { & $wrapper @base -Preset RadminGuest -Address $address | Out-Null }
    catch { $refused=$true }
    if(!$refused) {throw 'Invalid Radmin endpoint accepted'}
}
$refused=$false
try { & $wrapper @base -Preset LoopbackHost -Address '26.1.2.3' | Out-Null }
catch { $refused=$true }
if(!$refused) {throw 'Loopback preset accepted another interface'}
Write-Output 'PASS four preset configurations and seven endpoint refusals; no game access'

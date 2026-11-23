param(
    [Parameter(Mandatory=$true)][string]$GameDirectory,
    [Parameter(Mandatory=$true)]
    [ValidateSet('LoopbackHost','LoopbackGuest','RadminHost','RadminGuest')][string]$Preset,
    [string]$Address,
    [ValidateRange(1,65535)][int]$Port=28960,
    [Parameter(Mandatory=$true)]
    [ValidatePattern('^\d{8}-\d{6}-\d{3}$')][string]$TestProfileFrom,
    [string]$BuildDirectory='build/probe',
    [switch]$PrepareOnly
)
$ErrorActionPreference='Stop'
$isRadmin=$Preset -like 'Radmin*'
if($isRadmin -and [string]::IsNullOrWhiteSpace($Address)) {
    throw 'Specify the host Radmin IPv4 for a guest, or this PC Radmin IPv4 for a host'
}
if(!$isRadmin) {
    if($Address -and $Address -ne '127.0.0.1') {throw 'Loopback presets use 127.0.0.1'}
    $Address='127.0.0.1'
}
$parsed=$null
if($Address -notmatch '^(0|[1-9]\d{0,2})(\.(0|[1-9]\d{0,2})){3}$' -or
    ![Net.IPAddress]::TryParse($Address,[ref]$parsed) -or
    $parsed.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork -or
    $Address -eq '0.0.0.0' -or $Address -eq '255.255.255.255') {
    throw 'Provide a literal unicast IPv4; a host binds only its selected interface'
}
if($parsed.GetAddressBytes()[0] -eq 0 -or $parsed.GetAddressBytes()[0] -ge 224) {
    throw 'Multicast and reserved IPv4 ranges cannot be session endpoints'
}
$role=if($Preset -like '*Host'){'host'}else{'guest'}
$launch=@{
    GameDirectory=$GameDirectory; Observer=$true; SecondScreen=$true; WaitForExit=$true
    BuildDirectory=$BuildDirectory; TestProfileFrom=$TestProfileFrom
    NetworkRole=$role; Address=$Address; Port=$Port
}
if($PrepareOnly) { [pscustomobject]$launch; return }
Write-Output 'Diagnostic telemetry session only; remote avatar and story co-op are not enabled'
& (Join-Path $PSScriptRoot 'start-probe.ps1') @launch

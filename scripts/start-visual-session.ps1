param(
    [Parameter(Mandatory=$true)][ValidateSet('host','guest')][string]$Role,
    [Parameter(Mandatory=$true)][string]$Address,
    [string]$GameDirectory='C:\Games\Mafia II Classic',
    [ValidateRange(1,65535)][int]$Port=28960,
    [string]$BuildDirectory='build/visual-network-20261124',
    [string]$SessionInfoPath,
    [switch]$SecondScreen, [switch]$PrepareOnly
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'launcher-files.ps1')
$Role=$Role.ToLowerInvariant()
$profilePrefix=Join-Path $root 'work/probe-sessions/20000101-000000-000/profile-localappdata'
if($profilePrefix.Length+80 -ge 260) {
    throw (New-CoopSessionFailure 'path_too_long' 'Extract the preview into a shorter folder, for example C:\MafiaCoop. No game started.')
}
$parsed=$null
if($Address -notmatch '^(0|[1-9]\d{0,2})(\.(0|[1-9]\d{0,2})){3}$' -or
    ![Net.IPAddress]::TryParse($Address,[ref]$parsed) -or
    $parsed.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork) {
    throw 'Use a literal IPv4 address: the host Radmin IP, or 127.0.0.1 for local diagnostics'
}
$octets=$parsed.GetAddressBytes()
if($octets[0] -eq 0 -or $octets[0] -ge 224) {throw 'Use a unicast IPv4 address'}
if($Role -eq 'host') {
    $localAddresses=@([Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces() |
        ForEach-Object {$_.GetIPProperties().UnicastAddresses} |
        ForEach-Object {$_.Address.IPAddressToString})
    if($localAddresses -notcontains $Address) {throw (New-CoopSessionFailure 'host_address_unavailable' 'Host address is not assigned to this PC. Use the host Radmin IP.')}
}
$game=Join-Path $GameDirectory 'pc/mafia2.exe'
$gameHash='16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f'
if(!(Test-Path -LiteralPath $game) -or (Get-FileHash -LiteralPath $game).Hash.ToLowerInvariant() -ne $gameHash) {
    throw (New-CoopSessionFailure 'unsupported_game' 'This preview requires the pinned Mafia II Classic executable; no game started')
}
$manifestPath=Join-Path $root 'visual-preview-manifest.json'
if(Test-Path -LiteralPath $manifestPath) {
    $manifest=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if($manifest.schema -ne 1 -or $manifest.module_version -ne 6 -or
        $manifest.mode -ne 'visual-only-preview' -or @($manifest.files).Count -lt 6) {throw 'Unexpected preview manifest'}
    if($BuildDirectory -ne $manifest.build_directory) {throw 'Do not mix this preview with another build directory'}
    $boundary=[IO.Path]::GetFullPath($root).TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar
    foreach($file in $manifest.files) {
        $path=[IO.Path]::GetFullPath((Join-Path $root $file.path))
        if(!$path.StartsWith($boundary,[StringComparison]::OrdinalIgnoreCase) -or
            !(Test-Path -LiteralPath $path -PathType Leaf) -or
            (Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant() -ne $file.sha256) {
            throw (New-CoopSessionFailure 'package_changed' 'Preview files are incomplete or changed; no game started')
        }
    }
}
foreach($name in @('probe_launcher.exe','coop_observer.dll')) {
    if(!(Test-Path -LiteralPath (Join-Path (Join-Path $root $BuildDirectory) $name))) {throw "Missing preview build file: $name"}
}
# Always start both players from the same bundled isolated save. Do not reuse an
# advanced user profile under the fixed first-scene approval label.
Write-CoopSessionPhase $SessionInfoPath 'preparing_profile'
try {$installed=@(& (Join-Path $PSScriptRoot 'install-test-save.ps1'))}
catch {throw (New-CoopSessionFailure 'profile_preparation_failed' $_.Exception.Message $_.Exception)}
$seed=@($installed | ForEach-Object {if($_ -match 'TestProfileFrom=(\d{8}-\d{6}-\d{3})') {$Matches[1]}})
if($seed.Count -ne 1) {throw 'Fresh isolated profile installation was not confirmed'}
$launch=@{GameDirectory=$GameDirectory;Observer=$true;WaitForExit=$true;
    TestProfileFrom=$seed[0];VisualNetwork=$true;NetworkRole=$Role;Address=$Address;
    Port=$Port;VisualScene='classic-alley-mvp-v1';BuildDirectory=$BuildDirectory;
    Windowed=$true;SecondScreen=[bool]$SecondScreen}
$plan=[pscustomobject]@{schema=1;role=$Role;address=$Address;port=$Port;
    scene='classic-alley-mvp-v1';test_profile_from=$seed[0];game_started=$false;
    original_profile_written=$false;arguments=$launch}
$plan | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $root "work/probe-sessions/$($seed[0])/visual-launch-plan.json")
if($PrepareOnly) {return $plan}
if($SessionInfoPath){$launch.SessionInfoPath=$SessionInfoPath}
Write-Output "Visual preview: $Role / $Address`:$Port. Both players: choose Continue from the supplied profile."
& (Join-Path $PSScriptRoot 'start-probe.ps1') @launch

$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'launcher-game.ps1')
$fixture=Join-Path $root ('work/game-discovery-tests/'+[Guid]::NewGuid().ToString('N'))
$game=Join-Path $fixture 'Games/Mafia II Classic'
$pc=Join-Path $game pc
New-Item -ItemType Directory -Path $pc -Force | Out-Null
$exe=Join-Path $pc mafia2.exe
[IO.File]::WriteAllText($exe,'fixture; never executed')
foreach($inputPath in @($game,$pc,($pc+'\'),$exe,('"'+$exe+'"'),('  '+$game+'  '))) {
    if((Resolve-CoopGameDirectory $inputPath) -ne $game){throw "Root normalization failed: $inputPath"}
}
foreach($inputPath in @('',(Join-Path $fixture missing),(Join-Path $pc 'other.exe'))) {
    if($null -ne (Resolve-CoopGameDirectory $inputPath)){throw 'Non-game path accepted'}
}
$fake=Join-Path $fixture unrelated
New-Item -ItemType Directory -Path $fake | Out-Null
[IO.File]::WriteAllText((Join-Path $fake mafia2.exe),'not in pc')
if($null -ne (Resolve-CoopGameDirectory (Join-Path $fake mafia2.exe))){throw 'Wrong executable layout accepted'}
$found=@(Find-CoopGameDirectories -Candidates @($game,$pc,$exe,(Join-Path $fixture missing)))
if($found.Count -ne 1 -or $found[0] -ne $game){throw 'Discovery did not deduplicate or ignored missing candidate'}
$steam=Join-Path $fixture Steam
New-Item -ItemType Directory -Path (Join-Path $steam steamapps) -Force | Out-Null
$library=Join-Path $fixture 'Other library'
$encoded=$library.Replace('\','\\')
[IO.File]::WriteAllText((Join-Path $steam 'steamapps/libraryfolders.vdf'),('"libraryfolders" { "1" { "path" "'+$encoded+'" } }'))
$libraries=@(Get-CoopSteamLibraryRoots $steam)
if($libraries.Count -ne 2 -or $libraries[1] -ne $library){throw 'Steam library path not decoded'}
Write-Output 'PASS root/pc/exe/quoted/trailing paths, missing/wrong layout, candidate deduplication and Steam library paths'
Write-Output "Fixtures=$fixture"

$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'build-provenance.ps1')
$repository=Split-Path $PSScriptRoot -Parent
$fixture=Join-Path $repository ('work/provenance-tests/'+[Guid]::NewGuid().ToString('N'))
$out=Join-Path $fixture 'build'
foreach($directory in @('src/net','src/launcher','third_party/enet/include','scripts','build')) {
    [void][IO.Directory]::CreateDirectory((Join-Path $fixture $directory))
}
foreach($file in @('src/net/example.c','src/launcher/frontend.c','src/launcher/probe_launcher.c','third_party/enet/include/example.h',
    'scripts/build-provenance.ps1','scripts/build-probe.ps1','scripts/build-enet.ps1','scripts/build-launcher.ps1')) {
    [IO.File]::WriteAllText((Join-Path $fixture $file),'fixture; not executed')
}
$artifact=Join-Path $out 'fixture.dll'
[IO.File]::WriteAllText($artifact,'binary fixture; not executed')
function Refuse([scriptblock]$Action,[string]$Description) {
    $rejected=$false
    try {& $Action | Out-Null} catch {$rejected=$true}
    if(!$rejected){throw "Invalid provenance accepted: $Description"}
}
Refuse {Assert-CoopBuildProvenance $fixture $out runtime @('fixture.dll')} 'absent record'
$record=Start-CoopBuildProvenance $fixture $out runtime
Refuse {Assert-CoopBuildProvenance $fixture $out runtime @('fixture.dll')} 'incomplete record'
Complete-CoopBuildProvenance $fixture $out $record @('fixture.dll')
[void](Assert-CoopBuildProvenance $fixture $out runtime @('fixture.dll'))
Refuse {Assert-CoopBuildProvenance $fixture $out frontend @('fixture.dll')} 'wrong build kind'
Write-Output 'PASS missing/incomplete/wrong-kind records refused; exact completed record accepted'
[IO.File]::AppendAllText($artifact,'changed')
Refuse {Assert-CoopBuildProvenance $fixture $out runtime @('fixture.dll')} 'changed artifact'
[IO.File]::WriteAllText($artifact,'binary fixture; not executed')
$source=Join-Path $fixture 'src/net/example.c'
[IO.File]::AppendAllText($source,'changed')
Refuse {Assert-CoopBuildProvenance $fixture $out runtime @('fixture.dll')} 'changed source'
[IO.File]::WriteAllText($source,'fixture; not executed')
$added=Join-Path $fixture 'src/net/new.h'
[IO.File]::WriteAllText($added,'new source')
Refuse {Assert-CoopBuildProvenance $fixture $out runtime @('fixture.dll')} 'added input'
# Keep each scenario in its own valid snapshot; no fixture deletion is needed.
$record=Start-CoopBuildProvenance $fixture $out runtime
[IO.File]::AppendAllText($source,'changed during build')
Refuse {Complete-CoopBuildProvenance $fixture $out $record @('fixture.dll')} 'source changed during build'
Refuse {Assert-CoopBuildProvenance $fixture $out runtime @('fixture.dll')} 'failed completion'
Write-Output 'PASS changed artifact/source, added input and mutation during build refused'
$record=Start-CoopBuildProvenance $fixture $out runtime
Complete-CoopBuildProvenance $fixture $out $record @('fixture.dll')
[IO.File]::AppendAllText((Join-Path $fixture 'src/launcher/frontend.c'),'frontend only')
[void](Assert-CoopBuildProvenance $fixture $out runtime @('fixture.dll'))
[IO.File]::AppendAllText((Join-Path $fixture 'src/launcher/probe_launcher.c'),'runtime launcher change')
Refuse {Assert-CoopBuildProvenance $fixture $out runtime @('fixture.dll')} 'changed native runtime launcher'
$record=Start-CoopBuildProvenance $fixture $out runtime
Complete-CoopBuildProvenance $fixture $out $record @('fixture.dll')
[IO.File]::AppendAllText((Join-Path $fixture 'scripts/build-probe.ps1'),'build script change')
Refuse {Assert-CoopBuildProvenance $fixture $out runtime @('fixture.dll')} 'changed build script'
Write-Output 'PASS runtime excludes frontend and binds its actual build scripts'
Write-Output "Fixtures=$fixture"

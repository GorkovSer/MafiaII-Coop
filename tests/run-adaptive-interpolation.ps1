# Future opt-in runner. Do not invoke while the user's code-only hold is active.
param(
    [Parameter(Mandatory=$true)][string]$Toolchain,
    [string]$OutputDirectory='build/adaptive-interpolation',
    [switch]$ReplayTraces
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$cc=Join-Path $Toolchain 'bin/i686-w64-mingw32-clang.exe'
$version=& $cc --version
if($LASTEXITCODE -ne 0 -or $version[0] -notmatch 'clang version 23\.1\.5') {
    throw 'Expected pinned LLVM-MinGW 20261104 / clang 23.1.5'
}
$out=Join-Path $root $OutputDirectory
New-Item -ItemType Directory -Path $out -Force | Out-Null
$net=Join-Path $root 'src/net'
$sources=@((Join-Path $net 'remote_timeline.c'),(Join-Path $net 'remote_snapshot.c'),
    (Join-Path $net 'wire.c'),(Join-Path $net 'world_sync.c'))
$common=@('-std=c11','-Wall','-Wextra','-Werror','-static','-I',$net)
foreach($optimization in @('-O0','-O2')) {
    foreach($test in @('test_remote_timeline','test_interpolation_clock','test_adaptive_interpolation')) {
        $exe=Join-Path $out "$test$optimization.exe"
        & $cc @common $optimization (Join-Path $PSScriptRoot "$test.c") @sources -o $exe
        if($LASTEXITCODE -ne 0){throw "$test compilation failed; no successful result"}
        & $exe
        if($LASTEXITCODE -ne 0){throw "$test failed; no successful result"}
    }
}
if($ReplayTraces) {
    $tool=Join-Path $out 'interpolation_trace.exe'
    & $cc @common -O2 (Join-Path $PSScriptRoot 'interpolation_trace.c') @sources -o $tool
    if($LASTEXITCODE -ne 0){throw 'Trace tool compilation failed'}
    foreach($trace in @('stable','jitter','gap-burst','recovery','stop-turn','relocation')) {
        & $tool $trace (Join-Path $out $trace)
        if($LASTEXITCODE -ne 0){throw "Trace $trace failed"}
    }
}
Write-Output 'Completed requested synthetic scenarios; this is not in-game acceptance.'

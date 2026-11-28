param(
    [Parameter(Mandatory=$true)][string]$Toolchain,
    [string]$OutputDirectory='build/avatar-smoothing',
    [string]$BaselineRef
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
$timeline=Join-Path $net 'remote_timeline.c'
$include=$net
if($BaselineRef) {
    # Fixed read-only Git object lookup; never execute text obtained from Git.
    $baseline=Join-Path $out 'baseline'
    New-Item -ItemType Directory -Path $baseline -Force | Out-Null
    foreach($file in @('remote_timeline.c','remote_timeline.h')) {
        $content=& git -c "safe.directory=$($root.Replace('\','/'))" -C $root show "${BaselineRef}:src/net/$file"
        if($LASTEXITCODE -ne 0){throw "Cannot read baseline $file"}
        [IO.File]::WriteAllLines((Join-Path $baseline $file),[string[]]$content)
    }
    $timeline=Join-Path $baseline 'remote_timeline.c'
    $include=$baseline
}
$dependencies=@($timeline,(Join-Path $net 'remote_snapshot.c'),
    (Join-Path $net 'wire.c'),(Join-Path $net 'world_sync.c'))
$common=@('-std=c11','-Wall','-Wextra','-Werror','-static','-I',$include,'-I',$net)
$failed=0
foreach($optimization in @('-O0','-O2')) {
    $exe=Join-Path $out "test_remote_timeline$optimization.exe"
    & $cc @common $optimization (Join-Path $PSScriptRoot 'test_remote_timeline.c') @dependencies -o $exe
    if($LASTEXITCODE -ne 0){throw 'Timeline test build failed'}
    & $exe
    if($LASTEXITCODE -ne 0){++$failed}
}
if($BaselineRef) {
    if($failed){throw "Baseline failed in $failed optimization configurations (expected for regression reproduction)"}
    return
}
# Existing consumers/regressions, without changing their sources or global suite.
foreach($name in @('test_remote_actor','test_replication_foundation')) {
    $exe=Join-Path $out "$name.exe"
    $sources=@((Join-Path $PSScriptRoot "$name.c"),
        (Join-Path $root 'src/game/remote_actor.c'))
    if($name -eq 'test_replication_foundation') {
        $sources+=(Join-Path $root 'src/game/entity_map.c')
    }
    & $cc @common -O2 @sources @dependencies -o $exe
    if($LASTEXITCODE -ne 0){throw "$name build failed"}
    & $exe
    if($LASTEXITCODE -ne 0){++$failed}
}
if($failed){throw "$failed test configurations failed"}
Write-Output 'PASS: standalone timeline O0/O2 and existing actor/foundation regressions'

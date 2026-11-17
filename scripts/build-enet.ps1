param([Parameter(Mandatory=$true)][string]$Toolchain,
      [Parameter(Mandatory=$true)][string]$OutputDirectory)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$manifest=Get-Content (Join-Path $root 'dependencies/enet.json') -Raw | ConvertFrom-Json
$source=Join-Path $root 'third_party/enet'
foreach($entry in $manifest.files) {
    if ((Get-FileHash -LiteralPath (Join-Path $source $entry.path)).Hash.ToLowerInvariant() -ne $entry.sha256) {
        throw "Pinned ENet source changed: $($entry.path)"
    }
}
$out=Join-Path $root $OutputDirectory
New-Item -ItemType Directory -Path $out -Force | Out-Null
$cc=Join-Path $Toolchain 'bin/i686-w64-mingw32-clang.exe'
$ar=Join-Path $Toolchain 'bin/llvm-ar.exe'
$objects=@()
foreach($name in @('callbacks','compress','host','list','packet','peer','protocol','win32')) {
    $object=Join-Path $out "enet-$name.o"
    & $cc -std=c11 -O2 -I (Join-Path $source 'include') -c (Join-Path $source "$name.c") -o $object
    if ($LASTEXITCODE -ne 0) {throw "ENet compile failed: $name"}
    $objects += $object
}
& $ar rcs (Join-Path $out 'enet.a') @objects
if ($LASTEXITCODE -ne 0) {throw 'ENet archive failed'}

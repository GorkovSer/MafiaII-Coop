param([Parameter(Mandatory=$true)][string]$Toolchain)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$cc = Join-Path $Toolchain 'bin/i686-w64-mingw32-clang.exe'
if (!(Test-Path -LiteralPath $cc)) { throw 'LLVM-MinGW i686 compiler missing' }
$version = & $cc --version
if ($LASTEXITCODE -ne 0 -or $version[0] -notmatch 'clang version 23\.1\.5') {
    throw 'Expected pinned LLVM-MinGW 20261104 / clang 23.1.5'
}
$out = Join-Path $root 'build/probe'
New-Item -ItemType Directory -Path $out -Force | Out-Null
$common = @('-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-static')
& $cc @common -shared (Join-Path $root 'src/probe/probe.c') '-Wl,--kill-at' -o (Join-Path $out 'coop_probe.dll')
if ($LASTEXITCODE -ne 0) { throw 'Probe DLL build failed' }
& $cc @common -municode (Join-Path $root 'src/launcher/probe_launcher.c') -o (Join-Path $out 'probe_launcher.exe') -luser32
if ($LASTEXITCODE -ne 0) { throw 'Launcher build failed' }
& $cc @common -mwindows (Join-Path $root 'tests/probe_host.c') -o (Join-Path $out 'probe_host.exe')
if ($LASTEXITCODE -ne 0) { throw 'Probe test host build failed' }
Get-FileHash -LiteralPath (Join-Path $out 'coop_probe.dll'),(Join-Path $out 'probe_launcher.exe'),(Join-Path $out 'probe_host.exe') | Format-Table

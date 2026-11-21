param([Parameter(Mandatory=$true)][string]$Toolchain, [string]$OutputDirectory = 'build/probe')
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$cc = Join-Path $Toolchain 'bin/i686-w64-mingw32-clang.exe'
if (!(Test-Path -LiteralPath $cc)) { throw 'LLVM-MinGW i686 compiler missing' }
$version = & $cc --version
if ($LASTEXITCODE -ne 0 -or $version[0] -notmatch 'clang version 23\.1\.5') {
    throw 'Expected pinned LLVM-MinGW 20261104 / clang 23.1.5'
}
$out = Join-Path $root $OutputDirectory
New-Item -ItemType Directory -Path $out -Force | Out-Null
$common = @('-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-static')
& (Join-Path $PSScriptRoot 'build-enet.ps1') -Toolchain $Toolchain -OutputDirectory $OutputDirectory
$worldSource=Join-Path $root 'src/net/world_sync.c'
$netSources=@((Join-Path $root 'src/net/wire.c'),(Join-Path $root 'src/net/session.c'),$worldSource)
$registrySource=Join-Path $root 'src/game/entity_registry.c'
$commandSource=Join-Path $root 'src/game/command_queue.c'
$progressSource=Join-Path $root 'src/game/script_progress.c'
$remoteSource=Join-Path $root 'src/net/remote_snapshot.c'
$actorSource=Join-Path $root 'src/game/remote_actor.c'
$netLink=@('-I',(Join-Path $root 'third_party/enet/include'),(Join-Path $out 'enet.a'),'-lws2_32','-lwinmm','-lbcrypt')
& $cc @common -shared (Join-Path $root 'src/probe/probe.c') '-Wl,--kill-at' -o (Join-Path $out 'coop_probe.dll')
if ($LASTEXITCODE -ne 0) { throw 'Probe DLL build failed' }
& $cc @common -DCOOP_OBSERVER -shared (Join-Path $root 'src/probe/probe.c') (Join-Path $root 'src/probe/observer.c') (Join-Path $root 'src/probe/profile.c') (Join-Path $root 'src/game/telemetry.c') (Join-Path $root 'src/game/story_test.c') (Join-Path $root 'src/game/model_test.c') (Join-Path $root 'src/game/human_test.c') $registrySource $commandSource $progressSource $actorSource (Join-Path $root 'src/net/game_bridge.c') $remoteSource @netSources '-Wl,--kill-at' -o (Join-Path $out 'coop_observer.dll') -ladvapi32 @netLink
if ($LASTEXITCODE -ne 0) { throw 'Event observer DLL build failed' }
& $cc @common -DCOOP_PROFILE_TEST -shared (Join-Path $root 'src/probe/probe.c') (Join-Path $root 'src/probe/profile.c') '-Wl,--kill-at' -o (Join-Path $out 'coop_profile_probe.dll')
if ($LASTEXITCODE -ne 0) { throw 'Profile redirect test DLL build failed' }
& $cc @common -mwindows (Join-Path $root 'tests/profile_host.c') -o (Join-Path $out 'profile_host.exe') -lshell32
if ($LASTEXITCODE -ne 0) { throw 'Profile test host build failed' }
& $cc @common -municode (Join-Path $root 'src/launcher/probe_launcher.c') (Join-Path $root 'src/launcher/window.c') (Join-Path $root 'src/launcher/user_process.c') -o (Join-Path $out 'probe_launcher.exe') -luser32 -lpsapi -ladvapi32
if ($LASTEXITCODE -ne 0) { throw 'Launcher build failed' }
& $cc @common -mwindows (Join-Path $root 'tests/probe_host.c') -o (Join-Path $out 'probe_host.exe')
if ($LASTEXITCODE -ne 0) { throw 'Probe test host build failed' }
& $cc @common (Join-Path $root 'tests/test_wire.c') (Join-Path $root 'src/net/wire.c') $worldSource -o (Join-Path $out 'test_wire.exe')
if ($LASTEXITCODE -ne 0) {throw 'Wire test build failed'}
& $cc @common (Join-Path $root 'tests/test_world_sync.c') $worldSource -o (Join-Path $out 'test_world_sync.exe')
if ($LASTEXITCODE -ne 0) {throw 'World agreement fixture build failed'}
& $cc @common (Join-Path $root 'tests/test_remote_actor.c') $actorSource $remoteSource -o (Join-Path $out 'test_remote_actor.exe')
if ($LASTEXITCODE -ne 0) {throw 'Remote actor fixture build failed'}
& $cc @common -DCOOP_TELEMETRY_TEST (Join-Path $root 'tests/telemetry_host.c') (Join-Path $root 'src/game/telemetry.c') $progressSource -o (Join-Path $out 'telemetry_host.exe')
if ($LASTEXITCODE -ne 0) {throw 'Telemetry fixture build failed'}
& $cc @common (Join-Path $root 'tests/test_script_progress.c') $progressSource -o (Join-Path $out 'test_script_progress.exe')
if ($LASTEXITCODE -ne 0) {throw 'Script progress fixture build failed'}
& $cc @common (Join-Path $root 'tests/test_entity_registry.c') $registrySource -o (Join-Path $out 'test_entity_registry.exe')
if ($LASTEXITCODE -ne 0) {throw 'Entity registry fixture build failed'}
& $cc @common (Join-Path $root 'tests/test_command_queue.c') $commandSource -o (Join-Path $out 'test_command_queue.exe')
if ($LASTEXITCODE -ne 0) {throw 'Command queue fixture build failed'}
& $cc @common -mwindows (Join-Path $root 'tests/input_host.c') -o (Join-Path $out 'input_host.exe') -ldinput8 -ldxguid -luser32
if ($LASTEXITCODE -ne 0) {throw 'Optional input diagnostic build failed'}
& $cc @common -municode (Join-Path $root 'src/tools/native_input.c') -o (Join-Path $out 'coop_input.exe') -ladvapi32 -luser32
if ($LASTEXITCODE -ne 0) {throw 'Native menu input helper build failed'}
& $cc @common (Join-Path $root 'tests/session_peer.c') @netSources -o (Join-Path $out 'session_peer.exe') @netLink
if ($LASTEXITCODE -ne 0) {throw 'Session fixture build failed'}
& $cc @common -municode (Join-Path $root 'tests/network_bridge_host.c') (Join-Path $root 'src/net/game_bridge.c') $remoteSource @netSources -o (Join-Path $out 'network_bridge_host.exe') @netLink
if ($LASTEXITCODE -ne 0) {throw 'Network thread fixture build failed'}
& $cc @common (Join-Path $root 'tests/test_remote_snapshot.c') $remoteSource @netSources -o (Join-Path $out 'test_remote_snapshot.exe') @netLink
if ($LASTEXITCODE -ne 0) {throw 'Remote validity fixture build failed'}
& $cc @common (Join-Path $root 'src/tools/netcheck.c') @netSources -o (Join-Path $out 'coop_netcheck.exe') @netLink
if ($LASTEXITCODE -ne 0) {throw 'Passive network diagnostic build failed'}
Get-FileHash -LiteralPath (Join-Path $out 'coop_probe.dll'),(Join-Path $out 'probe_launcher.exe'),(Join-Path $out 'probe_host.exe') | Format-Table

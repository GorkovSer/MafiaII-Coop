param([string]$BuildDirectory='build/visual-network-20261124', [string]$OutputDirectory='dist',
      [string]$FrontendDirectory='build/launcher')
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$commit=(& git -C $root rev-parse HEAD).Trim()
if($LASTEXITCODE -ne 0) {throw 'Source commit not available'}
$pending=@(& git -C $root status --porcelain --untracked-files=normal)
if($LASTEXITCODE -ne 0) {throw 'Cannot verify the source worktree'}
if($pending.Count) {throw 'Commit source changes before packaging so the manifest identifies the exact source'}
$stamp=Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$stage=Join-Path $root "work/visual-package/$stamp/MafiaII-Coop-visual-preview"
if(Test-Path -LiteralPath $stage) {throw 'Package staging already exists; no overwrite performed'}
$files=@('scripts/coop-launcher.ps1','scripts/launcher-session.ps1',
    'scripts/launcher-diagnostics.ps1','scripts/launcher-status.ps1','scripts/launcher-game.ps1',
    'scripts/start-visual-session.ps1','scripts/start-probe.ps1',
    'scripts/install-test-save.ps1','scripts/launch-result.ps1','scripts/visual-session-report.ps1',
    'testdata/story-save.zip','testdata/story-save.json',
    'third_party/enet/LICENSE','docs/VISUAL_MVP.md','docs/LAUNCHER.md','docs/TWO_PC_TEST.md')
$binaryDirectory='build/visual-network-20261124'
$frontend=Join-Path (Join-Path $root $FrontendDirectory) 'MafiaII-Coop.exe'
if(!(Test-Path -LiteralPath $frontend -PathType Leaf)){throw 'Build the GUI entry point with scripts/build-launcher.ps1 before packaging'}
foreach($file in $files) {
    $source=Join-Path $root $file
    if(!(Test-Path -LiteralPath $source -PathType Leaf)) {throw "Missing package input: $file"}
}
foreach($name in @('probe_launcher.exe','coop_observer.dll')) {
    $source=Join-Path (Join-Path $root $BuildDirectory) $name
    if(!(Test-Path -LiteralPath $source -PathType Leaf)) {throw "Missing binary: $name"}
}
New-Item -ItemType Directory -Path $stage -Force | Out-Null
Copy-Item -LiteralPath $frontend -Destination (Join-Path $stage 'MafiaII-Coop.exe')
foreach($file in $files) {
    $destination=Join-Path $stage $file
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $root $file) -Destination $destination
}
$files+='MafiaII-Coop.exe'
foreach($name in @('probe_launcher.exe','coop_observer.dll')) {
    $relative="$binaryDirectory/$name"; $destination=Join-Path $stage $relative
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path (Join-Path $root $BuildDirectory) $name) -Destination $destination
    $files+=$relative
}
$readme=@'
# Mafia II Coop: visual preview 0.0.6

LAUNCHER DRAFT: visible startup and connection behavior are NOT accepted yet.
The latest changes were reviewed statically; functional tests await user approval.
Do not treat this archive as a newly verified release.
Russian step-by-step instructions: docs/TWO_PC_TEST.md.

Experimental fixed-scene preview: visible network model, movement and facing.
No walk animation, collision, combat, vehicles or shared story progress yet.
Two actual clients passed local movement/pause/disconnect/rejoin checks on one PC.
Mission replay safely withdraws both models; restart the preview to reapprove.
Real two-PC/Radmin and sustained quality/FPS acceptance remain pending.

Both PCs need the pinned Mafia II Classic game and must be on the same Radmin VPN.
Extract into a short path such as C:\MafiaCoop; keep the entire folder layout.
Double-click MafiaII-Coop.exe to open the separate connection window, without CMD.
Choose host or guest, enter the HOST's Radmin IPv4, select the game folder,
then launch. The launcher always uses port 28960; there is no port selection.
Both players use the same HOST IP. No in-game menu is installed.
The draft launcher uses the same isolated-profile launch scripts below.
Original saves are not transferred; both choose Continue from the control save.
Closing the launcher leaves the game running. Exit Mafia II normally.
The Crash logs button opens work/crashlogs, even before starting a session.
Failure reports contain launcher/worker exceptions and session log paths;
native engine minidumps are not automatically collected. No monitor checkbox.

Command-line alternative: open PowerShell in this folder.

Host (use the Radmin IP assigned to the HOST computer):
  .\scripts\start-visual-session.ps1 -Role host -Address 26.X.X.X -GameDirectory 'C:\Games\Mafia II Classic'

Wait for the host game window to open, then start the guest.
Guest (use that SAME host Radmin IP):
  .\scripts\start-visual-session.ps1 -Role guest -Address 26.X.X.X -GameDirectory 'C:\Games\Mafia II Classic'

Replace the example IP/path with the real values. Both players choose Continue
from the supplied fresh profile, then one player should step aside a few paces.
Original saves are separate. Stay in the initial
scene. Pausing withdraws the model and resuming restores it. Reloading/changing
the scene ends approval: close and restart the preview on both PCs.
The visual guest retries after a transport timeout or disconnect, with a
two-second delay between attempts. Old poses are cleared before reconnecting.
An observed protocol refusal blocks retries until the guest is restarted.
Reconnection never renews approval after reloading/changing the scene.

Use -SecondScreen to put the game on a second monitor. -PrepareOnly creates the
isolated profile and checks files without starting the game. Private diagnostics
are written under work/probe-sessions. This package contains no original game EXE
or game assets; the same previously authorized control save is included.

After closing the game, summarize a session without starting the game:
  .\scripts\visual-session-report.ps1 -SessionDirectory .\work\probe-sessions\SESSION_ID
The report summarizes evidence; an active session has no exit/save verdict.
'@
[IO.File]::WriteAllText((Join-Path $stage 'README.txt'),$readme,[Text.Encoding]::UTF8)
$files+='README.txt'
$records=@(foreach($file in $files) {
    $path=Join-Path $stage $file
    [pscustomobject]@{path=$file;sha256=(Get-FileHash -LiteralPath $path).Hash.ToLowerInvariant();bytes=(Get-Item -LiteralPath $path).Length}
})
$manifest=[pscustomobject]@{schema=1;mode='visual-only-preview';module_version=6;protocol=3;
    build_directory=$binaryDirectory;source_commit=$commit;launcher_validation='unverified-user-test';files=$records}
$manifest | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $stage 'visual-preview-manifest.json')
$out=Join-Path $root $OutputDirectory
New-Item -ItemType Directory -Path $out -Force | Out-Null
$zip=Join-Path $out "MafiaII-Coop-launcher-draft-0.0.6-$stamp.zip"
if(Test-Path -LiteralPath $zip) {throw 'Package already exists; no overwrite performed'}
Compress-Archive -LiteralPath $stage -DestinationPath $zip
[pscustomobject]@{archive=$zip;sha256=(Get-FileHash -LiteralPath $zip).Hash.ToLowerInvariant();
    stage=$stage;file_count=$records.Count;source_commit=$commit}

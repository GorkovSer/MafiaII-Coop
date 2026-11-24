# Visual-only MVP

The user approved a visible network-driven model without walk animation or
collision for the first MVP. This is not the former physical Human backend.
No Human/Player input, AI, physics or script-movement function is used on it.

`-TestVisualAvatar` explicitly enables one isolated offline batch. It rejects
networking and other native diagnostics. The DLL independently rejects conflicting
configuration. The default observer does not create anything.

The pinned FrameWrapper (type 0x37, size 0x88, vtable 0x196d6e0) owns one cloned
local-player model. Common game-entity setup 0x11a31b0 and activation 0x1192170
register and display it. The model reference transfers once through SetModel and
flag 0x800. Native visual position/direction setters update only that owned entity.
Identity, registry, game thread, isolated profile and model pointer are checked.
Release abandons the handle before the native call; registry absence confirms
retirement. No explicit destructor/free follows it. Scene-close invalidation is
preserved across reentry; a foreign-thread callback cannot discard ownership.

On 2026-11-24, one real career run combined appearance, movement, in-place turn,
stationary hold, removal, local-player input and profile/exit checks. The clone
was visible at two observed positions. Thirty model samples covered all four
phases, with exactly 2 units travel on X and Y, stable Z, 0 -> pi/2 facing and
stationary phases below 0.002 units spread. One Release and one registry absence
were observed. Local input released with focus retained. Native quit took
1656 ms; child/launcher exit 0, original profile changed files 0.
Private session: 20261124-034633-203. Actual local-player visual heading was also
read successfully. Full pinned build, loader/profile checks and four invalid
launch combinations passed.

Not yet accepted: network-driven visual avatar, two game clients/two PCs,
live-avatar scene unload, longer sessions or arbitrary models/scenes. The batch
is not a playable co-op claim. Next: reuse this proven model lifecycle for a
fresh, explicitly agreed fixed-scene visual session; keep physical actor/world
authority separate and fail closed on stale state or scene change.


## Network-driven batch (2026-11-24)

VisualNetwork now reuses the accepted FrameWrapper lifecycle. It takes fresh
ReadRemoteVisual snapshots, bounded existing interpolation, explicit facing and
current scope identity. It owns one model, retires on withdrawal/expiry/pause or
connection/world change, and waits for registry absence before another creation.
Only nearby poses (20 horizontal / 3 vertical units) are accepted. Scene agreement
is manual, visual-only and valid for the first scene load; reload/change requires
a new process/approval. Local Vito is not moved or locked by this renderer.

The accepted final native batch used session 20261124-042020-189 / PID 24020,
which has exited. One actual game exchanged data with an explicitly synthetic
peer: visible model, position/facing, app-state stall while ENet remained live,
expiry/retirement, recovery, deliberate peer reconnect, pause/cleanup, resume/new
binding, independent local Up input, and exit initiated with a live model.
88 accepted model samples; six creations, six single releases and six confirmed
registry absences; zero timeline refusals. Native quit took 1594 ms, child and
launcher exit 0, original profile changed files 0. A separate wrong-scene peer in
the preceding batch created no model. Real-game heading reading was accepted.

Two issues found in earlier combined runs were corrected before acceptance:
receiver-clock recomputation changed the same packet timestamp, and a trailing
paused-frame callback relatched pause after explicit unpause. Production fixtures
cover stable receipt, scope isolation and event ordering; the latest event fixture
also covers first-load approval and refusal to automatically approve a new scene.
Full pinned build, baseline network suite, visual match/mismatch and five invalid
configuration cases passed. Four unsafe visual-network launch combinations were
refused before launch. Escape was added to the PID/start-time/hash/foreground
bounded input helper and used for intro/pause; unrestricted input was not added.

This remains one game plus a synthetic peer, NOT two game clients or two PCs.
Real two-player/Radmin play, long-session stability and a simple distributable
launcher remain to be accepted. Animation, collision, combat, vehicles, mission
sync and shared saves remain outside this first MVP.


## Portable preview (2026-11-24)

scripts/start-visual-session.ps1 accepts only role/address plus optional game path,
port and second screen. It verifies the pinned game and packaged file hashes,
rejects overly long legacy profile paths and unassigned host addresses, normalizes
role casing, installs the same fresh isolated save, then delegates to start-probe.
PrepareOnly performs checks/profile preparation without starting a game. No user
save is adopted and no execution policy or firewall setting is changed.

scripts/package-visual-mvp.ps1 produces a ZIP with 11 manifest-listed files:
two custom binaries, launch/install helpers, the already authorized control save,
ENet license, instructions and evidence. There is no original game EXE or game
asset. Its manifest records file SHA256/size and source commit. Both players
should start with Continue, then one should step aside because the same initial
save puts both local players at the same coordinates.

Portable checks passed: extraction into a short path with spaces; host/guest
preparation and case normalization; Windows PowerShell 5 compatibility; six bad
endpoints, missing game and long path refused; a changed DLL refused before a
profile was created. The candidate archive itself then launched a REAL guest
against a synthetic host. The visible model bound successfully. Native mission
replay produced scene_approval_ended at event 32 and load_finished=2 without a
new model; the first-load consent boundary is now native-tested too. Native quit
1641 ms, child/launcher exit 0 and original profile changed files 0.
Private portable session: 20261124-044747-289 (PID 2568 exited).

Thus actual host and guest game paths have each been exercised separately against
synthetic peers, including launch from the archive. This is still not simultaneous
two-game/two-PC/Radmin acceptance or a long-play stability claim. The next required
acceptance needs another supported game client. The archive remains a visual
preview, not a full co-op release.

Build a package after building the pinned binaries and committing source changes
(packaging refuses a dirty source tree before creating an archive):

```powershell
./scripts/package-visual-mvp.ps1
```

From its extracted short folder, use the host computer's Radmin IPv4 on BOTH PCs:

```powershell
./scripts/start-visual-session.ps1 -Role host -Address 26.X.X.X
./scripts/start-visual-session.ps1 -Role guest -Address 26.X.X.X
```

These are two separate commands, one per PC. Replace the example IP with the
real host IP. GameDirectory defaults to C:\Games\Mafia II Classic and can be
supplied explicitly. The guest's own Radmin IP is not the connection target.

Start the guest after the host game window opens. A guest that times out before
the host starts must be closed and restarted; this preview has no automatic
reconnect.

For a compact, read-only session summary, use:

```powershell
./scripts/visual-session-report.ps1 -SessionDirectory ./work/probe-sessions/SESSION_ID
```

This reports samples, visual lifecycle events and per-model release/registry
counts. Exit/profile evidence remains null until the launcher has finished.
Balanced ownership is reported only after a completed native wait and does not
constitute visual or two-client gameplay acceptance.
For automated batches, `-WaitForExitSeconds 10` waits for the launcher's final
evidence before reading the log; an expired wait remains explicitly unknown.

### Packaged session stability

Additional accepted packaged guest run (2026-11-24): session
`20261124-050945-461`, package source `b9a56d565e3750a53cbb3ddb16cffabbb9cb58a5`.
Actual game connected to a synthetic host; the visible model was observed in the
alley. The run recorded 636 visual samples over 318390 ms, pause/resume and exit
with a live model. Three creations each had one release request, return and
registry absence confirmation. Native exit took 1563 ms; child/launcher exits
were zero and original profile changes were zero. During the final three-minute
sampling interval private bytes increased by 73728; this short observation is
not a leak-free or long-play guarantee. No game/fixture was left running.

An earlier launch in this batch timed out because the guest preceded the host;
it produced no visual samples and exited normally. It is retained as a negative
case for the session report, not counted as a successful visual run. The report
was also checked against an active session (unknown exit verdict), the earlier
88-sample accepted run, and Windows PowerShell 5.

The additional packaged HOST run `20261124-051745-124` used the assigned Radmin
adapter address, a synthetic guest and a normal Windows PowerShell 5 launch.
The avatar was observed in the alley. One combined run accepted application-state
expiry/recovery, guest disconnect/rejoin, local player input, pause/resume and
exit with an active model: 664 samples over 334219 ms; five complete model
ownership cycles, no visual refusals, two peer connections. Native exit took
1688 ms, child/launcher exits were zero and original profile changes were zero.
The bounded-wait report collected final evidence successfully and no test
process remained. Both endpoints were still on this PC: this verifies local
binding to the Radmin adapter, not a remote VPN path or two-client gameplay.

### Next acceptance: two actual game clients

Use the same archive on both PCs. Record its SHA256 and both manifest source
commits before starting. Start the host first, then the guest using the host's
Radmin address. Both choose Continue from the isolated control save and stay in
the initial alley. One player steps aside to separate the overlapping spawns.

Perform these checks in one paired run:

1. Walk a few paces and turn while the other player watches; exchange roles.
   Each receiver must show movement and standing turns in the correct direction,
   while its own player remains independently controllable.
2. Pause/resume each game in turn. The remote visual should withdraw during a
   pause and return after resume. No duplicate should remain behind.
3. Keep both connected for several minutes, within the preview's nearby range.
   Observe both screens; logs alone do not establish smooth or usable movement.
4. Close the guest normally while the host remains in the alley. The host must
   remove the guest model. Restart the guest to rejoin and repeat a short move.
5. Replay the mission on one client. Its approval must end; further visual
   spawning requires restarting the preview. Then close both games normally.

Run the session report on each PC after exit. Preserve both private session
folders, observed failures, model lifetime counts and launcher/profile results.
Only a paired run with visual observations on both clients can satisfy this
acceptance; the existing synthetic-peer runs cannot substitute for it.

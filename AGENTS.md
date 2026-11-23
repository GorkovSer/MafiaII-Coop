# Project instructions

Latest user override (2026-11-23, resumed at 22:50 Moscow): tests and GitHub
publication are authorized again; publish verified stages roughly every fifteen
minutes and report each successful update in chat. Do not repeatedly reread
instructions or dump large tool output. Stopping Computer Use stops UI control,
not source work/console checks; continue independent work, without bypassing the
UI interruption. The earlier interrupted loader suite remains historical; the
new foundation build completed all ten production fixtures and nine loader/profile
checks. Native avatar physical-stop and two-PC campaign acceptance remain absent.

User-authorized destination: `https://github.com/GorkovSer/MafiaII-Coop.git`, branch `development`.

Shared handoff: read `docs/COAUTHOR_HANDOFF.md` before starting work and before EVERY commit. The user explicitly requires keeping coauthor instructions current. In the same commit, update that file whenever the task, ownership, behavior, verified results, limitations or next action changes. If nothing relevant changed, review it without inventing a status update. Keep `TESTING.md` as the detailed evidence record; the handoff is the concise current state. Fetch before integration, preserve the other author's commits and never overwrite concurrent work.

Before game UI testing read `docs/GAME_UI.md`. User explicitly requests repeated native Enter from startup until loading begins, and native Alt+F4 followed IMMEDIATELY by repeated Enter for exit. Use `scripts/enter-story.ps1` and `scripts/exit-game.ps1`, bounded to our isolated diagnostic session. Do not insert screenshot/tool waits between Alt+F4 and Enter or re-run completed input diagnostics. Observe unexpected outcomes before retrying.

After each substantive completed change/stage, inspect the diff, run appropriate checks, review the exact staged file set, commit with an accurate message, push without force, and verify that `refs/heads/development` on origin equals the local commit. Do not request confirmation for each commit/push. Do not change repository visibility/access. Use existing authorization without printing credentials. See `docs/GITHUB.md`.

Never add original game assets/executables, saves/backups, private logs/dumps, credentials, temporary clones, build products, or materials without redistribution permission. Keep these in ignored `work/` or other ignored local directories. Review tracked files too; .gitignore cannot protect an already tracked secret.

Explicit later user override (2026-11-18): upload the user's tested save and instruct the coauthor to download/install it. The reviewed exception is ONLY `testdata/story-save.zip` plus its manifest/instructions. See `docs/TEST_SAVE.md`; install into a new isolated profile, preserve the user's original profile and do not publish other saves/backups/logs. Native quit is also explicitly authorized: immediate Alt+F4 followed by repeated Enter in our diagnostic game, implemented by `coop_input quit` without an intervening screenshot.

The full task is `docs/REQUIREMENTS.md`: two-player MAIN STORY co-op in Mafia II Classic through Radmin VPN, with host progression, NPC/combat/vehicle state, transitions and saves. Free roam, a loaded DLL, a compiled build or a protocol test does not satisfy it. Record actual evidence and untested capabilities separately. Do not copy the old multiplayer projects' freeride startup or patches disabling saves/story systems.

Target currently audited: GOG EXE SHA256 `16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f`. Revalidate before using game addresses. Back up discovered saves/config before tests. Do not call unvalidated SDK functions or initialize the SDK's bulk patch set.

Current code is a diagnostic x86 DLL/launcher with read-only telemetry and an ENet prototype. Build: `scripts/build-probe.ps1 -Toolchain <pinned LLVM-MinGW directory>`. Verify: `python tests/test_audit_game.py`, `scripts/test-probe.ps1`, and `scripts/test-network.ps1`. Run loopback tests outside the network-isolated sandbox. Runtime evidence remains local; publish only a sanitized test report.

Explicit diagnostic switches now include `-TestModelClone` and `-TestHuman`, both disabled by default and requiring observer plus an earlier isolated test profile. Human creation/removal is documented in `docs/HUMAN_TEST.md`; it is a local lifecycle test, not a network avatar. Never call it two-client gameplay. Use registry GUID resolution to establish removal rather than reading a freed pointer or relying on camera position.

Before implementing Human movement read `docs/HUMAN_CONTROL.md` and the pinned static audit. The upstream 0x58 allocation + vtable 0x19661bc is incompatible with this EXE (destructor accesses +0x78); the actual MoveDir vtable is 0x18f14b0. Human+0x18c is the free-list head, not a command selector. Do not copy raw queue/free-list writes or manual command delete. Static command evidence is not runtime/AI-control acceptance.

Queue absence is not proof of movement stop or command deletion: Human+0x1a0 separately retains a command. Do not zero it or release its borrowed pointer manually. The dispatcher skip bit 0x200000 is not an audited AI-only switch. SDK AI NewCommand(void*) expects a different payload than raw intrusive MoveDir; do not pass one as the other.

Human+0xa8 is populated during base construction with a controller (vtable 0x1967228), despite an earlier null store. Native update and teardown dereference it without null checks. Never zero/replace/free it as an AI-disable shortcut. Controller vtable+0x14 is not a verified pause API; read the lifetime/update evidence in HUMAN_CONTROL.md before implementing ownership.

CoopReadHumanAi is read-only layout evidence; neither agent absence nor controller+0x170 flags authorize movement. Human ScriptSetControlStyleStr dispatches to a stub at 0x91aac0 for this concrete class. Human deactivation also deactivates physics; do not use it as AI-only suppression. The -TestHuman AI lifecycle observations passed two isolated game-process cycles on 2026-11-23; this does not authorize AI suppression or remote control.

Alternative movement candidate script_move.c uses native ScrMoveV -> AI NewCommand and owns a Sync ref. The explicit -TestHuman -TestScriptMove diagnostic now calls it on one isolated mod-owned Human, with networking OFF and no coop-session.ini. This offline test uses local loaded lifetime/profile/thread/registry authority; it cannot grant network world agreement. Every remote backend START still requires fresh agreed-world authority. The first live test accepted motion and a native abort request, but the Human continued moving after command state 2; physical stop/AI arbitration are NOT accepted. Keep production backend/world gates closed. See docs/SCRIPT_MOVE.md.

CoopScriptMoveRequestEnd uses the pinned native abort-flag setter 0xf3c9a0 on our linked raw-state-0 command. The earlier command-state-3 candidate is superseded: task update 0x40f340 consumes command+0x0d through 0xf3c9b0, separately from command state. END_REQUESTED and TERMINAL_OBSERVED never mean physical stop/task removal. Retain the owned Sync, observe and verify stop separately; do not wire either result directly to the remote actor stop callback. See the exact factory/task/cleanup chain in docs/SCRIPT_MOVE.md; no native invocation has been accepted yet.

User requires all Mafia II test launches in windowed mode on the second display. Use `scripts/start-probe.ps1 -Observer -SecondScreen -WaitForExit` with the intended build directory; video settings are changed only in the isolated profile. Do not run a fullscreen or primary-display game test. Never force-close an existing user game to make a new test possible.

Latest explicit user override (2026-11-18): fullscreen testing is now authorized to investigate the separate game/system cursor. Keep video changes inside the isolated profile. The user specifically requires loading the save through the normal menu using arrows and Enter; do not substitute the opt-in internal Continue backend for this test. Fullscreen authorization overrides the older window-only instruction above; it does not authorize changing the original profile.

Explicit user approval (2026-11-18): a separate native scan-code helper may send arrows and Enter to our diagnostic Mafia II process. Use coop_input only with the PID returned by our launcher, verified EXE hash and creation time, and the observed foreground game. Do not target an existing user game. This approval is the exception to the computer-use skill's usual exclusive UI path; it does not authorize arbitrary text, global hooks or OS changes.

Do not tag or publish a Release until an actual roadmap stage's acceptance criteria are met. Unstable accepted builds must be prereleases, archives must exclude game files/secrets, and their descriptions must explicitly state whether TWO-PC Radmin VPN gameplay was tested.

No background saving after this chat is configured.

Current testing authorization (2026-11-23): the user resumed tests and explicitly authorized all needed tests on the second display. This supersedes the 2026-11-18 Cyberpunk testing hold. The current registry-guarded Human adapter has passed two isolated game-process creation/Release/registry-absence cycles; the full loader/profile suite and previously deferred fixtures/network suite passed. This does not establish mission-reload lifetime, remote control, physical stop or two-PC gameplay. See TESTING.md and docs/COAUTHOR_HANDOFF.md for the current evidence; older dated hold/compile-only descriptions are historical.

Future remote-avatar consumers must use `CoopNetworkReadRemoteWorld` each update; `CoopNetworkReadRemote` is telemetry-only. The new protocol-2/module-0.0.4 world offer/ack/commit gate requires a verified loaded-world producer, which is not implemented yet. Never supply a guessed key from missionNumber/missionPart or a save request. See `docs/WORLD_AGREEMENT.md`. Ready/active, session/connection metadata and local epoch alone do not authorize movement. World agreement and receiver-expiry fixtures passed on 2026-11-23; agreement between two loaded game worlds remains unverified.

Save/load candidate at 0x1cc39c4 is written from an operation input before SAVE data result checks (0xb40835). Neither a catalogue match nor an observed load request confirms a loaded mission/checkpoint. Keep world authorization closed until a separate loaded-state producer and lifetime are verified; see `docs/STORY_IDENTITY.md`.

Static chapter-3 script analysis separates missionPart selection, story event ID and SaveGame slot. Do not assume missionNumber/missionPart uniquely identifies a checkpoint; see `docs/SCRIPT_CHECKPOINTS.md`. Runtime reachability and campaign-wide mapping remain unverified.

The user has explicitly delegated all further control AND tests while asleep. Do not wait for manual game/menu steps; use the computer-use skill for observed Windows UI actions when available, or continue independent implementation and report unverified runtime work accurately. This does not authorize claiming a loopback fixture/passive telemetry peer is a second game client or a two-PC Radmin test. `--user` lowers only the launcher's own child token; do not change OS security settings or raise the computer-use helper's privileges.

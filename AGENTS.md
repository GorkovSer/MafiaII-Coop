# Project instructions

User-authorized destination: `https://github.com/GorkovSer/MafiaII-Coop.git`, branch `development`.

Shared handoff: read `docs/COAUTHOR_HANDOFF.md` before starting work and before EVERY commit. The user explicitly requires keeping coauthor instructions current. In the same commit, update that file whenever the task, ownership, behavior, verified results, limitations or next action changes. If nothing relevant changed, review it without inventing a status update. Keep `TESTING.md` as the detailed evidence record; the handoff is the concise current state. Fetch before integration, preserve the other author's commits and never overwrite concurrent work.

After each substantive completed change/stage, inspect the diff, run appropriate checks, review the exact staged file set, commit with an accurate message, push without force, and verify that `refs/heads/development` on origin equals the local commit. Do not request confirmation for each commit/push. Do not change repository visibility/access. Use existing authorization without printing credentials. See `docs/GITHUB.md`.

Never add original game assets/executables, saves/backups, private logs/dumps, credentials, temporary clones, build products, or materials without redistribution permission. Keep these in ignored `work/` or other ignored local directories. Review tracked files too; .gitignore cannot protect an already tracked secret.

The full task is `docs/REQUIREMENTS.md`: two-player MAIN STORY co-op in Mafia II Classic through Radmin VPN, with host progression, NPC/combat/vehicle state, transitions and saves. Free roam, a loaded DLL, a compiled build or a protocol test does not satisfy it. Record actual evidence and untested capabilities separately. Do not copy the old multiplayer projects' freeride startup or patches disabling saves/story systems.

Target currently audited: GOG EXE SHA256 `16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f`. Revalidate before using game addresses. Back up discovered saves/config before tests. Do not call unvalidated SDK functions or initialize the SDK's bulk patch set.

Current code is a diagnostic x86 DLL/launcher with read-only telemetry and an ENet prototype. Build: `scripts/build-probe.ps1 -Toolchain <pinned LLVM-MinGW directory>`. Verify: `python tests/test_audit_game.py`, `scripts/test-probe.ps1`, and `scripts/test-network.ps1`. Run loopback tests outside the network-isolated sandbox. Runtime evidence remains local; publish only a sanitized test report.

User requires all Mafia II test launches in windowed mode on the second display. Use `scripts/start-probe.ps1 -Observer -SecondScreen -WaitForExit` with the intended build directory; video settings are changed only in the isolated profile. Do not run a fullscreen or primary-display game test. Never force-close an existing user game to make a new test possible.

Latest explicit user override (2026-11-18): fullscreen testing is now authorized to investigate the separate game/system cursor. Keep video changes inside the isolated profile. The user specifically requires loading the save through the normal menu using arrows and Enter; do not substitute the opt-in internal Continue backend for this test. Fullscreen authorization overrides the older window-only instruction above; it does not authorize changing the original profile.

Explicit user approval (2026-11-18): a separate native scan-code helper may send arrows and Enter to our diagnostic Mafia II process. Use coop_input only with the PID returned by our launcher, verified EXE hash and creation time, and the observed foreground game. Do not target an existing user game. This approval is the exception to the computer-use skill's usual exclusive UI path; it does not authorize arbitrary text, global hooks or OS changes.

Do not tag or publish a Release until an actual roadmap stage's acceptance criteria are met. Unstable accepted builds must be prereleases, archives must exclude game files/secrets, and their descriptions must explicitly state whether TWO-PC Radmin VPN gameplay was tested.

No background saving after this chat is configured.

The user has explicitly delegated all further control AND tests while asleep. Do not wait for manual game/menu steps; use the computer-use skill for observed Windows UI actions when available, or continue independent implementation and report unverified runtime work accurately. This does not authorize claiming a loopback fixture/passive telemetry peer is a second game client or a two-PC Radmin test. `--user` lowers only the launcher's own child token; do not change OS security settings or raise the computer-use helper's privileges.

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

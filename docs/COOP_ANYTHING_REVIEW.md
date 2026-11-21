# coop-anything: applicability to Mafia II Classic

Reviewed 2026-11-21 at the user's request:
[cranebeaker/coop-anything](https://github.com/cranebeaker/coop-anything).
This review read upstream pages and selected source files; no upstream tools,
MCP server, installer, tests or games were executed. Main-branch pages can
change; no immutable upstream commit was obtained. Pin and review the actual
files before importing anything. A clone was not obtained: the escalated call
timed out in approval review and the sandbox retry could not resolve GitHub.
Browser-backed source reads succeeded. No dependency has been added.

## Highest-value findings

| Material | Evidence read | Application here |
| --- | --- | --- |
| [Story effect capture](https://github.com/cranebeaker/coop-anything/blob/main/playbook/effect-capture-story-sync.md) | Design, not a Mafia implementation | Candidate design: host decides story changes; guest receives ordered effects. A matching progress number does not replicate doors, actors, rewards or other consequences. |
| [Actor ownership](https://github.com/cranebeaker/coop-anything/tree/main/tools/ownership) | README plus ownership.h and ownership.cpp | Reusable C++17 state machine separating actor owner and ownership epoch; old-owner rejection and staged handoff. Our engine adapter still has to stop local AI and apply remote state. |
| [Flight recorder](https://github.com/cranebeaker/coop-anything/tree/main/tools/flight) | README plus cpp/cofr.h | Candidate cross-machine event recorder/analyzer. Our current logs cannot alone locate a shared-world divergence because that world/transaction model does not yet exist. |
| [UDP chaos relay](https://github.com/cranebeaker/coop-anything/tree/main/tools/chaos) | README; relay implementation not obtained | Candidate external impairment layer for ENet/Radmin: loss, delay, reorder and outages. Does not require replacing ENet. Integration and actual behavior here remain untested. |
| [Hot reload](https://github.com/cranebeaker/coop-anything/tree/main/tools/hotreload) | README; drain implementation not obtained | Could reduce restart/load time later. Requires a permanent loader, replaceable core and a verified drain/shutdown contract. Do not unload our present DLL while callbacks/worker/engine references can still enter it. x86 compatibility has not been established. |

These are candidates with different evidence strength, not validated Mafia II
features. Upstream test claims have not been independently reproduced here.

## Concrete integration constraints

Ownership's published API uses host slot 0 and at most 8 slots. Our ENet session
assigns host/guest IDs 1/2. Any adoption needs an explicit mapping, not a changed
host constant guessed at individual call sites. It also needs game-neutral actor
IDs qualified by the agreed world, separate from local GUIDs/pointers. Its
ownership epoch cannot replace our connection generation or a story world epoch.

The inspected ownership source exposes Transfer/Ack/Commit and forced timeout
completion. That library decides authority; it does not transport an actor's
final state, prove the target loaded the world, or implement Mafia AI ownership.
For the first story slice, prefer a fixed host authority for NPCs. Defer dynamic
ownership migration until a single host simulation and guest replicas work.

The flight header uses a bounded ring, a C++ observer API and stable state hashes.
Adoption needs a C boundary for our present C sources and explicit recording at
network send/receive and game-thread application. Align comparisons by agreed
world/sequence boundaries; don't compare different machines' local clocks or
local epochs as if identical. Keep traces private like existing runtime logs.

Effect capture is an architectural option, not permission to suppress all guest
Lua. Mafia II scripts also drive local presentation/loading and player-specific
behavior. First classify primitives and dependencies for one mission; identify
capture/apply sites and replay suppression. No Lua hook or guest suppression is
implemented by this review. Keep request, application and saved-host progress
as separate acknowledgements. ENet reliability alone does not prove an effect
was applied or persisted.

## A more directly related research lead

The directory links [JustAddMP/MafiaDE](https://github.com/JustAddMP/MafiaDE).
Its README describes story-following, host world simulation and guest-save
suppression, but explicitly limits its published testing to two copies on one
PC and selected chapters. These are author reports, not our verification.
Developer notes describe a separate game layer and shared
[JustAddMP/core](https://github.com/JustAddMP/core).

This targets Mafia: Definitive Edition, NOT Mafia II Classic. Its x64/D3D11
framework and native game bindings cannot be installed as our x86 mod. Treat
failure/restart flow, save ownership, streaming distance and mission-car
deduplication as design references. Its source/LICENSE availability still needs
review before any reuse; coop-anything's MIT license does not license separately
linked projects. The directory's Mafia II entries themselves are M2-MP and
m2o-reborn, not newly verified full story co-op.

## Recommended order for our project

1. Complete a usable remote-player adapter and shared-world transition contract
   for the first real mission; don't let tooling research replace this milestone.
2. Survey that mission's story effects and decide the host/guest responsibilities
   using our pinned EXE evidence and later authorized real-game observations.
3. Add bounded, correlated send/receive/apply records; assess the flight format
   before creating another incompatible trace format.
4. After the user resumes tests, run baseline real-client checks, then use the
   chaos relay to exercise stale packets, failure/reload and reconnect.
5. Consider portable ownership code only when NPC application exists; consider
   hot reload only after safe callback/worker shutdown is proven.

[Upstream MIT license](https://github.com/cranebeaker/coop-anything/blob/main/LICENSE)
permits reuse with its copyright/license notice retained. No upstream code was
copied in this stage. No installation or contribution to upstream was performed.
The user's test hold and original two-PC Radmin mission/failure/host-save
acceptance criteria remain unchanged.

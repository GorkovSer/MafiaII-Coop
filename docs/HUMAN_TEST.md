# Explicit local Human lifecycle test

This is an opt-in development test, disabled by default. It creates one local
Human, not a network player. Use the observer, an explicit earlier isolated
test profile and `-TestHuman`; do not combine with `-TestModelClone`.

The audited GOG build is required. The test waits for game initialization,
load completion, an unpaused tick and three seconds of settling. It clones
the local player's model into a new owned allocation, creates type 0x0e
through the entity factory, attaches the clone, calls Setup, places the Human
beside the local player's audited physics position and calls Activate.

Ownership flag 0x800 is explicit: the destructor at 0x11a7cc0 releases +0x60
only with that flag. SetModel itself adds no reference. The test transfers
the clone's one reference to Human; it does not release that model separately.
It clears flag 0x40 after Setup, matching the upstream wrapper's effective
mask. The semantic name of that bit and its independent visual effect have
not been established; do not invent a flag name from this experiment.

After 60 seconds, or an earlier done/unload/shutdown callback, the test drops
its owned handle before one Release call. The current implementation first
resolves its GUID through the bounded native registry reader and requires the
returned address to equal its owned handle, then validates the concrete vtable
and full GUID. No read of the stored entity pointer precedes registry validation.
It never reads the freed entity afterwards or appends destructor/Free.
After Release it polls the registry for confirmed absence, with a ten-second
deadline; unreadable/corrupt memory and GUID mismatch are not confirmed removal.
The lookup has no game API calls or writes and checks the actual manager slot
against the pinned 0x5a0c70 lookup before interpreting its container.
Unexpected allocation contracts are incomplete diagnostics, not cleanup acceptance.
If the fresh entity cannot be validated, the still-owned clone is released but
no speculative entity destructor is called. Successful creation/removal of this
new registry-guarded version remains unverified in the game.

## Actual results

- Early model-transform placement passed creation, Setup, Activate and a
  returned Release, but screenshots did not establish a visible avatar.
- Position logs exposed the early checkpoint placement issue. With the
  settling delay and audited physics source, a visible Human appeared in the
  intended position beside Vito. Setup retained the transferred model;
  active=1, and model/physics positions agreed at creation and five seconds.
- The Human then moved under the game's own simulation. Remote control,
  animation synchronization and AI ownership are not implemented by this test.
- After Release, read-only native-layout GUID lookup found the local player
  present and the test Human absent. This proves registry removal in one
  lifecycle; no leak-free/repeated-world-lifetime claim is made.
- The passive receiver continued receiving real local-player telemetry.
  It is not a second game client and sends no invented remote-player state.

The read-only lookup is implemented in `scripts/entity_audit.py`, based on
0x5a0c70 -> 0x59b130 -> 0x58ebe0. It bounds collision traversal and overflow
storage, distinguishes invalid memory from absence, and verifies the full
GUID on a returned entity. `inspect_runtime.py --entity-guid <id>` stores
private pointers only in the ignored JSON report and prints status summaries.

The same audited layout is now implemented by `src/game/entity_registry.c` for
game-thread handle validation. Its production-code fixture verifies the pinned
hash bucket, collision chain, full-GUID generation mismatch, empty/deleted values,
unreadable memory, cycle, index and arithmetic bounds, reverse-order overflow
fallback and malformed vectors. A full pinned build passed; this fixture and
the production telemetry fixture passed. The loader/profile suite was interrupted
at the user's request to play Cyberpunk; remaining checks and the real-game test
are deferred. Do not launch or inject game input until testing is authorized again.

The newest version also reads command-queue topology at creation and five seconds,
after fresh registry validation, recording only status/free/used counts. The native
reader detects invalid indices, cycles, overlapping/free-orphaned slots, invalid
reference headers and changed raw layout. It never creates/changes commands. Its
fixture was compiled but not run during the test hold. No queue result from the
real game has been established; see [HUMAN_CONTROL.md](HUMAN_CONTROL.md).

Next: make creation/removal repeatable across mission reloads, establish AI
ownership and controlled position updates, then connect the real received
snapshot mailbox. Two-PC Radmin mission acceptance remains outstanding.

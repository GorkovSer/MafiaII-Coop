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
its owned handle before one Release call. It validates concrete vtable and
GUID before calling; it never reads the freed entity afterwards or appends
destructor/Free. Registry-based ownership checks must replace this limited
pointer guard before general network/world use. Unexpected allocation
contracts are incomplete diagnostics, not cleanup acceptance.

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

Next: make creation/removal repeatable across mission reloads, establish AI
ownership and controlled position updates, then connect the real received
snapshot mailbox. Two-PC Radmin mission acceptance remains outstanding.

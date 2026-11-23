# Player control investigation

Pinned EXE SHA256: `16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f`.

The concrete Player is entity type `0x10`, factory `0x430970`, allocation
`0x3d0`, constructor `0x42f9a0`, vtable `0x18f14b8`. Its native lock query
at vtable+`0x14c` is `0x41fcd0`: it returns whether Player+`0x310` equals 10.
The setter at slot+`0x168` is `0x438230`, unlike Human's stub. The pinned
byte audit validates the descriptor, allocation, vtable, query and setter path.

This is not an entity-local ownership switch established for remote use.
The setter invokes `0x10810b0`, `0x10822c0`, and conditionally `0x1082f30`
through a shared subsystem (`0x10810b0` returns global `0x1abcaac`). Player
deactivation also reaches that subsystem; its precise side effects are unverified.
Creating another Player and locking/deactivating it therefore needs a separate
side-effect and ownership audit. No Player factory, lock setter, destructor,
style setter or activation method was called by this stage.

`player_control.c` observes the query's inputs without executing game code.
It checks type/GUID, exact class and dispatch slots, address bounds, and repeated
identity/mode reads. Refusal clears output. The observer also rechecks the current
game/player pointers, then logs status/mode every 500 ms with other samples.
Mode 10 is only the native query's result: it does not prove physical stop,
exclusive AI ownership, a movement lease, or permission to mutate local Vito.
Nothing is added to the network protocol or remote actor authority.

Validation on 2026-11-23: pinned full build and audit passed; the production
telemetry fixture covers each read failing, identity/mode/dispatch mutation,
wrong GUID/type/class/slots and address overflow. The loader/profile suite passed.
In an isolated observer launch, the user loaded the career. Accepted Player
samples first included mode 10 followed by mode 0. After foreground activation,
a one-second native Up input moved Vito; its release and retained focus were
verified. All 100 control samples were accepted: mode 10 once, 0 in 94 samples,
1 in three, and 5 in two. These raw modes do not establish mode semantics beyond
the audited native lock query. This is read-only runtime layout acceptance, not
remote movement or a deliberate lock/unlock test. Native quit completed in
2 seconds, child/launcher exit 0; original profile changed files: 0.
Private session: `20261123-231437-105`.

Next: establish separately owned Player setup/removal and global input effects
before considering any opt-in spawn test. Existing Human stop and world gates
remain closed. See [SCRIPT_MOVE.md](SCRIPT_MOVE.md).

# Scripted movement through the Human AI scheduler

This is a new candidate route for the remote actor, not an enabled backend.
It follows the engine's own scripted command lifecycle instead of placing a
raw MoveDir underneath an autonomous AI producer. Full two-PC mission acceptance
is still outstanding, and the user test hold remains active.

## Source and pinned EXE evidence

The local Mafia2-Online sources show CNetworkPlayer::Create -> CreateWrapperPed
-> CreateEnginePed -> OBJTYPE_Human. SetPosition calls CM2Ped::MoveVec, which
calls HumanScript::ScrMoveV. CreateEnginePlayerPed and a ProcessKeyboard hook
exist separately; they do not establish that this network path uses Player.
The old wrapper also teleports over large distances; we are not importing that
policy, the source code, global patches or its save/story bypasses.

m2o-reborn's CHumanScript.hpp identifies ScrMoveV at 0x9910b0 for this build.
The exact-byte audit now verifies the following path in the pinned GOG EXE:

- ECX is HumanScript (Human+0xa4); its first word is Human. There are five
  stack arguments: out Sync ref, destination pointer, mode, direction pointer,
  smooth-stop flag. Native return is ret 0x14; mode meanings still need live
  acceptance. Pointer/float data are not passed by value.
- 0x9910d5 checks controller presence; subsequent branches check active Human
  and a special Player condition. 0x99114b requests a 0x8c AI command, using
  constructors 0xc36560 / 0xc36590 according to the second vector sentinel.
- 0x9911b0 links the command to its Sync object via 0xf5d130. 0x9911be applies
  the final boolean. 0xc368b0 builds 12-byte Sync user data, vtable 0x197bd94,
  holding the correct command/control-block pair. It increments the separate
  control block count, rather than treating Command+4 as that pointer.
- 0x9912e9 -> 0xf9f880 submits that two-word AI handle to Human+0xa8. It may
  enqueue or mark failure according to the agent's active flag; existence of a
  returned Sync object therefore does not prove scheduler acceptance. A near
  target/direction branch can complete without that enqueue path.
- 0x991304 calls 0xf3c770, which resets an agent scheduling field when present.
- Sync factory 0x11e2200 allocates 0x28 bytes, vtable 0x190cc0c, refcount+4.
  0x99131f copies an owned out reference through 0x429820, which retains it;
  temporary release is 0x422d40. Native internal OOM paths have not been
  accepted at runtime and are not made safe by this wrapper.

scripts/audit_human_commands.py checks these sites against the full pinned SHA.
Generated wrapper PE inspection also confirmed ECX=script, five stack pushes
and no extra caller pop after the native ret-20 call. This is static ABI
evidence, not proof that calling it in a mission works.

## Implemented candidate

src/game/script_move.c/.h compiles into the observer but has no callers there.
CoopScriptMoveStart requires a guard for pinned image/base/game thread,
mod-owned registry identity and fresh agreed-world authority. It independently
checks the Human/controller layout, active Human/agent, script back-pointer,
finite bounded vectors and candidate mode range 0..4. It keeps one owned Sync
reference and refuses another Start until that reference is released. Busy
protects both calls against recursive mutation through guards/native callbacks;
callers must still serialize access on the verified game thread.

HANDLE_CREATED means only the expected Sync ref header was observed. INCOMPLETE
may preserve a nonzero ref for later investigation/cleanup; never reset it or
overwrite it to retry. The code does not mutate target fields, intrusive counts,
AI/queue pointers or free lists directly. It does not suppress the Human's AI.
Whether scripted commands give the required per-actor arbitration remains a
live acceptance question; this route must not bypass the raw MoveDir guard.

CoopScriptMoveRelease drops only the caller-owned Sync ref via 0x422d40, with
the transfer performed before destructor callbacks. Its guard must allow that
release after world/actor withdrawal without reading a dead Human. It does not
cancel the command, call Sync Done, establish physical stop or prove destruction.
Never implement a remote actor stop callback by returning true after Release.

## Required next work

Trace native command cancellation and supersession from the Sync user data/
command listener path, including a physical-stop condition and reference balance.
The old source's raw writes to command+0x0d and its differently named Sync fields
are not accepted bindings for this EXE. Sync Done is a listener notification path;
do not assume it cancels AI work. When testing resumes, first use one isolated
mod-owned Human, verify command acceptance, motion, cancellation and teardown,
then connect the existing remote-actor controller behind verified world agreement.
No production loaded-world key producer exists yet, so that gate stays closed.

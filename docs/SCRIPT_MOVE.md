# Scripted movement through the Human AI scheduler

This is a new candidate route for the remote actor, not an enabled backend.
It follows the engine's own scripted command lifecycle instead of placing a
raw MoveDir underneath an autonomous AI producer. Full two-PC mission acceptance
is still outstanding. The isolated offline -TestScriptMove diagnostic invoked
Start/End/Release on 23 November. Motion and command abort were observed, but
continued movement failed physical-stop acceptance. The remote backend remains
unwired. Later dated compile-only/hold accounts record their original status.

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

src/game/script_move.c/.h compiles into the observer; the explicit offline
Human diagnostic is its sole caller. No network backend calls it.
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

## State observation and cancellation evidence (2026-11-21)

CoopScriptMoveObserve now reads the owned Sync -> typed userdata -> command
chain through script_move_state.c. It checks Sync vtable/positive intrusive
count, owned-userdata flag, userdata vtable 0x197bd94, AI move vtable 0x197bccc,
the separate positive command counter, state range 0..3 and any remaining
command-to-Sync link. Two snapshots must agree on relevant pointers/state.
Output clears on refusal. No game function, ref mutation or Human dereference
is needed; its guard authorizes observation of our owned Sync on the game
thread even after actor/world withdrawal. A valid snapshot is not physical stop.

The separate AI counter is **four bytes**, allocated by 0x9570a0. Do not read
a second count from counter+4: other native handle families have two counters,
but that does not apply to this one. Userdata retains/releases the single
counter at 0xc368d9 / 0xc4242c. The command owns a four-byte Sync wrapper at
command+4; native state setter 0xf5d210 writes state+8, notifies Sync Done and
then releases/clears that wrapper. The Sync flag is byte+8; upper bytes are
not initialized by the factory and are not reported by the reader.

Native 0xf5d290 performs conditional state changes, using 0xf5d210 when the
previous state is in 1..3. The raw state codes are reported without promoting
them to a gameplay-success/cancellation verdict. 0xf95100 finds a command by
identity in a subsystem's active tasks and queued commands; it calls a task's
slot+0x1c, destroys/removes that task, and marks queued matches with state 2.
Its owning subsystem and complete physical-stop contract are not established;
do not call it with Human/controller/agent merely because one has a pointer.

The old raw command+0x0d write is still rejected as a cancellation binding.
The pinned byte is initialized and serialized/deserialized at 0xf88914,
0xf5d307 and 0xf5d3b9; a bare setter at 0xf3c9a0 has no discovered direct
call/table references in this file. This does not prove it aborts a running task.
Next trace the scheduler's concrete target-removal owner/caller and its task
cleanup, then verify actual stop/ref balance when tests resume. The new callback
fixture covers valid/terminal raw states, wrong link/class/count, overflow,
missing owned userdata and changed snapshots; it was compiled, not executed.

## Earlier end request (2026-11-21, superseded on 23 November)

CoopScriptMoveRequestEnd now uses the command's native conditional state-change
routine at 0xf5d290 with value 3. This avoids guessing a target-removal subsystem
pointer. Native task propagation at 0x40dce0 maps task result 2 to command state
1 and task result 3 to command state 2; native message teardown at 0xf66497 also
requests command state 3 in a reference-count-dependent path. This supports a
candidate end request, not proof of scheduler cancellation or physical stop.

The END guard must verify image/thread and the owned Sync/command while still
allowing cleanup after world withdrawal. The implementation rechecks the native
signature and complete typed Sync ownership chain. States 1..3 return
TERMINAL_OBSERVED without modification. Only state 0 with a matching remaining
command-to-owned-Sync link is changed; the owned Sync reference is preserved.
Busy prevents recursive calls during guards/notification, and clears on normal
returns. There are no callback callers and no automatic network stop behavior.

For previous state 0, 0xf5d290 directly writes state 3; it does not invoke the
Sync notification/release path on that branch. Existing state 1..3 can reach
0xf5d210, but our wrapper deliberately preserves those results instead. Thus
END_REQUESTED can precede task cleanup and Sync Done, or fail to produce them;
later observation must not equate state 3, link absence or a Done flag with
stationary physics. Keep the ref until the actual lifecycle/stop is verified.
Do not use a true remote actor stop result until that proof exists.

The alternate 0xf95100 target-removal function remains unbound. In one inspected
vtable it is secondary-interface slot+0x4c and its deleting destructor adjusts
this by -0x40. Neither Human, controller nor the controller+4 brain vtable is
that interface. Next trace the concrete command task and its physical cleanup,
then validate the end request, refs and actual stop once tests resume.

## Native abort predicate and concrete task (2026-11-23)

The current RequestEnd no longer writes command state 3. Exact pinned evidence
now connects ScrMoveV's command to its concrete task, and shows a different
predicate that requests task termination:

- Command vtable 0x197bccc slot+4 -> 0xc0ebf0 returns type 4. Registration at
  0xb77695 maps type 4 to C_AIAction_SetMoveToBBRec. Its factory descriptor at
  0x1aad0d0 uses vtable 0x1980d40 -> 0xcaef60, allocating 0x70 bytes and calling
  0xc653e0. That constructor installs task vtable 0x197ce28.
- Task factory 0xfa3c40 resolves the command's type/name and retains its two-word
  command handle at task+0x28/+0x2c. Do not infer a task from the command pointer
  or pass Human/controller as a task/subsystem interface.
- Task slot+0x34 -> 0xca3ae0 calls 0xc77550 -> 0x40f340. The latter obtains
  command+0x0d through 0xf3c9b0. If zero and task state is neither 2 nor 3, it
  invokes task slot+0x14 with result 3. That slot is 0x40dce0, which propagates
  command state 2 through the notifying native setter. Directly writing command
  state 3 does not clear this predicate and is not an accepted abort mechanism.
- Native 0xf3c9a0 is exactly `mov byte ptr [ecx+0x0d],0; ret`. RequestEnd now
  verifies that signature and invokes this setter only on the validated linked
  raw-state-0 owned command. It preserves command state and the owned Sync ref;
  repeated requests are idempotent. No direct field write was added.
- Scheduler 0xf94fac updates task slot+0x34. Its state-2/3 branches call cleanup
  slot+0x1c -> 0xc43010, then destroy/remove the task unless separately retained.
  Cleanup unregisters/releases task+0x68, clears it, and calls 0xc22ea0, which
  calls slot+0x3c -> 0xc0c4f0 to unregister/free the task+0x60 record, then
  0x40ca20. Base destruction releases the retained command handle.

This is static reachability and ownership evidence, not physical-stop or runtime
reference-balance acceptance. The concrete task creates/updates a separate child
command through task+0x68; withdrawing records/listeners alone is insufficient
proof that movement/physics has stopped. Test command acceptance, actual motion,
abort request, scheduler/Sync cleanup and stationary position independently on
one owned Human before enabling a backend. The loaded-world gate remains closed.

The production-linked reentry fixture uses refusing guards to attempt Start,
End, Observe and Release recursively from every outer operation; it verifies
that recursive calls do not replace identity or drop the owned Sync token.
It never invokes a game binding. The earlier dated state-3 description above
records the superseded implementation, not the current request contract.

## First offline native acceptance (2026-11-23)

The user-authorized isolated Human diagnostic now has explicit TestScriptMove
opt-in. Its guard uses loaded local lifetime, pinned image/base/thread, redirected
profile and fresh owned registry identity, with networking prohibited at both
launcher and DLL configuration boundaries. This is a local acceptance exception;
it supplies no invented agreed-world key and cannot authorize a remote consumer.

Start returned HANDLE_CREATED. Native abort returned END_REQUESTED; snapshots
changed from command state 0/abort byte 1 to state 2/byte 0. The Human moved
before the request and continued moving afterwards: 42 samples at >=3.5 seconds
after Start span 0.16166 / 8.35553 / 0.04629 units. Therefore command termination
is not sufficient evidence of physical stop or exclusive movement ownership.
After native Human release, owned Sync release returned REFERENCE_RELEASED and
the registry confirmed absence; normal child exit 0 and original-profile changes 0.

Additional static evidence: the recursive task creates a type-3 MoveToPos child
(vtable 0x197b0c4), copies its parent through 0xc42520 -> 0xf3c950, including
abort byte +0x0d, and updates that copy before its own base abort check. The
MoveToPos task vtable is 0x197ca78; update 0xcba8b0 also calls 0x40f340, and
cleanup is 0xc42c70. These sites pass the exact-byte audit. They do not identify
the cause of post-abort motion: remaining child work and resumed autonomous AI
still need to be distinguished. Next capture task/child lifetime and AI ownership
around abort; do not substitute repeated teleports or zero the controller.

## LockControls wrapper rejected as a reversible Human policy

Pinned static inspection shows HumanScript::LockControls at 0x95cd80 querying
Human vtable+0x14c before changing control state through slot+0x168 and the
embedded controller. For concrete Human vtable 0x1967058 these are 0x91aaa0
(always returns false) and 0x91aa90 (ret-4 stub). The unlock branch skips the
embedded-controller call when that query is false. Thus this wrapper does not
establish a reversible lock/unlock policy for this class. Exact-byte audit now
checks these sites. No native LockControls call was added or executed.

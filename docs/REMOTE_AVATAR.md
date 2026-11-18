# Remote Human: ownership evidence

Status: a local opt-in Human has been visibly created and removed from the
registry; see [HUMAN_TEST.md](HUMAN_TEST.md). A network-controlled avatar is
not implemented. A second Player may affect story ownership. All addresses below
apply only to the pinned GOG EXE identified in AGENTS.md.

## Concrete destruction path

Read-only disassembly establishes the following chain:

1. Human constructor 0x999050 installs vtable 0x1967058 at 0x999070.
2. Slot zero is 0x999160. It calls destructor body 0x990770, tests the low bit
   of its stack argument, and for bit 1 calls 0x4019f0 with the same object.
   That routine dispatches to allocator singleton 0x1ad1abc, vtable +0x10.
   This is the concrete scalar deleting-destructor pattern, not merely a
   generic Release interpretation borrowed from an SDK.
3. Release 0x119ddd0 has an immediate branch at 0x119de53 invoking slot zero
   with argument 1. The object can therefore be freed inside Release.
4. Another branch at 0x119de08 updates entity flags (clear 0x40, set 0x400),
   calls 0x1abe8dc slot +0x4c, and returns. Live read-only inspection resolved
   its vtable to 0x190af18 and slot +0x4c to 0x5b5b90. That function ONLY sets
   CGame flag +4 |= 0x10, ignoring its stack argument. Earlier wording that
   it hands the object to a manager queue was too strong: this is a sweep
   request, not pointer enqueueing.
5. Sweep 0x5c2530 consumes the CGame 0x10 flag, enumerates entities through
   0x1abfe20 slots +0x14/+0x18 and collects those with flags +0x20 & 0x400.
   It then processes that temporary list backwards, conditionally calls
   slot +0x10, invokes 0x11998e0, and dispatches slot zero with argument 1
   at 0x5c2614. CGame code registers 0x5c2530 as a callback at 0x5c9d92 and
   0x5c9dc1: event IDs 3 (tick) and 4 (tick_paused), priority 10000. It also
   calls the sweep directly at 0x5cfe47. Actual ordering relative to our
   observer and mission teardown still needs runtime validation.

## Model attachment and failure handling

Human vtable +0xb0 resolves to SetModel 0x1164d00: it exchanges +0x60 and
returns the previous pointer. It does not increment a reference or release
the previous model. Do not pass a borrowed local-player model as owned data.

CloneHierarchy 0x14ec8f0 returns a status, despite the upstream void wrapper.
Null source returns 0xfffffc1e. Otherwise it resets destination state through
0x14e97f0 and forwards the result of 0x14ec280, whose failure path propagates
a negative result. Check the signed result before Setup/Activate; non-null
allocation alone does not establish a usable clone.

## Opt-in model lifecycle test

`start-probe.ps1 -Observer -TestModelClone -TestProfileFrom <earlier-test-id>`
enables one model-only allocation/clone/release after game_init, load_finished
and an unpaused game-thread tick. The default is disabled. It requires actual
profile redirection and the pinned concrete source model class. It never
creates Human/Player, activates an entity or replaces the player's model.

The concrete factory 0x14ec940 accepts one allocator argument, popped by the
callee; NULL uses the default allocator and constructs a 0xa0-byte model.
Its base constructor initializes a single reference at +0x10. Release
0x14e5bc0 decrements that reference and destroys/deallocates at zero.
The test calls this release exactly once for its own clone, on clone success
or error; it never appends an explicit destructor/Free or reads the released
pointer. An unexpected allocation class/refcount refuses further calls and
reports an incomplete test; it cannot be treated as a cleanup success.

Actual pinned-game result: allocation succeeded, clone_status=0,
release_remaining=0, source_reference_intact=1 and completed=1 on the game
thread. Vito remained visible and movable afterwards; a passive receiver
received 57 active snapshots with changing coordinates. Normal exit returned
0 and original_profile_changed_files=0. This proves one successful model
resource lifecycle, not visible clone placement, error-path cleanup, repeated
mission transitions, a remote avatar or two-client gameplay.

Reproduce the bounded, exact-byte checks without executing game functions:

```powershell
python scripts/audit_avatar.py '<game>/pc/mafia2.exe' --output work/avatar-audit.json
```

The script checks the full SHA256 before interpreting addresses and refuses
unexpected bytes. The report always marks safe_spawn_or_removal_verified=false.
It publishes no game binary, memory dump, save or private runtime log.

## Consequences for implementation

- Never append an explicit destructor or Free after Release. Invalidate the
  adapter's owned handle as soon as it hands the object to engine removal;
  do not inspect the object afterwards to determine whether it was freed.
- Do not directly call the deleting destructor as a shortcut around manager
  bookkeeping. Validate the intended engine removal entry and its thread.
- Validate model clone ownership, SetModel and Human Setup before adding
  creation. A loaded local player's model cannot be adopted as owned storage.
- Next audit: Human Setup/Activate, model transfer and clone failure cleanup,
  sweep timing and the mission-before-close boundary. Runtime proof
  must include one creation, one removal, mission unload and repeat load.

The standalone audit remains read-only. The separate explicitly enabled test
now executes the validated model factory, clone and single-reference release;
the model-only test creates no entity. The separate Human test now exercises
factory/Setup/Activate/removal. Remote-state application is not implemented.

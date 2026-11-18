# Remote Human: ownership evidence

Status: no remote avatar has been created. Ordinary Human remains the intended
candidate; a second Player may affect story ownership. All addresses below
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
4. Another branch at 0x119de08 updates flags (clear 0x40, set 0x400), passes the
   object to manager 0x1abe8dc slot +0x4c, and returns. Its eventual lifetime
   and queue processing still need tracing; return does not prove destruction.

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
- Next audit: manager slot +0x4c implementation/queue drain, then model clone,
  setup failure cleanup and the mission-before-close boundary. Runtime proof
  must include one creation, one removal, mission unload and repeat load.

These are ownership requirements for the future adapter. No game-memory
write, factory call, cleanup call or native patch was added by this audit.

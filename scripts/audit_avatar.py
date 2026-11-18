"""Read-only, pinned-image audit of Human destruction; never invokes game code."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from audit_game import PE, PEError

EXPECTED = '16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f'


def audit(path):
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED:
        raise ValueError('Unknown executable; no ownership interpretation permitted')
    pe = PE(data)

    def read(address, size):
        section = pe.section_at_va(address)
        if not section:
            raise PEError('Unmapped ownership evidence')
        offset = address - pe.base - section['rva']
        if offset < 0 or size > section['raw_size'] - offset:
            raise PEError('Ownership evidence outside section raw data')
        return pe.take(section['raw_offset'] + offset, size)

    def match(address, hex_bytes):
        expected = bytes.fromhex(hex_bytes)
        if read(address, len(expected)) != expected:
            raise ValueError(f'Ownership evidence mismatch at {address:08x}')

    # Constructor installs Human vtable, whose slot zero is the concrete
    # scalar deleting destructor. Check actual file contents, not SDK labels.
    match(0x999070, 'c70658709601')
    deleting_destructor = struct.unpack('<I', read(0x1967058, 4))[0]
    if deleting_destructor != 0x999160:
        raise ValueError('Unexpected Human deleting destructor')
    match(0x999163, 'e80876ffff')  # call 0x990770: Human destructor body
    match(0x999168, 'f644240801740956e87b88a6ff83c404')
    # Tests flag bit 1, pushes this and calls 0x4019f0 (deallocation).
    match(0x99917b, 'c20400')
    match(0x4019f0, '8b44240485c0740e8b0dbc1aad018b11508b4210ffd0c3')
    # Release immediate path invokes vtable[0](this, 1).
    match(0x119de53, '8b168b026a018bceffd0')
    # Other Release path marks the entity and requests a later CGame sweep.
    match(0x119de08, '8b462083e0bf0d00040000894620')
    match(0x119de16, '8b0ddce8ab018b118b424c56ffd033c05ec3')
    match(0x190af18 + 0x4c, '905b5b00')
    match(0x5b5b90, '83490410c20400')  # only marks CGame, ignores stack pointer
    match(0x5c2530, '8b410483ec14a810')
    match(0x5c253f, '83e0ef55894104')  # consumes CGame sweep flag
    match(0x5c257e, 'f7402000040000')  # collects entities marked 0x400
    match(0x5c2614, '8b118b026a01ffd0')  # concrete deleting-destructor dispatch
    match(0x5c9d92, '6830255c005168102700006a03')
    match(0x5c9dc1, '6830255c005168102700006a04')
    match(0x1164d00, '8b5424048b4160895160c20400')  # raw SetModel exchange
    match(0x14ec8fc, '5fb81efcffff5ec20400')  # null source: negative status
    match(0x14ec929, 'e852f9ffff83c40c5f5ec20400')  # forwards clone status
    match(0x14ec940, '8b4c240468a0000000')  # concrete model factory, allocator arg
    match(0x14ec969, 'c20400')  # callee pops its single allocator argument
    match(0x14ec3e4, 'e87751f6ff')  # calls common frame constructor
    match(0x145158e, 'b90100000066894810')  # initial single reference
    match(0x14e5c05, '66ff4e100fb746106685c07f53')
    match(0x14e5c56, '6a00ffd050e890bdf1fe83c40433c0')
    match(0x999d60, '6820020000e8c67aa6ff83c40485c0')
    match(0x99f400, '83ec50535556578be9')
    match(0x99f63a, '83c450c3')  # Setup: no stack arguments
    match(0x1967058 + 0x7c, '60e99400')
    match(0x94e960, '568bf183be9001000000')
    match(0x94e9b6, '5ec20400')
    match(0x11a7cd3, 'f74620000800007408e8dfde3300')
    match(0x1192170, '568bf1f6462020')
    return dict(schema=1, sha256=EXPECTED, memory_writes=0, game_api_calls=0,
                human_vtable='01967058', deleting_destructor='00999160',
                destructor_body='00990770', deallocator='004019f0',
                release='0119ddd0', immediate_path_deallocates=True,
                alternate_path_requests_cgame_sweep=True,
                sweep='005c2530', sweep_flag=16, entity_removal_flag=1024,
                sweep_event_ids=[3, 4], sweep_priority=10000,
                set_model_adds_reference=False, clone_status_must_be_checked=True,
                safe_spawn_or_removal_verified=False,
                required_policy='Never append explicit destructor/Free after Release')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.exe)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()

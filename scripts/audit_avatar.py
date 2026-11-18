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
    # Other Release path marks flags and hands this to manager slot +0x4c,
    # then returns; do not infer synchronous destruction from Release return.
    match(0x119de08, '8b462083e0bf0d00040000894620')
    match(0x119de16, '8b0ddce8ab018b118b424c56ffd033c05ec3')
    return dict(schema=1, sha256=EXPECTED, memory_writes=0, game_api_calls=0,
                human_vtable='01967058', deleting_destructor='00999160',
                destructor_body='00990770', deallocator='004019f0',
                release='0119ddd0', immediate_path_deallocates=True,
                alternate_path_hands_off_to_manager=True,
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

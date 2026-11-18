"""Read-only pinned EXE evidence; does not establish live story identity."""
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
        raise ValueError('Unknown executable; story interpretation refused')
    pe = PE(data)

    def read(at, size):
        section = pe.section_at_va(at)
        if not section:
            raise PEError('Unmapped story evidence')
        offset = at - pe.base - section['rva']
        if offset < 0 or size > section['raw_size'] - offset:
            raise PEError('Story evidence outside section raw data')
        return pe.take(section['raw_offset'] + offset, size)

    def match(at, expected):
        if read(at, len(expected)) != expected:
            raise ValueError(f'Story evidence mismatch at {at:08x}')

    # All comparisons use the same string comparator; successful branches
    # store a fixed enum, never a dynamic chapter number. ESI=input, EDI=out.
    modes = (
        (0x44cf40, 0x18f4f44, 'PLAYING_CHAPTER', 0x44cf52, 0),
        (0x44cf57, 0x18f4f38, 'MAIN_MENU', 0x44cf69, 1),
        (0x44cf72, 0x18f4f2c, 'IDLE_GAME', 0x44cf84, 2),
        (0x44cf8d, 0x18f4f20, 'PAUSED_GAME', 0x44cf9f, 3),
        (0x44cfa8, 0x18f4f18, 'PLAYING', 0x44cfba, 4),
        (0x44cfc3, 0x18f4f00, 'BROWSING_FAMILY_ALBUM', 0x44cfd5, 5),
        (0x44cfde, 0x18f4ef4, 'END_LIST', 0x44cff0, 7),
    )
    for start, string, name, store, value in modes:
        match(string, name.encode('ascii') + b'\0')
        call = start + 6
        match(start, b'\x68' + struct.pack('<I', string) + b'\x56\xe8'
              + struct.pack('<i', 0xe73b3b - (call + 5)))
        # cmp result is zero on the first equality; the remaining branches
        # write explicit immediate enum values.
        if value == 0:
            match(start + 11, bytes.fromhex('83c40885c07505'))
            match(store, bytes.fromhex('8907b001c3'))
        else:
            match(start + 11, bytes.fromhex('83c40885c07509'))
            match(store, b'\xc7\x07' + struct.pack('<I', value)
                  + bytes.fromhex('b001c3'))
    match(0x44cff9, bytes.fromhex('32c0c3'))
    match(0x454005, bytes.fromhex('e8368fffff'))
    match(0x454013, bytes.fromhex('8b0d34c4bd016a006a0150e8cdd84101'))

    # The returned object is main+4: interface+0x68 is main+0x6c.
    match(0xb2d230, bytes.fromhex('b85c39cc01c3'))
    match(0xb2c690, bytes.fromhex('8b4168c3'))
    match(0xb4058f, bytes.fromhex('b95c39cc01e8f7c0feff508b47108bcaffd0'))
    # A different field is zeroed relative to the main object's this.
    # This is NOT evidence that the candidate above is cleared on unload.
    match(0xb2c9d0, bytes.fromhex('8b44240433d28941143bc27518895168'))
    match(0xb2cc30, bytes.fromhex('6a00b95839cc01e894fdffff'))
    match(0xb2c220, bytes.fromhex('8b44240489416cc20400'))

    return dict(schema=1, sha256=EXPECTED, game_api_calls=0, memory_writes=0,
                mode_values={name: value for _, _, name, _, value in modes},
                playing_chapter_is_chapter_number=False,
                save_interface_address='01cc395c',
                candidate_interface_offset=0x68, candidate_main_offset=0x6c,
                candidate_absolute_address='01cc39c4',
                neighboring_clear_main_offset=0x68,
                candidate_meaning_verified=False,
                candidate_lifetime_verified=False, world_identity_verified=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(audit(args.exe), indent=2))
    except (OSError, ValueError, struct.error) as error:
        parser.exit(1, f'Story audit refused: {error}\n')

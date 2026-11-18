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

    # Derived save interface slot +0x30 is the asynchronous load wrapper.
    # Its second argument is stored at interface+0x64 (main+0x68), while
    # the requested save ID is stored in the allocated operation at +8.
    match(0xb666a6, bytes.fromhex('c7460468469701'))
    match(0x1974668 + 0x30, struct.pack('<I', 0xb2d020))
    match(0x450ca3, bytes.fromhex('8b10578bc88b423056ffd0'))
    match(0xb2d020, bytes.fromhex('8b442408568b7424085750568bf9e80df7ffff'))
    match(0xb2c740, bytes.fromhex('8b442408894164c7411808000000c6412100c20800'))
    match(0xb2d041, bytes.fromhex('508d4ffcc7003c349701c6400400897008e869f9ffff'))
    match(0xb2c9c0, bytes.fromhex('8b442404894114894110c20400'))

    # Later operation writes the candidate BEFORE dispatching SAVE data work.
    # A matching candidate is not evidence that the requested load succeeded.
    match(0x1973444 + 4, struct.pack('<I', 0xb407f0))
    match(0xb407f0, bytes.fromhex('568bf18b460c83e800742d'))
    match(0xb40828, bytes.fromhex('807e0400750c8b460885c07405a3c439cc01'))
    match(0xb4083a, bytes.fromhex('803d7c39cc01007462'))
    match(0xb40843, bytes.fromhex('807e040075cf8b460885c074c8'))
    match(0xb40850, bytes.fromhex('8b0d480cac018b1168e8409701508b420cffd084c075b1'))
    match(0x19740e8, b'SAVE\0')
    # The false-result path schedules another operation; both true/false
    # branches are after the earlier candidate write. AL is not world proof.
    match(0xb40875, bytes.fromhex('8b4e080fb6560451528bc8e8ebeeffff'))
    match(0xb3f7b6, bytes.fromhex('c706d8409701c74604cc409701894e1c'))
    match(0x19740d8 + 8, struct.pack('<I', 0xb2ce40))
    match(0xb2cec1, bytes.fromhex('8b4e1c8a5618c70044349701885004894808c7400c00000000'))

    return dict(schema=1, sha256=EXPECTED, game_api_calls=0, memory_writes=0,
                mode_values={name: value for _, _, name, _, value in modes},
                playing_chapter_is_chapter_number=False,
                save_interface_address='01cc395c',
                candidate_interface_offset=0x68, candidate_main_offset=0x6c,
                candidate_absolute_address='01cc39c4',
                candidate_may_precede_save_data_result=True,
                candidate_operation_writer='00b40835',
                neighboring_clear_main_offset=0x68,
                load_operation_vtable='0197343c',
                load_operation_save_id_offset=8,
                load_request_mode_interface_offset=0x64,
                loaded_mission_confirmation=False,
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

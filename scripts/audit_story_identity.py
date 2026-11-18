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

    # Lua SaveGame resolves an asynchronous request ticket and then an object.
    # The ticket is generated separately from the Lua numeric argument.
    match(0x196e1f0, b'SaveGame\0')
    match(0x1aaa368, struct.pack('<II', 0x196e1f0, 0xad6620))
    match(0xad6633, bytes.fromhex('8d44241c506a0257e8f09d96ff'))
    match(0xad6648, bytes.fromhex('e8e36b05008bf08b168b4424108b5234508bceffd2'))
    match(0xad665d, bytes.fromhex('8b168b5238508d442410508bceffd2'))
    match(0x1974668 + 0x34, struct.pack('<I', 0xb6e130))
    match(0x1974668 + 0x38, struct.pack('<I', 0xb4efc0))
    match(0xb6e136, bytes.fromhex('f6461c01740933c05e83c40cc20400'))
    match(0xb6e158, bytes.fromhex('a1a0baaa018944240c40a3a0baaa01'))
    match(0xb6e167, bytes.fromhex('8b44241c8d4c241c5189442414'))
    match(0xb6e193, bytes.fromhex('8d54240c8d7e50528bcfe89eedffff'))
    match(0xb6cf84, bytes.fromhex('8b4424108b08890e8b500483c008508d4e08895604'))
    match(0xb4efd0, bytes.fromhex('39107506837808007540'))
    match(0xb6e20d, bytes.fromhex('8b44240c5f5b5e83c40cc20400'))

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

    # CGame's registered Game Restore callback is a virtual forwarding thunk.
    # In this installed CGame table its destination is an empty handler. This
    # does not describe other modules' handlers or establish the context type.
    match(0x1167210, bytes.fromhex('a1bce8ab01c3'))
    match(0x5c9e8b, bytes.fromhex('e880d3b900'))
    match(0x5c9e96, bytes.fromhex('8b108b521c6a006a00518b0d14feab01d91c24'))
    match(0x5c9ea9, bytes.fromhex('68a07759005168f40100006a138bc8ffd2'))
    match(0x5977a0, bytes.fromhex('8b01ff6030'))
    match(0x5c21dd, bytes.fromhex('c70618af9001'))
    match(0x190af18 + 0x14, struct.pack('<I', 0x5cd760))
    match(0x190af18 + 0x30, struct.pack('<I', 0x58a740))
    match(0x58a740, bytes.fromhex('c20400'))

    # Resolve the actual SAVE service and its read-side layer dispatcher.
    match(0x11cb73e, bytes.fromhex('8bc8e88bf8ffffa3480cac01'))
    match(0x11cafdb, bytes.fromhex('c7060cc79001'))
    match(0x190c70c + 0x0c, struct.pack('<I', 0x11c55f0))
    match(0x190c70c + 0x10, struct.pack('<I', 0x11c59a0))
    match(0x11c5a2f, bytes.fromhex('8b44242483f819740583f8187574'))
    match(0x11c5a3d, bytes.fromhex('397c2428756e'))
    match(0x11c5b70, bytes.fromhex('8d4c2414518d542424528d4e08e82ee0ffff'))
    match(0x11c5b8f, bytes.fromhex('8b48108b511c83fa097d3983fa087517'))
    match(0x11c5bc4, bytes.fromhex('e837eaffff8ad8885c241c84db7464'))
    match(0x11c468b, bytes.fromhex('8b48108b018b40048d54241052ffd0'))
    match(0x11c4615, bytes.fromhex('bec59d1c81b9e83e8f01b04c69f693010001'))
    match(0x18f3ee8, b'LAST\0')
    match(0x11ca364, bytes.fromhex('4783ff0b7ca6'))
    # IDs 0..10 are initialized layers; 11 is the stream end marker, not
    # an additional active chapter/checkpoint layer. Preserve engine spelling.
    layers = (
        (0x11bc8f4, 0x190c0e4, 'LAYER_PREINIT'),
        (0x11bc8ff, 0x190c0d8, 'LAYER_INIT'),
        (0x11bc90a, 0x190c0c8, 'LAYER_CLEANUP'),
        (0x11bc915, 0x190c0b0, 'LAYER_DATA_DEPENDENT'),
        (0x11bc920, 0x190c098, 'LAYER_PREREQUISITIES'),
        (0x11bc92b, 0x190c08c, 'LAYER_DATA'),
        (0x11bc936, 0x190c078, 'LAYER_DEPENDENCIES'),
        (0x11bc941, 0x190c06c, 'LAYER_DONE'),
        (0x11bc94c, 0x190c05c, 'LAYER_POSTLOAD'),
        (0x11bc957, 0x190c048, 'LAYER_LOAD_FAILED'),
        (0x11bc962, 0x190c02c, 'LAYER_CHECK_SAVE_ENABLED'),
        (0x11bc96d, 0x190c020, 'LAYER_LAST'),
    )
    for instruction, address, name in layers:
        match(instruction, b'\xb8' + struct.pack('<I', address) + b'\xc3')
        match(address, name.encode('ascii') + b'\0')
    match(0x11bc8f0, bytes.fromhex('85c07506'))
    for index, (instruction, _, _) in enumerate(layers[1:], 1):
        match(instruction - 5, b'\x83\xf8' + bytes([index]) + b'\x75\x06')

    # Application's save callback and its missionNumber/missionPart accesses.
    # These are script progress candidates, not yet correlated checkpoint IDs.
    match(0x4e23aa, bytes.fromhex('8dbe94000000'))
    match(0x4e23c5, bytes.fromhex('c707544f8f01'))
    match(0x18f4f54 + 4, struct.pack('<I', 0x507f60))
    match(0x473830, bytes.fromhex('8db79400000056e8646bd500'))
    match(0x473858, bytes.fromhex('8b0d480cac016a0856e83a6bd500'))
    match(0x507f60, bytes.fromhex('8b0d3828ab01e9c5fbffff'))
    match(0x507c8a, bytes.fromhex('8b0d24feab01d91c246848148f01e833050900'))
    match(0x507ca2, bytes.fromhex('8b0d24feab01d91c2468b0588f01e81b050900'))
    match(0x47395b, bytes.fromhex('8b0d24feab016848148f01e8d5461200'))
    match(0x47396f, bytes.fromhex('8b0d24feab0168b0588f01e8c1461200'))
    match(0x18f1448, b'missionNumber\0')
    match(0x18f58b0, b'missionPart\0')
    match(0x58b4b5, bytes.fromhex('8bb3b4040000'))
    match(0x58b4c4, bytes.fromhex('8d04768d7c8304'))
    match(0x58b4d0, bytes.fromhex('8b4ff483ef0c8d840bb8040000'))
    match(0x598056, bytes.fromhex('8d044066837c860c028d04867507d94008'))
    match(0x597ff4, bytes.fromhex('8bbeb404000083ff647c08'))
    match(0x5981f2, bytes.fromhex('8b5424108d04408d0486b9020000006689480c895008'))

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
                lua_save_binding='00ad6620',
                lua_save_request_method='00b6e130',
                lua_save_object_lookup='00b4efc0',
                lua_save_ticket_counter='01aabaa0',
                lua_save_ticket_is_completion=False,
                lua_save_durable_completion_verified=False,
                loaded_mission_confirmation=False,
                cgame_restore_callback='005977a0',
                cgame_restore_virtual_slot=0x30,
                cgame_restore_handler='0058a740',
                cgame_restore_handler_is_empty=True,
                restore_event_context_type_verified=False,
                save_service_vtable='0190c70c',
                save_service_read_method='011c59a0',
                save_layer_read_dispatcher='011c4600',
                save_layers={str(i): name for i, (_, _, name) in enumerate(layers)},
                save_layer_callbacks_verified=False,
                application_progress_read_callback='00507b30',
                script_variables_global='01abfe24',
                script_progress_names=['missionNumber', 'missionPart'],
                script_progress_checkpoint_mapping_verified=False,
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

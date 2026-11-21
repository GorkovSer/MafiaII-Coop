"""Read-only pinned-file evidence for Human movement commands; no game execution."""
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
        raise ValueError('Unknown executable; no command interpretation permitted')
    pe = PE(data)

    def read(at, size):
        section = pe.section_at_va(at)
        if not section:
            raise PEError('Unmapped command evidence')
        offset = at - pe.base - section['rva']
        if offset < 0 or size > section['raw_size'] - offset:
            raise PEError('Command evidence outside section raw data')
        return pe.take(section['raw_offset'] + offset, size)

    def match(at, expected):
        expected = bytes.fromhex(expected)
        if read(at, len(expected)) != expected:
            raise ValueError(f'Command evidence mismatch at {at:08x}')

    # Real command factory/enqueue path: this=Human, args=(out smart-ref, type).
    # Type 1 allocates 0x58, installs this class with zero initial references.
    match(0x95b0f0, '538b5c240c568bc333f62bc6578bf9')
    match(0x95b106, '6a58e82367aaff83c4043bc6743f')
    match(0x95b114, '81600cfffbffff0f57c0897004c700b0148f01')
    match(0x95b127, 'f30f114018f30f114010')
    match(0x95b153, '518bcc56e8b4e8acff538bcfe89c22ffff')
    match(0x95b164, '568b7424148bcee8a0e8acff')
    match(0x95b175, 'c20800')
    match(0x18f14b0, struct.pack('<I', 0x42d3e0).hex())
    match(0x42d3e0, 'f644240401568bf1c70694f98e01')
    match(0x42d3f0, '56e8fa45fdff83c404')

    # The upstream hard-coded class is for a different/larger command here.
    match(0x19661bc, struct.pack('<I', 0x953b80).hex())
    match(0x953b83, '8d4e78e8b5f1acff')
    match(0x95ed6a, '6898000000e8bc2aaaff')
    match(0x95ed90, 'c707bc619601')

    # AddCommand type-1 jump, initialization, queue insertion and argument dtor.
    match(0x94d500, struct.pack('<I', 0x94d428).hex())
    match(0x94d428, '8b4c241033f6e87d9bfeff')
    match(0x936fb8, 'c6410c0181610cfff4ffffc74108b0ebcb01')
    match(0x936fca, 'c74154ffffffff')
    match(0x937000, '81ca00080000894148f30f11413c89515089414489414cc3')
    match(0x94d4e1, '8d44241050568bcfe832feffff')
    match(0x94d4ee, '8d4c2410e8b958adff5f5ec20800')
    match(0x94d320, '8b918c0100008b84d13c01000089818c010000')
    match(0x94d369, '8d8cd1380100008910e8b9c6adff8b0681480c00040000')
    match(0x916e30, 'c7813c01000001000000')
    match(0x916e6c, 'c7816c01000007000000')
    match(0x916e79, '898174010000c7818c01000000000000')

    # Intrusive reference ownership: retain, assignment, release-to-delete.
    match(0x4237b0, '8b0185c07403ff4004c3')
    match(0x429a40, '8906e8699dffff85ff7410834704ff750a8b178b026a018bcfffd0')
    match(0x422db0, '8b0985c9740e834104ff75088b018b106a01ffd2c3')
    match(0x429a10, '8b442404568bf18906e8929dffff8bc65ec20400')
    match(0x40185a, '8b0dbc1aad018b018b5424048b400852ffd0c3')
    match(0x94d8d0, '83ec0c566a008bf1e8d395fcff')

    # Dispatch and lifetime are separate from queue membership / free capacity.
    match(0x94d540, '8b81780100005683f8ff')
    match(0x1967058 + 0xc8, 'a0189800')
    match(0x9818a6, 'f7462000002000751e')
    match(0x9818b7, 'e8e4c0fcff')
    match(0x94d9a0, '83ec08535556578bf133ed')
    match(0x94d9cb, '8d96a0010000')
    match(0x94d9e0, '8b0f8b410cc1e809a801')
    match(0x94d9ec, '8b41088b188b4004')
    match(0x94da05, '8b0f8b510cc1ea0bf6c2017515')
    match(0x94da55, '8b500850518b4a0cd91c248944241c8b420803ceffd0')
    match(0x94da6b, '84c0756b')
    match(0x94da7d, '81600cfffbffff')
    match(0x94dab2, '898c9e780100008b968c0100008994fe3c01000089be8c010000')
    match(0x94dade, '578d8ea0010000e846bfadff')
    match(0x94d7aa, '8d8ea0010000e87bc2adff')
    match(0x94d8f2, '81490c00010000')
    match(0x94d8fb, '8b48088b51108b4914')
    match(0x94d927, '81600cfffbffff')
    match(0x94d98f, '8b118b421483c40cffe0')
    # SDK's untyped AI NewCommand is a different payload contract, not MoveDir.
    match(0xf9f880, '538bd98b430cf6400401')
    match(0xf9f8c3, '8b41048b09894e1889461c85c97411ff00')

    # MoveDir descriptor lives in zero-initialized storage; its function
    # pointers are installed by this static initializer, not file data.
    match(0xef5ad0, '33c0a3c0ebcb01b8a007980033c9a3b0ebcb01b850709300')
    match(0xef5ae8, '890dc4ebcb01890db4ebcb01a3b8ebcb01890dbcebcb01c3')
    match(0x937055, '8b7c24388b470cc1e809')
    match(0x9370b6, '8b8b900100008b118b47548b523c5650ffd2')
    match(0x9370c8, 'f30f1047280f57c98bf0')
    match(0x9370fe, '8a4750884654d9473c8b4e54d95e48')
    match(0x937503, 'f30f104730dd0568188f01')
    match(0x937514, 'f30f1047348b4c242c816654ffffffef')
    match(0x9375c1, '8b4750c1e80883e0035d83f801')
    match(0x9375d7, '8b475025fffeffff0d00020000894750')

    return dict(schema=1, sha256=EXPECTED, game_api_calls=0, memory_writes=0,
                create_enqueue_candidate='0095b0f0', movement_type=1,
                movement_allocation_size=0x58, movement_vtable='018f14b0',
                movement_deleting_destructor='0042d3e0',
                upstream_vtable_rejected='019661bc',
                upstream_destructor_member_offset=0x78,
                upstream_class_observed_allocation_size=0x98,
                queue_pointer_offset=0x138, queue_link_offset=0x13c,
                queue_stride=8, queue_heads_offset=0x178,
                queue_head_count=5, free_head_offset=0x18c,
                queue_slot_count=8, references_offset=4,
                add_command_consumes_argument_reference=True,
                enqueue_retains_reference=True,
                creator_returns_additional_owned_reference=True,
                allocator_dispatch_global='01ad1abc',
                allocator_failure_behavior_verified=False,
                command_reference_constructor='00429a10',
                command_dispatcher='0094d9a0', dispatch_caller='009818a0',
                dispatch_skip_mask=0x200000, retained_reference_offset=0x1a0,
                retained_reference_may_outlive_queue_membership=True,
                get_move_command_returns_queue_head=True,
                clear_move_candidate='0094d8d0', clear_all_candidate='0094d6b0',
                clear_move_does_not_clear_retained_reference=True,
                ai_new_command_raw_movedir_compatible=False,
                movedir_descriptor='01cbebb0',
                movedir_descriptor_initializer='00ef5ad0',
                movedir_start_callback='009807a0', movedir_tick_callback='00937050',
                movedir_facing_offset=0x28, movedir_motion_offset=0x30,
                movedir_multiplier_offset=0x3c, movedir_mode_byte_offset=0x50,
                movedir_control_mask=0x300,
                allocation_failure_recovery_verified=False,
                ai_ownership_verified=False, runtime_control_verified=False,
                required_policy='Do not copy upstream command vtable or overwrite queue/free-list fields')


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

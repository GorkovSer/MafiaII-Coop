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

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

    # Human's controller is allocated inside the base constructor's 0x714
    # component block. Its early null store is overwritten before return.
    match(0x998cc8, '33dbc706f06e9601')
    match(0x998d35, '6814070000')
    match(0x998d46, '899ea8000000')
    match(0x998f0b, '536aff568bcfe80ad0ffff')
    match(0x998f1a, '81c7840100008986a8000000')
    match(0x995f27, '8b7c2410506a01578bf1e81a6c6100')
    match(0x995f36, '578d4e44c70628729601e82b97fcff')
    match(0xfacb59, '8b44240c33ff6a0cc70668d98f01897e04894608')
    match(0x99f3e1, '8b8e80000000518b8ea8000000e8adfcffff')
    match(0x99a928, '8b8ea8000000d91c2483c144e847f2ffff')
    match(0x95f763, '8b442414')
    match(0x95f779, '8986a0000000')
    match(0x999b88, '8b86a000000080781c10')
    match(0x9858b7, '8b8ea80000008b018b106a00ffd2')
    match(0x9858cb, 'c786a800000000000000')
    match(0x1967228, struct.pack('<I', 0x99f640).hex())
    match(0x1967228 + 0x14, struct.pack('<I', 0x99d860).hex())
    match(0x99d8b2, '8d4e44e896f8ffff')
    # Entity activation reaches Human initialization, which calls controller
    # slot+0x10. Deactivation has a paired slot+0x14 AND shuts down physics.
    match(0x1967058 + 0x18, struct.pack('<I', 0x981660).hex())
    match(0x1967058 + 0x1c, struct.pack('<I', 0x987220).hex())
    match(0x9816db, '8bcee82ea7fcff')
    match(0x94be45, '8b8ea80000008b018b501083c404ffd2')
    match(0x1967228 + 0x10, struct.pack('<I', 0x9960b0).hex())
    match(0x98598f, '8b8ea80000008b118b4214ffd0')
    match(0x9872b9, '8b8e900100008b118b4208ffd0')
    match(0x9872d1, '8bcee838e6ffff')
    # SDK ScriptSetControlStyleStr dispatches to a Human slot which is a stub
    # for this concrete class. It cannot be assumed to disable Human AI.
    match(0x91de50, '8b098b018b8070010000ffe0')
    match(0x1967058 + 0x170, struct.pack('<I', 0x91aac0).hex())
    match(0x91aac0, '32c0c20400')

    # Scripted movement uses the AI scheduler's proper command handle and
    # returns a separately retained Sync object, not a raw MoveDir.
    match(0x9910b0, '558bec83e4f883ec34')
    match(0x9910d5, '8b0f39b1a80000007512')
    match(0x99114b, '688c000000')
    match(0x99117c, '8b4d106aff5153568bc8e805542a00')
    match(0x9911b0, '8b4c24208d44240c50e872bf5c00')
    match(0x9911be, '8b4d18518b4c2424e8c5d92700')
    match(0x9911ef, '8d44242850c744241401000000e8af562a00')
    match(0x9912e9, '8b078b88a80000008d54242852e885e56000')
    match(0x991304, '8b0f8b89a8000000e85fb45a00')
    match(0x99131f, '8b75088bcee8f784a9ff')
    match(0x99133a, 'c21400')
    match(0x11e2210, '895004c7000ccc9001')
    match(0x11e2235, '8b44240489083bca7403ff4104c20400')
    match(0x429820, '8b442404568bf18b08890e8bcee81e9fffff')
    match(0x422d40, '8b0985c9740e834104ff75088b018b106a01ffd2c3')
    match(0xf9f883, '8b430cf64004017464')
    match(0xc368b6, 'c70094bd9701')
    match(0xc368ca, '8b4a048b1289500489480885d27402ff01c20400')
    # Shared AI command counter is a separate FOUR-byte allocation, unlike
    # other two-count native handle families. Sync userdata retains that count.
    match(0x9570b9, '6a04e870a7aaff83c40485c07414')
    match(0x9570c7, 'c70000000000893e894604ff00')
    match(0xc36579, 'c706ccbc9701')
    match(0xc42423, '837e040074288b4608ff08')
    match(0xf5d223, '894608')
    match(0xf5d25e, '8b56048b0ae818a42800')
    match(0xf5d27f, 'c7460400000000')
    match(0xf5d290, '8b41088b5424043bc274124883f8027709')
    match(0xf5d307, '0fb6470d')
    match(0xf5d3b9, '884b0d')
    match(0xf95100, '5156578bf98bb7800000003bb784000000')
    match(0xf95119, '8b41288b5424143b02753d')
    match(0xf95124, '8b018b501cffd2')
    match(0xf951b9, '8b4e086a02e8cd80fcff')
    # Native task result propagation maps task state 3 -> command state 2 and
    # task state 2 -> command state 1. End/abort path also uses state 3 directly.
    match(0x40dce0, '8b4110568b7424083bc6742c')
    match(0x40dcfd, '83e80175146a02e807f5b400')
    match(0x40dd0f, '6a01e8faf4b400')
    match(0xf66497, '8b461c85c0740f833802750a8b4e186a03e8e36dffff')
    # Subsystem slot+0x4c is a secondary interface here: its deleting dtor
    # adjusts this by -0x40. This is not Human/controller/brain's vtable.
    match(0x18fd35c + 0x4c, struct.pack('<I', 0xf95100).hex())
    match(0xf99ea0, '83e940e988290000')

    # ScrMoveV's command type 4 resolves to SetMoveToBBRec, then this task.
    # Native task update consumes the abort flag, NOT command state 3.
    match(0x197bccc+4, struct.pack('<I', 0xc0ebf0).hex())
    match(0xc0ebf0, 'b804000000c3')
    match(0xb77695, '68605597016a04e87f314300')
    match(0x1aad0d0, struct.pack('<II', 0x1980d40, 0x1975560).hex())
    match(0x1980d40, struct.pack('<I', 0xcaef60).hex())
    match(0xcaef66, '6a70e8931376ff85c074078bc8e96864fbff')
    match(0xc653f0, 'c70628ce9701')
    match(0x197ce28+0x34, struct.pack('<I', 0xca3ae0).hex())
    match(0xca3ae3, 'e898feffff8bcee8613afdff')
    match(0xc77553, 'e8e87d79ff')
    match(0x40f344, '8b4f2885c97421e860d6b20084c07518')
    match(0xf3c9b0, '8a410dc3')
    match(0xf3c9a0, 'c6410d00c3')
    match(0x40f361, '8b078b50146a038bcfffd2')
    match(0x197ce28+0x14, struct.pack('<I', 0x40dce0).hex())
    match(0x197ce28+0x1c, struct.pack('<I', 0xc43010).hex())
    match(0xf950ec+8, struct.pack('<II', 0xf9500b, 0xf9500b).hex())
    match(0xc43013, '8b476885c07469')
    match(0xc4307b, 'c74768000000005e8bcf5fe915fefdff')
    # Recursive movement copies the parent abort byte into its type-3 child.
    match(0xc361a0, 'c706c4b09701')
    match(0x197b0c4+4, struct.pack('<I', 0xc0ea00).hex())
    match(0xc0ea00, 'b803000000c3')
    match(0xc42529, 'e822a42f00')
    match(0xf3c95d, '0fb6510d88500d')
    match(0x197ca78+0x34, struct.pack('<I', 0xcba8b0).hex())
    match(0xcba8bf, 'e87c4a75ff')
    match(0x197ca78+0x1c, struct.pack('<I', 0xc42c70).hex())
    # SDK LockControls is not a reversible ownership policy for concrete Human:
    # its queried lock state is always false and its setter is a stub.
    match(0x1967058+0x14c, struct.pack('<I', 0x91aaa0).hex())
    match(0x1967058+0x168, struct.pack('<I', 0x91aa90).hex())
    match(0x91aaa0, '32c0c3')
    match(0x91aa90, 'c20400')
    match(0x95cda0, '84c0743e6a006a008bcfe8918effff')

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
                human_ai_pointer_offset=0xa8,
                human_ai_constructor='00995f20', human_ai_reserved_span=0x184,
                human_ai_vtable='01967228', human_ai_owner_offset=8,
                human_ai_embedded_update_offset=0x44,
                human_ai_update_caller='0099a890',
                human_ai_update='00999b80',
                human_ai_activate='009960b0', human_ai_deactivate='0099d860',
                human_deactivate_also_deactivates_physics=True,
                human_control_style_string_slot_is_stub=True,
                script_move_candidate='009910b0', script_move_callee_pop_bytes=20,
                script_move_ai_allocation_size=0x8c,
                script_move_sync_vtable='0190cc0c',
                script_move_sync_ref_copy='00429820', script_move_sync_ref_release='00422d40',
                script_move_ai_user_data_vtable='0197bd94',
                script_move_cancellation_verified=False,
                script_move_command_vtable='0197bccc', script_move_state_offset=8,
                script_move_ai_counter_size=4,
                script_move_state_setter='00f5d210',
                script_move_state_change_candidate='00f5d290',
                script_move_targeted_remove_candidate='00f95100',
                script_move_targeted_remove_owner_verified=False,
                script_move_flag_d_serialized=True,
                script_move_task_success_command_state=1,
                script_move_task_failure_command_state=2,
                script_move_end_request_setter='00f3c9a0',
                script_move_end_request_flag_offset=0x0d,
                script_move_end_request_flag_value=0,
                script_move_task_vtable='0197ce28',
                script_move_task_update='00ca3ae0',
                script_move_task_abort_predicate='0040f340',
                script_move_task_cleanup='00c43010',
                script_move_end_request_runtime_verified=False,
                human_ai_teardown_requires_nonnull_pointer=True,
                human_ai_null_pointer_is_disable_switch=False,
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

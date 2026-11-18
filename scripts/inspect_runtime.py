"""Read-only inspection of sourced SDK candidate pointers in an audited game.

No injection, game calls, hooks, writes or binary dumps. Output stays local.
"""
import argparse
import ctypes as c
from ctypes import wintypes as w
import hashlib
import json
from pathlib import Path
import struct
import math
import time
from audit_game import PE


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('pid', type=int)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--sample-seconds', type=float, default=0)
    args = parser.parse_args()
    k = c.WinDLL('kernel32', use_last_error=True)
    k.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
    k.OpenProcess.restype = w.HANDLE
    k.CloseHandle.argtypes = [w.HANDLE]
    k.QueryFullProcessImageNameW.argtypes = [w.HANDLE, w.DWORD, w.LPWSTR, c.POINTER(w.DWORD)]
    k.ReadProcessMemory.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
    handle = k.OpenProcess(0x1000 | 0x10, False, args.pid)  # QUERY_LIMITED_INFORMATION | VM_READ
    if not handle:
        raise c.WinError(c.get_last_error())
    try:
        path = c.create_unicode_buffer(32768)
        size = w.DWORD(len(path))
        if not k.QueryFullProcessImageNameW(handle, 0, path, c.byref(size)):
            raise c.WinError(c.get_last_error())
        data = Path(path.value).read_bytes()
        expected = '16c79a3be483e582719e43d7b451e887e89414be81ca7ecfc0536dd8185e6c2f'
        if hashlib.sha256(data).hexdigest() != expected:
            raise ValueError('Refusing inspection of unknown EXE')
        pe = PE(data)

        def read(address, length):
            buf = c.create_string_buffer(length)
            count = c.c_size_t()
            if not k.ReadProcessMemory(handle, address, buf, length, c.byref(count)) or count.value != length:
                return None
            return buf.raw

        def u32(address):
            raw = read(address, 4)
            return None if raw is None else struct.unpack('<I', raw)[0]

        # Current audit assumes the preferred base; refuse if image isn't there.
        if read(pe.base, 2) != b'MZ':
            raise ValueError('Preferred image base unavailable; resolve relocation before inspecting')
        result = dict(schema=1, sha256=expected, pid=args.pid, memory_writes=0,
                      game_api_calls=0, interpretation='Candidate observations, not SDK validation',
                      candidates={})
        for name, address in {'reborn_game': 0x1abfe14, 'rc4_game': 0x1aacd64,
                              'reborn_ticked_manager': 0x1abe8bc}.items():
            ptr = u32(address)
            vtable = u32(ptr) if ptr else None
            first = u32(vtable) if vtable else None
            section = pe.section_at_va(first) if first else None
            result['candidates'][name] = dict(
                global_readable=ptr is not None, pointer_nonzero=bool(ptr),
                pointee_readable=vtable is not None, vtable_first_readable=first is not None,
                first_target_executable=bool(section and section['executable']))
        # CGame.hpp's ICGame places local ped at +0x118. A candidate pointer
        # alone is not a validated object. Keep addresses in private output
        # for matching the actual vtable implementation to on-disk code.
        game = u32(0x1abfe14)
        player = u32(game + 0x118) if game else None
        vtable = u32(player) if player else None
        position = u32(vtable + 0x8c) if vtable else None
        result['local_player_candidate'] = dict(
            player_pointer=player, vtable=vtable, get_position_target=position,
            target_executable=bool(position and (pe.section_at_va(position) or {}).get('executable')),
            entity_guid=u32(player + 0x1c) if player else None)
        physics = u32(player + 0x190) if player else None
        physics_vtable = u32(physics) if physics else None
        result['local_player_candidate']['physics_position_target'] = (
            u32(physics_vtable + 0x54) if physics_vtable else None)
        save_manager=u32(0x1ac0c50)
        begin=u32(save_manager+0x0c) if save_manager else None
        end=u32(save_manager+0x10) if save_manager else None
        count=(end-begin)//4 if begin is not None and end is not None and end>=begin and (end-begin)%4==0 else None
        result['save_manager_candidate']=dict(entries=count,ids=[])
        if count is not None and count<=256:
            for index in range(count):
                record=u32(begin+index*4)
                result['save_manager_candidate']['ids'].append(u32(record+8) if record else None)
        model = u32(player + 0x60) if player else None
        if model:
            values = [read(model + offset, 4) for offset in (0x44, 0x54, 0x64)]
            if all(values):
                result['local_player_candidate']['model_position'] = [struct.unpack('<f', v)[0] for v in values]
        if not 0 <= args.sample_seconds <= 60:
            raise ValueError('Sample duration must be between 0 and 60 seconds')
        if args.sample_seconds:
            samples = []
            until = time.monotonic() + args.sample_seconds
            while time.monotonic() < until:
                game = u32(0x1abfe14)
                player = u32(game + 0x118) if game else None
                vtable = u32(player) if player else None
                target = u32(vtable + 0x8c) if vtable else None
                physics = u32(player + 0x190) if player else None
                physics_vtable = u32(physics) if physics else None
                getter = u32(physics_vtable + 0x54) if physics_vtable else None
                if target == 0x938250 and getter == 0x18ae690:
                    raw = read(physics + 0x1d8, 12)
                    xyz = struct.unpack('<fff', raw) if raw else None
                    if xyz and all(math.isfinite(v) and abs(v) < 1000000 for v in xyz):
                        samples.append(dict(t=time.monotonic(), guid=u32(player + 0x1c), xyz=xyz))
                time.sleep(0.2)
            result['position_samples'] = samples
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print(json.dumps({k: v for k, v in result.items() if k not in ('local_player_candidate', 'position_samples')}, indent=2))
        print('local_player_candidate_present=' + str(bool(player)))
        if args.sample_seconds:
            print('position_samples=' + str(len(samples)))
            print('position_changed=' + str(bool(samples and any(s['xyz'] != samples[0]['xyz'] for s in samples))))
    finally:
        k.CloseHandle(handle)


if __name__ == '__main__':
    main()

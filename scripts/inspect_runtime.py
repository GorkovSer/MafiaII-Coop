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
from audit_game import PE


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('pid', type=int)
    parser.add_argument('--output', type=Path, required=True)
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
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print(json.dumps(result, indent=2))
    finally:
        k.CloseHandle(handle)


if __name__ == '__main__':
    main()

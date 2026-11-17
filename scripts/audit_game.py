"""Read-only PE32 inventory. Does not execute, patch, or copy game resources."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zlib


class PEError(ValueError):
    pass


class PE:
    def __init__(self, data):
        self.data = data
        if self.take(0, 2) != b'MZ':
            raise PEError('Missing DOS signature')
        nt = self.number(60, 'I')
        if self.take(nt, 4) != b'PE\0\0':
            raise PEError('Missing PE signature')
        self.machine = self.number(nt + 4, 'H')
        count = self.number(nt + 6, 'H')
        optional_size = self.number(nt + 20, 'H')
        opt = nt + 24
        if optional_size < 96 or self.number(opt, 'H') != 0x10b:
            raise PEError('Expected PE32 optional header')
        self.take(opt, optional_size)
        self.base = self.number(opt + 28, 'I')
        self.entry = self.number(opt + 16, 'I')
        self.image_size = self.number(opt + 56, 'I')
        self.headers_size = self.number(opt + 60, 'I')
        self.sections = []
        for i in range(count):
            at = opt + optional_size + 40 * i
            header = self.take(at, 40)
            name = header[:8].rstrip(b'\0').decode('ascii', errors='replace')
            size, rva, raw_size, raw = struct.unpack_from('<IIII', header, 8)
            flags = struct.unpack_from('<I', header, 36)[0]
            self.take(raw, raw_size)
            self.sections.append(dict(name=name, rva=rva, virtual_size=size,
                                      raw_size=raw_size, raw_offset=raw,
                                      executable=bool(flags & 0x20000000)))

    def take(self, offset, size):
        if offset < 0 or size < 0 or offset > len(self.data) - size:
            raise PEError('Truncated PE or invalid file offset')
        return self.data[offset:offset + size]

    def number(self, offset, fmt):
        return struct.unpack('<' + fmt, self.take(offset, struct.calcsize(fmt)))[0]

    def section_at_va(self, va):
        rva = va - self.base
        for section in self.sections:
            if section['rva'] <= rva < section['rva'] + max(
                    section['virtual_size'], section['raw_size']):
                return section
        return None


def audit(path):
    data = path.read_bytes()
    pe = PE(data)
    # These are upstream candidate locations, never validated call targets.
    # References: m2o-reborn vendor/m2sdk/include/m2sdk.h and CGame.hpp.
    candidates = {'reborn_module_install': 0x4f2f05,
                  'reborn_game_singleton': 0x1abfe14,
                  'rc4_game_singleton': 0x1aacd64}
    return dict(schema=1, file_name=path.name, bytes=len(data),
                sha256=hashlib.sha256(data).hexdigest(),
                crc32=f'{zlib.crc32(data):08X}', machine=f'{pe.machine:04X}',
                architecture='x86' if pe.machine == 0x14c else 'other',
                image_base=f'{pe.base:08X}', entry_rva=f'{pe.entry:08X}',
                image_size=pe.image_size, sections=pe.sections,
                candidate_sections={name: (pe.section_at_va(va) or {}).get('name')
                                    for name, va in candidates.items()},
                sdk_compatibility='UNVERIFIED', game_modified=False,
                note='Mapped section alone does not prove an address is valid.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    result = audit(args.exe)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: result[k] for k in ('sha256', 'crc32', 'architecture',
                                            'sdk_compatibility')}, indent=2))


if __name__ == '__main__':
    main()

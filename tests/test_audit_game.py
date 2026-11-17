"""Malformed input handling matters before trusting a binary compatibility gate."""
import importlib.util
from pathlib import Path
import struct


spec = importlib.util.spec_from_file_location(
    'audit_game', Path(__file__).resolve().parents[1] / 'scripts/audit_game.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


def fixture():
    data = bytearray(1024)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 60, 128)
    data[128:132] = b'PE\0\0'
    struct.pack_into('<HH', data, 132, 0x14c, 1)
    struct.pack_into('<H', data, 148, 224)
    struct.pack_into('<H', data, 152, 0x10b)
    struct.pack_into('<I', data, 180, 0x400000)
    data[376:384] = b'.text\0\0\0'
    struct.pack_into('<IIII', data, 384, 0x200, 0x1000, 0x200, 512)
    struct.pack_into('<I', data, 412, 0x60000020)
    return data


class PEValidation:
    @staticmethod
    def assertIsNone(value):
        assert value is None

    @staticmethod
    def assertIsNotNone(value):
        assert value is not None

    @staticmethod
    def assertEqual(a, b):
        assert a == b, (a, b)

    @staticmethod
    def rejects(data):
        try:
            module.PE(data)
        except module.PEError:
            return
        raise AssertionError('Malformed PE accepted')

    def test_reject_truncated_header(self):
        for data in (b'', b'MZ', fixture()[:153], fixture()[:400]):
            self.rejects(data)

    def test_reject_raw_section_outside_file(self):
        data = fixture()
        struct.pack_into('<I', data, 396, 0xfffffff0)
        self.rejects(data)

    def test_reject_wrong_optional_header(self):
        data = fixture()
        struct.pack_into('<H', data, 152, 0x20b)
        self.rejects(data)

    def test_section_bounds(self):
        pe = module.PE(fixture())
        self.assertIsNone(pe.section_at_va(0x400fff))
        self.assertEqual(pe.section_at_va(0x401000)['name'], '.text')
        self.assertIsNotNone(pe.section_at_va(0x4011ff))
        self.assertIsNone(pe.section_at_va(0x401200))
        self.assertIsNone(pe.section_at_va(0xffffffff))


if __name__ == '__main__':
    suite = PEValidation()
    tests = [name for name in dir(suite) if name.startswith('test_')]
    for name in tests:
        getattr(suite, name)()
        print('PASS', name)
    print(f'{len(tests)} tests passed')

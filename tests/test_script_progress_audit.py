"""Synthetic reads only; does not prove story/checkpoint identity in the game."""
import struct
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from script_progress_audit import inspect_script_progress


def main():
    manager = 0x2000
    pool = b'missionNumber\0missionPart\0' + bytes(64)
    words = {0x1abfe24: manager, manager+0x4b4: 2}
    records = struct.pack('<IfHH', 0, 2.0, 2, 0) + struct.pack('<IfHH', 14, 3.0, 2, 0)

    def read(address, size):
        if (address, size) == (manager+4, len(records)):
            return records
        offset = address - (manager+0x4b8)
        if 0 <= offset <= len(pool)-size:
            return pool[offset:offset+size]
        return None

    def audit(): return inspect_script_progress(words.get, read)
    good = audit()
    assert good['status'] == 'observed_script_progress'
    assert (good['mission_number'], good['mission_part']) == (2, 3)
    assert not good['identity_verified'] and not good['checkpoint_mapping_verified']
    for count, status in [(0, 'missing_variables'), (101, 'invalid_count')]:
        words[manager+0x4b4] = count
        assert audit()['status'] == status
    words[manager+0x4b4] = 2
    for value, kind in [(float('nan'), 2), (float('inf'), 2), (-1.0, 2),
                        (2.5, 2), (65536.0, 2), (2.0, 1)]:
        records = struct.pack('<IfHH', 0, value, kind, 0) + struct.pack('<IfHH', 14, 3.0, 2, 0)
        result = audit()
        assert result['status'] == 'invalid_progress_value'
        assert result['mission_number'] is None and result['mission_part'] is None
    records = struct.pack('<IfHH', 0, 2.0, 2, 0) + struct.pack('<IfHH', 4096, 3.0, 2, 0)
    assert audit()['status'] == 'invalid_name_offset'
    records = struct.pack('<IfHH', 0, 2.0, 2, 0) + struct.pack('<IfHH', 14, 3.0, 2, 0)
    original_pool = pool
    pool = b'otherNumber\0xxotherPart\0' + bytes(64)
    assert audit()['status'] == 'missing_variables'
    pool = original_pool
    words[manager+0x4b4] = 3
    # Native reverse lookup semantics: newest duplicate wins; wrong type must
    # not fall back to the older good missionNumber entry.
    records += struct.pack('<IfHH', 0, 9.0, 2, 0)
    assert audit()['mission_number'] == 9
    records = records[:-12] + struct.pack('<IfHH', 0, 9.0, 1, 0)
    assert audit()['status'] == 'invalid_progress_value'
    records = records[:-12]
    words[manager+0x4b4] = 2
    calls = 0

    def changed(address, size):
        nonlocal calls
        if address == manager+4:
            calls += 1
            if calls == 2:
                return bytes(size)
        return read(address, size)

    assert inspect_script_progress(words.get, changed)['status'] == 'changed'
    words[0x1abfe24] = 0xfffffffc
    assert audit()['status'] == 'unavailable'
    assert inspect_script_progress({}.get, read)['status'] == 'unavailable'
    print('PASS bounded script progress reads; world identity remains unverified')


if __name__ == '__main__':
    main()

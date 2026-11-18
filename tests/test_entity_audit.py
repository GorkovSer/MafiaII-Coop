"""Missing/corrupt registry memory must never be reported as confirmed absence."""
import struct
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from entity_audit import inspect_entity


def main():
    memory = {}
    manager, buckets, entries, obj, vector = 0x1000, 0x2000, 0x5000, 0x7000, 0x8000
    base = manager + 4

    def put(at, data):
        memory.update({at+i: value for i, value in enumerate(data)})

    def word(at, value):
        put(at, struct.pack('<I', value))

    def read(at, size):
        if any(at+i not in memory for i in range(size)):
            return None
        return bytes(memory[at+i] for i in range(size))

    def check(guid=0x10e):
        return inspect_entity(manager, guid, read)['status']

    word(base, buckets); word(base+4, entries)
    word(base+0x10, vector); word(base+0x14, vector)
    put(buckets, struct.pack('<h', 0) * 4096)
    put(entries, struct.pack('<Ihh', 2, 1, 0))
    put(entries+8, struct.pack('<Ihh', 1, -1, 0))
    word(base+0x20, obj); word(obj+0x1c, 0x10e)
    assert check() == 'present'  # collision chain
    assert check(0x30e) == 'absent'
    word(obj+0x1c, 0x110)
    assert check() == 'guid_mismatch'
    word(obj+0x1c, 0x10e)
    put(entries, struct.pack('<Ihh', 2, 0, 0))
    assert check() == 'invalid'  # cycle
    put(buckets, struct.pack('<h', 6000) * 4096)
    assert check() == 'invalid'  # fixed table bounds
    put(buckets, struct.pack('<h', -1) * 4096)
    put(vector, struct.pack('<II', obj, 1))
    word(base+0x14, vector+8)
    assert check() == 'present'  # overflow vector
    word(base+0x14, vector+7)
    assert check() == 'invalid'
    word(base+0x14, vector+4097*8)
    assert check() == 'invalid'
    del memory[base]
    assert check() == 'invalid'
    assert inspect_entity(0xfffffff0, 0x10e, read)['status'] == 'invalid'
    assert check(0) == 'invalid'
    print('PASS entity lookup: collision, absence, GUID mismatch, cycle, bounds, fallback and missing memory')


if __name__ == '__main__':
    main()

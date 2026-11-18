"""Traversal failures must not be mistaken for a usable game constructor."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from factory_audit import inspect_factory


def main():
    memory = {100: 200, 104: 14, 116: 1000, 200: 0, 204: 16, 216: 2000}

    def audit(first=100, limit=256):
        return inspect_factory(first, memory.get, lambda at: at in (1000, 2000), limit)

    result = audit()
    assert result['complete'] and result['records'] == 2
    assert result['candidates']['human']['callback'] == 1000
    assert result['candidates']['player']['target_executable']
    assert audit(None)['reason'] == 'unreadable_head'
    assert audit(0)['complete'] and not audit(0)['candidates']
    assert audit(limit=1)['reason'] == 'record_limit'
    memory[200] = 100
    assert audit()['reason'] == 'cycle'
    memory[200] = 0
    memory[204] = 14
    assert audit()['reason'] == 'duplicate_type'
    memory[204] = 16
    memory[216] = 77
    assert not audit()['candidates']['player']['target_executable']
    del memory[216]
    assert audit()['reason'] == 'unreadable_record'
    print('PASS bounded factory audit, missing memory, cycles, duplicate types and invalid targets')


if __name__ == '__main__':
    main()

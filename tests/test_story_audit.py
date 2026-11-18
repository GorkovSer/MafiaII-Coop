"""Synthetic read-only memory cases, never game/world-identity acceptance."""
import struct
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'scripts'))
from story_audit import inspect_story


def main():
    game, blob = 0x1000, 0x2000
    roots = [b'/test/a/', b'/test/b/', b'/test/c/', b'/test/d/']
    payload = b'\0'.join(roots) + b'\0'
    words = {game+4: 2, game+0x2c: blob, 0x1cc395c+0x68: 100}
    offset = 0
    for field, root in zip((0x0c, 0x10, 0x14, 0x18), roots):
        words[game+field] = blob+16+offset
        offset += len(root)+1
    buffers = {blob: struct.pack('<4I', 0x676d7072, 6, 0xe001, len(payload)), blob+16: payload}

    def read(address, size):
        data = buffers.get(address)
        return data if data is not None and len(data) == size else None

    def audit(): return inspect_story(game, words.get, read, [1, 100, 200])
    good = audit()
    assert good['map_header_status'] == 'validated_asset_roots'
    assert good['asset_roots'] == [root.decode() for root in roots]
    assert good['candidate_matches_catalogue'] and not good['identity_verified']
    words[0x1cc395c+0x68] = 999
    assert not audit()['candidate_matches_catalogue']
    words[game+4] = 0
    assert audit()['map_header_status'] == 'not_open' and not audit()['asset_roots']
    words[game+4] = 2
    words[game+0x10] += 1
    assert audit()['map_header_status'] == 'invalid_root_pointer'
    words[game+0x10] -= 1
    buffers[blob] = struct.pack('<4I', 0x676d7072, 5, 0xe001, len(payload))
    assert audit()['map_header_status'] == 'unknown_header'
    buffers[blob] = struct.pack('<4I', 0x676d7072, 6, 0xe001, 4097)
    assert audit()['map_header_status'] == 'invalid_size'
    buffers[blob] = struct.pack('<4I', 0x676d7072, 6, 0xe001, len(payload))
    buffers[blob+16] = b'X' + payload[1:]
    assert audit()['map_header_status'] == 'invalid_asset_root' and not audit()['asset_roots']
    buffers[blob] = struct.pack('<4I', 0x676d7072, 6, 0xe001, len(payload)+1)
    buffers[blob+16] = payload+b'x'
    assert audit()['map_header_status'] == 'unexpected_header_data'
    buffers[blob] = struct.pack('<4I', 0x676d7072, 6, 0xe001, len(payload))
    buffers[blob+16] = payload[:-1] + b'x'
    assert audit()['map_header_status'] == 'invalid_root_pointer'
    del buffers[blob+16]
    assert audit()['map_header_status'] == 'unavailable'
    words[game+0x2c] = 0xfffffff8
    assert audit()['map_header_status'] == 'unavailable'
    assert inspect_story(None, {}.get, read, [])['save_load_value_candidate'] is None
    print('PASS bounded map roots and unverified save/load candidates; no mission identity claimed')


if __name__ == '__main__':
    main()

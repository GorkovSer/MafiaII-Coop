"""Read-only candidates from the pinned GOG image, not network world identity.

CGame::Open 0x5cd760 loads /games/%s.bin; parser 0x5c9090 assigns four
asset-root pointers at +0x0c..+0x18 into the resident GMP buffer at +0x2c.
0xb2d230 returns the interface address 0x1cc395c (not a pointer stored there).
0xb2c690 returns its +0x68 value. Its gameplay meaning remains unverified.
"""
import struct


def inspect_story(game, read_u32, read_bytes, save_ids):
    value = read_u32(0x1cc395c + 0x68)
    result = dict(identity_verified=False, save_load_value_candidate=value,
                  candidate_matches_catalogue=value is not None and value in save_ids,
                  game_open=None, map_header_status='unavailable', asset_roots=[])
    if not game:
        return result
    flags = read_u32(game + 4)
    if flags is None:
        return result
    result['game_open'] = bool(flags & 2)  # 0x5b5b30: (flags >> 1) & 1.
    if not result['game_open']:
        result['map_header_status'] = 'not_open'
        return result
    blob = read_u32(game + 0x2c)
    if not blob or blob > 0xffffffff - 16:
        return result
    header = read_bytes(blob, 16)
    if header is None or len(header) != 16:
        return result
    magic, version, kind, size = struct.unpack('<4I', header)
    if (magic, version, kind) != (0x676d7072, 6, 0xe001):
        result['map_header_status'] = 'unknown_header'
        return result
    if not 4 <= size <= 4096 or blob + 16 + size > 0x100000000:
        result['map_header_status'] = 'invalid_size'
        return result
    data = read_bytes(blob + 16, size)
    if data is None or len(data) != size:
        return result
    offset = 0
    roots = []
    for field in (0x0c, 0x10, 0x14, 0x18):
        end = data.find(b'\0', offset)
        if end < 0 or read_u32(game + field) != blob + 16 + offset:
            result['map_header_status'] = 'invalid_root_pointer'
            return result
        raw = data[offset:end]
        if not raw.startswith(b'/') or any(byte < 32 or byte > 126 for byte in raw):
            result['map_header_status'] = 'invalid_asset_root'
            return result
        roots.append(raw.decode('ascii'))
        offset = end + 1
    if offset != size:
        result['map_header_status'] = 'unexpected_header_data'
        return result
    result.update(map_header_status='validated_asset_roots', asset_roots=roots)
    return result

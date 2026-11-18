"""Bounded read-only equivalent of pinned CEntityManager GUID lookup.

5a0c70 -> 59b130 -> 58ebe0. No game calls; caller enforces image identity.
"""
import struct


def inspect_entity(manager, guid, read):
    result = dict(status='invalid', pointer=None, guid=guid)
    if not manager or not isinstance(guid, int) or not 0 < guid <= 0xffffffff:
        return result

    def take(at, count):
        if not 0 < at <= 0xffffffff or count > 0x100000000 - at:
            return None
        raw = read(at, count)
        return raw if raw is not None and len(raw) == count else None

    def u32(at):
        raw = take(at, 4)
        return None if raw is None else struct.unpack('<I', raw)[0]

    def finish(pointer):
        if pointer is None:
            return result
        if not pointer:
            return dict(result, status='absent')
        actual = u32(pointer + 0x1c)
        if actual is None:
            return result
        return dict(result, status='present' if actual == guid else 'guid_mismatch', pointer=pointer)

    base = manager + 4
    buckets, entries = u32(base), u32(base + 4)
    if not buckets or not entries:
        return result
    key = guid >> 8
    mask = 0xffffffff
    mixed = (~(key << 15) + key) & mask
    mixed ^= mixed >> 10
    mixed = (mixed * 9) & mask
    mixed ^= mixed >> 6
    mixed = (mixed + ~(mixed << 11)) & mask
    mixed ^= mixed >> 16
    raw = take(buckets + (mixed & 0xfff) * 2, 2)
    if raw is None:
        return result
    index = struct.unpack('<h', raw)[0]
    seen = set()
    while index >= 0:
        if index >= 6000 or index in seen:
            return result
        seen.add(index)
        raw = take(entries + index * 8, 8)
        if raw is None:
            return result
        found, index_next = struct.unpack_from('<Ih', raw)
        if found == key:
            return finish(u32(base + 0x1c + index * 4))
        index = index_next
    begin, end = u32(base + 0x10), u32(base + 0x14)
    if begin is None or end is None or end < begin or (end - begin) % 8 or (end - begin) // 8 > 4096:
        return result
    for at in range(end - 8, begin - 1, -8):
        raw = take(at, 8)
        if raw is None:
            return result
        pointer, found = struct.unpack('<II', raw)
        if found == key:
            return finish(pointer)
    return dict(result, status='absent')

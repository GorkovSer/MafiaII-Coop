"""Bounded external reads of script variables; not verified world identity."""
import math
import struct


def inspect_script_progress(read_u32, read_bytes):
    result = dict(status='unavailable', identity_verified=False,
                  checkpoint_mapping_verified=False,
                  mission_number=None, mission_part=None)
    global_address = 0x1abfe24
    manager = read_u32(global_address)
    if not manager or manager & 3 or manager > 0xffffffff - 0x4b8:
        return result
    count = read_u32(manager + 0x4b4)
    # 100 fixed 12-byte slots precede the count at +0x4b4.
    if count is None:
        return result
    if not 0 <= count <= 100:
        result['status'] = 'invalid_count'
        return result
    if not count:
        result['status'] = 'missing_variables'
        return result
    raw = read_bytes(manager + 4, count * 12)
    if raw is None or len(raw) != count * 12:
        return result
    names = {b'missionNumber\0': 'mission_number', b'missionPart\0': 'mission_part'}
    observed, values = [], {}
    # Native lookup scans backwards and returns the first equal name. Do not
    # fall back to an older duplicate if its newest entry has the wrong type.
    for index in reversed(range(count)):
        offset, value, kind = struct.unpack_from('<IfH', raw, index * 12)
        for name, field in names.items():
            if field in values:
                continue
            # Conservative diagnostic read budget, not a claimed engine pool
            # size. Refuse unknown layouts instead of scanning arbitrary RAM.
            if offset > 4096 - len(name) or manager + 0x4b8 + offset > 0xffffffff - len(name) + 1:
                result['status'] = 'invalid_name_offset'
                return result
            address = manager + 0x4b8 + offset
            text = read_bytes(address, len(name))
            if text is None or len(text) != len(name):
                return result
            observed.append((address, text))
            if text != name:
                continue
            if kind != 2 or not math.isfinite(value) or not 0 <= value <= 65535 or value != int(value):
                result['status'] = 'invalid_progress_value'
                return result
            values[field] = int(value)
        if len(values) == 2:
            break
    if (read_u32(global_address) != manager or
            read_u32(manager + 0x4b4) != count or
            read_bytes(manager + 4, count * 12) != raw or
            any(read_bytes(address, len(text)) != text for address, text in observed)):
        result['status'] = 'changed'
        return result
    if len(values) != 2:
        result['status'] = 'missing_variables'
        return result
    result.update(status='observed_script_progress', **values)
    return result

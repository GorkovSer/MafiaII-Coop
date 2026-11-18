"""Read-only registry traversal sourced from the pinned GOG CreateEntity code.

0x1167a60 follows next at +0, compares type at +4, then jumps to +0x10.
No callbacks are invoked. Targets remain candidates until separately audited.
"""


def inspect_factory(first, read_u32, executable, limit=256):
    result = dict(complete=False, reason='unreadable_head', records=0, candidates={})
    if first is None:
        return result
    seen = set()
    node = first
    while node:
        if node in seen:
            result['reason'] = 'cycle'
            return result
        if len(seen) >= limit:
            result['reason'] = 'record_limit'
            return result
        seen.add(node)
        values = [read_u32(node + offset) for offset in (0, 4, 0x10)]
        if any(value is None for value in values):
            result['reason'] = 'unreadable_record'
            return result
        following, kind, target = values
        result['records'] += 1
        if kind in (0x0e, 0x10):
            name = 'human' if kind == 0x0e else 'player'
            # Duplicate registrations are ambiguous; never silently pick one.
            if name in result['candidates']:
                result['reason'] = 'duplicate_type'
                return result
            result['candidates'][name] = dict(type=kind, callback=target,
                                             target_executable=bool(executable(target)))
        node = following
    result.update(complete=True, reason='end_of_list')
    return result

"""Validate private runtime evidence and print a summary without raw coordinates."""
import argparse
import json
import math
from pathlib import Path


def summarize(lines):
    initialized = loaded = False
    epoch = 0
    previous_ms = None
    epochs = {}
    callback_threads = set()
    reader_threads = set()
    for number, line in enumerate(lines, 1):
        if line.startswith('observer='):
            fields = dict(part.split('=', 1) for part in line.split())
            event = fields['observer']
            if event == 'mission_open':
                epoch += 1
                initialized = loaded = False
            elif event == 'game_init':
                initialized = True
            elif event == 'load_started':
                loaded = False
            elif event == 'load_finished':
                loaded = True
            elif event in ('mission_before_close', 'mission_close', 'game_done', 'shutdown'):
                initialized = loaded = False
            if event not in ('startup_hook_installed', 'refused_unknown_image'):
                callback_threads.add(int(fields['thread']))
        elif line.startswith('player_reader_status='):
            fields = dict(part.split('=', 1) for part in line.split())
            reader_threads.add(int(fields['thread']))
        elif line.startswith('player_sample '):
            fields = dict(part.split('=', 1) for part in line.split()[1:])
            current_epoch = int(fields['epoch'])
            ms = int(fields['ms'])
            xyz = tuple(float(value) for value in fields['xyz'].split(','))
            if not initialized or not loaded or current_epoch != epoch:
                raise ValueError(f'Sample outside initialized mission at line {number}')
            if len(xyz) != 3 or not all(math.isfinite(v) and abs(v) < 1000000 for v in xyz):
                raise ValueError(f'Invalid position at line {number}')
            if previous_ms is not None and ms <= previous_ms:
                raise ValueError(f'Nonmonotonic sample at line {number}')
            previous_ms = ms
            state = epochs.setdefault(epoch, dict(samples=0, guids=set(), low=list(xyz), high=list(xyz)))
            state['samples'] += 1
            state['guids'].add(int(fields['guid']))
            for axis in range(3):
                state['low'][axis] = min(state['low'][axis], xyz[axis])
                state['high'][axis] = max(state['high'][axis], xyz[axis])
    if not epochs:
        raise ValueError('No actual player samples; runtime acceptance is unproven')
    if len(reader_threads) != 1 or reader_threads != callback_threads:
        raise ValueError('Reader and observed callbacks do not share one thread')
    return dict(schema=1, epochs=[dict(epoch=key, samples=value['samples'],
        distinct_local_guids=len(value['guids']),
        axis_ranges=[round(high-low, 5) for low, high in zip(value['low'], value['high'])])
        for key, value in epochs.items()], one_game_thread=True,
        all_samples_within_loaded_mission=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    args = parser.parse_args()
    print(json.dumps(summarize(args.log.read_text(encoding='utf-8').splitlines()), indent=2))


if __name__ == '__main__':
    main()

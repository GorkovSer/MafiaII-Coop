"""Acceptance evidence must fail closed on stale or absent game observations."""
import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location('summary',
    Path(__file__).resolve().parents[1] / 'scripts/summarize_telemetry.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

prefix = ['observer=mission_open value=1 thread=7 detail=0',
          'observer=game_init value=1 thread=7 detail=0',
          'observer=load_finished value=1 thread=7 detail=0',
          'player_reader_status=0 epoch=1 thread=7']
sample = 'player_sample ms=1000 epoch=1 guid=10 xyz=1,2,3 model_valid=0 model=0,0,0'


def rejects(lines):
    try:
        module.summarize(lines)
    except ValueError:
        return
    raise AssertionError('Invalid runtime evidence was accepted')


def main():
    result = module.summarize(prefix + [sample, sample.replace('1000', '1500').replace('xyz=1,2,3', 'xyz=4,2,3')])
    assert result['epochs'][0]['axis_ranges'] == [3, 0, 0]
    rejects(prefix)  # Registration alone cannot prove player state.
    rejects([line for line in prefix if 'load_finished' not in line] + [sample])
    rejects(prefix + ['observer=game_done value=1 thread=7 detail=0', sample])
    rejects(prefix + [sample.replace('epoch=1', 'epoch=2')])
    rejects(prefix + [sample, sample])
    rejects(prefix + [sample.replace('xyz=1,2,3', 'xyz=nan,2,3')])
    rejects([line.replace('thread=7', 'thread=8') if line.startswith('player_') else line for line in prefix] + [sample])
    print('PASS moving state summary and 7 invalid runtime evidence cases')


if __name__ == '__main__':
    main()

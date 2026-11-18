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


def rejects(lines, **kwargs):
    try:
        module.summarize(lines, **kwargs)
    except ValueError:
        return
    raise AssertionError('Invalid runtime evidence was accepted')


def main():
    result = module.summarize(prefix + [sample, sample.replace('1000', '1500').replace('xyz=1,2,3', 'xyz=4,2,3')])
    assert result['epochs'][0]['axis_ranges'] == [3, 0, 0]
    assert not result['lifecycle_acceptance']
    rejects(prefix)  # Registration alone cannot prove player state.
    rejects([line for line in prefix if 'load_finished' not in line] + [sample])
    rejects(prefix + ['observer=game_done value=1 thread=7 detail=0', sample])
    rejects(prefix + [sample.replace('epoch=1', 'epoch=2')])
    rejects(prefix + [sample, sample])
    rejects(prefix + [sample.replace('xyz=1,2,3', 'xyz=nan,2,3')])
    rejects([line.replace('thread=7', 'thread=8') if line.startswith('player_') else line for line in prefix] + [sample])
    rejects([line for line in prefix if 'mission_open' not in line] +
            [sample.replace('epoch=1', 'epoch=0')])
    moving = prefix + [sample, sample.replace('1000', '1500').replace('xyz=1,2,3', 'xyz=4,2,3')]
    close = ['observer=mission_close value=1 thread=7 detail=0',
             'observer=no_game_start value=2 thread=7 detail=0']
    second = [line.replace('epoch=1', 'epoch=2').replace('ms=1000', 'ms=2000').replace(
        'ms=1500', 'ms=2500') for line in moving]
    shutdown = ['observer=shutdown value=1 thread=7 detail=0']
    lifecycle = moving + close + second + close + shutdown
    launch = dict(schema=1, launcher_exit=0, waited_for_exit=True, child_exit_code=0,
                  original_profile_changed_files=0)
    assert module.summarize(lifecycle, require_lifecycle=True, launch_result=launch)['lifecycle_acceptance']
    rejects(shutdown + lifecycle[:-1], require_lifecycle=True, launch_result=launch)
    rejects(moving + close + shutdown + second + close, require_lifecycle=True, launch_result=launch)
    rejects(moving + close + second + shutdown + close, require_lifecycle=True, launch_result=launch)
    rejects(moving + [close[0]] + second + [close[0]] + shutdown + [close[1]],
            require_lifecycle=True, launch_result=launch)
    pause = ['observer=game_paused value=1 thread=7 detail=0']
    resume = ['observer=game_unpaused value=1 thread=7 detail=0']
    rejects(prefix + pause + moving[len(prefix):])
    rejects(prefix + [pause[0].replace('game_paused', 'tick_paused')] + moving[len(prefix):])
    rejects(prefix + pause + moving[len(prefix):] + close + second + close + shutdown,
            require_lifecycle=True, launch_result=launch)
    resumed = prefix + pause + resume + moving[len(prefix):] + close + second + close + shutdown
    assert module.summarize(resumed, require_lifecycle=True, launch_result=launch)['lifecycle_acceptance']
    rejects(lifecycle, require_lifecycle=True)  # A shutdown callback is not an OS process-exit result.
    for field, value in [('child_exit_code', 1), ('child_exit_code', None), ('child_exit_code', False),
                         ('waited_for_exit', False), ('launcher_exit', 9), ('original_profile_changed_files', 1)]:
        rejects(lifecycle, require_lifecycle=True, launch_result=dict(launch, **{field: value}))
    rejects(moving, require_lifecycle=True)
    rejects(lifecycle[:-1], require_lifecycle=True, launch_result=launch)
    rejects([line for line in lifecycle if 'no_game_start' not in line], require_lifecycle=True, launch_result=launch)
    rejects(moving + close + [line for line in second if 'ms=2500' not in line] + close + shutdown,
            require_lifecycle=True, launch_result=launch)
    rejects(lifecycle + [sample.replace('1000', '3000').replace('epoch=1', 'epoch=2')])
    print('PASS telemetry validity and strict two-load lifecycle acceptance, including incomplete runs')


if __name__ == '__main__':
    main()

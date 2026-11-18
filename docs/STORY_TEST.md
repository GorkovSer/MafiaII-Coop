# Opt-in main-story Continue diagnostic

This is a research aid, not a playable co-op feature. It is disabled by default.
Use the observer with `-ContinueMainStory -TestProfileFrom <earlier-test-session>`
on `scripts/start-probe.ps1`, together with `-SecondScreen -WaitForExit`.
The source profile must be an existing timestamped diagnostic session. It is
copied into a new isolated session; neither it nor the original profile is
overwritten. Do not enable this option in a normal user play session.

The observer checks the entire supported EXE hash before installing callbacks.
The helper additionally checks the installed Continue and main-menu-query
instruction prefixes. It runs only on the registered game thread's no-game tick,
requires actual profile redirection, readable GameBase/SaveManager objects, a
nonempty bounded save catalogue and one second of a positive main-menu query.
It makes at most one Continue request. It does not initialize freeride or patch
the save/DLC systems.

Installed GOG binary analysis: the Continue branch of `0x476250` dispatches through
`0x450c10` to `0x473c80` with ECX from `0x1ab2838`. This backend takes one stack
argument and returns AL with `ret 4`. Save lookup `0x11c3690` uses category 1 for
main-story save IDs 1 and 100..1999. The cdecl main-menu query `0x48c9c0` takes no
arguments and returns AL with plain `ret`. These observations apply only to the
pinned EXE, not other editions or versions.

Observed limits: save enumeration becomes available before DLC enumeration.
The earlier catalogue-only attempt returned 1 but showed a missing-DLC message
during LoadingDLC. The main-menu-gated attempt emitted mission_open, game_init
and load_finished and a read-only inspection found a local player. The GUI
remained over the paused mission. This is not proof of a visible playable scene,
automated movement, active network telemetry or a second player. Further work
must validate the normal GUI transition and unpause instead of treating the
backend's return value as successful gameplay.

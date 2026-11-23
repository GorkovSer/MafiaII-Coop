# Session presets — diagnostic transport

These configure the diagnostic telemetry transport, not playable story co-op.
The wrapper's preparation paths and interpolation/entity-bookkeeping components
passed their fixtures on 23 November. LoopbackHost also launched the real game
and transferred moving-player telemetry to a passive peer. Radmin two-PC
connectivity and a remotely controlled avatar have not been accepted.

`scripts/start-diagnostic-session.ps1` accepts LoopbackHost, LoopbackGuest,
RadminHost and RadminGuest. It requires GameDirectory and TestProfileFrom,
keeps the isolated profile, second-display placement and owned-process launcher,
and waits for normal game exit. It does not enable Human/native movement tests.
`-PrepareOnly` returns the selected launch parameters without accessing the game;
`scripts/test-session-presets.ps1` checks four presets and seven invalid endpoints.

Loopback presets use 127.0.0.1. Radmin presets require an explicit Address:
the host selects its own Radmin adapter IPv4, the guest selects the host's IPv4.
The wrapper does not install Radmin, modify firewall rules or expose all local
interfaces. Port defaults to 28960. No real addresses are stored in this repo.

The INI files show the existing coop-session.ini format. The Radmin .example
files intentionally contain invalid placeholders that must be replaced. The
wrapper generates the session-specific configuration through start-probe.ps1;
do not copy presets into the user's original game profile.

Remote-coordinate smoothing is a separate game-thread component. It keeps up
to eight accepted snapshots, delays interpolation by 100 ms and refuses to
bridge gaps over 250 ms. It requires current agreed-world authority on every
call and clears history on withdrawal, disconnect, expiry or identity changes.
There is no extrapolation. It is off by default in CoopRemoteActor and cannot
enable an engine backend or create world authority. Its fixtures do not establish
in-game interpolation quality or tune these values for Radmin latency.

# Shared-world agreement: implemented, execution deferred

2026-11-21. This adds the network contract required before remote actors can
enter a story world. It does not identify the loaded Mafia II checkpoint yet,
load a guest world, synchronize story effects or apply an avatar to the game.

## Contract

`CoopNetworkPublishWorld(loaded_epoch, verified_key)` accepts a nonzero local
load epoch and a 32-byte identity supplied by a future verified game producer.
The identity must describe the same mission/checkpoint/content on both PCs.
It must not be a pointer, save request ticket, arbitrary local epoch, or an
unverified hash of missionNumber/missionPart. A real load/rewind must change
loaded_epoch even when the identity is unchanged. The epoch must also agree
with the position producer's local epoch. Renew observation while loaded;
the current local TTL is 2000 ms. NULL withdraws authority immediately.

There is deliberately no production caller publishing a nonzero identity yet.
The diagnostic reader remains available, but `CoopNetworkReadRemoteWorld`
stays closed until a producer supplies one and both peers agree.

## Protocol 2 / module 0.0.4

HELLO/WELCOME compatibility still requires matching build/capabilities/module.
The protocol number changed from 1 to 2. Old builds cannot join this one.
All fields use the existing explicit big-endian encoding; packets remain at
most 64 bytes. Reliable channel 0 carries:

| Type | Payload (40 bytes) | Meaning |
| --- | --- | --- |
| 4 OFFER | host generation u32, host load epoch u32, key[32] | Host announces a world or withdraws it with zero epoch/key. |
| 5 ACK | offered generation u32, guest load epoch u32, key[32] | Guest confirms its own loaded identity or withdraws with zero epoch/key. |
| 6 COMMIT | offered generation u32, acknowledged guest epoch u32, key[32] | Host confirms the exact guest observation it accepted. |

Populated keys require a nonzero epoch; empty keys require zero epoch. Every
control needs a nonzero generation. COMMIT cannot be empty. Session, role and
channel are validated before applying controls. A conflicting equal-generation
offer or a future acknowledgement/commit is rejected. Older generations are
ignored without changing authority.

STATE grows from 48 to 52 bytes: the existing 24-byte header and position
payload are followed by host world generation at offset 48. Source player epoch
is still the local load epoch, not the host generation. Generation 0 explicitly
means telemetry without shared-world authority. Such data may be observed but
must never drive an actor. Nonzero-generation STATE is discarded unless both
generation and the acknowledged sender epoch match the local agreement.

## State machine and invalidation

The host increments generation on local loaded identity/epoch changes, including
withdrawal. No generation wraps: exhaustion closes authority until a new
connection. Guest readiness starts after receiving COMMIT; host readiness starts
after successfully queuing that COMMIT following a matching guest ACK. These
are per-peer gates, not a claim of simultaneous execution or durable progress.

A guest load change clears readiness and sends another ACK/withdrawal. A COMMIT
for its previous local load is ignored rather than disconnecting a legitimate
peer. The host checks the newest acknowledged guest epoch when accepting STATE.
Disconnect resets the entire agreement; connection identity and the existing
session/sequence checks protect against packets from a previous peer.

The game-thread publisher and network worker exchange copies under the existing
mailbox lock. World publication revision prevents an old worker read from
restoring authority after withdrawal. A changed revision forces worker-side
withdrawal before the latest observation, even if intermediate publications were
coalesced. Read-time checks also require current local activity, epoch agreement,
fresh local world/position observations, fresh remote state and a fresh worker.
`CoopNetworkReadRemoteWorld` clears its output on refusal. It is the required
API for future actor application; the older read API remains telemetry-only.

## Evidence and remaining work

The pure C world state machine, wire changes, worker integration and deferred
fixtures compile in the pinned x86 warnings-as-errors build. Tests were authored
for offer/ack/commit, local-load races, mismatch/withdrawal/recovery, old world
packets, malformed tokens and generation exhaustion. A two-process synthetic
world scenario is wired into test-network.ps1. None of these fixtures or suites
was executed under the user's continuing test hold. No game/input was run.

This is independently written code. The separation of world/actor authority and
ordered acknowledgement follows the design lessons reviewed in
[COOP_ANYTHING_REVIEW.md](COOP_ANYTHING_REVIEW.md); no external source was copied.

Next: a verified loaded-world producer and guest load transition, then actual
Human movement/stop/AI ownership on the game thread behind this gate. Story
effect application, NPC/combat/vehicle sync, host save completion and two-PC
Radmin mission/failure/reload acceptance remain outstanding. This handshake
compares supplied identities; it cannot prove that the game producer is correct.

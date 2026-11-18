# Script part, story event and save slot

Static findings, 2026-11-19. These are observations of chapter 3 bytecode,
not runtime acceptance or a campaign-wide checkpoint model. No Lua VM or game
was executed. Original assets and extracted chunks remain local and ignored.

Archive: `pc/sds/missionscript/m03_main.sds`, SHA256
`c3600efe7cffdaca8771bb787f2132774126f0a3aeb8fc37a40c24e3b35e1e1e1c`.
The Script resource contains four chunks. Prototype paths below start at root
`0`; child indices and instruction PCs are zero-based.

## Part selection

`M03Parts.lua`, chunk SHA256
`b614e4ed5d990b291e0c7c333425d21439d9cf12c72d220717d22ab380a25ece`:

- Prototype `0/1`, byte offset 547, debug lines 16–39: PCs 4–8 read
  `game.datastore:GetNumber("missionPart")`; PCs 10–15 read `missionNumber`.
  Six branches compare the part with 1 through 6 and call the corresponding
  part function.
- Prototypes `0/3` through `0/7`, offsets 2816/3298/3780/4262/4744:
  PCs 16–21 call datastore `SetNumber` for `missionPart`, with values 2–6
  respectively.

Thus missionPart is a part selector in this inspected chapter. This does not
establish its range in other chapters or prove checkpoint uniqueness.

## Save event and save slot

`M03Saves.lua`, chunk SHA256
`7c7554d541d6e2d7d418f30c91b43999fc986a9f09e685dac3db5d282f05cb48`:

- Root `0`, offset 12, PCs 0–35 constructs `listOfSaves`: story ID 300 has
  type `chapter`; IDs 305, 310, 315, 320, 321, 322, 323, 324, 325 and 330
  have type `progress`.
- `0/1`, offset 540, debug lines 26–39 matches its supplied story ID to this
  table. PCs 15–19 call `game.game:SaveGame(supplied_story_id)` for the chapter
  branch. PCs 26–30 call `game.game:SaveGame(1)` for the progress branch.
  This function does not set missionNumber or missionPart.
- `0/2`, offset 816, debug lines 41–49 handles a message and passes its third
  field to the save function. The story event ID is distinct message data.

Part selector, save event ID and save slot therefore need separate treatment.
Multiple progress events target the same SaveGame argument. This is NOT a
runtime demonstration of two distinct checkpoints sharing the same script pair:
external code could change the pair before these calls, and reachability has
not been established. Uniqueness of the pair remains unproven. Do not authorize
remote actors or checkpoint restoration from that pair alone.

## Static reader

`scripts/read_lua51.py` accepts an already extracted raw chunk. For example:

```powershell
python scripts/read_lua51.py <local-chunk.luac> --watch missionNumber missionPart SaveGame
```

It emits a chunk hash and matching references with prototype/PC coordinates;
it does not emit source, constant tables or full disassembly. The supported
header is exactly `1b4c75615100010404040400` (the observed Lua 5.1 PC layout).
Input is capped at 8 MiB, 4096 prototypes, depth 64 and 200000 instructions.
Counts, bounds, string terminators, relevant constant indices and instruction
extension lengths are checked. This is not a full bytecode verifier or control
flow analysis. Output explicitly says `executed=false` and
`reachability_verified=false`.

Independently written using the official format and opcode references:
[Lua undumper](https://www.lua.org/source/5.1/lundump.c.html),
[Lua opcodes](https://www.lua.org/source/5.1/lopcodes.h.html).
An SDS extractor is not included in this stage; original assets must not be
committed. Static reads of six local chunks completed, including four M03
chunks and one each from M01part2 and M02_mission_end. The latter two did not
contain the watched references; this narrow result says nothing campaign-wide.

Next: trace the native SaveGame binding and story event producer, then, only
after the user resumes tests, correlate actual checkpoint/failure/reload events
with script progress, save event data and host transition generation.

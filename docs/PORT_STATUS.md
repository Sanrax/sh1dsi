# Port status

The native build starts in Midwich's music room with Harry at the piano.
He can walk, turn, approach and solve the keyboard puzzle, watch or skip the
medallion fall, and collect the silver medallion. Declining leaves it on the
floor. Accepting keeps one for the session. Harry can cross the second-floor hallway (room 32),
enter the eastern hallway (room 31), walk down its stair ramp and return through
both doors. The first-floor load and saving are pending. The full game is unplayable.

## Verified build and inputs

- BlocksDS 1.24.0 and Wonderful GCC 16.2.0 produce a DSi-exclusive ROM
  (unit code 3). Repeated clean builds and local conversions were byte-identical.
- USA 1.1, one MODE2/2352 track, 616,494,480 bytes, SHA-1
  `34278d31d9b9b12b3b5db5e45bcbe548991ecbc7`. All non-XA file-table entries
  agreed with upstream silentassets. MAP1_S01 matched the upstream SHA-1.
- melonDS 1.1 direct boot used external BIOS, firmware and a private NAND
  copy. The SDK filesystem sample and port reported TWL mode and melonDS
  reported 16 MB RAM. Virtual console SD and emulated DLDI each completed the
  puzzle, decline, second pickup, accept and failed repeat interaction. B
  cancelled on DLDI. The lower LCD stayed black. Physical SD and DSpico
  launches remain unverified.

## Timing

PS1 measurements used the Beetle PSX software core (Debian build
0.9.38.6+git20260515+ds-2), USA 1.1, digital pad and a 59.94 Hz clock.
The owner's SCPH-7001 BIOS differed from the core's expected SCPH-5501.
The street route starts a new game. The school routes seed a private Continue
slot at the piano, then play the original game with controller inputs.
Reproduction instructions are on `diagnostics/dsi`.

| Case | Sample | Measured interval |
| --- | --- | --- |
| PS1 opening street | 368 buffer-change intervals | All 2 VBlanks, 33.37 ms |
| PS1 school keyboard | 1,322 buffer-change intervals | All 1 VBlank, 16.68 ms |
| PS1 silver pickup and first corridor view | 47 intervals on each of two prompts and 67 in room 32 | All 2 VBlanks, 33.37 ms |
| PS1 crossing room 32 after pickup | 474 buffer-change intervals | 442 at 33.37 ms and 32 at 50.05 ms |
| PS1 eastern hallway through stair descent | 357 buffer-change intervals | All 33.37 ms |
| melonDS idle music room with Harry | 300 scene submissions | 16.715 / 33.317 / 33.432 ms min/mean/max, 2.893 ms mean ARM9 submission time |
| melonDS rotating silver pickup with prompt | 300 scene intervals | 16.713 / 33.374 / 33.432 ms min/mean/max, 3.291 ms mean ARM9 submission time |
| melonDS idle corridor after collecting the medallion | 300 scene intervals | 16.713 / 33.317 / 33.432 ms min/mean/max, 4.137 ms mean ARM9 submission time |
| melonDS idle eastern hallway with medallion | 300 scene intervals | 16.717 / 33.373 / 49.966 ms min/mean/max, 5.506 ms mean ARM9 submission time |
| melonDS idle keyboard | 600 display-loop intervals | 16.713 / 16.715 / 16.717 ms min/mean/max |

The PS1 uncapped VBlank counter corroborated the display-buffer measurements.
The pickup and corridor each had a one-VBlank minimum and no intervals over 34 ms.
At the room 32 spawn the GPU retained 246 polygons and 922 vertices in one
frame. Room 31 retained 77 polygons and 309 vertices and had one interval
above 34 ms in the 300 samples.
The native simulation advances in 16.683 ms steps. Real DSi timing is pending.

## Scene and interaction checks

- The 256×192 room uses SCFD00, four texture pages for scenery and Harry
  (112 KiB), and 1,504 bytes of palettes. Eight room instances, the piano
  lid and medallion submit 764 quads per view. Unarmed Harry adds 364
  primitives across 19 meshes and transforms 332 distinct vertices once per
  update. The two idle poses and held pickup pose match all 17 non-root
  PS1 bone transforms. Character shading and room lighting differ.
- The flat-floor solver uses 51 original splits and Harry's 0.3 m radius.
  A 1,020-VBlank walking route matched all 13 sampled headings. The largest
  sampled XZ error was 3.6 cm, the west-wall stop differed by 3.4 mm and
  the final piano-side stop by 1.5 cm. Turning away or backing out of range
  prevented piano interaction. Running, footsteps, camera damping, dynamic
  obstacles and collision on other floors are pending.
- Piano key regions, cursor snapping, sound delays, incorrect-sequence
  resets and the five-key solution follow MAP1_S01. All 49,152 overview
  pixels matched the prepared 5-bit colors. The initial cursor changed 49
  pixels. Four MEP101.VAB samples play through native channels. A key strike
  and medallion impact peaked at 15,096 and 15,710 in isolated signed 16-bit
  recordings. PS1 envelopes and reverb remain to be matched.
- The original fall holds, swings, drops and holds for 90, 32, 44 and 62
  VBlanks. The port then holds the landing camera for two seconds and restores
  gameplay. Whole-frame mean absolute RGB errors against the resampled PS1
  were 4.0/255 for the keyboard, 10.5/255 hanging, 8.7/255 fallen, and
  16.8/255 and 15.3/255 for the pre- and post-puzzle gameplay views. The
  reference uses the 320 active columns beginning at column 14 of the
  core output. Perspective texture interpolation and whole-polygon alpha differ.
- Pickup uses the map's facing sector, HB_M1S01.ANM clips 873–899, 156
  textured Gouraud triangles from UNQ62.TMD, a 64×64 TIM01.TIM patch, map
  message 30 and FONT16.TIM glyphs. The rotation changed 2,113 central
  pixels across 40 melonDS samples. All 329 bright green item-name pixels
  matched the resampled PS1 frame. Item lighting and selection audio still differ.
- The original game's `door` route verifies flag 70 and one item 98 across
  a round trip after the pickup. MAP_POINTS[41] leads from music room 39 to
  corridor room 32 via point 39. Input-to-gameplay intervals were 83 VBlanks
  outbound and 63 returning in this replay. Corridor spawn X/Z was
  (-116326, 77004) in Q12, yaw 0. Returning placed Harry at (-417792, 95437),
  yaw 2048. A melonDS diagnostic recorded both native spawns and headings
  exactly and confirmed the collected medallion after both crossings. The console SD production replay solved the puzzle, collected the
  medallion and crossed both doors. DLDI also completed an unsolved round trip.
- SCFF00 loads 1,263 corridor quads, two 4-bit textures (64 KiB) and 20
  level-path collision edges. Harry's texture stays in VRAM during the
  crossing. The load screen includes the original one-second minimum.
  Whole-frame mean absolute RGB errors were 19.3/255 at corridor entry and
  27.6/255 at reentry. The corridor chase camera and lighting still differ.
- Both doors play the original BASE.VAB opening and closing samples. On DLDI,
  all four onsets matched the local samples (correlation 0.74–0.90, best-fit
  rates within 0.3%). The PS1 recording gave 0.89–0.96. Opening to closing
  took 1.00 s in both melonDS crossings and 1.34/1.01 s in the PS1 replay.

- The east exit (MAP_POINTS 36 ↔ 51) streams SCFE01. Its 1,958 quads use
  80 KiB of textures. Actor palettes stay resident while the scenery changes.
  The 43 collision edges include the stair ramp and landing. The two doors
  use different original BASE.VAB sound pairs. In isolated recordings the
  second pair's playback rates were within 0.3% of the PS1 reference.
- The eastern doorway matched both original player spawns. Room 31 starts at
  (-208896, 201113), yaw 1024, and room 32 resumes at (-44236, 81920), yaw 3072
  (Q12). On emulated DLDI the production ROM completed the round trip with
  the bottom screen black. A console SD diagnostic solved the piano, collected
  the medallion and verified the inventory on return. Whole-frame RGB errors were 17.9/255 outbound and
  16.7/255 returning. The eastern doorway is darker than the reference.
- The original game's `stairs` replay follows the ramp south of the eastern
  entrance. Point 80 loads MAP1_S00 room 12 with one medallion. The native
  collision replay reached the lower landing. Camera smoothing, enemies and
  the transition to the first floor remain incomplete.

## Next

1. Load SC0201 at the original stair trigger and verify the return trip with the medallion.
2. Follow the first-floor doors toward the original save point.
3. Match the original walking and turning sounds and check moving corridor lighting.

Based on the original GPL-3.0 decompilation, Copyright (C) 2026
shdecompilations. [Pinned upstream source](https://github.com/shdecompilations/silent-hill-decomp/tree/ecbb0d8e3881cf2f0cb9fa700f3bb1a74b6d34ce)

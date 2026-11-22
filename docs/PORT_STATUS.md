# Port status

Harry starts at the Midwich piano. He can solve the keyboard puzzle, watch
or skip the falling silver medallion, decline or accept it, walk through the
eastern hallway, descend the stairs, unlatch the first-floor door and enter
the southern hallway. The first-floor stairs reach the basement and boiler
room, and Harry can return the same way. The switch powers the boiler after
Harry places the silver medallion, and the infirmary notebook saves that state.
The courtyard connects the southern and northern halls.
Harry can inspect the clock tower door and place both medallions. With the
boiler powered he can enter the tower and cross the underground passage.
Both ladders work in both directions. Harry can use the far tower's outer
door, watch or skip the Otherworld arrival scene and walk the courtyard.
Y switches the flashlight after the scene, changing the light and fog.
The north doors reach the Otherworld north hall and lead back to the courtyard.
The north hall connects to the lobby, where Harry can collect the first-aid
kit and handgun bullets and inspect the fans. The north hall also connects
through the side hall to the Otherworld east hall. Harry can take the Picture
Card from the north classroom and cross the south classroom to get past
the east hall barrier and reach reception through the Otherworld south hall.
Harry can use the Picture Card in reception to open the west hall.
The women's restroom takes Harry from there to the upstairs west hall
and back. The two first-floor teachers' rooms provide a route around
the west-hall barrier. Facing the north room's exit triggers the
original phone call. Start skips it and returns control to Harry. The
doors beyond the barrier unlock from the west hall and reconnect it to
the north hall. The storage room opens from the north hall, and Harry can
inspect, decline or collect the Rubber Ball.
Upstairs, the east corridor connects through the north hall to the chemistry lab
and equipment room. Harry can collect the Chemical, open the hand statue
and take the gold medallion.
In the infirmary he can pick up both medical supplies and use the notebook.
A cold boot can resume at the desk with the puzzle,
door latch and pickups intact. The full game remains unplayable.

## Build and media

- BlocksDS 1.24.0 and Wonderful GCC 16.2.0 produce a TWL-exclusive ROM
  (unit code 3). A forced rebuild and repeat conversion reproduced the ROM
  and the local data files byte for byte.
- USA 1.1 CUE, one MODE2/2352 track, 616,494,480 bytes, SHA-1
  `34278d31d9b9b12b3b5db5e45bcbe548991ecbc7`. Non-XA file entries agreed
  with upstream silentassets. All converted data stays on the owner's machine.
- melonDS 1.1 direct boot uses external BIOS, firmware and private NAND.
  The SDK filesystem sample and port verified TWL mode, and the emulator
  reported 16 MB RAM. Physical console SD and DSpico/Pico-Launcher remain
  untested. The official DSpico DLDI driver built from `8ba45f6` fits the
  ROM's 16 KiB patch area. The lower screen stays black in production.
  [DSpico driver source](https://github.com/LNH-team/dspico-dldi/tree/8ba45f65690bc40d9279e663e1d89ca806451cc1)

## Frame pacing

PS1 references use Beetle PSX software rendering, USA 1.1, a digital pad
and the 59.94 Hz clock. The street replay starts a new game.
School replays seed a private Continue slot and then use controller input.
See the replay instructions on `diagnostics/dsi`. Native simulation uses
16.683 ms steps. Hardware timing is pending.

| Case | Sample | Measured interval |
| --- | --- | --- |
| PS1 opening street | 368 buffer-change intervals | All 2 VBlanks, 33.37 ms |
| PS1 school keyboard | 1,322 buffer-change intervals | All 1 VBlank, 16.68 ms |
| PS1 silver pickup and first corridor view | 47 intervals on each of two prompts and 67 in room 32 | All 2 VBlanks, 33.37 ms |
| PS1 crossing room 32 after pickup | 474 buffer-change intervals | 442 at 33.37 ms and 32 at 50.05 ms |
| PS1 eastern hallway through stair descent | 357 buffer-change intervals | All 33.37 ms |
| PS1 basement stair descent and return | 490 buffer-change intervals | All 33.37 ms |
| PS1 boiler room approach and return | 316 buffer-change intervals | 1 at 16.68 ms, 292 at 33.37 ms and 23 at 50.05 ms |
| PS1 crossing the southern hallway | 176 buffer-change intervals | 70 at 33.37 ms and 106 at 50.05 ms |
| PS1 infirmary from entrance to save interaction | 255 buffer-change intervals | 1 at 16.68 ms, 247 at 33.37 ms and 7 at 50.05 ms |
| PS1 health drink and first-aid kit events | 130 drink and 126 kit buffer-change intervals | Drink had 9 at 33.37 ms and 121 at 50.05 ms. Kit intervals were all 50.05 ms |
| PS1 courtyard entry, walk and return approach | 183 buffer-change intervals | 45 at 33.37 ms, 117 at 50.05 ms and 21 at 66.73 ms. Mean 47.86 ms |
| PS1 northern hallway entry and walk to the return door | 206 buffer-change intervals | 1 at 16.68 ms, 203 at 33.37 ms and 2 at 50.05 ms. Mean 33.45 ms |
| PS1 upstairs north hall on the way to the lab | 392 buffer-change intervals | 372 at 33.37 ms and 20 at 50.05 ms. Mean 34.22 ms |
| PS1 chemistry lab entry and turn | 101 buffer-change intervals | All 33.37 ms |
| PS1 equipment room entry and return approach | 87 buffer-change intervals | 60 at 33.37 ms and 27 at 50.05 ms. Mean 38.54 ms |
| PS1 Chemical pickup | 126 buffer-change intervals across two prompts | All 50.05 ms |
| PS1 hand statue pour and dissolve | 235 buffer-change intervals | 102 at 33.37 ms and 133 at 50.05 ms |
| PS1 gold pickup | 127 buffer-change intervals across two prompts | 4 at 33.37 ms and 123 at 50.05 ms |
| melonDS idle music room with Harry | 300 scene submissions | 16.715 / 33.317 / 33.432 ms min/mean/max, 2.893 ms mean ARM9 submission time |
| melonDS rotating silver pickup with prompt | 300 scene intervals | 16.713 / 33.374 / 33.432 ms min/mean/max, 3.291 ms mean ARM9 submission time |
| melonDS idle corridor after collecting the medallion | 300 scene intervals | 16.713 / 33.317 / 33.432 ms min/mean/max, 4.137 ms mean ARM9 submission time |
| melonDS idle eastern hallway with medallion | 300 scene intervals | 16.717 / 33.373 / 49.966 ms min/mean/max, 5.506 ms mean ARM9 submission time |
| melonDS first-floor stair entry with medallion | 300 scene intervals | 16.717 / 33.374 / 33.438 ms min/mean/max, 5.468 ms mean ARM9 submission time |
| melonDS idle keyboard | 600 display-loop intervals | 16.713 / 16.715 / 16.717 ms min/mean/max |
| melonDS southern hallway with medallion | 300 scene intervals | 16.717 / 33.373 / 49.968 ms min/mean/max, 5.138 ms mean ARM9 submission time |
| melonDS infirmary entrance with medallion | 300 scene intervals | 16.711 / 33.317 / 33.434 ms min/mean/max, 3.920 ms mean ARM9 submission time |
| melonDS infirmary health drink | 300 scene submissions during the pickup event | 16.713 / 33.374 / 33.432 ms min/mean/max, 4.263 ms mean ARM9 submission time |
| melonDS courtyard entry, straight walk and turn | 300 scene intervals | 16.711 / 33.318 / 33.457 ms min/mean/max, 6.911 ms mean ARM9 submission time |
| melonDS northern hallway entry, walk and turn | 300 scene intervals | 16.711 / 33.374 / 33.443 ms min/mean/max, 3.756 ms mean ARM9 submission time |
| melonDS upstairs north hall entry, walk and turn | 300 scene intervals | 16.719 / 33.374 / 33.443 ms min/mean/max, 3.869 ms mean ARM9 submission time |
| melonDS chemistry lab from entry to the east aisle | 300 scene intervals | 16.713 / 33.374 / 33.447 ms min/mean/max, 3.702 ms mean ARM9 submission time |
| melonDS equipment room from entry to the west aisle | 300 scene intervals | 16.719 / 33.374 / 33.445 ms min/mean/max, 4.413 ms mean ARM9 submission time |
| melonDS rotating Chemical with prompt | 300 scene intervals | 16.713 / 33.318 / 33.432 ms min/mean/max, 4.748 ms mean ARM9 submission time |
| melonDS statue close camera | 271 scene intervals | 16.711 / 33.368 / 34.249 ms min/mean/max, 4.014 ms mean ARM9 submission time |

## Room geometry submission

- The loader packs the room vertices into geometry FIFO commands and
  flushes the data cache once. DMA submits each visible batch in its
  original order. The existing local sidecars work with this build.
- In melonDS DSi mode, each case sampled 128 scene intervals. Mean
  intervals stayed at 33.43 ms in the music room and courtyard and
  33.30 ms at the infirmary desk.

| Scene | Previous mean submission | Packed mean submission |
| --- | --- | --- |
| Music room | 2.94 ms | 2.02 ms |
| Moving medallion | 2.94 ms | 2.00 ms |
| Courtyard entrance | 6.98 ms | 3.74 ms |
| Courtyard below the ramp | 7.00 ms | 3.66 ms |
| Infirmary desk | 3.99 ms | 2.43 ms |

- All 49,152 pixels matched in each of six comparisons covering the music
  room, moving medallion, two courtyard views and both infirmary item
  states. Collecting the infirmary items changed 541 pixels in both builds
  at the fixed diagnostic view.
- The courtyard geometry buffer grew from 82.75 to 144.81 KiB, and its
  emulator room-load sample rose from 45.80 to 47.98 ms. The music-room
  buffer grew from 24.69 to 43.20 KiB. Real hardware timing is pending.
- The production ROM streamed the music room and its corridor in both
  directions over DLDI. The virtual console SD replay crossed from the
  infirmary through the southern hall to the courtyard, turned and
  returned through the southern door. Both retained their expected
  spawn poses, inventory state and black lower screen.

### Compact vertex commands

- Each batch starts with a full vertex and uses two-coordinate commands
  where an axis matches the preceding vertex. The host check decoded
  all 67,096 vertices and UVs in the eleven room streams exactly.
  [BlocksDS vertex commands](https://blocksds.skylyrac.net/libnds/videoGL_8h.html)
- Six matched melonDS frames were pixel-identical, including the moving
  medallion and both infirmary item states. Decoding the DMA buffers
  from those runs also matched every vertex, UV and batch boundary.
- The courtyard allocation fell from 144.82 to 108.80 KiB and the music
  room from 43.21 to 32.76 KiB. Mean courtyard submission below the ramp
  fell from 3.62 to 3.35 ms over 128 intervals. Scene intervals averaged
  33.30–33.43 ms. The courtyard's sampled load rose from 48.70 to 52.12 ms.
- The production build also passed the music-room corridor round trip over
  DLDI and the courtyard round trip over virtual console SD.

### Room-frustum culling

- Static room geometry is grouped into spans of up to 32 quads. The renderer
  skips a span only when its bounds lie past a clip plane, with a guard for
  fixed-point rounding. It sends visible spans in their existing order and
  submits animated and event geometry with their existing transforms.
- The host check decoded all 67,096 positions and UVs exactly. In 18 matched
  views across 11 rooms in melonDS DSi mode, every pixel and both GPU RAM
  counts matched. Peak use was 1,143 polygons and 4,581 vertices, with no
  overflow flag. [DS geometry RAM limits](https://mgba-emu.github.io/gbatek/#ds-3d-status)
- Across 1,134 measured intervals, mean submission fell from 2.46 to 2.02 ms.
  It fell from 2.45 to 1.48 ms at the upstairs stairwell and from 3.00 to
  1.83 ms at the courtyard tower. The dense southern entrance rose from
  2.72 to 2.87 ms. Mean intervals were 33.32 and 33.36 ms, with four over
  34 ms in each run, all on the first interval of a test view. The longest
  was 42.36 ms before and 47.57 ms after. The remaining intervals topped
  out at 33.43 ms in both runs.
- The bounds table adds 2,982 bytes of main RAM. Restarting coordinates at
  span boundaries adds 200 bytes to the courtyard command buffer. Hardware
  timing remains pending.
- Production replays passed the music-room corridor round trip and the
  full falling-medallion sequence over DLDI, plus the southern hall to
  courtyard round trip over virtual console SD.

### Reusing visibility checks

- Static chunks reuse their visibility results when all 16 entries in
  the clip matrix match. A camera change recalculates visibility during
  submission, and a room load clears the cache. The cache uses 278
  bytes of main RAM.
- Six melonDS DSi views contributed 128 samples each. Mean ARM9
  submission time fell from 2.143 to 2.055 ms. Forcing a recalculation
  on every submission measured 2.152 ms. Mean frame intervals were
  33.39 ms in all three runs. All six 256×192 captures matched, including
  the held medallion event frame. Peak geometry RAM use remained
  1,143 polygons and 4,581 vertices with no overflow.
- The 16- and 8-quad experiments submitted fewer offscreen quads and
  raised mean ARM9 time to 2.184 and 2.412 ms. Deferring empty batch
  setup measured 2.146 ms. The production build uses 32-quad chunks.
- The production ROM passed the infirmary round trip over DLDI and
  refreshed the visibility cache while crossing the courtyard ramp
  over virtual console SD.

### Room loading

- Room streams use a 16 KiB read-ahead buffer. The loader takes the file
  length from the open descriptor when available, preserving the buffer
  through the switch to geometry. Large audio reads use 2 KiB requests.
  The room files keep their existing layout. [BlocksDS filesystem I/O](https://blocksds.skylyrac.net/docs/internal/design_guide/#5-filesystem-support)
- In melonDS DSi mode, each room was timed for six loads after one warm-up,
  from opening the file through closing it. The mean of the 11 room medians
  fell from 61.99 to 20.75 ms on virtual console SD and from 45.97 to
  44.24 ms on DLDI. The virtual SD courtyard fell from 76.22 to 28.40 ms
  and the lab from 86.36 to 28.50 ms. Real hardware timing remains pending.
- All eleven rooms matched geometry and VRAM checksums on both paths.
  The resident lab audio buffer matched on virtual console SD. Production
  replays passed the music-room corridor and courtyard round trips. The
  loader rejected both truncation cases with the existing error screen.

## Animated character submission

- The actor draw uses ARM instructions for the fixed-point math and packs
  each transformed position once for the indexed corners. The packed
  position buffer adds 768 bytes. [GCC ARM function attributes](https://gcc.gnu.org/onlinedocs/gcc-11.5.0/gcc/ARM-Function-Attributes.html)
- Over 64 draws of each of ten poses in melonDS DSi mode, mean actor draw
  time fell from 1.233 to 1.073 ms. Bone interpolation fell from 0.112 to
  0.096 ms and vertex transforms from 0.350 to 0.196 ms. Geometry submission
  rose from 0.771 to 0.780 ms. Scene intervals averaged about 33.4 ms.
- The ten held poses matched every bone matrix, transformed vertex and
  top-screen pixel. The host check matched all 332 positions in each of
  8,316 frame and blend samples across the eight clips.
- The production DLDI run solved the piano and took the silver medallion
  through the kneeling inspection. A separate DLDI run passed the corridor
  round trip. The virtual console SD run walked and turned in the courtyard
  and returned to the southern hall.

### Actor geometry DMA

- Each palette batch sends pairs of textured vertices through the geometry
  FIFO. One scratch buffer serves all batches and uses 4,480 bytes for
  the current model. Odd batches send their last vertex directly.
- In melonDS DSi mode, 64 draws at each of ten fixed poses across three
  rooms reduced mean actor draw time from 1.038 to 0.983 ms. Submission,
  including packing and cache maintenance, fell from 0.741 to 0.695 ms.
  Mean scene intervals were 33.35 ms in both builds. All ten 256×192
  frames matched pixel for pixel. Real hardware timing remains pending.
- The production build passed the music-room corridor round trip over
  DLDI and the courtyard round trip over virtual console SD.

## Scene and interaction checks

- The top screen uses the native 256×192 viewport. Harry has 19 meshes and
  332 transformed vertices. All 17 non-root transforms matched the original
  in sampled idle and pickup poses. A 1,020-VBlank movement replay matched
  13 sampled headings with up to 3.6 cm of horizontal position error.
- The five-key puzzle, wrong-key resets and medallion drop use the original
  assets and event timings. The native overview matched all 49,152 prepared
  5-bit pixels. The rotating pickup uses original model, pose, text and font.
  Virtual console SD and DLDI replays declined, accepted and checked the
  single-item inventory. The native prompt matched all 329 bright green
  item-name pixels in the resampled PS1 image.
- Music room 39, hallways 32 and 31, and first-floor room 12 stream from
  local sidecars. Two door pairs use their original BASE.VAB samples.
  The PS1 and native stair warps matched in both directions. Whole-frame
  mean absolute RGB error (out of 255) was 4.0 at the keyboard, 19.3 at
  the first corridor entrance, 17.9 at the eastern entrance, 17.5 on the
  first floor and 15.3 upstairs. Camera smoothing, shading, footsteps,
  enemies, fog and PS1 audio envelopes/reverb remain incomplete.
- Point 50 uses the original half-metre facing ray, plays BASE.VAB effect
  1342, waits 0.2 s and displays MAP1_S00 message 13. Acknowledging the
  message releases the latch. The next action enters room 13 through point
  34. A console SD diagnostic kept the medallion on two visits and matched
  PS1 spawns at (611123, 81920), yaw 3072, and (438271, 201113), yaw 1024.
  The production DLDI build displayed the unlock message (243 bright pixels)
  and matched both spawn positions and headings using the release ELF symbols.
  Room 13 streams SC0300, with 1,843 quads, three 4-bit textures and 43
  collision edges. At entry, the GPU retained 1,143 polygons and 4,581
  vertices. One of 300 intervals exceeded 34 ms. Entry RGB error was 17.7/255, and return error was 18.0/255.
- The PS1 `infirmary` route checks flag 1079 after the latch, carries one
  medallion into room 9, and operates the save point at MAP1_S00 point 89.
  The separate PS1 `save` replay writes and verifies the first slot on a
  scratch memory card, including its checksum, location 4, flags and item
  count. In this replay the PS1 stored (251381, 576233), yaw 1002.

- The DLDI production build streamed room 9 (1,211 scenery quads and 45
  collision edges). Harry landed at (233472, 587366),
  yaw 2048, and returned to room 13 at (591462, 77004), yaw 0. Both matched
  the PS1 trace. He could reach the eastern desk camera, walk into the counter
  twice to the same stopping X position and back away. A console SD replay
  kept the medallion through both doors. At entry the GPU retained 411
  polygons and 1,589 vertices. Whole-frame RGB error
  was 23.2/255 at entry and 27.0/255 at return. Lighting and camera transitions
  still differ from PS1.
- The notebook, drink and kit add 34 original BG_ITEM.PLM quads, four CLUTs
  and an 8 KiB texture page. The production DLDI build reached the desk
  with a whole-frame RGB error of 30.3/255 against the PS1 desk view.
  A console SD diagnostic hid and restored the items at a fixed camera:
  they changed 98 stationary pixels at the desk and 133 near the cabinet.
  At the entrance none were in view. With items loaded, 300 idle intervals
  measured 16.717 / 33.318 / 33.430 ms, with 3.992 ms mean submission time.

## Notebook checkpoint

- The notebook at point 89 uses the original first-visit text, font and save
  background. All 1,155 bright PS1 text pixels matched after resampling to
  256×192. Slot selection and confirmation use native controls. The port
  writes its own checkpoint format, distinct from a PS1 memory-card file.
- In melonDS 1.1, DLDI loaded a seeded desk position with the medallion.
  The port created slot 2, cancelled and confirmed overwrites, then cold-loaded
  its own slot at (250931, 576228), yaw 1028. The inventory contained item
  98 ×1 and the first-floor latch remained open. Host checks validated both
  generations, all eleven slots and recovery at each of 32 truncated lengths.
- On virtual console SD, the supplied melonDS 1.1 stalled during both the
  SDK sample write and the port’s write. Upstream build `361ffa9` completed
  both. A fresh playthrough walked from the piano, unlocked the latch and
  created a checkpoint. With the emulator restarted, the same ROM returned
  to (250982, 576220), yaw 1028, with the latch open and inventory empty.
  Both modes reported 16 MB; physical storage remains untested.
  [Upstream SD controller fix](https://github.com/melonDS-emu/melonDS/pull/2438)

### Boot and notebook timing

- The notebook renderer writes two RGB555 pixels per aligned word. Across
  three cold launches with a saved slot in melonDS DSi mode, median time
  from the app timer starting to the first load-list draw fell from 75.95
  to 70.63 ms on virtual console SD and from 151.18 to 145.86 ms on DLDI.
  The first notebook draw saved 5.32 ms on both paths. With an empty card,
  first scene submission took 71.69 ms on virtual console SD and 138.59 ms
  on DLDI in both builds.
- The timed save runs created and verified a generation in 6.78–7.18 ms
  and overwrote and verified it in 2.05–3.02 ms. Selecting Continue to
  first scene submission measured 35.25 ms on virtual console SD and
  51.97 ms on DLDI. Physical storage timings remain pending.
- Production replays on both paths cancelled creation, created and
  overwrote a slot, checked both record generations and cold-loaded the
  desk pose and flags. Ten menu captures per path matched after masking
  the elapsed-time field on the final cold load.

### Startup scene loading

- The load list draws before the game reads the first room. Continue reads
  the shared player data from the existing music-room file. It skips the
  80 KiB scenery texture prefix and geometry packing. New Game loads the
  full room.
- Over three cold launches per case in melonDS DSi mode, the load list
  drew at 37.83 ms on virtual console SD and 96.35 ms on DLDI, down from
  71.26 and 146.50 ms. Continue to first scene submission took 35.10 ms
  on SD in both builds and rose from 51.81 to 85.24 ms on DLDI. Adding the
  two intervals, with player wait excluded, gives 72.93 ms on SD and
  181.59 ms on DLDI, down from 106.36 and 198.31 ms.
- With a saved slot present, choosing New Game took 35.09 ms on SD and
  68.52 ms on DLDI after selection. Empty-card launches reached the first
  scene at 54.96 ms on SD and 138.57 ms on DLDI. All eight fixed menu and
  scene captures matched pixel for pixel across the two paths.
- Production builds resumed through the southern hall on both media, walked
  the courtyard round trip over virtual console SD, and passed the music-room
  corridor round trip over DLDI. The missing-room fallback and two truncated
  room cases matched the previous screens on an empty card.

## Infirmary pickups

- MAP1_S00 points 97 and 98 use the original standing pickup flow, with
  locally converted models, texture and prompts. The native prompts matched
  all 265 bright green drink-name pixels and all 254 kit-name pixels in the
  resampled PS1 captures. The scene camera and item lighting still differ.
- The melonDS 1.1 DLDI run loaded an existing-format slot with the medallion.
  Harry declined and then took each item, left and re-entered the infirmary,
  and wrote a new notebook slot with the supplies, medallion and latch.
  After restarting melonDS, the same build loaded the saved pose and flags.
  The same replay passed on virtual console SD in upstream build `361ffa9`.
  The regenerated sidecar is required for the interactions.
- The silver medallion and medical prompts use BASE.VAB effects 1305–1307
  for changing the selection, declining and accepting. An isolated melonDS
  recording verified three distinct PCM onsets and silence when pressing
  Up on the already selected first option.

## Stair transitions

- A stair link now carries its trigger, arrival pose and camera anchor
  through the room load. Stair links skip door sounds.
- In melonDS DSi mode, both DLDI and virtual console SD resumed an infirmary
  checkpoint, crossed the southern hallway and walked the existing stairs
  in both directions. Both PS1 and native replays reached Q12 X/Z
  (454656, 188006) downstairs and (−192512, 174899) upstairs with yaw 3072.
  The native replays retained the items and latch and kept the lower LCD black.
- The basement stair and boiler room each need 80 KiB of scenery textures,
  which fits before the resident player texture. The boiler props use index
  zero for transparency. The conflicting index belongs to an unused row.
  The PS1 replay matched the authored stair planes at both arrivals
  (−3600 Q12 Y in the basement and 3552 on the first floor). Across 276
  interior slope samples, the basement plane was within 1.5 cm of the
  PS1 player's Y. The basement ramp has four corners and its upper
  landing has five.

### Room floor planes

- Courtyard sidecars now carry a floor plane count and a corner count
  per plane. The loader supports convex footprints with three to eight
  corners and reads both older courtyard layouts. It uses 14 planes
  in the courtyard and clears them on exit.
- The host check matched the old height and slope at 1,640,961 courtyard
  positions for each of the two legacy layouts and the counted layout.
  A five-corner fixture checked the slope and clipped corner. Malformed
  counts and truncated records were rejected.
- In melonDS DSi mode, the counted sidecar passed the courtyard ramp
  round trip over DLDI and virtual console SD. The DLDI replay also
  matched both arrivals on the upper stair round trip. The courtyard
  stream grew by 16 bytes and the floor plane array by 252 bytes.
  The counted floor data is ready for the basement stream.

### Room collision edge counts

- The eastern stairwell and first floor now store their edge counts in
  their sidecars. The loader uses the same count validation as the
  courtyard and accepts the earlier sidecars. The two streams each grow
  by two bytes.
- Host checks matched 659,976 collision moves per room against the
  previous loader. Synthetic 25- and 35-edge tables also loaded.
  Truncated tables and counts that exceeded storage were rejected.
- New sidecars passed the existing stair round trip in melonDS DSi mode
  over DLDI and virtual console SD. Both routes matched the PS1 arrival
  poses and stopped at Q12 X 437861 on two attempts to back into the
  first-floor double door. The DLDI replay also passed with the earlier
  sidecars.

### Stairwell edge heights

- The eastern stairwell stream now preserves the height at both ends of
  each wall edge. The native solver compares the grounded foot height
  with the interpolated edge height before pushing the player.
  [Original edge height calculation](https://github.com/shdecompilations/silent-hill-decomp/blob/ecbb0d8e3881cf2f0cb9fa700f3bb1a74b6d34ce/src/bodyprog/collision/collision.c)
- In melonDS DSi mode, a synthetic room crossed a ramp-to-landing seam,
  cleared an edge below the actor's feet and stopped at a closed door.
  The production stair round trip matched the PS1 arrival poses on DLDI
  and virtual console SD. Both retained the first-floor door stop.
- The eastern stream grew by 172 bytes and the edge array by 512 bytes.
  The previous eastern stream matched all 659,976 host collision checks.
  The current ROM also completed the DLDI route with that stream.

### Shared room header

- The eastern stairwell and courtyard now share a static room header.
  Its four layout bytes identify the room and describe edge heights,
  floor planes and vertex scale. A byte comparison matched the rest of
  both streams to the previous converter output.
- In melonDS DSi mode, both layouts passed the stairwell round trip on
  DLDI and the courtyard round trip on virtual console SD. Captures at
  matched entry poses included 16 distinct pixel-identical stairwell
  frames and a pixel-identical courtyard frame. The courtyard arrivals
  matched the PS1 replay. A virtual SD fixture loaded 88 height-aware
  edges alongside all 14 floor planes and completed the same courtyard
  route with a pixel-identical entry frame.
- A read-only check put the basement stair and boiler room meshes within
  the normal vertex scale. They contain 581 and 1,556 static quads.
  The boiler room still needs a peak visible-vertex measurement.

### Room origins

- The eastern stairwell and courtyard streams now store their IPD-derived
  world origins. The new header adds eight bytes and keeps the existing
  geometry and collision payloads. The loader still reads earlier headers.
- In melonDS DSi mode the stairwell round trip passed on DLDI with both
  header versions, with 16 distinct pixel-identical captures. The virtual
  console SD courtyard route crossed the ramp and returned. Its entry
  capture matched the previous release pixel for pixel. The arrival and
  return poses matched the 59.94 Hz PS1 replay.
- Shifting the courtyard origin by two metres in a fixture preserved
  1,640,961 sampled floor results and 412,164 collision moves exactly.
  The shifted virtual SD replay also passed the route. Its entry capture
  differed at 214 of 49,152 pixels, with a mean channel error of 0.042/255.

### Streamed stair floors

- Room streams can use ground planes above or below the map datum. The
  eastern stairwell now reads its ramp and landing from three floor
  records. The converter splits the concave landing along an internal
  edge. The stream grows by 79 bytes. The older eastern stream uses the
  previous floor equations.
- An independent host check matched the IPD floor boundaries and
  equations across 1,346,321 samples. The prior rectangular approximation
  differed at 65,676 of those points, including points outside walkable
  space. The other 14 sidecars and 2,012,876 sampled courtyard floor
  results matched the previous release.
- The DLDI and virtual console SD routes climbed and descended the
  eastern ramp and matched both PS1 stair arrival poses. DLDI repeated
  the route with the previous stream. The PS1 replay recorded 416 frame
  intervals of two VBlanks (33.37 ms) during eastern stairwell gameplay.
- The boiler room materials total 80 KiB and 22 palette rows, within the
  current allocation. Over 662 camera samples from the original route,
  host frustum clipping of the static mesh peaked at 762 polygons and
  3,061 vertices, including backfaces.

### Basement route

- The first-floor stair triggers, boiler door and return route use the
  original MAP1_S00 room links. The converter reads both room chunks from
  the owner’s disc. The 101,974-byte stair stream has 581 quads and two
  ground planes. The 133,030-byte boiler stream has 1,556 quads. Both fit
  the existing 80 KiB scenery texture allocation.
- DSi-mode DLDI and virtual console SD replays retained the infirmary
  inventory and latch and matched all four PS1 arrival poses. The ramp
  reaches Q12 Y 0 at the foot and −7,328 at the landing. The closed boiler
  door and machinery stop movement. Both replays kept the lower screen black.
- Two fresh BlocksDS builds yielded identical ROMs and all 17 room and
  event sidecars. The 15 existing sidecars match the previous release.
- Over 455 scene intervals from boiler entry to the machinery in melonDS
  DSi mode, mean frame time was 33.36 ms (16.68–33.49 ms), and mean ARM9
  submission time was 1.85 ms. Geometry RAM peaked at 693 polygons and
  2,717 vertices with Harry visible and no overflow. Hardware timing and
  the switch event itself still need measurement.
- The matched DLDI and PS1 entry and return captures had mean RGB errors
  of 18.62–22.30/255 across four views. Lighting and moving cameras still
  differ from PS1.

### Texture palette conversion

- When a texture page has multiple transparency indices, the converter
  checks the palette rows referenced by textured batches, including event
  props. It remaps the selected key to index zero and rejects conflicts
  between referenced rows. [DS 3D texture parameters](https://mgba-emu.github.io/gbatek/#ds-3d-texture-attributes)
- All 15 stock sidecars and the BlocksDS ROM reproduced byte for byte.
  A fixture placed a second key in an unused infirmary row. All 42
  sampled frames per path matched stock captures in melonDS DSi mode over
  DLDI and virtual console SD. Host checks also rejected conflicts in a
  static row and a row used only by an event prop.

## Downstairs collision

- Four matched PS1 and melonDS approaches reproduced walk-through at the
  first-floor double doors, the western wall fixture, the southern hall
  lockers and the infirmary door. The port now keeps the relevant IPD
  edges and blocks walking across the closed doors. All four stops are
  within 1 cm of PS1 along the blocked axis. The first-floor latch and
  both action-button transitions still work.

## Wall decorations

- All 28 first-floor poster faces now receive 62.5 mm of clearance. The
  converter checks sloped wall panels and posters authored 3.9 mm off the
  wall. In 126 matched melonDS DSi views, removing the backing panels
  exposed 22,952 marked pixels with the previous data and nine isolated
  edge pixels out of 462,895 with the new data. The ROM, collision,
  textures and other sidecars were byte-identical after regeneration.
- Production replays over DLDI and virtual console SD entered from the
  southern hall, climbed the stairs and returned at the expected landing pose.

## Infirmary camera

- The infirmary now applies the PS1 vertical framing limit that keeps
  Harry in view. With matching player poses in PS1 and melonDS, whole-frame
  mean RGB error fell from 46.2 to 32.2/255 by the drink and from 32.7 to
  27.5/255 by the kit. Lighting and camera transitions still differ.

## Courtyard

- MAP1_S00 points 5 and 3 connect the southern hallway to room 23. The
  room streams SCFFFE with 2,648 scenery quads, 88 collision edges, 14
  ramp and landing planes, and three paletted texture pages occupying
  80 KiB. The conversion retains the tree and window cutouts.
- The original NTSC-U replay measured the entry at X/Z (−80691, −280576),
  yaw 0, and the return at (573440, 93389), yaw 2048, in Q12 coordinates.
  The production ROM matched both poses in melonDS 1.1 over DLDI and
  `361ffa9` over virtual console SD. Both replays kept the medallion,
  medical supplies and first-floor latch. A separate probe verified the
  0.60 m entry landing and ground height after the ramp.
- A diagnostic pass reading geometry RAM before each swap peaked at 1,025
  polygons and 4,071 vertices with no overflow during the entry, walk and
  turn. That pass included one 49.97 ms interval.
- The PS1 trace had fog disabled in this scene. Low-light shading and
  camera following still differ. In the DLDI replay, whole-frame mean RGB
  error was 20.6/255 on entry, 17.1/255 after descending the ramp and
  13.9/255 after returning to the hall.

### Native depth fog

- The USA 1.1 PS1 replay confirmed that the courtyard uses fog off.
  Four opening-street samples used RGB (108, 100, 116) and reached full
  fog near 13.5 m. A diagnostic W-buffer table with 0.5 m spacing reproduced
  the street's depth ramp with 0.39 percentage points of mean density error
  and 2.32 at worst from 1 to 13.5 m. This measurement covers the depth
  ramp. Object tint and lighting still need separate comparisons.
  [Original fog calculations](https://github.com/shdecompilations/silent-hill-decomp/blob/ecbb0d8e3881cf2f0cb9fa700f3bb1a74b6d34ce/src/bodyprog/gfx/bodyprog_80055028.c)
- The synthetic panel probe matched calculated RGB within 1/255 in
  melonDS 1.1 DSi mode, over both DLDI and virtual console SD. At a fixed
  courtyard entry camera, the Z and W cases and forced scenery fog each
  averaged 2.86 ms of ARM9 submission and 33.30–33.43 ms between
  submissions over 127 intervals per case. Each used 900 polygons and
  3,605 vertices. W-depth changed 43 of 49,152 pixels with fog off. The
  production renderer still uses Z-depth. Compare depth ordering at
  matched cameras when a fog-enabled scene reaches the renderer.
  [DS fog table and blending](https://mgba-emu.github.io/gbatek/#ds-3d-toon-edge-fog-alpha-blending-anti-aliasing)

## North hallway

- MAP1_S00 points 4 and 6 connect the courtyard to first-floor room 16.
  SC0304 streams 1,143 quads, 24 IPD collision edges and 80 KiB of
  textures. The transition uses the original double-door sound pair.
- In melonDS DSi mode, both DLDI and virtual console SD crossed the
  courtyard, entered at Q12 X/Z (546816, 772914), yaw 0, and returned at
  (−80691, −216678), yaw 2048. Both poses matched the original and kept
  the medallion, supplies and latch. Mean whole-frame RGB error was
  11.9/255 on entry, 17.8/255 while walking and 19.9/255 on return.
- The PS1 trace had fog off in the hall. The port still differs in
  lighting and chase camera motion. The other hallway doors are pending.

## Clock tower

- MAP1_S00 points 13–15 show the locked door, the stopped clock and both
  empty medallion slots. The port loads the original messages and TIM art
  from the locally converted sidecar. The views use the PS1 texture sample
  positions and 5-bit dither at native 256×192.
- The PS1 lock message drew at 50.05 ms intervals. The three close-ups
  drew at 16.68 ms intervals when fully visible. The PS1 inspection replays
  returned to the courtyard with the medallion and latch intact.
- DSi-mode replays in melonDS 1.1 over DLDI and `361ffa9` over virtual
  console SD reached all three points from the infirmary, dismissed each
  view and kept the inventory, latch and player pose. All 830 sampled
  bright text pixels matched the PS1 at 5-bit precision. In the top 152
  rows, 99.5–99.7% of close-up pixels were within one 5-bit step per channel.
- The two plaques now sit 7.8 mm in front of their coplanar masonry. In a
  16-angle melonDS DSi-mode sweep, the lowest counts of plaque-colored
  pixels in the 112-pixel test regions rose from 1 to 87 for the moon and
  from 0 to 112 for the sun.
- The original tower planes raise Harry along the ramp to the 0.60 m
  landing. The tower perimeter stops further forward movement. The old
  sidecar kept Harry at ground level and allowed him through the tower.
  The new sidecar ascended, stopped at the door and descended back to
  ground in melonDS. A host check at 81 PS1 replay positions matched 73
  heights within 4 mm. The largest difference was 3.1 cm.
- The medallion-use events and tower interior are pending. The current
  build starts directly at the piano and allows inspection of the tower
  with both slots empty.

## Chase camera clearance

- The original chase roads reserve one metre at each edge. The port now
  applies that inset to its chase-camera targets and uses the centre line
  if the road is under two metres wide. Door and fixed views retain their
  own positions.
- At the southern hall test pose, the old DSi camera settled 22 cm behind
  the wall. The updated camera settled 78 cm inside and the PS1 camera was
  1.43 m inside. The eye still approaches from a different angle. Whole-frame
  RGB error was 21.0/255 before the fix and 22.6/255 with the wall clearance.
- The final ROM kept the camera inside at four headings beside that wall
  on virtual console SD. The north round trip passed again over DLDI.
- The first-floor chase now uses the PS1's heading-dependent clearance to
  keep the camera away from Harry while it stays within the authored road.
  At Q12 X/Z (444000, 250000), facing east, its horizontal distance rose
  from 0.67 m to 2.49 m. The same PS1 pose measured 2.38 m. The native
  look target went from 3.62 m below the floor to 0.69 m above it, matching
  the PS1 target height. Mean whole-frame RGB error fell from 28.1 to
  26.6/255. Camera motion during turns still differs.
- The production DLDI replay walked into the first-floor hall, turned and
  crossed the stairs in both directions with the original warp poses.
  A shorter virtual console SD replay also passed both stair transitions.
- The southern hall now uses the same clearance along its length and
  the authored 1.69 m chase height. Across eight matched views, horizontal
  camera distance rose from 0.39–0.91 m to 2.08–2.77 m. The locker pose
  moved from 0.43 m to 2.77 m against 2.95 m on PS1. All eight native
  look targets were above the floor. Whole-frame RGB error at the locker
  rose from 22.51 to 22.99/255, while the centre pose improved from 44.20
  to 31.95/255. Both first-floor camera poses matched the previous build.
- At the settled locker turn endpoint, 64 melonDS scene intervals
  averaged 33.43 ms before and after the change. ARM9 submission averaged
  5.18 ms before and 5.17 ms after. The original NTSC-U idle locker trace
  had 33 intervals at 33.37 ms and 16 at 50.05 ms.
- The production routes entered from the infirmary, turned in the hall
  and crossed through the courtyard doors in both directions over DLDI
  and virtual console SD. Both routes preserved the inventory and latch.
- The music-room corridor, eastern hallway, both north halls and eastern
  equipment aisle now apply the same clearance along each road's long
  axis. Across ten matched views, horizontal distance rose from
  0.39–0.86 m to 2.37–2.69 m. The PS1 measured 2.26–3.35 m. All ten
  native look targets were above the floor after the change. Four of the
  five initial views moved closer to the PS1 in whole-frame RGB error.
  The eastern hallway rose from 23.78 to 24.15/255.
- Across 320 melonDS scene intervals after the turns, the mean was
  33.33 ms before and after. Mean ARM9 submission was 4.23 ms in both
  builds. The original samples mostly used 33.37 ms intervals and
  included 50.05 ms intervals in the eastern hallway and equipment aisle.
- The production DLDI run walked from the music room to the equipment
  aisle and returned to the upstairs north hall. The virtual console SD
  run crossed the courtyard, turned in the downstairs north hall and
  returned through the same door. Both verified the original spawn poses
  and a black lower screen.

## Chemistry lab route

- The PS1 replay used upstairs east corridor point 52 to enter room 33 at
  Q12 X/Z (−62668, 784384), yaw 3072. It crossed to point 45 and entered
  chemistry lab room 34 at (−261324, 398130), yaw 0. The return through
  points 49 and 42 spawned at (−118374, 789299), yaw 2048, then
  (−208896, 292864), yaw 1024.
- The production ROM walked this route from the piano and matched all four
  PS1 door poses in melonDS DSi mode with DLDI and virtual console SD.
  Mean entry and return RGB errors were 19.1, 12.8, 19.8 and 15.3 out of
  255. The PS1 replay had fog off in both rooms. Lighting still differs.
- SCFF04 streams 1,166 quads and 32 collision edges. SCFE02 streams 1,083
  quads, 30 edges and the 15 original obstacle cylinders. Each room uses
  64 KiB of textures. The lab uses its two original camera positions, and
  the melonDS replay verified the switch when crossing to the east aisle.
- At the west bench, melonDS stopped 7 mm from the PS1 stop. A host check
  backed away within the narrow aisle and reached the same contact again.
- The lab event requires the Chemical item before it releases the gold
  medallion. The intact hand adds 83 original SCFE02 quads. The production
  DLDI replay walked around the benches, opened the hand close-up with A,
  then dismissed it with B. Mean RGB error was 1.18/255 and all 271 sampled
  bright text pixels matched the PS1 close-up. The lower screen stayed black.

## Music-room posters

- Conversion gives 35 decoration faces 46.9 mm of wall clearance. It
  moves their vertices along the wall normal and preserves the textures,
  UVs, batch counts and file size.
- Across 144 matched camera positions in melonDS DSi mode, the previous
  7.8 mm clearance let the walls cover 8,025 marked poster pixels. The
  new clearance matched all 133,538 visible poster pixels in the
  comparison with the backing walls hidden. The ROM and all other data
  files were byte-identical on regeneration.
- The production ROM booted with the regenerated data and passed the
  music-room corridor round trip over DLDI in melonDS 1.1 and virtual
  console SD in `361ffa9`. The lower screen stayed black.

## Equipment room

- MAP1_S01 points 46 and 50 connect the upstairs hall to room 35. SCFD02
  supplies 1,424 quads, 32 collision edges and three paletted texture pages
  occupying 80 KiB. The west aisle uses two measured PS1 views with an
  approximate switch point. The original replay had fog disabled.
- The production ROM reached the Chemical shelf and returned to the hall
  in melonDS DSi mode on both DLDI and virtual console SD. Both door poses
  matched PS1 at Q12 X/Z (−403865, 398130), yaw 0, and (−131481, 789299),
  yaw 2048. Whole-frame mean RGB error was 19.0/255 on entry, 16.2/255 at
  the shelf in the SD replay and 18.5/255 on return. The lower screen stayed
  black.
- The PS1 Chemical event declined with flag 68 clear and item 96 absent,
  then accepted with the flag set and one item added. The model reached
  full scale after 48–49 VBlanks. Examining the empty shelf opened MAP1_S01
  message 19 and kept the item count at one.
- The native pickup uses the original point 13 facing segment, the 55-quad
  shelf prop and 188-triangle rotating item. The prompt streams a 2 KiB
  texture tile with one CLUT. In melonDS over DLDI and virtual console SD,
  Harry declined, cancelled, accepted before solving the piano, and found
  the shelf still empty after leaving and re-entering the room. All 182
  bright green pixels in the item name and 184 bright pixels in the empty
  shelf message matched the resampled PS1 captures. Lighting still differs.
- A diagnostic shortcut reached the infirmary notebook after pickup on both
  storage paths. The production ROM cold-loaded each written checkpoint
  with the Chemical, silver medallion, latch and desk pose intact. The host
  checkpoint recovery fixture also passed with the new flag.
- The PS1 statue callbacks preserve the Chemical after use and offer the
  gold medallion once the hand opens. The reference supplies the item
  selection directly to the event router. Initial lab traces used the
  core's embedded OpenBIOS because the expected US firmware path was
  absent. The statue replay was repeated with the owner's SCPH-7001 via
  a local firmware alias. The inspection and pour images were identical.
  [Beetle PSX firmware selection](https://github.com/libretro/beetle-psx-libretro/blob/master/libretro.c)

## Hand statue and gold medallion

- Select uses the Chemical when Harry faces the hand. This is a direct
  item-use control until the original inventory is ported. The event uses
  the original pour clip and fixed camera, the original smoke tiles with
  DS alpha blending, the ambient hiss loop and the two original hand models.
- The production DLDI replay walked from the piano, collected the Chemical,
  played the full pour, declined and accepted the gold medallion and
  returned to the empty hand after reloading the lab. The pour warped
  Harry to the PS1 Q12 position (−242483, 405094), yaw 0. All 286 bright
  green pixels in the gold medallion name matched the resampled PS1 capture.
- The native full sequence reached the end at 610 simulation ticks. The
  PS1 runs reached it at 609–610 VBlanks. Mean whole-frame RGB errors were
  20.0/255 during the pour, 20.4/255 during dissolution and 18.0/255 at the
  open hand. Lighting and smoke blending differ. A probe peaked at 400
  polygons and 1,541 vertices. An isolated melonDS audio recording captured
  the hiss during the smoke with silence after the sound stopped.
- Over virtual console SD, a lab fixture loaded a legacy checkpoint,
  completed the full event, and saved the open hand and both items in the
  notebook. The production ROM cold-loaded the new SHC2 slot. Start also
  finished the event and stopped the sound during the active hiss. The host
  fixture kept the older SHC1 slot readable across all 36 partial writes.

## Medallion placement

- Both placement events use the original empty and filled images, placement
  sounds and messages. A 17-step transition blends two native bitmap layers.
  The filled views remain available to inspect, and Select places each item
  once. Both medallions remain in inventory in PS1 and native replays.
- In the DLDI release, Harry walked from the infirmary to both slots. Mean
  RGB error was 0.22/255 for the filled sun and 0.24/255 for the filled moon
  against the resampled PS1 captures. The best sampled transition frames
  measured 1.45/255 and 1.77/255. The original 12:00 and 5:00 clock views
  measured 0.31/255 and 0.37/255. The locked-door check matched all 259
  sampled bright text pixels with each clock reading.
- The PS1 runs with the owner's SCPH-7001 completed the sun placement after
  364 VBlanks and the moon after 362. Both slots filled while the boiler
  stayed off. The tower door requires the separate boiler switch event.

## Boiler switch

- MAP1_S00 uses silver placement to enable the switch. Before placement,
  confirming the prompt plays the switch and shows the no-effect notice.
  The virtual console SD replay declined, then confirmed with the moon
  slot empty. Power stayed off, matching the PS1 event.
- In the DLDI replay, a checkpoint with the moon placed enabled startup.
  The event blinks the green indicator. Power was set at simulation tick
  279, and control returned at tick 372. The PS1 event used the same
  intervals from confirmation. The working notice, return to the infirmary,
  save and cold boot passed. An independent host check rejected a powered
  checkpoint with the moon slot empty and preserved the earlier valid slot.
- All 477 bright pixels in the switch question and 266 in the no-effect
  notice matched the resampled PS1 reference. Boiler effects and door
  sounds come from MEP100. The hum uses the original repeat point with
  room-based volume and fixed pan. A separate melonDS DSi fixture verified
  all five effects against the emulated channel sample rates, lengths and
  repeat settings and observed nonzero mixer output. Spatial falloff and
  PS1 lighting remain pending.
  [melonDS audio state](https://github.com/melonDS-emu/melonDS/blob/1.1/src/SPU.cpp)
- Two BlocksDS builds reproduced the ROM and all 17 sidecars. The 20
  pre-existing sounds from the medallion build matched byte for byte.
  A DLDI smoke test also loaded a checkpoint with the older SHP7 data.

## Clock tower entrance

- MAP1_S00 points 13 and 20 connect the courtyard and the one-way tower
  entrance. The powered door streams the original interior and plays
  the entrance sample. The courtyard inspection is available while the
  boiler is off. The door inside gives the original closed-door message.
- The interior uses the original fixed camera and shaft boundaries. The
  51,476-byte room stream includes 486 quads, 12 collision edges and one
  32 KiB texture page. The arrival pose matches PS1 at Q12 X/Z
  (−409600, −244121), yaw 2048. The wall, door and hatch stops are all
  within 0.01 metre of the corresponding PS1 blocking coordinates.
- Both powered routes passed in melonDS 1.1 DSi mode over DLDI and virtual
  console SD. The DLDI run confirmed the 14,998 Hz opening sample in the
  emulated sound channel. A second DLDI run kept the door locked with power
  off while using the previous SHP9 sound bank.
- On the PS1, 702 of 707 measured frame intervals in room 20 were two
  VBlanks (33.37 ms). The other five were one VBlank (16.68 ms).
- The melonDS DSi profile averaged 33.35 ms across 221 frame intervals
  (16.72–33.44 ms). Mean ARM9 submission time was 1.48 ms. Geometry RAM
  peaked at 526 polygons and 2,062 vertices without overflow.
- All 202 bright pixels in the interior message match the resampled PS1
  capture. Mean RGB error in the matched entry view is about 23.7/255,
  and the native interior is darker. Source extraction and the BlocksDS
  ROM reproduced across two builds. The 16 other sidecars are byte-identical.

## Underground passage

- Point 19 opens the tower hatch. Point 23 streams room 22, the original
  `SCFEFE` chunk, with Harry's 37-frame ladder clip. The descent freezes
  movement, runs for 264 simulation steps and can be skipped with Start.
  Both paths restore control at PS1 Q12 X/Z (−245760, −260915), yaw 0.
- The passage uses two camera roads, five authored floor planes and the
  inner shaft boundaries. Both ladder stops agree with the PS1 replay
  within 0.01 metre. The far landing is 0.60 metre below the entry floor.
- Two independent BlocksDS builds reproduced the ROM and 19 sidecars.
  The 18 existing data files matched byte for byte. The new
  `passage.dat` is 107,571 bytes.
- PS1 buffer changes during the descent gave 129 intervals at 33.37 ms
  and one at 50.05 ms. Walking and standing in the passage gave 424 at
  33.37 ms and 43 at 50.05 ms.
- The full descent passed in melonDS 1.1 DSi mode over DLDI. The virtual
  console SD replay skipped the descent with Start. Both crossed the
  passage in both directions with a black lower screen. The standalone
  BlocksDS depth-fog sample also ran in DSi mode.
- The melonDS profile averaged 33.430 ms over 129 ladder frame intervals
  and 33.348 ms over 408 passage intervals. Mean ARM9 submission times
  were 1.593 and 1.844 ms. Geometry RAM peaked at 797 polygons and
  3,121 vertices across both views, with no overflow.
- The native camera easing and fade steps approximate the original. Three
  resampled reference views gave mean RGB errors of 12.24 to 19.81 on
  a 0–255 scale, with darker native walls. The original passage ambience
  and movement audio remain pending.
## Passage ascents and far tower

- MAP1_S00 points 16 and 17 lead back to room 20 and on to room 21.
  Both ascents use the original 37-frame climb and restore the matching
  PS1 arrival pose. The far hatch descends to the north landing at Q12 X/Z
  (−245760, −230604), yaw 2048. Start skips either climb once motion starts.
- The far tower uses its own `SCFCFE` stream and fixed overhead camera.
  A horizontal up vector keeps the camera basis accurate in fixed point.
  The far door stops Harry at Q12 Z −247812, within 1 cm of the PS1 stop.
- The production round trip passed over DLDI and virtual console SD in
  melonDS 1.1 DSi mode. Each run exercised both ascents and descents.
  Between them, all four climbs completed normally and with Start skip.
  The lower LCD stayed black in the sampled views.
- Two BlocksDS builds produced identical TWL ROMs and 20 sidecars.
  Eighteen prior sidecars stayed byte-identical. `passage.dat` adds 5,772
  bytes for the second clip and `fartower.dat` adds 48,754 bytes.
- The PS1 ascent callback and destination load took 369–370 VBlanks.
  All 126 measured shaft intervals and 149 of 150 far-tower intervals
  were 33.37 ms. The one remaining tower interval was 16.68 ms.
- In melonDS 1.1 DSi mode, 129 ascent intervals averaged 33.301 ms and
  154 far-tower intervals averaged 33.430 ms. Mean ARM9 submission took
  1.253 and 1.458 ms, respectively. The maximum was 528 polygons and
  2,064 vertices, with no geometry overflow.
- At the sampled poses, the mean RGB error against PS1 was about 12.8/255
  on the ladder and 22.5/255 in the far tower. The native scenes are
  darker, and the original ambience and movement audio remain pending.

## Otherworld courtyard

- The far tower's outer door loads the original MAP1_S02 courtyard
  chunk, `SUFFFE`. Six floor planes cover the entrances and diagonal
  tower ramp. Harry follows the intro's walk and camera pan, plays the
  original 47-frame look animation and regains control in the courtyard.
- The US disc's three dialogue lines decode locally to 37.8 kHz stereo
  PCM. The loader releases the 959,616-byte buffer after the scene. The
  converted glyphs retain the original font and line breaks.
  [PlayStation XA audio format](https://psx-spx.consoledev.net/cdromformat/#cdrom-xa-audio-adpcm-compression)
- The PS1 intro took 1,611 VBlanks. Most of its frame intervals were
  50.05 ms (476 of 520), with 42 at 66.73 ms and one each at 33.37 ms
  and 133.47 ms. The 19 sampled gameplay intervals after the intro
  were 15 at 50.05 ms and four at 66.73 ms.
- Two BlocksDS builds generated identical TWL ROMs and 21 sidecars. The
  new `otherworld.dat` is 1,146,672 bytes and the previous 20 files match
  byte for byte.
- melonDS 1.1 in DSi mode completed the full intro over DLDI and skipped
  it during the first voice over virtual console SD. Normal and skipped
  arrivals match the PS1 poses at Q12 X/Z (−74458, −254955) and
  (−74465, −254935), both with yaw 3584. Movement resumed and both sound
  channels stopped. The lower screen stayed black. The stereo channel
  buffers matched the local conversion and the mixer produced sound for
  all three lines. [Emulator sound state](https://github.com/melonDS-emu/melonDS/blob/1.1/src/SPU.cpp)
- The DLDI room read took 276.60 ms in the instrumented run. Mean frame
  intervals were 33.367 ms across 797 intro intervals and 33.430 ms
  across 59 courtyard gameplay intervals. Mean ARM9 submission was
  2.066 and 2.392 ms. Peak use was 779 polygons and 3,054 vertices
  with no geometry overflow.
- The sampled images had mean RGB errors of 11.2–20.8 out of 255.
  The native ramp view is brighter. Camera easing and scripted walk
  use interpolated positions.

## Otherworld flashlight and fog

- The converted courtyard includes the original LM normals. The runtime
  shades 5,214 shared position and normal pairs for 10,548 corners.
  Harry's animated torso positions the lamp and his heading aims it.
  Y switches the light after the intro with a 15-step fade.
- The PS1 replay measured full beam intensity at 0.70 and disabled fog.
  With the light off, the original uses a blue-gray tint and black fog
  near 9.30 metres. The DSi uses a W-buffer fog table and a top-screen
  blend for the original brightness pass. Harry's mesh uses a shared
  tint, and the DSi fog ramp and color quantization differ from PS1.
  [DS fog registers](https://mgba-emu.github.io/gbatek/#ds-3d-toon-edge-fog-alpha-blending-anti-aliasing)
  [DS 2D blending of 3D output](https://mgba-emu.github.io/gbatek/#ds-3d-final-2d-output)
- In six matched melonDS 1.1 DSi views, RGB mean absolute error against
  resampled PS1 captures fell from 16.83–20.97 to 1.29–8.41 on a 0–255
  scale. The captures cover the intro, both flashlight settings, turning
  and walking. The PS1 replay is `clock-otherworld-light` on the local
  diagnostics branch.
- Stable courtyard views averaged 33.43 ms per scene submission (29.9
  FPS) in the DSi emulator. The per-view mean ARM9 submission times were
  5.94–7.97 ms, compared with 6.46–11.30 ms in the first per-corner
  implementation. Peak geometry was 769 polygons and 3,014 vertices
  without overflow. Physical frame pacing still needs measurement.
- The uninstrumented build passed a checkpoint-to-courtyard replay over
  both melonDS DLDI and virtual console SD, including a full intro and
  a Start skip. Both runs switched the beam off and back on, then turned
  and walked. The lower screen stayed black.
- Two clean BlocksDS builds reproduced the TWL ROM and all 21 sidecars.
  The updated `otherworld.dat` is 1,156,622 bytes. The other 20 data
  files are byte-identical. The previous courtyard data still loads
  with its static lighting.
- The rain and original music remain pending.

## Otherworld north hall

- MAP1_S02 points 7 and 9 link the courtyard and the north hall.
  The original `SU0304` chunk supplies 951 quads, two texture pages,
  1,947 shared lighting points, 16 collision edges and a floor plane.
  The native door, wall and both locker contacts match the PS1 stops
  within 7 mm in the four sampled approaches.
- Both crossings match the PS1 arrival poses at Q12 X/Z
  (546816, 772914), yaw 0, and (−80691, −216678), yaw 2048. The
  flashlight state survives the load, and returning to the courtyard
  skips the intro and its dialogue buffer. The uninstrumented
  melonDS replays passed from the infirmary checkpoint on DLDI with
  the beam on and virtual console SD with the beam off. Both toggled
  it inside the hall and sampled a black lower screen.
- The matched entry, walking and return views had mean RGB errors of
  4.28–9.47/255 with the flashlight on and 0.39–0.71/255 with it off.
  Native highlights are brighter in the lit views. The PS1 replay used
  Beetle PSX with the owner's SCPH7001 BIOS, which the core reported
  differs from its expected SCPH5501 hash.
- The pair 13 open and close effects come from the original
  `BASE.VAB` at 11,370 and 9,022 Hz. A melonDS DSi test recorded both
  mixer outputs on each crossing. The converted sound bank preserves
  all 26 earlier tones and their PCM.
- The PS1 hall replay measured 198 intervals at 33.37 ms and one at
  16.68 ms. In melonDS 1.1 DSi mode, the instrumented hall run averaged
  32.776 ms across 460 intervals (16.702–33.472 ms). Mean and maximum
  ARM9 submission took 4.470 and 5.735 ms. The peak was 647 polygons
  and 2,531 vertices, with no geometry overflow. The DLDI load reads
  and setup took 47.26 ms into the hall and 81.48 ms back to the
  courtyard, including one VBlank before each read.
- Two fresh BlocksDS builds reproduced the TWL ROM and all 22 data files.
  The new `otherworldnorth.dat` is 101,059 bytes. Twenty previous
  sidecars match byte for byte. The expanded `piano.dat` carries
  the two original door sounds.
- The hall's original ambience and the courtyard's south exit remain pending.

## Otherworld north lobby

- MAP1_S02 points 59 and 62 link the hall and lobby. The port loads
  the original `SU0203` geometry, bench obstacles and floor. The Q12
  arrivals match PS1 at (409600, 561970), yaw 0, and (546816, 789299),
  yaw 2048. The doorway views use the measured PS1 cameras. The
  release build passed the checkpoint-to-lobby round trip over DLDI
  with the flashlight on and virtual console SD with it off. Both
  runs read the warning, collected the supplies and returned to
  the hall with the same flashlight setting and a black lower screen.
- The two fan models use the original rotation speed ranges and a
  local random seed. Harry can walk between the benches. The sampled
  fan stop is within 6.1 mm of PS1. The two warning pages use the original glyphs.
  All 389 sampled bright text pixels in their bottom text rows match.
- The original first-aid kit and handgun bullet models sit on the
  benches. The pickups can be declined or accepted, and the flags
  prevent a second pickup. The PS1 replay verifies one kit and 15
  bullets. Using supplies remains pending in the port. Updated host
  checkpoint tests recover older records and preserve the new flags.
- The PS1 lobby replay measured 148 intervals at 33.37 ms, 25 at
  50.05 ms and one at 16.68 ms. The instrumented melonDS 1.1 DSi run
  averaged 33.366 ms across 1,822 intervals (16.709–49.968 ms).
  ARM9 submission averaged 4.708 ms and peaked at 6.166 ms. Maximum
  geometry use was 640 polygons and 2,502 vertices with no overflow.
  The DLDI room read and setup took 62.08 ms including one VBlank.
- Mean RGB error against the matched PS1 images was 2.63/255 at lobby
  entry and 7.73/255 at the hall return. The native highlights are
  brighter. The pickup model orientation and lighting differ, and
  the fan sound and original ambience are pending. The fan loop was
  identified as effect 1455 in `MEP102.VAB`.
- Two clean BlocksDS builds reproduced the TWL ROM and all 23 local
  files. `otherworldlobby.dat` is 149,817 bytes. All 22 earlier sidecars
  are byte-identical.

## Otherworld side and east halls

- MAP1_S02 points 60 and 72 connect the north and side halls.
  Points 71 and 70 connect the side and east halls. The room streams
  use `SU0303` and `SU0201`, with original normals and collision.
  The four arrival poses match PS1, and the flashlight state persists.
  The uninstrumented round trips passed from the infirmary checkpoint
  in melonDS DSi mode over DLDI with the beam on and virtual console SD
  with it off. The lower screen stayed black in the sampled views.
- The side hall uses the measured door cameras. In the east hall, the
  barrier stopped Harry at Q12 Z 248412, about 2 mm from PS1. The
  loader preserves the floor and stair planes along the adjoining
  southern chunk. The native play route currently reaches the barrier.
- The pair 12 opening and closing sounds use the original `BASE.VAB`
  samples at 11,370 Hz. The instrumented melonDS replay recorded both
  sounds on all four crossings. All 28 earlier tones remain byte-identical.
- The PS1 measured 193 intervals at 33.37 ms and one at 16.68 ms
  in the side hall. The east hall had 689 at 33.37 ms, 12 at 50.05 ms
  and one at 16.68 ms. In melonDS 1.1 DSi mode the respective averages
  were 33.356 ms across 1,123 intervals and 33.362 ms across 735.
  Mean submission took 3.604 and 7.890 ms. The east hall peaked at
  1,288 polygons and 5,104 vertices without overflow. DLDI setup and
  reads took 45.01 and 55.24 ms, each including a VBlank.
- The matched entry, barrier and return views had mean RGB errors
  of 4.64–8.20/255. The native east hall is brighter in the sampled
  lit views, and the chase camera remains an approximation.
- Two fresh BlocksDS builds reproduced the TWL ROM and all 25
  sidecars. `otherworldside.dat` is 107,150 bytes and
  `otherworldeast.dat` is 122,135 bytes. The sound bank adds pair 12.
  The other 22 existing files match byte for byte.
- Three separate PS1 replay fixtures verify the classroom detour
  through points 69, 78 and 73. The last door reaches the far side
  of the east hall barrier. Point 29 in the first classroom starts
  the original Picture Card event.

## Otherworld classrooms

- The north and south classrooms use original `SU0302` and `SU0301`
  geometry, textures, normals and interior collision. All four door
  connections retain their paired arrival points and pair 12 sounds.
  The two doors to the north classroom each keep their own PS1 arrival
  view. Chase cameras use the classroom roads and the east hall rail
  continues south of the barrier.
- The north classroom uses the original desk prop, inspection model and
  prompt for the Picture Card. Declining leaves the card on the desk.
  Accepting sets the inventory flag and removes the prop. Interacting
  with the remaining cards shows the original message.
- Both classrooms replayed on the verified USA 1.1 disc in Beetle PSX.
  Each had 175 gameplay intervals at 33.367 ms during the entry and walk.
  The original eight door directions were captured in separate runs.
- BlocksDS 1.24.0 and Wonderful GCC 16.2.0 reproduced the unit-code-3
  ROM and 27 local data files in two clean builds. The 25 earlier data
  files matched the previous milestone. The new files are 131,222 and
  112,364 bytes. The host checkpoint test preserves the Picture Card
  flag alongside legacy SHC1/SHC2 compatibility and interrupted writes.
- The uninstrumented ROM ran from a powered-boiler checkpoint over
  virtual console SD and DLDI in melonDS 1.1 DSi mode. Both runs went through the
  tower, passage, courtyard and halls, then declined and accepted the
  Picture Card, read the scattered-cards notice, and returned to the
  east hall below the barrier. The three door poses matched the PS1 X/Z
  and headings exactly. The measured entry views differed by 7.06–11.46
  RGB values on average on a 0–255 scale. The native prompt view differed
  by 9.08. Lighting and the moving chase camera remain approximate.
- A DLDI timing run in melonDS 1.1 DSi mode averaged 33.368 ms in the
  north classroom across 1,893 intervals and 33.357 ms in the south
  classroom across 1,148. The 95th percentile was 33.430 ms in both
  rooms. The respective longest intervals were 49.968 ms and 33.448 ms.
  Peak geometry use was 610 polygons and 2,389 vertices in the north
  classroom, and 514 and 2,003 in the south, with no overflow in either.
  Peak submission time was 3.75 ms and 3.14 ms respectively. The DLDI
  run exercised all eight classroom door directions and matched each
  arrival's PS1 X/Z and heading. A separate DSi test showed and dismissed
  the original first-floor latch notice before using the unlocked door.
- Jammed-door messages, classroom enemies and original ambience remain pending. Physical DSi and Pico-Launcher
  testing remains pending.

## Otherworld south hall

- MAP1_S02 points 64 and 48 connect the lower east hall to the south
  hall in both directions. The south hall uses the original `SU0300`
  geometry, normals, floor and both locker banks. The doors use the
  PS1 arrival cameras and pair 13 sounds.
- Original PS1 and native melonDS DSi arrivals matched exactly in X/Z
  and heading. The south hall entry and east hall return differed from
  the PS1 views by 2.83 and 8.95 RGB values on average on a 0–255 scale.
  The native locker stops were within 18 Q12 units (4.4 mm) of PS1 along
  both contact axes in the diagnostic run. The uninstrumented ROM
  repeated both locker stops and door crossings after reaching the
  south hall from the powered-boiler checkpoint over virtual console SD.
- The PS1 east locker replay held 33.367 ms across 161 intervals. The
  west view averaged 35.718 ms across 149 intervals, including 21
  three-VBlank frames. The native hall averaged 33.362 ms across 2,698
  intervals in melonDS 1.1 DSi mode. Its 95th percentile was 33.430 ms
  and its longest interval was 33.476 ms. Peak use was 967 polygons,
  3,822 vertices and 4.705 ms of submission time with no overflow.
- The reproducible unit-code-3 build adds `otherworldsouth.dat`
  (105,887 bytes). The previous 27 data files are byte-identical.

## Otherworld reception

- MAP1_S02 points 50 and 53 link the south hall and reception. The
  reception room uses the original `SU0100` textures, geometry,
  normals and wall and desk collision. Both doors use measured
  arrival views and the original pair 12 sounds.
- The DLDI diagnostic replay in melonDS 1.1 DSi mode matched both
  PS1 arrivals in X/Z and heading. The entry and return views differed
  from PS1 by 7.44 and 6.68 RGB values on average on a 0–255 scale.
  Harry stopped at the card-slot wall within 15 Q12 units (3.7 mm)
  of the PS1 stop along its contact axis. The uninstrumented virtual-SD
  run reached reception from the powered-boiler checkpoint, walked to
  the slot wall and returned to the south hall with the card.
- The PS1 walking sample averaged 34.975 ms across 166 intervals,
  including 16 three-VBlank frames. Native reception averaged 33.379 ms
  across 972 intervals with a 33.430 ms 95th percentile and 49.970 ms
  maximum. Peak use was 615 polygons, 2,395 vertices and 4.980 ms of
  submission time with no overflow. These are emulator results.
- Two clean BlocksDS builds reproduced the unit-code-3 ROM and 29 data
  files byte for byte. `reception.dat` is 125,586 bytes. The 28 previous
  data files matched the south hall build.
- PS1 replay covers the slot's first and repeat inspections and the
  original use event. The original keeps one Picture Card after use,
  sets event flag 1207, and opens the crossing to MAP1_S02 room 11.
  The two inspection TIMs match more closely with V scaled to 239.
  The repeat message has 46 glyphs. The use replay records projection
  376 and both hand-animation poses for the next milestone.

## Picture Card slot and west hall

- The native slot inspection shows the two original reception views and
  the first and repeat messages. The first visit blends the two views
  during the surprise line. The matched frames differed from PS1 by
  0.17–1.97 RGB values on average on a 0–255 scale.
- MAP1_S02 event 11 uses the original Harry keyframes from
  `HB_M1S02.ANM`, the measured camera pose and projection, the card
  insertion message, and the two original sounds. The original event
  trace took 266 VBlanks with the scripted message hold. The native
  animation uses 30 Hz submission, fixed fade lengths, and the port's
  existing room lighting. The card stays in inventory after use and
  the open door persists across room changes. The uninstrumented
  virtual-SD replay started at the powered-boiler checkpoint, collected
  the card in the classroom, opened the door and returned to reception.
  A focused DLDI test verified both ARM7 sound-channel allocations and
  the original item emphasis color in the insertion message.
- The `SU0101` stream adds the west hallway and retains its original
  floor planes, barrier and locker geometry. Both door directions in
  melonDS 1.1 DSi mode over DLDI matched the PS1 X/Z and heading.
  Harry stopped at z=242692 (Q12) against the barrier, compared with
  242711 on PS1. This is a 4.6 mm difference along the contact axis.
- The PS1 west hall held 33.367 ms across 205 gameplay intervals. The
  instrumented DSi run averaged 33.360 ms across 2,157 intervals with
  a 33.430 ms 95th percentile and 33.478 ms maximum. Peak use was
  1,338 polygons and 5,300 vertices, with 6.245 ms peak submission
  time and no overflow. These are emulator measurements.
- BlocksDS 1.24.0 and the verified USA 1.1 CUE produce a reproducible
  unit-code-3 ROM and 31 data files. The new files are
  `cardslot.dat` (125,228 bytes) and `otherworldwest.dat` (144,357 bytes).
  `piano.dat` adds the two original effects and keeps previous tones
  and PCM identical. The other 28 previous data files match.
- The PS1 reference also confirms the women's restroom crossing at
  point 81 from MAP1_S02 room 11 to MAP1_S03 room 4. The settled
  arrival is Q12 (93675, 0, 81510), heading 3072.

## Otherworld women's restroom and upstairs west hall

- Points 81, 9, 58 and 10 use the restroom to change floors. The two
  restroom visits share the original `SU0000` stream and use their own
  exits. The upstairs hall adds `SUFD01`, including its lockers,
  barriers and floor planes. Door transitions use the original pair 12
  sounds. The reception door still requires the Picture Card.
- All four arrivals matched the USA 1.1 PS1 replay in X/Z and heading.
  The flashlight and the card survived both overlay crossings. With
  the flashlight off, the four entry views differed from PS1 by
  1.73–2.30 RGB values on average on a 0–255 scale. The restroom uses
  the two measured fixed cameras with a native switch at the stall
  partition. Moving chase and lighting behavior remain approximations.
- The uninstrumented DSi virtual-SD replay started at the powered-boiler
  checkpoint and resumed from the north hall after a missed waypoint in
  the test harness. It collected the card in the classroom, opened the
  west hall and completed all four crossings with the flashlight on.
  With the beam on, the same four entry views differed from the PS1 by
  8.63–10.17 RGB values on average.
- In melonDS 1.1 DSi mode over DLDI, the restroom stall stopped Harry
  7 Q12 units (1.7 mm) from the PS1 result on X. The upstairs barrier
  stopped him 25 Q12 units (6.1 mm) from the PS1 result on Z.
- The PS1 motion samples averaged 35.268 ms in the restroom (193
  intervals) and 35.078 ms in the upstairs hall (195 intervals).
  The instrumented DSi runs averaged 33.354 ms (1,091 intervals) and
  33.356 ms (1,130 intervals). The 95th percentile was 33.430 ms and
  33.432 ms. Peak geometry use was 965 polygons and 3,803 vertices in
  the restroom, and 1,394 and 5,528 in the hall, with no overflow.
  Peak submission time was 5.576 and 6.703 ms respectively. These
  are emulator measurements.
- BlocksDS 1.24.0 produced two identical unit-code-3 ROMs from the
  verified USA 1.1 CUE. The 33 local data files include
  `restroom.dat` (133,158 bytes) and `otherworldupperwest.dat`
  (132,936 bytes). The previous 31 data files are byte-identical.

## Otherworld teachers' rooms

- MAP1_S02 door pairs 83/87, 84/88, 86/91 and 89/92 connect the two
  teachers' rooms and both sides of the first-floor west-hall barrier.
  The `SU0002` and `SU0003` streams include the original inside walls,
  desk and chair collision, both phone parts, camera bounds and door
  sound pair 12.
- In melonDS 1.1 DSi mode over DLDI, all eight arrivals matched the
  PS1 X/Z coordinates and heading. Entry views with the flashlight on
  differed by 7.18–9.58 RGB values on average on a 0–255 scale.
- The production virtual-SD replay started at the powered-boiler
  checkpoint and used the original route to collect the Picture Card.
  It opened the west hall, took all three detour doors and emerged
  beyond the barrier with the card, unlocked reception door and
  flashlight state intact.
- The PS1 northbound walks averaged 33.367 ms in the southern room
  (178 intervals) and 35.894 ms in the northern room (165 intervals).
  During the instrumented DSi route the averages were 33.379 ms
  (665 intervals) and 33.361 ms (1,221 intervals). The 95th percentile
  was 33.430 ms in both rooms, with a 49.970 ms maximum during the
  route. Peak submission was 3.728 and 7.310 ms respectively. Peak
  geometry use was 287 polygons and 1,096 vertices in the south and
  778 polygons and 3,061 vertices in the north, with zero overflows.
  These are emulator measurements. Beetle PSX ran the verified disc
  with the owner's SCPH-7001 BIOS and logged its expected SCPH-5501
  hash warning.
- BlocksDS 1.24.0 produced two identical unit-code-3 ROMs. The 35
  local data files include `teacherssouth.dat` (105,940 bytes) and
  `teachersnorth.dat` (128,100 bytes). The other 33 files match the
  previous build byte for byte.

## Teachers' room phone call

- MAP1_S02 event 14 starts automatically at point 91. The event uses
  the original animation clips, left-hand receiver, measured camera
  shots, timed subtitles, two stereo XA tracks and two VAB effects.
  The native approach and fades approximate the PS1 timing. The
  render path uses the existing DSi room lighting.
- A focused melonDS 1.1 DSi-mode DLDI run played through both completion
  paths. The settled X/Z and heading matched PS1 after the call and
  after early Start skip. An early skip preserved the off flashlight
  state. The full call turned it on. Returning through the exit did
  not trigger the call again in either run. The event state lasts
  for the current play session.
- The production virtual-SD replay started at the powered-boiler
  checkpoint. The harness resumed the same ROM at the classroom card
  prompt and during the northern room load after reading the wrong
  dialog counter and checking the arrival pose before loading finished.
  The build completed the call over virtual SD and crossed into the
  far side of the west hall with the Picture Card and reception unlock intact.
- The original replay completed in 1,980 VBlanks with the scripted
  reply dismissal and 155 VBlanks with early Start. The eight paired
  DSi shots differed by 6.66–9.88 RGB values on average on a 0–255
  scale. Audio captured from melonDS contained both stereo tracks
  and both handset effects. The tracks' 10 ms amplitude envelopes
  correlated 0.93–0.94 with the locally decoded PCM.
- The DSi profiling run took 1,968 VBlanks with an earlier reply
  dismissal. It averaged 33.126 ms across 990 render intervals,
  with a 33.430 ms 95th percentile and a 34.920 ms maximum.
  The PS1 replay averaged 41.373 ms across 796 intervals with
  a 50.050 ms 95th percentile and an 83.417 ms maximum.
  The native peak geometry was 607 polygons and 2,375 vertices,
  with no overflow and 7.026 ms peak submission time. The DLDI
  read and validation of the phone assets took 1.509 seconds.
  These are emulator measurements.
- BlocksDS 1.24.0 produced two identical unit-code-3 ROMs with 36
  local data files. The `phone.dat` sidecar is 5,437,094 bytes and
  the updated `teachersnorth.dat` is 130,200 bytes. The 34 other
  data files are byte-identical. The sidecar preloads on entry until
  the event is complete and is released on departure. It holds
  eleven seconds of the ring that the scene cuts after 10.4 seconds,
  plus the full dialogue track.

## Otherworld west-hall latch

- MAP1_S02 points 80 and 56 connect the west and north halls. The north
  side starts locked. The west side displays the unlock message and
  makes later crossings possible in both directions for the current run.
  Both notices and the door use the original sounds.
- A focused melonDS 1.1 DSi-mode DLDI replay checked both latch directions,
  repeated crossings and the north jamb. The collision stop differed from
  PS1 by 1.2 mm along the collision axis. Settled entry views differed by
  6.33 and 7.58 RGB values on average on a 0–255 scale. The production
  virtual-SD replay started at the powered-boiler checkpoint and reached
  both sides through the Picture Card route and the teachers' room call.
- The north-hall walk averaged 33.430 ms over 110 melonDS intervals
  and 33.367 ms over 97 PS1 intervals. The melonDS maximum interval was
  33.442 ms, peak submission was 5.853 ms, and geometry peaked at
  872 polygons and 3,438 vertices with zero overflows.
- BlocksDS 1.24.0 produced two identical unit-code-3 ROMs and 37 local
  data files. `westlatch.dat` is 3,864 bytes. `piano.dat` adds one tone
  and retains the existing samples. The other 35 data files match.

## Otherworld storage room

- MAP1_S02 points 58 and 61 connect the north hall to the storage room.
  The room uses original wall and pile collision, door sound pair 12,
  the Rubber Ball model and the original prompt. Accepting the pickup
  removes the model and keeps the ball across room loads.
- The production melonDS 1.1 DSi-mode replay used virtual SD to reach
  the room from the powered-boiler checkpoint. It declined, collected
  and returned to the hall. A focused DLDI replay also went back to
  the shelf and verified that the pickup could only be taken once.
  Both arrival poses matched the PS1. The compared storage arrival and
  north-hall return frames differed by 10.85 and 9.00 RGB values on
  average on a 0–255 scale. The native chase camera approximates the
  original moving view, and the pickup frames differed by 13.22–13.44.
- Within continuous scene and pickup segments, melonDS averaged 33.379 ms
  (1,650 intervals) and 33.269 ms (104). PS1 averaged 46.826 ms (238)
  and 62.368 ms (107). Both native 95th percentiles were 33.43 ms.
  Peak submission was 11.393 ms, geometry reached 1,383 polygons and
  5,302 vertices, and no overflow was observed. Hardware timing is pending.
- BlocksDS 1.24.0 produced two identical unit-code-3 ROMs. The 38 local
  data files include `storageroom.dat` (176,374 bytes). The other 37 match
  the previous build byte for byte.

## Inspection fades

- The notebook and inspection screens keep their bitmap layer hidden
  until the new page is ready. In a production melonDS 1.1 DSi replay,
  Harry opened the infirmary notebook, checked the locked clock tower
  and inspected the sun plaque. The old build briefly showed the clock
  close-up again during the plaque fade. The fixed virtual-SD build
  showed the courtyard until the fade reached the new plaque image.

## Collision on the route out of the music room

- The East Corridor stream includes three missing locker banks. The
  music-room corridor includes the north counter and the inner hallway
  shell. The south hall includes the inner walls and the closed double
  doors west of the infirmary.
- Eleven approach replays over emulated DLDI stopped within 7 mm of the
  PS1 along the direction of travel. The original native build passed
  0.50–1.52 m beyond the reference at the affected surfaces. The same
  DSi replay still opened the courtyard doors from the south hall.
  The converter changed `corridor.dat`, `east.dat` and `south.dat`.

## Courtyard and school cutouts

- The converter checks both the PS1 primitive's blend flag and the
  palette's STP bit. The courtyard trees and bushes keep their cutout
  gaps and draw their visible texels opaquely. The same fix applies to
  school materials with those palette flags.
- The melonDS DSi entry comparison changed 4,241 pixels around the
  courtyard canopy. The production virtual-SD run matched all of
  those pixels in the emulated DLDI capture and verified the west
  double-door stop on the way. The PS1 courtyard remains darker.

## Infirmary and north-side posters

- The converter matches the wall behind the decals even when the
  source places them one or two Q8 units away. It gives the matched
  faces 62.5 mm of clearance, using the same offset as the first-floor
  stairwell. The change affects the infirmary, the normal north hall
  and the Otherworld west hall.
- Five matched camera angles per room in melonDS DSi mode confined
  visible changes to the adjusted wall decorations. The normal north
  hall had 4,832 changed pixels summed over the five views.

## Harry's joint faces

- The converter splits the 73 quads that span different bones along
  the original PS1 strip diagonal. The DSi can cull each half as the
  joint moves. The actor uses 1,482 corners and the same animation data.
- In 64 matched idle views in melonDS, the new mesh restored 413 pixels
  of coverage, including 58 inside the previous silhouette. Eleven
  pixels lost coverage with the different triangle rasterization.
- The music room, Otherworld upper west hall and Otherworld east hall
  each averaged 33.430 ms across 123 settled frame intervals. Peak
  submission was 9.718 ms, and geometry peaked at 1,081 polygons and
  4,190 vertices with no overflow in those samples.
- Two independent BlocksDS 1.24.0 builds matched the unit-code-3 ROM
  and all 38 converted files byte for byte. The production build
  turned Harry in the music room and crossed into the corridor over
  virtual console SD.

## Next

1. Port the Otherworld west stairs using the original transition and ramp trace, then connect the rooftop route.
2. Compare the remaining school doors, walls and posters as the route expands, and implement the Rubber Ball drain puzzle.
3. Measure boot, scene pacing and checkpoint I/O on physical console SD and DSpico.

Based on the original GPL-3.0 decompilation, Copyright (C) 2026
shdecompilations. [Pinned upstream source](https://github.com/shdecompilations/silent-hill-decomp/tree/ecbb0d8e3881cf2f0cb9fa700f3bb1a74b6d34ce)

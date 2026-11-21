# Silent Hill for Nintendo DSi

Native port in development using BlocksDS. Build with GNU Make and an installed
Wonderful Toolchain with BlocksDS 1.24.0.

```sh
make -j2
```

The output is `sh1dsi.nds`. Launch it in TWL mode from a DSi homebrew launcher
on console SD or from Pico-Launcher on DSpico. Physical boot testing is pending.
Development measurements and the upstream revision are in [port status](docs/PORT_STATUS.md).

Prepare local runtime data with Python 3.11 or later. Supply the CUE path and
leave the original BIN next to it. The converter verifies the entire track
before reading named files from the game's archive.

```sh
python3 tools/prepare_assets.py "/path/to/Silent Hill (USA).cue"
```

Run `make sdcard` after conversion. Copy the contents of `dist/` to the
card root. The directory includes the ROM and its local runtime data. Keep the
generated game data on your own devices. Rerun the converter when updating an
existing installation.

The build starts with Harry standing at Midwich's piano. Up and Down walk forward and backward.
Left and Right turn him. Face the keys and press A to inspect the piano, then A again
to examine the keyboard. Move the cursor with the D-pad and
press A to strike a key. B backs out one view at a time. Solving the puzzle
starts the silver medallion cutscene. Start skips back to the room camera.
Walk up to the fallen medallion, face it and press A. Up and Down select a
reply. A confirms it and B declines. Accepting keeps the medallion in
inventory. Face the music-room door and press A to enter the
corridor. Turn back toward the same door to return, or follow the corridor
east and press A as you approach the double door. It leads to the adjoining
second-floor hallway. Follow it past the stair landing to the far double
doors to reach the upstairs north hall. Its two single doors lead to the
chemistry lab and equipment room. In the equipment room, go around the
near end of the central shelves and face the Chemical bottle on the west
shelf. Press A to inspect it and choose Yes to take it. In the lab, face the
hand on the bench. A inspects it and Select uses the Chemical if you have it.
Start skips the pour. After the hand opens, press A and choose Yes to take
the gold medallion. Walk onto the stair
landing in the second-floor hallway to reach the first floor, and turn back
up the stairs to return.
On the first floor, face the double
door on the western wall near the stairs and press A to unlock it. Dismiss the
message with A or B, then press A to enter the southern hallway. The door
stays unlocked. Walk west along the southern hallway and face
the single door on the south wall to enter the infirmary. Face the health
drink or first-aid kit and press A to inspect it. Choose Yes to take it or
press B to leave it. The double doors on the other side of the southern
hallway lead to the courtyard. Turn around and face those doors to return.
Cross the courtyard and use the opposite double doors to reach the north hall.
Face the clock tower door or either medallion slot and press A to inspect it.
With a medallion in inventory, face its matching slot and press Select to
place it. Inspect the clock again to see the new reading.
Take the first-floor stairs to the basement and face the boiler switch.
Press A to operate it. The silver medallion placement enables the switch.
Once the boiler is running, face the clock tower door and press A to enter.
The courtyard door is one-way. Walk to the hatch and press A to descend into
the passage. Face either ladder and press A to climb to its tower. Start skips
a climb once Harry starts moving. The far tower's outer door opens onto the
Otherworld courtyard. The arrival scene uses the original dialogue, and Start
skips it. Harry can walk the courtyard after the scene. Press Y to switch the
flashlight on or off. Cross the courtyard and face the north double doors.
Press A to enter the Otherworld north hall. Turn around and use the same
doors to return. The flashlight keeps its setting across both crossings.
The double doors straight across the hall lead to the north lobby. Inside,
face either supply on the benches and press A to inspect it. Accept or
decline it with the same controls as the infirmary pickups. Press A at the
far opening to read both pages of the fan warning. The lobby entrance
returns to the north hall. The single door immediately
right of the lobby double doors leads to the side hall. Walk across
the side hall to the single door on the same wall to reach the east
hall. Use either of the first two classroom doors on the east wall. Face
the card on the desk in the north classroom and press A to inspect it.
Pick it up, then use the door at the south end to cross into the next
classroom. Its southwest door opens onto the far side of the barrier.
Use the double doors across this section of the east hall to enter the
south hall. Follow the hall west and use the single door on the south
wall to enter reception. The same door returns to the hall. Face the slot
in reception's west wall and press A to inspect it. Press Select there
with the Picture Card in inventory. After the animation and message,
press A to go through the unlocked door into the west hall.

Face the notebook on the eastern desk and press A. A or B advances the first
visit’s message. Choose a slot with Up or Down, press A, then confirm. Left
or Right changes the confirmation choice. After saving, B returns to the room.
On a fresh boot, A loads the highlighted checkpoint and B starts at the piano.
The game keeps up to eleven slots in `/sh1dsi/` on the card containing the game
data. The notebook remembers the Chemical, the statue, the filled tower
slots, boiler power, the medallions and the medical supplies. Keep the
`note*.sav` files when updating the port.

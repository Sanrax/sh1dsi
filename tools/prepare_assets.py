"""Prepare the local SD data used by the school piano event."""
import argparse
import os
import struct
import tempfile
from pathlib import Path

from disc import open_disc
from notebook import notebook_data
from pickup import prompt_data
from vab import school_bank
from room import school_room

OUTPUT = Path(__file__).resolve().parents[1] / "dist/sh1dsi/piano.dat"


def indexed_tim(data, mode):
    magic, flags, pal_length, _, _, colors, rows = struct.unpack_from("<III4H", data)
    if (magic, flags) != (16, 8 | mode) or colors != (16, 256)[mode]:
        raise ValueError("unexpected indexed TIM format")
    if pal_length != 12 + 2 * colors * rows:
        raise ValueError("invalid TIM palette length")
    img = 8 + pal_length
    length, _, _, words, height = struct.unpack_from("<I4H", data, img)
    if length != 12 + words * 2 * height or img + length > len(data):
        raise ValueError("invalid TIM image length")
    return data[20:20 + 2 * colors], data[img + 12:img + length], words * 2, height


def background(data, uv_height=240):
    palette, pixels, stride, height = indexed_tim(data, 1)
    if (stride, height) != (320, 240):
        raise ValueError("expected a 320 by 240 event background")
    # Sample the center of each native LCD pixel across the whole PS1 frame.
    columns = [(2*x + 1)*5//8 for x in range(256)]
    rows = [((2*y + 1)*5//8*uv_height + uv_height//2)//240 for y in range(192)]
    image = bytes(pixels[v*stride+u] for v in rows for u in columns)
    return palette + image


def piano_cursor(data):
    # Gfx_CursorDraw samples a 32x32 region at UV (0,64) using CLUT row 0.
    palette, pixels, stride, height = indexed_tim(data, 0)
    if stride < 16 or height < 96:
        raise ValueError("cursor atlas is too small")
    colors = struct.unpack("<16H", palette)
    sprite = bytearray(16 * 16 * 2)
    for y in range(13):
        for x in range(13):
            u, v = (2*x + 1)*16//13, 64 + (2*y + 1)*16//13
            index = (pixels[v*stride + u//2] >> (4*(u % 2))) & 15
            color = colors[index]
            if color:
                struct.pack_into("<H", sprite, 2*(16*y + x), color | 0x8000)
    return sprite


def tower_data(disc):
    # Screen_BackgroundImgDrawBlackFadeIn maps V=239 to the edge at Y=240.
    images = b"".join(background(disc.read(f"TIM/{name}.TIM"), uv_height=239)
                      for name in ("CLOCK", "GOLD1", "SILVER1", "GOLD2", "SILVER2", "CLOCK12", "CLOCK5"))
    offsets = []
    messages = bytearray()
    for number in (15, 16, 19, 20, 17, 18):
        text = prompt_data(disc, number, first_floor=True, choices=False, map_bearings=True)
        if not 0 < struct.unpack_from("<H", text)[0] <= 40:
            raise ValueError("invalid clock tower message length")
        offsets.append(28+len(images)+len(messages))
        messages.extend(text)
    return struct.pack("<4s6I", b"SHT2", *offsets) + images + messages


def prepare(cue):
    with open_disc(cue) as disc:
        overlay = disc.read("VIN/MAP1_S01.BIN")
        # USA/maps/sym.map1_s01.txt and map1_s01.yaml identify the two tables.
        offset = 0x800DD500 - 0x800C9578
        tables = overlay[offset:offset+11] + overlay[offset+12:offset+17]
        data = (b"SHPC" + tables + piano_cursor(disc.read("TIM/BG_ETC.TIM"))
                + background(disc.read("TIM/PIANO1.TIM"))
                + background(disc.read("TIM/PIANO2.TIM"))
                + school_bank(disc.read("SND/MEP101.VAB"), disc.read("1ST/BASE.VAB"),
                              disc.read("SND/MEP100.VAB")))
        notebook = notebook_data(disc, background)
        tower = tower_data(disc)
        hand = (b"SHV1" + background(disc.read("TIM/HAND.TIM"), uv_height=239)
                + prompt_data(disc, 15, choices=False, map_bearings=True))
        room = school_room(disc)
        corridor = school_room(disc, 32)
        east = school_room(disc, 31)
        first_floor = school_room(disc, 12)
        south = school_room(disc, 13)
        north = school_room(disc, 16)
        upper_north = school_room(disc, 33)
        laboratory = school_room(disc, 34)
        equipment = school_room(disc, 35)
        courtyard = school_room(disc, 23)
        infirmary = school_room(disc, 9)
        basement = school_room(disc, 3)
        boiler = school_room(disc, 2)
        clock_room = school_room(disc, 20)
        passage = school_room(disc, 22)
        far_tower = school_room(disc, 21)
        otherworld = school_room(disc, 24)
        otherworld_north = school_room(disc, 25)
        otherworld_lobby = school_room(disc, 26)
        otherworld_side = school_room(disc, 27)
        otherworld_east = school_room(disc, 28)
        classroom_north = school_room(disc, 29)
        classroom_south = school_room(disc, 30)
    write_asset(OUTPUT.with_name("notebook.dat"), notebook)
    write_asset(OUTPUT.with_name("tower.dat"), tower)
    write_asset(OUTPUT.with_name("hand.dat"), hand)
    write_asset(OUTPUT, data)
    write_asset(OUTPUT.with_name("room.dat"), room)
    write_asset(OUTPUT.with_name("corridor.dat"), corridor)
    write_asset(OUTPUT.with_name("east.dat"), east)
    write_asset(OUTPUT.with_name("firstfloor.dat"), first_floor)
    write_asset(OUTPUT.with_name("south.dat"), south)
    write_asset(OUTPUT.with_name("north.dat"), north)
    write_asset(OUTPUT.with_name("uppernorth.dat"), upper_north)
    write_asset(OUTPUT.with_name("laboratory.dat"), laboratory)
    write_asset(OUTPUT.with_name("equipment.dat"), equipment)
    write_asset(OUTPUT.with_name("courtyard.dat"), courtyard)
    write_asset(OUTPUT.with_name("infirmary.dat"), infirmary)
    write_asset(OUTPUT.with_name("basement.dat"), basement)
    write_asset(OUTPUT.with_name("boiler.dat"), boiler)
    write_asset(OUTPUT.with_name("clockroom.dat"), clock_room)
    write_asset(OUTPUT.with_name("passage.dat"), passage)
    write_asset(OUTPUT.with_name("fartower.dat"), far_tower)
    write_asset(OUTPUT.with_name("otherworld.dat"), otherworld)
    write_asset(OUTPUT.with_name("otherworldnorth.dat"), otherworld_north)
    write_asset(OUTPUT.with_name("otherworldlobby.dat"), otherworld_lobby)
    write_asset(OUTPUT.with_name("otherworldside.dat"), otherworld_side)
    write_asset(OUTPUT.with_name("otherworldeast.dat"), otherworld_east)
    write_asset(OUTPUT.with_name("classroomnorth.dat"), classroom_north)
    write_asset(OUTPUT.with_name("classroomsouth.dat"), classroom_south)


def write_asset(output, data):
    output.parent.mkdir(exist_ok=True, parents=True)
    with tempfile.NamedTemporaryFile(dir=output.parent, delete=False) as tmp:
        try:
            tmp.write(data)
            tmp.close()
            os.replace(tmp.name, output)
        finally:
            Path(tmp.name).unlink(missing_ok=True)
    print(f"Prepared {output.name} ({len(data)} bytes) in dist/sh1dsi/")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cue", type=Path)
    try:
        prepare(parser.parse_args().cue)
    except (KeyError, OSError, ValueError, struct.error) as error:
        parser.exit(1, f"Asset preparation failed: {error}\n")

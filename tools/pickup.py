"""Prepare USA school pickup models and top-screen dialogs from the disc."""
import re
import struct

from notebook import font_advance


def prompt_data(disc, number, *, first_floor=False, choices=True, map_bearings=False, subtitle=False, otherworld=False):
    scene = 2 if subtitle or otherworld else 0 if first_floor else 1
    overlay = disc.read(f"VIN/MAP1_S{scene:02d}.BIN")
    table = (0x800dd1bc, 0x800dcf5c, 0x800e1854)[scene]
    base = 0x800c9578

    def message(number):
        address, = struct.unpack_from("<I", overlay, table-base+4*number)
        offset = address-base
        return overlay[offset:overlay.index(0, offset)].decode("ascii")

    font = disc.read("1ST/FONT16.TIM")
    image = 8 + struct.unpack_from("<I", font, 8)[0]
    if struct.unpack_from("<I4H", font, image) != (8204, 0, 0, 256, 16):
        raise ValueError("unexpected map font")
    palette = font[20:52]
    bitmap = font[image+12:image+8204]

    def glyph(c):
        idx = ord(c)-39
        if not 0 <= idx < 84:
            raise ValueError("unsupported prompt glyph")
        base_x = (idx//21)*256 + (idx%21)*12
        return [[(bitmap[512*y+(base_x+x)//2] >> (4*(x%2))) & 15
                 for x in range(12)] for y in range(16)]

    def advance(c, mask):
        # These font bearings consume the last ink column without an extra
        # gap in FONT_12X16_GLYPH_WIDTHS for the USA pickup dialog.
        ink_width = 1 + max(i for line in mask for i, p in enumerate(line) if p)
        if map_bearings:
            return font_advance(c, ink_width)
        if c == "I":
            return ink_width + 2
        return ink_width + (c not in "HTYNakvu'")

    # Keep the draw order for text rollout. Ink groups 2 and 3 let the
    # native selection flash by changing only their LCD palette entries.
    commands = []
    widths = []
    x = row = color = 0
    text = message(number)
    if subtitle:
        text = re.sub(r"^~J0\([0-9.]+\)", "", text)
    for token in re.findall(r"~[A-Z][0-9]?|[^\s]", text):
        if token == "~N":
            widths.append(x)
            x = 0
            row += 1
        elif token == "~C2":
            color = 1
        elif token == "~C3" and not choices:
            color = 2
        elif token == "~C7":
            color = 0
        elif token in ("~S4", "~E"):
            break
        elif token == "_":
            x += 6
        else:
            mask = glyph(token)
            commands.append((x, row, color, mask))
            x += advance(token, mask)
    widths.append(x)
    start_x = 160-max(widths)//2
    commands = [(start_x+x, 196-16*row+16*r, ink, mask) for x,r,ink,mask in commands]
    prompt_length = len(commands)
    for option in ((0, 1) if choices else ()):
        x = 32
        for c in message(option):
            mask = glyph(c)
            commands.append((x, 106+16*option, 2+option, mask))
            x += advance(c, mask)

    output = bytearray(struct.pack("<HH", len(commands), prompt_length)+palette)
    for x,y,ink,mask in commands:
        # Sample glyphs on the same 4:3 pixel grid as the PS1 backgrounds.
        left, top = (4*x)//5, (4*y)//5
        output.extend(struct.pack("4B", left, top, ink, 0))
        for v in range(14):
            for u in range(11):
                sx, sy = (2*(left+u)+1)*5//8-x, (2*(top+v)+1)*5//8-y
                output.append(mask[sy][sx] if 0 <= sx < 12 and 0 <= sy < 16 else 0)
    return output


def model_corners(tmd, counts, material, first_row, row_offset, *, u_offset=0):
    magic, flags, objects, vp, nv, np, nn, pp, faces, scale = struct.unpack_from("<10I", tmd)
    if (magic, flags, objects, scale) != (65, 0, 1, 0) or (nv, nn, faces) != counts:
        raise ValueError("unexpected pickup TMD")
    data = bytearray()
    for i in range(faces):
        p = 12+pp+28*i
        if tmd[p:p+4] != bytes((9,6,0,0x34)):
            raise ValueError("unsupported pickup primitive")
        for j in range(3):
            u,v,attr = struct.unpack_from("<BBH", tmd, p+4+4*j)
            n,k = struct.unpack_from("<HH", tmd, p+16+4*j)
            if (not 0 <= u-u_offset < 64 or not first_row <= v < first_row+64 or k >= nv or n >= nn or
                    (j < 2 and attr != material[j])):
                raise ValueError("invalid pickup material or index")
            x,y,z = struct.unpack_from("<3h", tmd, 12+vp+8*k)
            nx,ny,nz = struct.unpack_from("<3h", tmd, 12+np+8*n)
            # Map PS1 view coordinates into the DS camera basis.
            normal = sum((max(-512,min(511,c//8)) & 1023) << (10*axis)
                         for axis,c in enumerate((nx,-ny,-nz)))
            data.extend(struct.pack("<3hBBI", x,-y,-z,u-u_offset,v-row_offset,normal))
    return data


def key_item_tile(disc, first_row, clut, *, column=0):
    tim = disc.read("ITEM/TIM01.TIM")
    image = 8 + struct.unpack_from("<I", tim, 8)[0]
    if struct.unpack_from("<I4H", tim, image) != (32780, 0, 0, 64, 256):
        raise ValueError("unexpected key item texture")
    pixels = b"".join(tim[image+12+y*128+column//2:image+44+y*128+column//2] for y in range(first_row, first_row+64))
    return pixels, tim[20+clut*32:20+(clut+1)*32]


def silver_pickup(disc):
    data = model_corners(disc.read("ITEM/UNQ62.TMD"), (80, 108, 156), (138, 14), 128, 128)
    return data + prompt_data(disc, 30), *key_item_tile(disc, 128, 2)


def chemical_pickup(disc):
    data = model_corners(disc.read("ITEM/UNQ60.TMD"), (96, 91, 188), (10, 14), 0, 0)
    data.extend(prompt_data(disc, 18, map_bearings=True))
    data.extend(prompt_data(disc, 19, choices=False, map_bearings=True))
    return data, *key_item_tile(disc, 0, 0)


def gold_pickup(disc):
    data = model_corners(disc.read("ITEM/UNQ61.TMD"), (80, 108, 156), (74, 14), 64, 64)
    data.extend(prompt_data(disc, 17, map_bearings=True))
    data.extend(prompt_data(disc, 20, choices=False, map_bearings=True))
    return data, *key_item_tile(disc, 64, 1)


def medical_pickups(disc):
    # TIM00 rows 0–127 and its first two CLUTs are shared with the props.
    data = bytearray()
    for name, counts, clut, first_row, message in (
            ("UNQ21", (64, 66, 124), 75, 64, 6),
            ("UNQ20", (40, 34, 76), 11, 0, 5)):
        data.extend(model_corners(disc.read(f"ITEM/{name}.TMD"), counts, (clut, 15), first_row, 0))
        data.extend(prompt_data(disc, message, first_floor=True))
    return data

def picture_card(disc):
    model = model_corners(disc.read("ITEM/UNQ55.TMD"), (8, 2, 4), (458, 14), 192, 192, u_offset=64)
    model.extend(prompt_data(disc, 29, map_bearings=True, otherworld=True))
    model.extend(prompt_data(disc, 45, choices=False, map_bearings=True, otherworld=True))
    return model, *key_item_tile(disc, 192, 7, column=64)


def lobby_pickups(disc):
    kit = model_corners(disc.read("ITEM/UNQ20.TMD"), (40, 34, 76), (11, 15), 0, 0)
    bullets = model_corners(disc.read("ITEM/UNQC0.TMD"), (24, 21, 44), (779, 15), 0, -64, u_offset=192)
    return (kit + prompt_data(disc, 5, otherworld=True) +
            bullets + prompt_data(disc, 8, otherworld=True))

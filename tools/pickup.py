"""Prepare the USA music-room pickup model and top-screen dialog from the disc."""
import re
import struct


def prompt_data(disc, latch=False):
    overlay = disc.read("VIN/MAP1_S00.BIN" if latch else "VIN/MAP1_S01.BIN")
    table = 0x800dd1bc if latch else 0x800dcf5c
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
        if c == "I":
            return ink_width + 2
        return ink_width + (c not in "TYNakvu'")

    # Keep the draw order for text rollout. Ink groups 2 and 3 let the
    # native selection flash by changing only their LCD palette entries.
    commands = []
    widths = []
    x = row = color = 0
    for token in re.findall(r"~[A-Z][0-9]?|[^\s]", message(13 if latch else 30)):
        if token == "~N":
            widths.append(x)
            x = 0
            row += 1
        elif token == "~C2":
            color = 1
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
    for option in (() if latch else (0, 1)):
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


def silver_pickup(disc):
    tmd = disc.read("ITEM/UNQ62.TMD")
    magic, flags, objects, vp, nv, np, nn, pp, faces, scale = struct.unpack_from("<10I", tmd)
    if (magic, flags, objects, nv, nn, faces, scale) != (65, 0, 1, 80, 108, 156, 0):
        raise ValueError("unexpected silver medallion TMD")
    data = bytearray()
    for i in range(faces):
        p = 12+pp+28*i
        if tmd[p:p+4] != bytes((9,6,0,0x34)):
            raise ValueError("unsupported silver medallion primitive")
        for j in range(3):
            u,v,attr = struct.unpack_from("<BBH", tmd, p+4+4*j)
            n,k = struct.unpack_from("<HH", tmd, p+16+4*j)
            if u >= 64 or not 128 <= v < 192 or k >= nv or n >= nn or (j < 2 and attr != (138,14)[j]):
                raise ValueError("invalid silver medallion material or index")
            x,y,z = struct.unpack_from("<3h", tmd, 12+vp+8*k)
            nx,ny,nz = struct.unpack_from("<3h", tmd, 12+np+8*n)
            # Map PS1 view coordinates into the DS camera basis.
            normal = sum((max(-512,min(511,c//8)) & 1023) << (10*axis)
                         for axis,c in enumerate((nx,-ny,-nz)))
            data.extend(struct.pack("<3hBBI", x,-y,-z,u,v-128,normal))

    tim = disc.read("ITEM/TIM01.TIM")
    image = 8 + struct.unpack_from("<I", tim, 8)[0]
    if struct.unpack_from("<I4H", tim, image) != (32780, 0, 0, 64, 256):
        raise ValueError("unexpected key item texture")
    pixels = b"".join(tim[image+12+y*128:image+12+y*128+32] for y in range(128,192))
    return data + prompt_data(disc), pixels, tim[20+2*32:20+3*32]

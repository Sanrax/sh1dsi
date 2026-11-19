"""Prepare the infirmary notebook text, font and save-screen image locally."""
import re
import struct


def notebook_data(disc, background):
    overlay = disc.read("VIN/MAP1_S00.BIN")
    base = 0x800c9578
    address, = struct.unpack_from("<I", overlay, 0x800dd1bc-base+4*2)
    start = address-base
    text = re.sub(rb"\s+", b"", overlay[start:overlay.index(0, start)])
    if not text.endswith(b"~E") or text.count(b"~N") != 2:
        raise ValueError("unexpected notebook message")
    text = text[:-2].replace(b"~N", b"\n").replace(b"_", b" ")
    if len(text) >= 128:
        raise ValueError("notebook message too long")

    tim = disc.read("1ST/FONT16.TIM")
    image = 8 + struct.unpack_from("<I", tim, 8)[0]
    if struct.unpack_from("<I4H", tim, image) != (8204, 0, 0, 256, 16):
        raise ValueError("unexpected notebook font")
    widths = bytearray()
    glyphs = bytearray()
    for i in range(84):
        x = (i//21)*256 + (i%21)*12
        ink_width = 0
        for y in range(16):
            offset = image+12+512*y+x//2
            row = tim[offset:offset+6]
            glyphs.extend(row)
            for u in range(12):
                if (row[u//2] >> (4*(u%2))) & 15:
                    ink_width = max(ink_width, u+1)
        c = chr(39+i)
        # USA FONT_12X16_GLYPH_WIDTHS bearings relative to the last ink column.
        bearing = 0 if c in "'ACHKNRTVWY[\\]_`afkuvwy" else 1
        if c in "()Ijpz": bearing = 2
        if c == "=": bearing = -1
        widths.append(ink_width + bearing)
    return (b"SHN1" + text.ljust(128, b"\0") + widths + glyphs + tim[20:52]
            + background(disc.read("TIM/SAVELOAD.TIM")))

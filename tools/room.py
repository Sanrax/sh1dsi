"""Build Midwich room streams from the original IPD instances and embedded LM."""
from collections import defaultdict
import math
import struct

from harry import courtyard_look_frames, ladder_frames, player_data, pour_frames
from phone import receiver_texture
from pickup import chemical_pickup, gold_pickup, lobby_pickups, medical_pickups, picture_card, prompt_data, rubber_ball, silver_pickup
from vab import statue_sound
from xa import courtyard_voices


ROOM_EDGE_HEIGHTS = 1
ROOM_FLOOR_PLANES = 2
ROOM_WIDE_VERTICES = 4
ROOM_GROUND_PLANES = 8
ROOM_BOILER_EVENT = 16
ROOM_DOOR_NOTICE = 32
ROOM_LADDER_ASCENT = 64
ROOM_NORMALS = 128

ROOM_LAYOUTS = {
    2: ROOM_BOILER_EVENT,
    3: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_GROUND_PLANES,
    6: ROOM_EDGE_HEIGHTS | ROOM_NORMALS,
    7: ROOM_EDGE_HEIGHTS | ROOM_NORMALS,
    8: ROOM_EDGE_HEIGHTS | ROOM_NORMALS,
    10: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_WIDE_VERTICES | ROOM_GROUND_PLANES | ROOM_NORMALS,
    11: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_WIDE_VERTICES | ROOM_NORMALS,
    13: ROOM_EDGE_HEIGHTS,
    14: ROOM_EDGE_HEIGHTS | ROOM_NORMALS,
    17: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_WIDE_VERTICES | ROOM_GROUND_PLANES | ROOM_NORMALS,
    18: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_GROUND_PLANES | ROOM_NORMALS,
    19: ROOM_EDGE_HEIGHTS | ROOM_NORMALS,
    36: ROOM_EDGE_HEIGHTS | ROOM_WIDE_VERTICES | ROOM_NORMALS,
    20: ROOM_DOOR_NOTICE,
    21: 0,
    22: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_GROUND_PLANES | ROOM_LADDER_ASCENT,
    23: ROOM_FLOOR_PLANES,
    24: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_NORMALS,
    25: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_WIDE_VERTICES | ROOM_GROUND_PLANES | ROOM_NORMALS,
    26: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_GROUND_PLANES | ROOM_NORMALS,
    27: ROOM_EDGE_HEIGHTS | ROOM_NORMALS,
    28: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_WIDE_VERTICES | ROOM_GROUND_PLANES | ROOM_NORMALS,
    29: ROOM_EDGE_HEIGHTS | ROOM_NORMALS,
    30: ROOM_EDGE_HEIGHTS | ROOM_NORMALS,
    31: ROOM_EDGE_HEIGHTS | ROOM_FLOOR_PLANES | ROOM_WIDE_VERTICES | ROOM_GROUND_PLANES,
    32: ROOM_EDGE_HEIGHTS,
}


def convex_parts(edges):
    # Follow the authored edges. Sorting around a centre can change a concave footprint.
    successors = dict(edges)
    if len(successors) != len(edges):
        raise ValueError("branched floor boundary")
    start = min(successors)
    corners = [start]
    while corners[-1] in successors:
        end = successors.pop(corners[-1])
        if end == start:
            break
        corners.append(end)
    if end != start or successors:
        raise ValueError("open or disconnected floor boundary")

    def cross(a, b, p):
        return (b[0]-a[0])*(p[1]-a[1]) - (b[1]-a[1])*(p[0]-a[0])

    if sum(cross((0, 0), a, b) for a, b in zip(corners, corners[1:]+corners[:1])) < 0:
        corners.reverse()
    # Emit an ear only when necessary. Convex footprints use one record.
    while len(corners) > 3:
        triples = list(zip(corners[-1:]+corners[:-1], corners, corners[1:]+corners[:1]))
        if all(cross(*triangle) > 0 for triangle in triples):
            break
        for index, (a, b, c) in enumerate(triples):
            if cross(a, b, c) > 0 and not any(
                    p not in (a, b, c) and cross(a, b, p) >= 0 and cross(b, c, p) >= 0 and cross(c, a, p) >= 0
                    for p in corners):
                yield (a, b, c)
                del corners[index]
                break
        else:
            raise ValueError("invalid ground floor footprint")
    if len(corners) < 3 or cross(*corners[:3]) <= 0:
        raise ValueError("degenerate ground floor footprint")
    yield corners


def collision_edges(ipd, room):
    layout = ROOM_LAYOUTS.get(room)
    edge_heights = layout is not None and layout & ROOM_EDGE_HEIGHTS
    collision = memoryview(ipd)[0x54:]
    x, z, vertices, surfaces, count, cylinders, vert, surface, polygon = struct.unpack_from("<ii4B3I", collision)
    expected = {6: (0, 20480, 16, 3, 16, 0), 7: (0, 30720, 36, 5, 36, 0),
                2: (0, -10240, 35, 9, 43, 0), 3: (10240, -10240, 25, 4, 26, 0),
                39: (-30720, 0, 51, 8, 51, 0), 32: (-10240, 0, 60, 10, 60, 0),
                31: (-20480, 10240, 69, 15, 71, 0), 12: (20480, 10240, 80, 21, 85, 0),
                13: (30720, 0, 80, 12, 80, 0), 11: (30720, 0, 33, 7, 35, 0), 9: (10240, 30720, 45, 11, 49, 0), 8: (10240, 0, 34, 6, 34, 0),
                10: (10240, 10240, 40, 9, 42, 0),
                14: (0, 0, 35, 6, 36, 0), 17: (-30720, 10240, 54, 14, 56, 0),
                18: (-20480, -10240, 19, 6, 24, 0),
                19: (20480, 20480, 38, 5, 38, 0),
                36: (-10240, -10240, 58, 8, 59, 0),
                20: (-30720, -20480, 16, 3, 16, 0), 21: (-40960, -20480, 16, 3, 16, 0),
                22: (-20480, -20480, 28, 8, 32, 0),
                23: (-10240, -20480, 204, 61, 236, 0), 24: (-10240, -20480, 172, 31, 175, 0),
                16: (30720, 40960, 33, 6, 34, 0), 25: (30720, 40960, 24, 4, 24, 0),
                26: (20480, 30720, 61, 7, 63, 0), 27: (30720, 30720, 16, 5, 20, 0),
                28: (20480, 10240, 51, 13, 53, 0),
                29: (30720, 20480, 34, 6, 34, 0), 30: (30720, 10240, 40, 6, 42, 0),
                33: (-10240, 40960, 40, 8, 40, 0), 34: (-20480, 20480, 38, 5, 38, 15),
                35: (-30720, 20480, 40, 7, 40, 0)}[room]
    if (x, z, vertices, surfaces, count, cylinders) != expected:
        raise ValueError("unexpected school collision header")
    output = bytearray()
    for i in range(count):
        tx, tz, length, a, b, floor, other = struct.unpack_from("<HHh4B", collision, polygon+10*i)
        if (max(a, b) >= vertices or floor >= surfaces or
                (other != 255 and other >= surfaces) or length <= 0):
            raise ValueError("invalid school collision split")
        # SCFF00 has an inner shell along the hallway, plus the benches
        # and counters. Its roof extends over the adjoining rooms.
        if room == 32 and not (floor in (1, 2, 3, 4, 5, 6) or
                               i in (36, 37, 49, 50, 51)):
            continue
        # SCFE01 includes the locker banks, stair ramp and landing.
        # Leave the shared walkable edges open.
        if room == 31 and (floor == 14 or other != 255):
            continue
        # The upper and basement stair flights share this first-floor chunk.
        if room == 12 and (floor in (0, 1, 2, 10, 11) or other != 255):
            continue
        # The boiler obstacles have shared internal splits. The basement
        # ramp meets the flat floor at edge zero, with a ceiling above both.
        if room == 2 and other != 255:
            continue
        if room == 3 and (i == 0 or floor == 3 or other != 255):
            continue
        # The outer roof perimeter lies beyond the playable shaft walls.
        if room in (20, 21) and i < 4:
            continue
        # Passable ground joins at shared edges. The inner shaft ledges
        # can block movement from the passage even though their tops cannot.
        if room == 22 and ((floor < 3 and i not in (4, 6, 10, 12)) or other != 255):
            continue
        # SC0300's inside walls and door jambs are part of the shell
        # above the ground. Keep its hall-facing outline.
        if room == 13 and (floor == 3 or (floor == 4 and not 29 <= i <= 45)):
            continue
        # The alternate southern hallway's inside walls and the lockers.
        if room == 11 and not (i in (0, 4) or 7 <= i <= 14 or i >= 23):
            continue
        # SC0304's inner shell follows the usable hallway around its alcove.
        if room == 16 and (floor == 1 or (floor == 0 and not 5 <= i <= 12)):
            continue
        # SU0304 has an inner hallway shell and two locker banks. The
        # outer shells belong to the roof and the full ground plane.
        if room == 25 and (i < 4 or floor == 1):
            continue
        # Use the lobby's inner walls, the fan opening and the benches.
        if room == 26 and (other != 255 or i in (7, 8, 9, 12, 13, 14) or floor == 2):
            continue
        # The side hall's other splits outline the roof and its joins.
        if room == 27 and i not in (6, 8, 9, 10, 15, 18):
            continue
        # SU0201's center barrier and two stair flights share the hall.
        # The shared splits connect walkable planes.
        if room == 28 and (other != 255 or i < 5 or floor in (1, 3, 4)):
            continue
        # Use the inner shell and the furniture silhouettes on both floors.
        if room == 33 and 24 <= i < 32:
            continue
        if room == 34 and 22 <= i < 30:
            continue
        # The equipment room starts with eight edges outside the inner shell.
        if room == 35 and i < 8:
            continue
        # The infirmary walls meet at shared splits. Keep their exposed faces.
        if room == 9 and other != 255:
            continue
        # Keep the tower perimeter, raised planters, trees and posts.
        # The entrance and tower ramps remain traversable.
        if room == 23 and (floor in (1, 30, 37, 38, 39, 40) or 50 <= floor < 60 or other != 255):
            continue
        # The alternate courtyard includes two entrance ramps and the
        # diagonal tower ramp. Their surfaces belong to the floor query.
        if room == 24 and (other != 255 or floor in (7, 15, 16, 17, 18, 28, 29)):
            continue
        # Keep the inside shell and the desks. The outer shell is beyond
        # the usable classrooms. Shared edges 11 and 13 in room 30 lie outside it.
        if (room == 29 and i < 8) or (room == 30 and (i < 8 or i in (11, 13))):
            continue
        # SU0000 shares an outer shell with the stall partition.
        if room == 14 and (i < 4 or 8 <= i <= 11 or i == 19):
            continue
        # Keep SUFD01's corridor walls, two barriers and the lockers.
        if room == 17 and not (5 <= i <= 10 or i in (26, 27) or i >= 32):
            continue
        # SU0101's inside shell, cross-hall barrier and locker banks.
        if room == 10 and not (6 <= i <= 11 or i in (13, 14) or i >= 30):
            continue
        # The teachers' rooms share outer shells and overhead beams.
        # The northern room also has a desk and two low chairs.
        if room in (6, 7) and (i < 8 or 12 <= i < 16):
            continue
        # The roof flight's walkable planes share two ramp edges. Keep
        # the inside shell along the stair and door.
        if room == 18 and i not in (0, 10, 11, 17, 18, 22, 23):
            continue
        # The roof's railings, drain curb, and walls border the flat path.
        if room == 36 and not (i < 12 or 18 <= i <= 29 or i in (30, 34, 35, 36, 37, 38)):
            continue
        # Storage has separate outer shells, inside walls and pole/pile edges.
        if room == 19 and (i < 4 or 8 <= i < 12):
            continue
        if room == 8 and i < 8:
            continue
        if other != 255:
            raise ValueError("unexpected shared collision edge")
        ax, ay, az = struct.unpack_from("<3h", collision, vert+6*a)
        bx, by, bz = struct.unpack_from("<3h", collision, vert+6*b)
        # The top two bits of each tangent component are trigger flags.
        slope = struct.unpack_from("<2h", collision, surface+12*floor+8)
        expected_slope = {(3, 0): (-2735, 0), (31, 11): (2731, 0), (32, 5): (0, -31), (12, 7): (2778, 5), (12, 9): (-2753, 0),
                          (22, 4): (0, 2867), (22, 6): (0, 2841), (28, 5): (2785, 6), (28, 7): (-2740, 0)}.get((room, floor), (0, 0))
        if (tx | tz) & 0xc000 or slope != expected_slope:
            raise ValueError("unsupported school collision flags or slope")
        tx = ((tx ^ 8192) - 8192)
        tz = ((tz ^ 8192) - 8192)
        scale = 16 if room == 39 else 1
        output.extend(struct.pack("<6h", (ax-5120)*scale, (az-5120)*scale,
                                  (bx-5120)*scale, (bz-5120)*scale, tx, tz))
        if edge_heights:
            output.extend(struct.pack("<2h", ay, by))
    if room == 34:
        cylinder_offset, = struct.unpack_from("<I", collision, 0x18)
        for i in range(cylinders):
            flags, px, py, pz, radius = struct.unpack_from("<H4h", collision, cylinder_offset+10*i)
            if flags != 1 or py != -147 or not 0 < radius < 256:
                raise ValueError("unexpected lab obstacle cylinder")
            output.extend(struct.pack("<3h", px-5120, pz-5120, radius))
    if layout is not None or room == 12:
        stride = 16 if edge_heights else 12
        output = struct.pack("<H", len(output)//stride) + output
    if layout is None or not layout & ROOM_FLOOR_PLANES:
        return output

    floors = bytearray()
    floor_planes = {17: range(1, 8), 18: (0, 1, 2), 10: (2, 3, 4, 5), 11: (3,), 3: (0, 1), 22: range(3, 8), 31: (11, 12), 23: (*range(37, 41), *range(50, 60)),
                    24: (15, 16, 17, 18, 28, 29), 25: (1,), 26: (2,), 28: range(1, 8)}[room]
    floor_count = 0
    for plane in floor_planes:
        edges = []
        for i in range(count):
            a, b, left, right = struct.unpack_from("4B", collision, polygon+10*i+6)
            if plane not in (left, right) or left == right:
                continue
            p, q = (struct.unpack_from("<3h", collision, vert+6*k)[::2] for k in (a, b))
            edges.append((p, q) if plane == left else (q, p))
        if room == 28 and plane in (2, 4, 5):
            # The floor and lower stair continue to SU0200. Close their
            # polygon footprints where the source omits the chunk seam.
            starts = {a for a, _ in edges}
            ends = {b for _, b in edges}
            start, = starts-ends
            end, = ends-starts
            if start[1] != -54 or end[1] != -54:
                raise ValueError("unexpected east hall floor seam")
            edges.append((end, start))
        if (room == 10 and plane == 4) or (room == 17 and plane in (2, 4, 5)):
            # Close the flat floor where the source joins the northern chunk.
            starts = {a for a, _ in edges}
            ends = {b for _, b in edges}
            start, = starts-ends
            end, = ends-starts
            if start[1] != 10268 or end[1] != 10268:
                raise ValueError("unexpected west hall floor seam")
            edges.append((end, start))
        if layout & ROOM_GROUND_PLANES:
            parts = convex_parts(edges)
        else:
            corners = set(point for edge in edges for point in edge)
            center_x = sum(x for x, _ in corners)
            center_z = sum(z for _, z in corners)
            n = len(corners)
            parts = (sorted(corners, key=lambda p: math.atan2(n*p[1]-center_z, n*p[0]-center_x)),)
        xref, y, zref, _, sx, sz = struct.unpack_from("<3hH2h", collision, surface+12*plane)
        equation = struct.pack("<5h", xref-5120, y, zref-5120, sx, sz)
        for part in parts:
            if not 3 <= len(part) <= 8:
                raise ValueError("unsupported floor boundary size")
            floors.append(len(part))
            for x, z in part:
                floors.extend(struct.pack("<2h", x-5120, z-5120))
            floors.extend(equation)
            floor_count += 1
    return output + struct.pack("<H", floor_count) + floors


def model_directory(data):
    _, count, model_ptr = struct.unpack_from("<III", data, 4)
    models = {}
    for i in range(count):
        name, meshes, base, _, _, ptr = struct.unpack_from("<8s4BI", data, model_ptr + 16*i)
        models[name.rstrip(b"\0")] = data, meshes, base, ptr
    return models


def infirmary_items(disc):
    # BG_ITEM.PLM uses four opaque CLUTs across two PS1 pages. The three
    # models sample only the first 64 columns. Pack those rows into one page.
    pixels = bytearray(64*256//2)
    colors = bytearray()
    for name, words, rows, dest, cluts in (
            ("ITEM/TIM00.TIM", 64, 128, 0, (0, 1)),
            ("TIM/BG_ETC.TIM", 32, 32, 128, (9, 10))):
        tim = disc.read(name)
        magic, flags, length, _, _, width, palettes = struct.unpack_from("<III4H", tim)
        image = 8 + length
        size, _, _, stride, height = struct.unpack_from("<I4H", tim, image)
        if (magic, flags, width, stride, height) != (16, 8, 16, words, 256) or max(cluts) >= palettes:
            raise ValueError("unexpected common item texture")
        if size != 12+2*stride*height or image+size > len(tim):
            raise ValueError("truncated common item texture")
        for y in range(rows):
            start = image+12+y*stride*2
            pixels[(dest+y)*32:(dest+y+1)*32] = tim[start:start+32]
        for row in cluts:
            palette = tim[20+row*32:52+row*32]
            if 0 in struct.unpack("<16H", palette):
                raise ValueError("unexpected transparent common item color")
            colors.extend(palette)

    overlay = disc.read("VIN/MAP1_S00.BIN")
    # g_CommonWorldObjectPoses[3..4] and Map_WorldObjectsInit's notebook.
    poses = [(b"PAD_NEAR", (int(61.8531*4096), int(-0.805*4096), int(140.6099*4096)),
              int(92.9*4096/360))]
    for i, name in ((3, b"DRINK_NE"), (4, b"AIDKIT_N")):
        x,y,z,rx,ry,rz = struct.unpack_from("<3i3h", overlay, 0x800dd71c-0x800c9578+20*i)
        if rx or rz:
            raise ValueError("unsupported common item rotation")
        poses.append((name, (x,y,z), ry))
    return pixels, colors, poses


def vertical_bounds(points, depth_spread):
    # Some music-room decorations are slanted or split into triangles.
    for axis, across in ((0, 2), (2, 0)):
        depths = [p[axis] for p in points]
        if max(depths)-min(depths) > depth_spread:
            continue
        horizontal = [p[across] for p in points]
        vertical = [p[1] for p in points]
        winding = sum(a[across]*b[1]-a[1]*b[across] for a, b in zip(points, points[1:]+points[:1]))
        if not winding:
            continue
        side = 1 if (winding > 0) == (axis == 0) else -1
        depth = min(depths) if side == 1 else max(depths)
        return (axis, depth, side), (min(horizontal), max(horizontal), min(vertical), max(vertical))
    return None


def vertical_rectangle(points):
    for axis, across in ((0, 2), (2, 0)):
        if any(p[axis] != points[0][axis] for p in points[1:]):
            continue
        horizontal = sorted({p[across] for p in points})
        vertical = sorted({p[1] for p in points})
        if len(horizontal) != 2 or len(vertical) != 2 or len(set(points)) != 4:
            return None
        a, b, c, _ = points
        winding = (b[across]-a[across])*(c[1]-a[1]) - (b[1]-a[1])*(c[across]-a[across])
        # The renderer keeps clockwise faces. Move toward their visible side.
        side = 1 if (winding > 0) == (axis == 0) else -1
        return (axis, points[0][axis], side), (*horizontal, *vertical)
    return None


def smoke_atlas(disc):
    # The PS1 smoke uses BLD.TIM's CLUT 4 and the three 32-pixel tiles
    # at U=224, V=160/192/224. Translate additive intensity to DS alpha.
    tim = disc.read("TIM/BLD.TIM")
    if struct.unpack_from("<III4H", tim) != (16, 8, 524, 0, 0, 16, 16):
        raise ValueError("unexpected smoke TIM palette")
    image = 532
    if struct.unpack_from("<I4H", tim, image) != (32780, 0, 0, 64, 256):
        raise ValueError("unexpected smoke TIM image")
    clut = struct.unpack_from("<16H", tim, 20+4*32)
    alpha = [((color & 31)*5+5)//11 << 5 for color in clut]
    pixels = bytearray()
    for y in range(160,256):
        offset = image+12+y*128+112
        for byte in tim[offset:offset+16]:
            pixels.extend((alpha[byte & 15], alpha[byte >> 4]))
    return pixels + bytes(32*32), struct.pack("<H", 0x7fff) + bytes(62)


def school_room(disc, room=39):
    cx, cz = {39: (253, 0), 32: (255, 0), 31: (254, 1), 12: (2, 1), 13: (3, 0), 11: (3, 0), 6: (0, 2), 7: (0, 3), 8: (1, 0), 10: (1, 1), 14: (0, 0), 17: (253, 1), 18: (254, 255), 19: (2, 2), 36: (255, 255), 9: (1, 3),
              23: (255, 254), 24: (255, 254), 25: (3, 4), 26: (2, 3), 27: (3, 3), 28: (2, 1), 29: (3, 2), 30: (3, 1), 16: (3, 4), 33: (255, 4), 34: (254, 2), 35: (253, 2),
              3: (1, 255), 2: (0, 255), 20: (253, 254), 21: (252, 254), 22: (254, 254)}[room]
    prefix = "SU" if room in (6, 7, 8, 10, 11, 14, 17, 18, 19, 24, 25, 26, 27, 28, 29, 30, 36) else "SC"
    data = disc.read(f"BG/{prefix}{cx:02X}{cz:02X}.IPD")
    if data[:4] != bytes((20, 0, cx, cz)):
        raise ValueError("unexpected school chunk coordinates")
    lm, = struct.unpack_from("<I", data, 4)
    mat_count = data[lm + 3]
    mat_ptr, = struct.unpack_from("<I", data, lm + 4)
    models = model_directory(memoryview(data)[lm:])

    pages = bytearray()
    palettes = bytearray()
    page_info = []
    color_keys = []
    scene_textures = []
    remapped_regions = {}
    for i in range(mat_count):
        material = lm + mat_ptr + i*24
        name = data[material:material+8].rstrip(b"\0").decode("ascii")
        tim = disc.read(f"BG/{name}.TIM")
        magic, flags, length, _, _, colors, rows = struct.unpack_from("<III4H", tim)
        image = 8 + length
        size, _, _, words, height = struct.unpack_from("<I4H", tim, image)
        if (magic, flags, colors, height) != (16, 8, 16, 256) or words not in (32, 64):
            raise ValueError(f"unexpected texture layout for {name}")
        page_info.append((len(pages), words*4, len(palettes)//32, rows))
        cluts = list(struct.iter_unpack("<16H", tim[20:20 + rows*32]))
        pixels = bytearray(tim[image+12:image+size])
        regions = ()
        if room in (23, 24) and name == "SC1FSE2":
            # The tree and window palettes each have their own cutout index.
            regions = ((2, 14, 0, (0, 63, 0, 127)), (4, 15, 0, (64, 159, 0, 63)))
        if room == 22 and name == "SC1FURA2":
            # The end-wall tile can share the shaft palette's cutout index.
            regions = ((5, 2, 15, (128, 239, 208, 255)),)
        if room == 26 and name == "SU1FTE1":
            regions = ((6, 2, 0, (32, 159, 160, 255)),)
        if room == 8 and name == "SU1FL2":
            # Rekey the two disjoint cutout tiles. The first tile uses
            # indices 0 through 10, with 10 transparent.
            for y in range(112, 256):
                start = y*words*2
                if any((b & 15) > 10 or b >> 4 > 10 for b in pixels[start:start+48]):
                    raise ValueError("unexpected reception cutout indices")
            cluts[1] = cluts[1][:11] + (cluts[1][0],)*5
            regions = ((1, 10, 0, (0, 95, 112, 255)), (4, 15, 0, (112, 175, 0, 95)))
        if room == 14 and name == "SU1FWW1":
            # The stall tile uses 10 as its key and stops before index 11.
            for y in range(128, 176):
                start = y*words*2+40
                if any((b & 15) > 10 or b >> 4 > 10 for b in pixels[start:start+24]):
                    raise ValueError("unexpected restroom cutout indices")
            cluts[1] = cluts[1][:11] + (cluts[1][0],)*5
            regions = ((1, 10, 0, (80, 127, 128, 175)),)
        if room == 14 and name == "SU1FWW2":
            regions = ((6, 15, 0, (96, 191, 128, 255)),)
        if room == 6 and name == "SU2FC11":
            # The chain's tile uses index 0 while the door and window use 1.
            regions = ((3, 0, 1, (160, 207, 0, 143)),)
        if room == 18 and name == "SU3FTH":
            regions = ((1, 14, 15, (64, 127, 0, 63)), (2, 14, 15, (0, 63, 0, 47)))
        for row, key, target, rect in regions:
            if {j for j, value in enumerate(cluts[row]) if not value} != {key}:
                raise ValueError("unexpected cutout palette")
            remapped_regions[i, row] = rect
            left, right, top, bottom = rect
            mask = key ^ target
            for y in range(top, bottom+1):
                for j in range(y*words*2+left//2, y*words*2+right//2+1):
                    pixels[j] ^= mask | mask << 4
            cluts[row] = tuple(cluts[row][j ^ mask] for j in range(16))
        if room in (6, 7, 29) and name == "SU2FC11":
            if {i for i, color in enumerate(cluts[2]) if color == 0} != {0, 15}:
                raise ValueError("unexpected classroom cutout palette")
            # Both transparent indices share the door tile. Move them to
            # index 1, already used by the page's other cutout palette.
            remapped_regions[i, 2] = (208, 255, 0, 127)
            indices = (1, 0, *range(2, 15), 1)
            for y in range(128):
                for x in range(y*128+104, y*128+128):
                    value = pixels[x]
                    pixels[x] = indices[value & 15] | indices[value >> 4] << 4
            color = cluts[2]
            cluts[2] = (color[1], 0, *color[2:15], color[1])
        if room == 19 and name == "SUBFO2":
            if {i for i, color in enumerate(cluts[7]) if color == 0} != {0, 13}:
                raise ValueError("unexpected storage cutout palette")
            remapped_regions[i, 7] = (176, 223, 160, 255)
            indices = (15, *range(1, 13), 15, 14, 0)
            for y in range(160, 256):
                for x in range(y*128+88, y*128+112):
                    value = pixels[x]
                    pixels[x] = indices[value & 15] | indices[value >> 4] << 4
            color = cluts[7]
            cluts[7] = (color[15], *color[1:13], color[15], color[14], 0)
        if room == 36 and name == "SU3F11":
            # This railing tile has four transparent palette entries.
            keys = {10, 13, 14, 15}
            palette = cluts[0]
            if {j for j, color in enumerate(palette) if not color} != keys:
                raise ValueError("unexpected roof railing cutout palette")
            remapped_regions[i, 0] = (0, 63, 0, 79)
            indices = tuple(15 if j in keys else j for j in range(16))
            for y in range(80):
                for j in range(y*128, y*128+32):
                    value = pixels[j]
                    pixels[j] = indices[value & 15] | indices[value >> 4] << 4
            cluts[0] = tuple(palette[1] if j in keys and j != 15 else color
                             for j, color in enumerate(palette))
        if room == 23 and name == "SC1FSEH":
            # The last CLUT is unreferenced in this IPD.
            cluts = cluts[:7]
            page_info[-1] = (*page_info[-1][:3], len(cluts))
        color_keys.append([0 in clut for clut in cluts])
        scene_textures.append((name, len(pixels), cluts))
        pages.extend(pixels)
        for clut in cluts:
            palettes.extend(struct.pack("<16H", *clut))

    # Group opaque polygons by texture and CLUT without duplicating any texture.
    groups = defaultdict(bytearray)
    normals = defaultdict(bytearray)
    walls = defaultdict(list)
    decorations = []
    music_decorations = []
    layout = ROOM_LAYOUTS.get(room)
    lit = layout is not None and layout & ROOM_NORMALS
    wide = room in (12, 16) if layout is None else (layout & ROOM_WIDE_VERTICES) != 0
    vertex_scale = 4 if wide else 8
    def add_model(name, matrix, position, state=0, *, item_page=None):
        model_data, meshes, base, ptr = models[name]
        for j in range(meshes):
            faces, nverts, normal_count, ref_count, prim, xy, z, normal_ptr, refs = struct.unpack_from(
                "<4B5I", model_data, ptr + 24*j
            )
            if lit:
                # The LM maps every corner to a normal and the vertex
                # used for the PS1 lighting query.
                corner_normals = []
                for i in range(normal_count):
                    *normal, count = struct.unpack_from("<3bB", model_data, normal_ptr+4*i)
                    rotated = [sum(matrix[3*a+k]*normal[k] for k in range(3)) >> 12 for a in range(3)]
                    encoded = struct.pack("<3bx", *(max(-128, min(127, n)) for n in rotated))
                    corner_normals.extend([encoded] * count)
                if len(corner_normals) != ref_count:
                    raise ValueError("unexpected LM normal map")
            vertices = []
            for v in range(nverts):
                coord = (*struct.unpack_from("<hh", model_data, xy+4*v),
                         *struct.unpack_from("<h", model_data, z+2*v))
                transformed = [position[a] + (sum(matrix[3*a+k]*coord[k] for k in range(3)) >> 12)
                               for a in range(3)]
                x, y, zpos = transformed
                # The longer hallways need the wider V16 range.
                vertices.append(((x-5120)*vertex_scale, -y*vertex_scale, -(zpos-5120)*vertex_scale))
            for k in range(faces):
                face = model_data[prim+20*k:prim+20*(k+1)]
                mat = face[7] & 127
                row = int.from_bytes(face[2:4], "little") >> 6
                u_offset = v_offset = 0
                untextured = mat == 127
                if untextured:
                    # Model_MaterialFlagsApply uses the no-material sentinel.
                    mat = row = 0
                if item_page is not None and room == 26:
                    # Two 64x64 tiles from TIM00 share the lobby item atlas.
                    if mat or row not in (0, 12):
                        raise ValueError("unexpected lobby item material")
                    row = int(row == 12)
                    u_offset, v_offset = 192*row, 64*row
                    mat = item_page
                elif item_page is not None:
                    # BG_ETC rows 9/10 occupy CLUTs 2/3 in the atlas.
                    if (mat, row) not in ((0, 0), (0, 1), (1, 9), (1, 10)):
                        raise ValueError("unexpected common item material")
                    if mat == 1:
                        row -= 7
                        v_offset = 128
                    mat = item_page
                if mat >= len(page_info) or row >= page_info[mat][3]:
                    raise ValueError(f"invalid material {mat} or CLUT {row} on {name!r}")
                if remapped_regions:
                    us, vs = zip(*(face[i:i+2] for i in (0, 4, 8, 10)))
                    for (page, clut), (left, right, top, bottom) in remapped_regions.items():
                        if page != mat:
                            continue
                        if row == clut:
                            valid = left <= min(us) <= max(us) <= right and top <= min(vs) <= max(vs) <= bottom
                        else:
                            valid = max(us) < left or min(us) > right or max(vs) < top or min(vs) > bottom
                        if not valid:
                            raise ValueError("texture region overlaps a different palette")
                # The PS1 blends textured pixels only when the CLUT entry
                # also has STP set. A blend-enabled cutout can be opaque.
                translucent = face[7] >> 7
                if translucent and not untextured and item_page is None:
                    translucent = any(color & 0x8000 for color in scene_textures[mat][2][row])
                flags = translucent | (color_keys[mat][row] << 1) | (untextured << 2)
                key = state, mat, row, flags
                group = groups[key]
                start = len(group)
                points = []
                for corner in (0, 1, 3, 2):  # PS1 quad strip to DS perimeter
                    idx = face[12+corner] - base
                    if not 0 <= idx < nverts:
                        raise ValueError("vertex outside LM mesh")
                    uv = (0, 4, 8, 10)[corner]
                    u, v = face[uv:uv+2]
                    if item_page is not None:
                        height = 64 if room == 26 else 32 if v_offset else 128
                        if not 0 <= u-u_offset < 64 or v >= height:
                            raise ValueError("common item UV outside atlas")
                    point = vertices[idx]
                    group.extend(struct.pack("<3hBB", *point, u-u_offset, v+v_offset))
                    if lit:
                        index = face[16+corner]
                        if index >= ref_count or model_data[refs+index] != face[12+corner]:
                            raise ValueError("LM normal refers to a different lighting vertex")
                        normals[key].extend(corner_normals[index])
                    points.append(point)
                if not state and item_page is None and not flags & 1:
                    music = room == 39
                    poster = name == b"NULL_NEA" and mat == 2 and (j == 0 or (j == 1 and 40 <= k <= 47))
                    if music and poster:
                        face = vertical_bounds(points, 2*vertex_scale)
                        if face:
                            music_decorations.append((*face, group, start))
                        continue
                    # Stairwell wall panels can be trapezoids along the slope.
                    if room == 12 or (music and name in (b"NULL1", b"W5_FAR")):
                        surface = vertical_bounds(points, 0)
                    else:
                        surface = vertical_rectangle(points)
                    if surface is not None:
                        plane, bounds = surface
                        if name == b"NULL_NEA":
                            # The tower plaques share a plane with several
                            # masonry triangles from SC1FSE1.
                            plaque = room == 23 and j == 0 and k in (32, 33)
                            decorations.append((plane, bounds, group, start, plaque))
                        else:
                            walls[plane].append(bounds)

    infos, buffers = struct.unpack_from("<II", data, 20)
    for i in range(data[9]):
        buf = buffers + 24*i
        instances, = struct.unpack_from("<I", data, buf+12)
        for j in range(data[buf]):
            model, *pose = struct.unpack_from("<I9h2x3i", data, instances+36*j)
            info = infos + 16*model
            if data[info]:
                raise ValueError("music room references an external PLM")
            add_model(bytes(data[info+4:info+12]).rstrip(b"\0"), pose[:9], pose[9:])

    # The PS1 ordering table layers wall decorations over their backing
    # faces. Posters in these rooms need more clearance at shallow angles.
    gap = (16 if room in (9, 10, 12, 16) else 2) * vertex_scale
    for (axis, depth, side), bounds, group, start, plaque in decorations:
        left, right, bottom, top = bounds
        # Some posters are authored one or two Q8 units off their wall.
        if room == 12:
            depths = (depth, depth-side*vertex_scale)
        elif room in (9, 10, 16):
            depths = tuple(depth + offset*vertex_scale for offset in (0, -1, 1, -2, 2))
        else:
            depths = (depth,)
        for wall in depths:
            if plaque or any(bounds != b and b[0] <= left and b[1] >= right and b[2] <= bottom and b[3] >= top
                             for b in walls[axis, wall, side]):
                for offset in range(start+2*axis, start+32, 8):
                    struct.pack_into("<h", group, offset, wall+side*gap)
                break

    # The music-room walls can still win depth tests with a two-Q8-unit
    # gap at oblique views. Clamp the posters to a larger clearance,
    # keeping shared corners together across their triangles.
    for (axis, depth, side), bounds, group, start in music_decorations:
        left, right, bottom, top = bounds
        for delta in range(-2*vertex_scale, 2*vertex_scale+1, vertex_scale):
            wall = depth+delta
            if not any(b[0] <= left and b[1] >= right and b[2] <= bottom and b[3] >= top
                       for b in walls[axis, wall, side]):
                continue
            minimum = side*wall + 12*vertex_scale
            for offset in range(start+2*axis, start+32, 8):
                coord, = struct.unpack_from("<h", group, offset)
                struct.pack_into("<h", group, offset, side*max(side*coord, minimum))

    if room == 7:
        # Map_WorldObjectsInit places both parts of the original phone.
        angle = int(-78.3*4096/360)*math.tau/4096
        c, s = round(math.cos(angle)*4096), round(math.sin(angle)*4096)
        x, y, z = (int(v*4096)>>4 for v in (18.974, -0.9, 143.515))
        for name in (b"PHONE3_H", b"PHONE2_H"):
            add_model(name, (c,0,s,0,4096,0,-s,0,c), (x,y,z-30720), 17)
        receiver, clut = receiver_texture(disc)
        page_info.append((len(pages), 64, len(palettes)//32, 1))
        pages.extend(receiver)
        palettes.extend(clut)

    if room == 26:
        tim = disc.read("ITEM/TIM00.TIM")
        image = 8+struct.unpack_from("<I", tim, 8)[0]
        if (struct.unpack_from("<III4H", tim) != (16,8,524,176,0,16,16) or
                struct.unpack_from("<I4H", tim, image) != (32780,960,0,64,256)):
            raise ValueError("unexpected lobby item texture")
        page = len(page_info)
        page_info.append((len(pages), 64, len(palettes)//32, 2))
        for tile, clut in ((0, 0), (192, 12)):
            for y in range(64):
                offset = image+12+128*y+tile//2
                pages.extend(tim[offset:offset+32])
            palette = tim[20+32*clut:52+32*clut]
            if 0 in struct.unpack("<16H", palette):
                raise ValueError("unexpected transparent lobby item color")
            palettes.extend(palette)
        color_keys.append([False]*2)
        chunk_x, chunk_z = struct.unpack_from("<ii", data, 0x54)
        identity = (4096,0,0,0,4096,0,0,0,4096)
        # Map_WorldObjectsInit places two fans on the same axis.
        for state, position_z in ((14, 142.9), (15, 145.25)):
            add_model(b"FAN_HIDE", identity, (5120,-410,(int(position_z*4096)>>4)-chunk_z), state)
        models.update(model_directory(disc.read("BG/BG_ITEM.PLM")))
        overlay = disc.read("VIN/MAP1_S02.BIN")
        # g_CommonWorldObjectPoses[3..4].
        for i, name in ((3, b"AIDKIT_N"), (4, b"BULLET_N")):
            x,y,z,rx,ry,rz = struct.unpack_from("<3i3h", overlay, 0x800e1eec-0x800c9578+20*i)
            if rx or rz:
                raise ValueError("unsupported lobby prop rotation")
            angle = ry*math.tau/4096
            c, s = round(math.cos(angle)*4096), round(math.sin(angle)*4096)
            add_model(name, (c,0,s,0,4096,0,-s,0,c), ((x>>4)-chunk_x,y>>4,(z>>4)-chunk_z),
                      9+i, item_page=page)
    if room == 9:
        page = len(page_info)
        pixels, colors, poses = infirmary_items(disc)
        page_info.append((len(pages), 64, len(palettes)//32, 4))
        pages.extend(pixels)
        palettes.extend(colors)
        color_keys.append([False]*4)
        plm = disc.read("BG/BG_ITEM.PLM")
        models.update(model_directory(plm))
        # Keep separate batches for the two props gated by pickup flags.
        for state, (name, (x,y,z), heading) in zip((0, 3, 4), poses, strict=True):
            angle = heading * math.tau/4096
            c, s = round(math.cos(angle)*4096), round(math.sin(angle)*4096)
            add_model(name, (c,0,s,0,4096,0,-s,0,c), ((x>>4)-10240, y>>4, (z>>4)-30720), state,
                      item_page=page)
    if room != 39:
        magic = b"SHM2" if layout is not None else {
            9: b"SHI3", 12: b"SHF4", 13: b"SHS2", 16: b"SHU1", 32: b"SHC1",
            33: b"SHH1", 34: b"SHL3", 35: b"SHQ2"
        }[room]
        extra = b""
        if room == 2:
            # MAP1_S00 places the two switch models and the green indicator.
            chunk_x, chunk_z = struct.unpack_from("<ii", data, 0x54)
            identity = (4096,0,0,0,4096,0,0,0,4096)
            for state, (name, x, y, z) in enumerate((
                    (b"REDX_HID", 20.4568, -0.8345, -17.97),
                    (b"R_HIDE_I", 19.5799, 0, -18.3),
                    (b"G_HIDE_I", 19.5799, 0, -18.3)), 9):
                position = ((int(x*4096)>>4)-chunk_x, int(y*4096)>>4, (int(z*4096)>>4)-chunk_z)
                add_model(name, identity, position, state)
            extra = b"".join(prompt_data(disc, n, first_floor=True, choices=n==29, map_bearings=True)
                             for n in (29, 30, 31))
        elif room == 20:
            extra = prompt_data(disc, 21, first_floor=True, choices=False, map_bearings=True)
        elif room == 22:
            extra = ladder_frames(disc)
        elif room == 24:
            extra = courtyard_look_frames(disc) + b"".join(
                prompt_data(disc, n, choices=False, map_bearings=True, subtitle=True) for n in (15, 16, 17)
            ) + courtyard_voices(disc)
        elif room == 29:
            # Map_WorldObjectsInit places the card with a 46.2-degree yaw.
            angle = int(46.2*4096/360)*math.tau/4096
            c, s = round(math.cos(angle)*4096), round(math.sin(angle)*4096)
            x, y, z = (int(v*4096)>>4 for v in (139.199, -0.8753, 99.3733))
            add_model(b"KEY_HIDE", (c,0,s,0,4096,0,-s,0,c), (x-30720,y,z-20480), 16)
            extra, pixels, colors = picture_card(disc)
            page_info.append((len(pages), 64, len(palettes)//32, 1))
            pages.extend(pixels)
            palettes.extend(colors)
        elif room == 19:
            # WorldObject_PlacementInit places BOLL_HID on the pile.
            x, y, z = (int(v*4096)>>4 for v in (100.124, -1.08, 101.166))
            add_model(b"BOLL_HID", (4096,0,0,0,4096,0,0,0,4096), (x-20480,y,z-20480), 18)
            extra, pixels, colors = rubber_ball(disc)
            page_info.append((len(pages), 64, len(palettes)//32, 1))
            pages.extend(pixels)
            palettes.extend(colors)
        elif room == 36:
            # MAP1_S03 places the valve separately from the roof geometry.
            x, y, z = (int(v*4096)>>4 for v in (-10.49, -0.7, -12.92))
            add_model(b"VALVE_HI", (4096,0,0,0,4096,0,0,0,4096), (x+10240,y,z+10240))
        elif room == 26:
            extra = lobby_pickups(disc) + b"".join(
                prompt_data(disc, n, choices=False, map_bearings=True, otherworld=True) for n in (48, 49)
            )
        elif room == 9:
            extra = medical_pickups(disc)
        elif room == 12:
            extra = prompt_data(disc, 13, first_floor=True, choices=False)
        elif room == 34:
            # These world objects share the original MAP1_S01 hand position.
            for state, name in enumerate((b"HAND_HID", b"HAND2_HI", b"PANELG_H"), 6):
                add_model(name, (4096,0,0,0,4096,0,0,0,4096), (5324,-256,4992), state)
            pickup, pixels, colors = gold_pickup(disc)
            page_info.append((len(pages), 64, len(palettes)//32, 1))
            pages.extend(pixels)
            palettes.extend(colors)
            smoke, colors = smoke_atlas(disc)
            page_info.append((len(pages), 32, len(palettes)//32, 2))
            pages.extend(smoke)
            palettes.extend(colors)
            extra = pour_frames(disc) + statue_sound(disc.read("SND/MEP101.VAB")) + pickup
        elif room == 35:
            # Map_WorldObjectsInit places ACID_HID at (-101.2, -1.3, 100.3).
            add_model(b"ACID_HID", (4096,0,0,0,4096,0,0,0,4096), (4812,-333,5196), 5)
            extra, pixels, colors = chemical_pickup(disc)
            page_info.append((len(pages), 64, len(palettes)//32, 1))
            pages.extend(pixels)
            palettes.extend(colors)
    else:
        magic = b"SHR5"
        # Map_WorldObjectsInit and Map_WorldObjectsUpdate, EventFlag_71 set.
        def local(x,y,z):
            return round((x+120)*256), round(y*256), round(z*256)
        identity = (4096,0,0,0,4096,0,0,0,4096)
        add_model(b"FUTA1_HI", identity, local(-98.2798,-0.58,19.93))
        add_model(b"SILVER_H", identity, local(-99,-2.68,23.73), 1)
        angle = math.radians(28.5)
        c, s = round(math.cos(angle)*4096), round(math.sin(angle)*4096)
        add_model(b"SILVER_H", (c,-s,0,0,0,4096,-s,-c,0), local(-98.8,0,22.8), 2)

        player, skin, cluts = player_data(disc)
        page_info.append((len(pages), 256, len(palettes)//32, len(cluts)//32))
        pages.extend(skin)
        palettes.extend(cluts)
        pickup, texture, clut = silver_pickup(disc)
        pages.extend(texture)
        palettes.extend(clut)
        extra = player + pickup
    # PS1 transparency uses a zero palette word. The DS can key only index 0,
    # so convert the texels and CLUTs together after all textured batches are known.
    for page, (name, size, cluts) in enumerate(scene_textures):
        row_keys = [{idx for idx, color in enumerate(clut) if color == 0} for clut in cluts]
        if room in (10, 11, 17, 25, 28) and name == "SU1FT1":
            # CLUT 4 has unused zero entries after index 10. Its tile and
            # CLUT 0 both use index 1 for transparency.
            offset, width, _, _ = page_info[page]
            for (_, mat, row, flags), quads in groups.items():
                if mat != page or row != 4 or flags & 4:
                    continue
                if any(not (32 <= u < 64 and 192 <= v < 224)
                       for u, v in zip(quads[6::8], quads[7::8], strict=True)):
                    raise ValueError("unexpected SU1FT1 transparent tile coordinates")
            for y in range(192, 224):
                start = offset+y*width//2+16
                if any((b & 15) > 10 or (b >> 4) > 10 for b in pages[start:start+16]):
                    raise ValueError("unexpected SU1FT1 transparent tile indices")
            if row_keys[4] != {1, 11, 12, 13, 14, 15}:
                raise ValueError("unexpected SU1FT1 tile CLUT")
            row_keys[4] = {1}
        indices = set().union(*row_keys)
        if len(indices) > 1:
            indices = set().union(*(row_keys[row] for _, mat, row, flags in groups
                                    if mat == page and not flags & 4))
            if len(indices) > 1:
                raise ValueError(f"incompatible transparent indices on {name}")
        key = next(iter(indices), 0)
        if key:
            offset, _, first, count = page_info[page]
            mask = key | key << 4
            pages[offset:offset+size] = bytes(byte ^ mask for byte in pages[offset:offset+size])
            palettes[first*32:(first+count)*32] = b"".join(
                struct.pack("<16H", *(clut[idx ^ key] for idx in range(16))) for clut in cluts
            )

    result = bytearray(struct.pack("<4sHHII", magic, len(page_info), len(groups), len(pages), len(palettes)))
    if layout is not None:
        chunk_x, chunk_z = struct.unpack_from("<ii", data, 0x54)
        result.extend(struct.pack("<BBHii", room, layout, 0, chunk_x+5120, chunk_z+5120))
    for page in page_info:
        result.extend(struct.pack("<IHHI", *page))
    result.extend(pages)
    result.extend(palettes)
    result.extend(collision_edges(data, room))
    result.extend(extra)
    batches = sorted(groups.items(), key=lambda group: (group[0][3] & 1, group[0]))
    if lit:
        # A position and normal can be shared even across texture seams.
        points = {}
        corners = []
        for key, quads in batches:
            batch = bytearray()
            for vertex, normal in zip(struct.iter_unpack("<6s2s", quads),
                                      struct.iter_unpack("<4s", normals[key]), strict=True):
                position, uv = vertex
                point = points.setdefault(position + normal[0], len(points))
                batch.extend(struct.pack("<H2s", point, uv))
            corners.append(batch)
        result.extend(struct.pack("<H", len(points)))
        result.extend(b"".join(points))
    else:
        corners = (quads for _, quads in batches)
    for (key, quads), batch in zip(batches, corners, strict=True):
        result.extend(struct.pack("<4BI", *key, len(quads)//32))
        result.extend(batch)
    return result

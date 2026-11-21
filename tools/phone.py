"""Prepare the USA 1.1 teachers' room phone assets for native playback."""
import struct

from pickup import prompt_data
from vab import convert_tones
from xa import stereo_xa


def receiver_texture(disc):
    tim = disc.read("ITEM/PHONE.TIM")
    if (struct.unpack_from("<III4H", tim) != (16, 8, 44, 0, 0, 16, 1) or
            struct.unpack_from("<I4H", tim, 52) != (1036, 0, 0, 16, 32)):
        raise ValueError("unexpected phone receiver texture")
    # Use the existing 64x64 room item slot.
    return tim[64:1088] + bytes(1024), tim[20:52]


def receiver_mesh(disc):
    model = disc.read("ITEM/PHONE.PLM")
    _, count, directory = struct.unpack_from("<III", model, 4)
    name, meshes, base, _, _, ptr = struct.unpack_from("<8s4BI", model, directory)
    faces, vertices, _, _, prim, xy, z, _, _ = struct.unpack_from("<4B5I", model, ptr)
    if (count, name.rstrip(b"\0"), meshes, base, faces, vertices) != (1, b"PHONE", 1, 0, 38, 44):
        raise ValueError("unexpected phone receiver mesh")
    output = bytearray()
    for i in range(faces):
        face = model[prim+20*i:prim+20*(i+1)]
        if face[7] or int.from_bytes(face[2:4], "little") >> 6:
            raise ValueError("unexpected phone receiver material")
        for corner in (0, 1, 3, 2):
            index = face[12+corner]
            tex = (0, 4, 8, 10)[corner]
            u, v = face[tex:tex+2]
            if index >= vertices or u >= 64 or v >= 32:
                raise ValueError("invalid phone receiver corner")
            output.extend(model[xy+4*index:xy+4*index+4])
            output.extend(model[z+2*index:z+2*index+2])
            output.extend((u, v))
    return output


def phone_data(disc):
    # MAP1_S02 Harry states 97 and 98 use clips 59 and 60.
    anim = disc.read("ANIM/HB_M1S02.ANM")
    motion = b"".join(anim[(first-676)*156:(last+1-676)*156]
                      for first, last in ((1062, 1110), (1119, 1147)))
    if len(motion) != 78*156:
        raise ValueError("incomplete Harry phone animations")
    pages = b"".join(prompt_data(disc, page, choices=False, map_bearings=True, subtitle=True)
                     for page in range(32, 37))
    # SD_Call(19) cuts the ring at the handset lift, about 10.4 seconds in.
    # Retain eleven seconds from each plane and all of the dialogue stream.
    ring = stereo_xa(disc.track, 0xa227, 554, 10, 0)
    side = len(ring)//2
    ring_bytes = 11*37800*2
    ring = ring[:ring_bytes] + ring[side:side+ring_bytes]
    voice = stereo_xa(disc.track, 0xa228, 462, 10, 1)
    sounds = bytearray()
    tones = convert_tones(disc.read("SND/MEP102.VAB"), ((1, 75, 57), (1, 76, 57)), sounds)
    return (struct.pack("<4sIII", b"SHPH", ring_bytes, len(voice)//2, len(sounds)) +
            motion + receiver_mesh(disc) + pages + tones + ring + voice + sounds)

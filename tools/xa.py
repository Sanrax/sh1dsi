"""Convert the courtyard's stereo XA dialogue to planar 16-bit PCM."""
from array import array
import struct
import sys


def stereo_xa(track, lba, sectors, file_number, channel):
    # The USA voice bank interleaves eight 37.8 kHz stereo streams.
    pcm = [array("h"), array("h")]
    history = [(0, 0), (0, 0)]
    filters = ((0, 0), (60, 0), (115, -52), (98, -55))
    for i in range(sectors):
        track.seek((lba + 8*i)*2352 + 16)
        data = track.read(8 + 18*128)
        submode = 0xe4 if i == sectors-1 else 0x64
        subheader = bytes((file_number, channel, submode, 1))
        if len(data) != 8+18*128 or data[:8] != subheader*2:
            raise ValueError("unexpected XA voice sector")
        for offset in range(8, len(data), 128):
            block = data[offset:offset+128]
            for unit in range(8):
                side = unit % 2
                old, older = history[side]
                parameter = block[4+unit]
                if parameter >> 4 >= len(filters) or (parameter & 15) > 12:
                    raise ValueError("invalid XA voice predictor")
                f0, f1 = filters[parameter >> 4]
                shift = 12 - (parameter & 15)
                for sample in range(28):
                    nibble = (block[16+sample*4+unit//2] >> (side*4)) & 15
                    value = (((nibble ^ 8)-8) << shift) + ((old*f0 + older*f1 + 32) >> 6)
                    value = max(-32768, min(32767, value))
                    pcm[side].append(value)
                    old, older = value, old
                history[side] = old, older
    if sys.byteorder != "little":
        for channel_pcm in pcm:
            channel_pcm.byteswap()
    return pcm[0].tobytes() + pcm[1].tobytes()


def courtyard_voices(disc):
    # MAP1_S02's commands 1053, 104F and 103D select these USA gSDXATable
    # ranges in g_FileXaLoc[2]. The sector counts include the XA EOF sectors.
    lines = ((2140, 19, 15, 4), (1898, 33, 13, 2), (599, 67, 11, 7))
    voices = [stereo_xa(disc.track, 0xa227+start, count, file, channel)
              for start, count, file, channel in lines]
    return struct.pack("<3I", *(len(voice)//2 for voice in voices)) + b"".join(voices)

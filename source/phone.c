// SPDX-License-Identifier: GPL-3.0-only
// MAP1_S02 event 14, timed against the original USA 1.1 room replay.
// Copyright (C) 2026 shdecompilations
#include <nds.h>
#include <stdlib.h>
#include <string.h>
#include "glyph.h"
#include "phone.h"

enum {
    BordersIn = 96, Approach = 162, WalkView = 285, Reach = 379,
    AnswerView = 414, DeskHidden = 421, Lift = 623, ReceiverShown = 666,
    Voice = 873, Conversation = 876, FirstLine = 936, SecondLine = 1122,
    ThirdLine = 1305, VoiceTextEnd = 1429, DeskView = 1431, Hang = 1538, Reply = 1603,
    ReplyEnd = 1744, FadeOut = 92, Blackout = 27, Recover = 66,
    SkipFade = 28, PhonePages = 5, ReceiverCorners = 38*4
};
enum { DeskX = 83294, DeskZ = 587893, DeskYaw = 3072 };
static const CameraPose walkView = {73973, -8151, 584785, 85811, 1105, 591339};
static const CameraPose deskView = {71720, -13025, 590028, 83066, -1720, 586629};

typedef struct {
    uint32_t offset, size;
    uint16_t rate;
    uint8_t volume, pan;
} Tone;

static struct {
    char magic[4];
    uint32_t voiceBytes[2], effectBytes;
} header;
static Tone effects[2];
static RoomVertex receiver[ReceiverCorners];
typedef struct {
    uint16_t count, prompt;
    uint16_t palette[16];
    Glyph glyphs[48];
} PhonePage;
static PhonePage pages[PhonePages];

static uint8_t *phoneSamples;
static int voiceChannels[2] = {-1, -1};
static int effectChannel = -1;
static int phoneAge = -1, skipAge, recoveryAge = -1, previousText = -1;
static unsigned shown, previousCount;
static bool phoneCompleted;
static Checkpoint launch;

bool phoneComplete(void) { return phoneCompleted; }
bool phoneActive(void) { return phoneAge >= 0; }

bool phoneLoad(const char *directory)
{
    char path[40];
    snprintf(path, sizeof path, "%s/phone.dat", directory);
    FILE *file = fopen(path, "rb");
    if (!file) return false;
    setvbuf(file, NULL, _IOFBF, 32*1024);
    bool valid = fread(&header, sizeof header, 1, file) == 1 &&
                 !memcmp(header.magic, "SHPH", 4) &&
                 header.voiceBytes[0] == 11*37800*2 && header.voiceBytes[1] == 462*4032 &&
                 header.effectBytes && header.effectBytes <= 32*1024 && !(header.effectBytes & 3) &&
                 playerLoadPhoneAnimation(file) &&
                 fread(receiver, sizeof receiver, 1, file) == 1;
    for (unsigned i = 0; valid && i < ReceiverCorners; i++)
        valid = receiver[i].u < 64 && receiver[i].v < 32 &&
                abs(receiver[i].x) <= 1024 && abs(receiver[i].y) <= 1024 && abs(receiver[i].z) <= 1024;
    for (unsigned i = 0; valid && i < PhonePages; i++) {
        const size_t bytes = offsetof(PhonePage, glyphs);
        valid = fread(&pages[i], bytes, 1, file) == 1 && pages[i].count &&
                pages[i].count == pages[i].prompt && pages[i].count <= 48 &&
                glyphsRead(file, pages[i].glyphs, pages[i].count);
    }
    valid = valid && fread(effects, sizeof effects, 1, file) == 1;
    for (unsigned i = 0; valid && i < 2; i++) {
        const Tone *tone = &effects[i];
        valid = tone->size && tone->rate && tone->volume <= 127 && tone->pan <= 127 &&
                !((tone->offset | tone->size) & 3) && tone->offset <= header.effectBytes &&
                tone->size <= header.effectBytes-tone->offset;
    }
    if (valid) {
        size_t bytes = 2u*(header.voiceBytes[0]+header.voiceBytes[1])+header.effectBytes;
        phoneSamples = malloc(bytes);
        valid = phoneSamples && fread(phoneSamples, 1, bytes, file) == bytes && fgetc(file) == EOF && !ferror(file);
        if (valid) DC_FlushRange(phoneSamples, bytes);
    }
    fclose(file);
    if (!valid) phoneUnload();
    return valid;
}

void phoneUnload(void)
{
    free(phoneSamples);
    phoneSamples = NULL;
}

static void stopVoice(void)
{
    for (unsigned i = 0; i < 2; i++) {
        if (voiceChannels[i] >= 0) soundKill(voiceChannels[i]);
        voiceChannels[i] = -1;
    }
}

static void playVoice(unsigned clip)
{
    stopVoice();
    const uint8_t *pcm = phoneSamples+(clip ? 2*header.voiceBytes[0] : 0);
    unsigned bytes = header.voiceBytes[clip];
    voiceChannels[0] = soundPlaySample(pcm, SoundFormat_16Bit, bytes, 37800, 84, 0, false, 0);
    voiceChannels[1] = soundPlaySample(pcm+bytes, SoundFormat_16Bit, bytes, 37800, 84, 127, false, 0);
}

static void playEffect(unsigned id)
{
    const Tone *tone = &effects[id];
    const uint8_t *pcm = phoneSamples+2*(header.voiceBytes[0]+header.voiceBytes[1])+tone->offset;
    effectChannel = soundPlaySample(pcm, SoundFormat_16Bit, tone->size, tone->rate,
                                    tone->volume, tone->pan, false, 0);
}

bool phoneStart(void)
{
    if (!phoneSamples || phoneCompleted || !playerFacingPhone()) return false;
    playerSavePose(&launch);
    phoneAge = skipAge = 0;
    shown = 0;
    recoveryAge = -1;
    playVoice(0);
    return true;
}

static int mix(int from, int to, int step, int duration)
{
    if (step <= 0) return from;
    if (step >= duration) return to;
    return from + (to-from)*step/duration;
}

static int turnToward(int from, int to, int step, int duration)
{
    int turn = ((to-from+2048) & 4095)-2048;
    return mix(from, from+turn, step, duration) & 4095;
}

static bool phoneApproach(void)
{
    if (phoneAge < Approach) return playerTick(0, 0, PlayerMove);
    if (phoneAge < 222) {
        int turn = ((2648-launch.yaw+2048) & 4095)-2048;
        playerScriptedPose(launch.x, launch.z, turnToward(launch.yaw, 2648, phoneAge-Approach, 55));
        return playerTick(0, 0, turn < 0 ? PlayerScriptedTurnLeft : PlayerScriptedTurnRight);
    }
    if (phoneAge < 360) {
        // Follow the original acceleration, camera cut and desk approach.
        static const struct { int at, x, z; } path[] = {
            {222, 93389, 595558}, {240, 92277, 594722},
            {283, 89019, 592238}, {320, 86289, 590162}, {360, DeskX, DeskZ}
        };
        unsigned i = 1;
        while (phoneAge > path[i].at) i++;
        int t = phoneAge-path[i-1].at, span = path[i].at-path[i-1].at;
        int x = mix(path[i-1].x, path[i].x, t, span);
        int z = mix(path[i-1].z, path[i].z, t, span);
        x += mix(launch.x-path[0].x, 0, phoneAge-222, 61);
        z += mix(launch.z-path[0].z, 0, phoneAge-222, 61);
        playerScriptedPose(x, z, 2648);
        return playerTick(0, 0, PlayerScriptedWalk);
    }
    playerScriptedPose(DeskX, DeskZ, turnToward(2648, DeskYaw, phoneAge-360, 12));
    return playerTick(0, 0, phoneAge < 372 ? PlayerScriptedTurnRight : PlayerMove);
}

static void finish(bool skip)
{
    stopVoice();
    if (effectChannel >= 0) soundKill(effectChannel);
    effectChannel = -1;
    playerFinishPhone(skip);
    phoneCompleted = true;
    phoneAge = -1;
    setBrightness(1, -16);
}

bool phoneTick(unsigned keys)
{
    if (!skipAge && (keys & KEY_START) && phoneAge >= 4 && recoveryAge < FadeOut+Blackout+Recover-3)
        skipAge = 1;
    if (skipAge) {
        setBrightness(1, -16*skipAge/SkipFade);
        if (++skipAge > SkipFade) finish(true);
        return true;
    }

    ++phoneAge;
    if (phoneAge == WalkView) lightingEnableFlashlight();
    if (phoneAge == Lift) {
        stopVoice();
        playEffect(0);
    }
    if (phoneAge == Voice) {
        // The lift effect has ended before the dialogue reuses the channel.
        effectChannel = -1;
        playVoice(1);
    }
    if (phoneAge < Reach) return phoneApproach();

    if (phoneAge >= Hang) {
        unsigned motionAge = phoneAge < Reply ? phoneAge-Hang :
                             Reply-Hang+(recoveryAge < 0 ? 0 : recoveryAge);
        unsigned blend = motionAge < 6 ? (6-motionAge)*4096/6 : 0;
        unsigned frame = (motionAge > 6 ? motionAge-6 : 0)*4096/6;
        unsigned last = 28*4096;
        playerPhoneFrame(true, frame < last ? frame : last, blend);
    } else {
        unsigned motionAge = phoneAge-Reach;
        unsigned blend = motionAge < 6 ? (6-motionAge)*4096/6 : 0;
        unsigned frame = (motionAge > 6 ? motionAge-6 : 0)*4096/10;
        unsigned last = 48*4096;
        playerPhoneFrame(false, frame < last ? frame : last, blend);
    }

    if (phoneAge >= Reply && recoveryAge < 0) {
        if (phoneAge == ReplyEnd) shown = 0;
        unsigned count = pages[phoneAge < ReplyEnd ? 3 : 4].count;
        if (!(phoneAge & 1) && shown < count) shown += count-shown < 2 ? count-shown : 2;
        if (keys & (KEY_A | KEY_B)) {
            if (shown < count) shown = count;
            else if (phoneAge < ReplyEnd) {
                phoneAge = ReplyEnd;
                shown = 0;
            } else {
                recoveryAge = 0;
            }
        }
        if (phoneAge >= ReplyEnd+122) recoveryAge = 0;
    } else if (recoveryAge >= 0) {
        ++recoveryAge;
        if (recoveryAge == FadeOut+Blackout) {
            stopVoice();
            playEffect(1);
        }
        if (recoveryAge >= FadeOut+Blackout+Recover) {
            finish(false);
            return true;
        }
        setBrightness(1, -mix(0, 16, recoveryAge-FadeOut, Blackout));
    }
    // Camera, bones and handheld model update at the native scene cadence.
    return !(phoneAge & 1);
}

int phoneBorder(void)
{
    if (!phoneActive()) return 6;
    if (recoveryAge >= FadeOut+Blackout)
        return mix(19, 6, recoveryAge-FadeOut-Blackout, Recover);
    return mix(6, 19, phoneAge-BordersIn, Approach-BordersIn);
}

int phoneProjection(void)
{
    if (recoveryAge >= FadeOut+Blackout+60) return 224;
    return mix(224, 376, phoneAge-BordersIn, Approach-BordersIn);
}

bool phoneView(CameraPose *view)
{
    if (!phoneActive() || phoneAge < Approach) return false;
    if (phoneAge >= DeskView) {
        *view = deskView;
    } else if (phoneAge >= AnswerView) {
        int shift = mix(0, -11468, phoneAge-Conversation, 390);
        *view = (CameraPose){DeskX+16384+shift, -6144, DeskZ, DeskX+shift, -4096, DeskZ};
    } else if (phoneAge >= WalkView) {
        *view = walkView;
    } else {
        static const CameraPose start = {86002, -8601, 592896, 105512, -4864, 598458};
        static const CameraPose end = {76062, -8601, 585538, 87357, -4864, 590645};
        int t = phoneAge-Approach, n = WalkView-Approach;
        *view = (CameraPose){mix(start.x, end.x, t, n), start.y, mix(start.z, end.z, t, n),
                             mix(start.watchX, end.watchX, t, n), start.watchY,
                             mix(start.watchZ, end.watchZ, t, n)};
    }
    return true;
}

bool phoneDeskVisible(void)
{
    return phoneAge < DeskHidden || recoveryAge >= FadeOut+Blackout;
}

bool phoneHolding(void)
{
    return phoneAge >= ReceiverShown && recoveryAge < FadeOut+Blackout;
}

void phoneDrawText(int bg)
{
    int page = -1;
    unsigned count = 0;
    if (!skipAge && phoneAge >= FirstLine && phoneAge < VoiceTextEnd) {
        page = phoneAge < SecondLine ? 0 : phoneAge < ThirdLine ? 1 : 2;
        static const unsigned lineStart[] = {FirstLine, SecondLine, ThirdLine};
        count = page == 2 ? pages[page].count : ((phoneAge-lineStart[page])/2)*2;
        if (count > pages[page].count) count = pages[page].count;
    } else if (!skipAge && recoveryAge < 0 && phoneAge >= Reply) {
        page = phoneAge < ReplyEnd ? 3 : 4;
        count = shown;
    }
    if (page == previousText && count == previousCount) return;
    previousText = page;
    previousCount = count;
    if (page < 0) {
        bgHide(bg);
        return;
    }
    uint16_t *pixels = bgGetGfxPtr(bg);
    dmaFillHalfWords(0, pixels, 256*192);
    for (unsigned i = 1; i < 16; i++) BG_PALETTE[i] = pages[page].palette[i] & 0x7fff;
    glyphsDraw(pixels, pages[page].glyphs, count);
    bgShow(bg);
}

void phoneDrawReceiver(unsigned texture, unsigned palette, unsigned fog)
{
    GFX_TEX_FORMAT = texture;
    GFX_PAL_FORMAT = palette*2;
    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_FRONT | fog);
    glBegin(GL_QUADS);
    for (unsigned i = 0; i < ReceiverCorners; i++) {
        const RoomVertex *v = &receiver[i];
        glTexCoord2t16(v->u << 4, v->v << 4);
        glVertex3v16(v->x*8, -v->y*8, -v->z*8);
    }
    glEnd();
}

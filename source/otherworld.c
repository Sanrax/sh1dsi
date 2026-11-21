// MAP1_S02 courtyard event, timing checked against the NTSC-U intro.
#include <nds.h>
#include "glyph.h"
#include "otherworld.h"

enum { DescendRamp = 785, StopWalking = 1139, IntroEnd = 1611 };
static const struct {
    int begin, end;
} subtitles[] = {{226, 299}, {664, DescendRamp}, {1315, 1543}};
static int introAge = -1;
static int displayed = -1;
static uint16_t palette[16];
static struct {
    unsigned count;
    Glyph glyphs[48];
    uint32_t channelBytes;
    int16_t *pcm;
} dialogue[3];
static int channels[2] = {-1, -1};
static int16_t *voiceSamples;

bool otherworldLoadIntro(FILE *file)
{
    if (!playerLoadCourtyardLook(file)) return false;
    for (unsigned i = 0; i < 3; i++) {
        uint16_t counts[2];
        if (fread(counts, sizeof counts, 1, file) != 1 || !counts[0] ||
            counts[0] > 48 || counts[0] != counts[1] ||
            fread(palette, sizeof palette, 1, file) != 1 ||
            !glyphsRead(file, dialogue[i].glyphs, counts[0])) return false;
        dialogue[i].count = counts[0];
    }
    uint32_t lengths[3];
    if (fread(lengths, sizeof lengths, 1, file) != 1) return false;
    static const unsigned sectors[] = {19, 33, 67};
    unsigned bytes = 0;
    for (unsigned i = 0; i < 3; i++) {
        if (lengths[i] != sectors[i]*4032u) return false;
        bytes += 2*lengths[i];
    }
    int16_t *samples = malloc(bytes);
    if (!samples) return false;
    if (fread(samples, 1, bytes, file) != bytes) {
        free(samples);
        return false;
    }
    DC_FlushRange(samples, bytes);
    voiceSamples = samples;
    for (unsigned i = 0; i < 3; i++) {
        dialogue[i].channelBytes = lengths[i];
        dialogue[i].pcm = samples;
        samples += lengths[i];
    }
    return true;
}

void otherworldStartIntro(void)
{
    introAge = 0;
    displayed = -1;
}

bool otherworldIntroActive(void)
{
    return introAge >= 0;
}

static void stopVoices(void)
{
    for (unsigned i = 0; i < 2; i++) {
        if (channels[i] >= 0) soundKill(channels[i]);
        channels[i] = -1;
    }
}

static int lerp(int from, int to, int time, int duration)
{
    if (time >= duration) return to;
    return from + (to-from)*time/duration;
}

bool otherworldTickIntro(bool skip)
{
    if (++introAge >= IntroEnd || (skip && introAge > 7 && introAge < 1543)) {
        stopVoices();
        free(voiceSamples);
        voiceSamples = NULL;
        playerFinishOtherworld(skip && introAge < IntroEnd);
        introAge = -1;
        setBrightness(1, 0);
        return true;
    }
    if (introAge <= 45) setBrightness(1, -16*(45-introAge)/45);
    for (unsigned i = 0; i < 3; i++) {
        if (introAge != subtitles[i].begin) continue;
        stopVoices();
        unsigned size = dialogue[i].channelBytes;
        channels[0] = soundPlaySample(dialogue[i].pcm, SoundFormat_16Bit, size, 37800, 84, 0, false, 0);
        channels[1] = soundPlaySample(dialogue[i].pcm+size/2, SoundFormat_16Bit, size, 37800, 84, 127, false, 0);
    }
    PlayerAction action = PlayerMove;
    if (introAge >= DescendRamp && introAge < StopWalking) {
        // State 53 walks toward the authored (-18.18, -62.24) waypoint.
        // Sample the acceleration, the ramp exit, and the steady walk.
        static const struct { int time, x, z; } path[] = {
            {DescendRamp, -54556, -278859}, {795, -54635, -278766},
            {810, -55420, -277829}, {855, -57663, -275144},
            {StopWalking, -74458, -254955}
        };
        unsigned i = 1;
        while (introAge > path[i].time) i++;
        int t = introAge-path[i-1].time, span = path[i].time-path[i-1].time;
        int x = lerp(path[i-1].x, path[i].x, t, span);
        int z = lerp(path[i-1].z, path[i].z, t, span);
        int yaw = introAge >= StopWalking-8 ? lerp(3644, 3584, introAge-StopWalking+8, 8) :
                                             lerp(3584, 3644, introAge-DescendRamp, 10);
        playerScriptedPose(x, z, yaw);
        action = PlayerScriptedWalk;
    } else if (introAge >= 299 && introAge < DescendRamp) {
        action = PlayerLookAround;
    } else if (introAge == StopWalking) {
        playerScriptedPose(-74458, -254955, 3584);
    }
    return playerTick(0, 0, action);
}

CameraPose otherworldIntroView(void)
{
    // MAP1_S02 opens at a fixed view. The scripted pan starts when
    // Harry walks off the ramp, then moves toward the central courtyard.
    static const struct { int time; CameraPose view; } views[] = {
        {DescendRamp, {-81756, -5734, -251699, -70205, -6103, -263249}},
        {795, {-81958, -5868, -251343, -53657, -4915, -279756}},
        {960, {-86084, -8679, -243978, -53657, -4915, -279756}},
        {975, {-86163, -8934, -243340, -80158, 0, -250716}},
        {1050, {-84171, -10210, -240073, -80158, 0, -250716}},
        {1500, {-73093, -17875, -221920, -80158, 0, -250716}},
        {IntroEnd, {-73084, -19727, -221905, -80158, 0, -250716}}
    };
    if (introAge <= DescendRamp) return views[0].view;
    unsigned i = 1;
    while (introAge > views[i].time) i++;
    int t = introAge-views[i-1].time, span = views[i].time-views[i-1].time;
    CameraPose a = views[i-1].view, b = views[i].view;
    return (CameraPose){lerp(a.x, b.x, t, span), lerp(a.y, b.y, t, span), lerp(a.z, b.z, t, span),
                        lerp(a.watchX, b.watchX, t, span), lerp(a.watchY, b.watchY, t, span),
                        lerp(a.watchZ, b.watchZ, t, span)};
}

int otherworldIntroProjection(void)
{
    return lerp(224, 376, introAge, 71);
}

void otherworldDrawSubtitle(int bg)
{
    int line = -1;
    for (unsigned i = 0; i < 3; i++)
        if (introAge >= subtitles[i].begin && introAge < subtitles[i].end) line = i;
    if (line == displayed) return;
    displayed = line;
    if (line < 0) {
        bgHide(bg);
        return;
    }
    uint16_t *pixels = bgGetGfxPtr(bg);
    dmaFillHalfWords(0, pixels, 256*192);
    glyphsDraw(pixels, dialogue[line].glyphs, dialogue[line].count);
    for (unsigned i = 1; i < 16; i++) BG_PALETTE[i] = palette[i] & 0x7fff;
    bgShow(bg);
}

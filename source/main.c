#include <fat.h>
#include <nds.h>
#include <stdio.h>
#include <string.h>
#include "piano.h"
#include "room.h"

typedef struct {
    uint16_t palette[256];
    uint8_t pixels[256 * 192];
} Background;

static struct {
    char magic[4];
    PianoKeyboard keys;
    uint16_t cursor[16 * 16];
    Background images[2];
} assets;

_Static_assert(sizeof assets == 99860, "piano file layout");

typedef struct {
    uint32_t offset, bytes;
    uint16_t rate;
    uint8_t volume, pan;
} Tone;

static Tone tones[9];
static uint8_t samples[64 * 1024] __attribute__((aligned(32)));
static bool hasAudio;
static const char *roomPath;

static bool loadTones(FILE *file)
{
    if (fread(tones, sizeof tones, 1, file) != 1)
        return false;
    size_t size = fread(samples, 1, sizeof samples, file);
    for (unsigned i = 0; i < 9; i++) {
        const Tone *tone = &tones[i];
        if (!tone->rate || !tone->bytes || ((tone->offset | tone->bytes) & 3) ||
            tone->offset > size || tone->bytes > size - tone->offset ||
            tone->volume > 127 || tone->pan > 127)
            return false;
    }
    DC_FlushRange(samples, size);
    return true;
}

static bool loadPiano(void)
{
    // DSi SD and DLDI can mount independently. Try the launcher's drive first.
    bool mounted = fatInitDefault();
    bool cart = strcmp(fatGetDefaultDrive(), "fat:/") == 0;
    const char *sd = "sd:/sh1dsi/piano.dat", *fat = "fat:/sh1dsi/piano.dat";
    FILE *file = fopen(cart ? fat : sd, "rb");
    if (!file && (mounted || !cart)) {
        cart = !cart;
        file = fopen(cart ? fat : sd, "rb");
    }
    roomPath = cart ? "fat:/sh1dsi/room.dat" : "sd:/sh1dsi/room.dat";
    if (!file)
        return false;
    bool valid = fread(&assets, 1, sizeof assets, file) == sizeof assets
                 && (memcmp(assets.magic, "SHP1", 4) == 0 ||
                     memcmp(assets.magic, "SHP2", 4) == 0);
    if (valid && assets.magic[3] == '2') {
        hasAudio = loadTones(file);
        valid = hasAudio;
    }
    valid = valid && fgetc(file) == EOF && !ferror(file);
    fclose(file);
    return valid;
}

static void stop(const char *message)
{
    PrintConsole console;
    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    consoleInit(&console, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
    puts(message);
    for (;;) swiWaitForVBlank();
}

int main(void)
{
    lcdMainOnTop();
    videoSetModeSub(MODE_0_2D);
    setBrightness(2, -16);
    if (!isDSiMode())
        stop("DSi mode required");
    if (!loadPiano())
        stop("Silent Hill\nMissing or invalid game data.\nInstall /sh1dsi/piano.dat");

    videoSetMode(MODE_5_3D);
    glInit();
    FILE *roomFile = fopen(roomPath, "rb");
    bool hasRoom = roomFile != NULL;
    if (roomFile) {
        bool ok = roomLoad(roomFile);
        fclose(roomFile);
        if (!ok) stop("Invalid music room data");
    }
    bgHide(0);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankB(VRAM_B_MAIN_SPRITE);
    int bg = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
    u16 *sprite = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_Bmp);
    if (!sprite)
        stop("Cursor allocation failed");
    memcpy(sprite, assets.cursor, sizeof assets.cursor);
    oamSet(&oamMain, 0, 0, 0, 0, 15, SpriteSize_16x16, SpriteColorFormat_Bmp,
           sprite, -1, false, true, false, false, false);

    if (hasAudio) soundEnable();
    Piano piano = {0};
    bool inRoom = hasRoom;
    int previousImage = -1;
    systemCounterSetup();
    uint32_t previousTime = systemCounterGetTicks(), debt = 0, pressed = 0;
    // MAP1_S01 keyboard updates once per NTSC-U VBlank in the PS1 reference.
    const unsigned stepUs = 16683;
    const uint32_t step = systemCounterUsecsToTicks(stepUs);
    for (;;) {
        swiWaitForVBlank();
        scanKeys();
        pressed |= keysDown();
        unsigned held = keysHeld();
        uint32_t now = systemCounterGetTicks();
        uint32_t elapsed = now - previousTime;
        previousTime = now;
        debt += elapsed > 4 * step ? 4 * step : elapsed;
        while (debt >= step) {
            int sound = -1;
            if (inRoom) {
                if ((pressed & KEY_A) && !piano.solved)
                    inRoom = false;
            } else if (!piano.checking && (pressed & KEY_B) && hasRoom) {
                inRoom = true;
            } else {
                sound = pianoUpdate(&piano, &assets.keys,
                        !!(held & KEY_RIGHT) - !!(held & KEY_LEFT),
                        !!(held & KEY_DOWN) - !!(held & KEY_UP),
                        pressed & KEY_A, pressed & KEY_B, stepUs);
                if (piano.solved && hasRoom) inRoom = true;
            }
            if (hasAudio && sound >= 0) {
                const Tone *tone = &tones[sound];
                soundPlaySample(samples + tone->offset, SoundFormat_16Bit, tone->bytes,
                                tone->rate, tone->volume, tone->pan, false, 0);
            }
            pressed = 0;
            debt -= step;
        }

        int imageIndex = inRoom ? 2 : piano.checking;
        if (inRoom && previousImage != imageIndex) {
            bgHide(bg);
            roomDraw(piano.solved);
            bgShow(0);
        } else if (!inRoom && previousImage != imageIndex) {
            bgHide(0);
            bgShow(bg);
            const Background *image = &assets.images[piano.checking];
            memcpy(bgGetGfxPtr(bg), image->pixels, sizeof image->pixels);
            memcpy(BG_PALETTE, image->palette, sizeof image->palette);
        }
        previousImage = imageIndex;
        oamSetXY(&oamMain, 0, ((piano.x >> 12) + 160) * 4 / 5,
                              ((piano.y >> 12) + 120) * 4 / 5);
        oamSetHidden(&oamMain, 0, !piano.checking || piano.waitUs != 0);
        oamUpdate(&oamMain);
    }
}

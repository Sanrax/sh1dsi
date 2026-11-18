#include <fat.h>
#include <nds.h>
#include <stdio.h>
#include <string.h>
#include "piano.h"

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

static bool loadPiano(void)
{
    // DSi SD and DLDI can mount independently. Try the launcher's drive first.
    bool mounted = fatInitDefault();
    bool cart = strcmp(fatGetDefaultDrive(), "fat:/") == 0;
    const char *sd = "sd:/sh1dsi/piano.dat", *fat = "fat:/sh1dsi/piano.dat";
    FILE *file = fopen(cart ? fat : sd, "rb");
    if (!file && (mounted || !cart))
        file = fopen(cart ? sd : fat, "rb");
    if (!file)
        return false;
    bool valid = fread(&assets, 1, sizeof assets, file) == sizeof assets
                 && memcmp(assets.magic, "SHP1", 4) == 0 && fgetc(file) == EOF
                 && !ferror(file);
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

    videoSetMode(MODE_5_2D);
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

    Piano piano = {0};
    int previousImage = -1;
    systemCounterSetup();
    uint32_t previousTime = systemCounterGetTicks(), debt = 0, pressed = 0;
    // Provisional event cadence until the same PS1 event is measured.
    const uint32_t step = systemCounterUsecsToTicks(16667);
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
            pianoUpdate(&piano, &assets.keys,
                        !!(held & KEY_RIGHT) - !!(held & KEY_LEFT),
                        !!(held & KEY_DOWN) - !!(held & KEY_UP),
                        pressed & KEY_A, pressed & KEY_B, 16667);
            pressed = 0;
            debt -= step;
        }

        if (previousImage != (int)piano.checking) {
            const Background *image = &assets.images[piano.checking];
            memcpy(bgGetGfxPtr(bg), image->pixels, sizeof image->pixels);
            memcpy(BG_PALETTE, image->palette, sizeof image->palette);
            previousImage = piano.checking;
        }
        oamSetXY(&oamMain, 0, ((piano.x >> 12) + 160) * 4 / 5,
                              ((piano.y >> 12) + 120) * 4 / 5);
        oamSetHidden(&oamMain, 0, !piano.checking || piano.waitUs != 0);
        oamUpdate(&oamMain);
    }
}

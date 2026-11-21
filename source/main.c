#include <fat.h>
#include <nds.h>
#include <stdio.h>
#include <string.h>
#include "piano.h"
#include "room.h"
#include "player.h"
#include "pickup.h"
#include "notebook.h"
#include "statue.h"

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

enum { DoorOpen = 10, DoorClose, HallOpen, HallClose, DoorUnlock,
       PromptMove, PromptCancel, PromptConfirm, ToneCount };
static const int pickupSounds[] = {
    [PickupMove] = PromptMove, [PickupCancel] = PromptCancel, [PickupConfirm] = PromptConfirm
};
static Tone tones[ToneCount];
static uint8_t samples[144 * 1024] __attribute__((aligned(32)));
static unsigned toneCount;
static const char *assetDir;

static FILE *openRoom(SchoolRoom room)
{
    const char *name;
    switch (room) {
    case MusicRoom: name = "room"; break;
    case SchoolCorridor: name = "corridor"; break;
    case EastCorridor: name = "east"; break;
    case FirstFloor: name = "firstfloor"; break;
    case SouthHall: name = "south"; break;
    case NorthHall: name = "north"; break;
    case UpperNorthHall: name = "uppernorth"; break;
    case ChemistryLab: name = "laboratory"; break;
    case EquipmentRoom: name = "equipment"; break;
    case Infirmary: name = "infirmary"; break;
    case Courtyard: name = "courtyard"; break;
    case BasementStair: name = "basement"; break;
    case BoilerRoom: name = "boiler"; break;
    default: return NULL;
    }
    char path[40];
    snprintf(path, sizeof path, "%s/%s.dat", assetDir, name);
    FILE *file = fopen(path, "rb");
    if (file) setvbuf(file, NULL, _IOFBF, 16*1024);
    return file;
}

static bool loadTones(FILE *file, unsigned count)
{
    if (fread(tones, sizeof(Tone), count, file) != count)
        return false;
    size_t size = fread(samples, 1, sizeof samples, file);
    for (unsigned i = 0; i < count; i++) {
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
    assetDir = cart ? "fat:/sh1dsi" : "sd:/sh1dsi";
    if (!file)
        return false;
    bool valid = fread(&assets, 1, sizeof assets, file) == sizeof assets
                 && memcmp(assets.magic, "SHP", 3) == 0 &&
                 assets.magic[3] >= '1' && assets.magic[3] <= '7';
    if (valid && assets.magic[3] != '1') {
        static const unsigned counts[] = {0, 9, 10, 12, 14, 15, ToneCount};
        toneCount = counts[assets.magic[3]-'1'];
        valid = loadTones(file, toneCount);
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

static void playTone(int id)
{
    if (id < 0 || (unsigned)id >= toneCount) return;
    const Tone *tone = &tones[id];
    soundPlaySample(samples + tone->offset, SoundFormat_16Bit, tone->bytes,
                    tone->rate, tone->volume, tone->pan, false, 0);
}

static void changeRoom(RoomExit exit)
{
    SchoolRoom from = roomCurrent(), next = exit.room;
    bool east = next == EastCorridor || from == EastCorridor;
    bool hall = east || from == Courtyard || next == Courtyard ||
                (from == FirstFloor && next == SouthHall) || (from == SouthHall && next == FirstFloor);
    FILE *file = openRoom(next);
    if (!file) stop("Missing room data.\nRun the asset converter.");
    if (!exit.stairs) playTone(hall ? HallOpen : DoorOpen);
    uint32_t start = systemCounterGetTicks();
    // Let the last geometry submission retire before remapping texture VRAM.
    setBrightness(1, -16);
    bgHide(0);
    swiWaitForVBlank();
    bool ok = roomLoad(file, next);
    fclose(file);
    if (!ok) {
        setBrightness(1, 0);
        stop("Invalid school room data.");
    }
    playerEnterRoom(from, exit.stairs);
    // GameBoot_InGameStartup step 11 keeps the load screen for at least
    // one second, including time already spent on disc or card reads.
    while ((uint32_t)(systemCounterGetTicks()-start) < systemCounterUsecsToTicks(1000000))
        swiWaitForVBlank();
    if (!exit.stairs) playTone(hall ? HallClose : DoorClose);
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
    FILE *roomFile = openRoom(MusicRoom);
    bool hasRoom = roomFile != NULL;
    bgHide(0);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankB(VRAM_B_MAIN_SPRITE);
    int bg = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
    bgSetPriority(0, 1);
    bgSetPriority(bg, 0);
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);
    u16 *sprite = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_Bmp);
    if (!sprite)
        stop("Cursor allocation failed");
    memcpy(sprite, assets.cursor, sizeof assets.cursor);
    oamSet(&oamMain, 0, 0, 0, 0, 15, SpriteSize_16x16, SpriteColorFormat_Bmp,
           sprite, -1, false, true, false, false, false);

    if (toneCount) soundEnable();
    Piano piano = {0};
    Medallion medal = {0};
    Checkpoint point = {0};
    NotebookResult boot = hasRoom ? notebookMenu(assetDir, true, &point) : NotebookClosed;
    if (boot == NotebookMissingArt) {
        fclose(roomFile);
        stop("Missing notebook.dat");
    }
    if (roomFile) {
        bool ok = boot == NotebookLoaded ? roomLoadShared(roomFile) : roomLoad(roomFile, MusicRoom);
        fclose(roomFile);
        if (!ok) stop("Invalid music room data");
    }
    if (boot == NotebookLoaded) {
        FILE *savedRoom = openRoom(Infirmary);
        if (!savedRoom) stop("Missing infirmary.dat");
        bool ok = roomLoad(savedRoom, Infirmary);
        fclose(savedRoom);
        if (!ok) stop("Invalid infirmary data");
        playerRestorePose(&point);
        piano.solved = (point.flags & PianoSolved) != 0;
        medal.phase = piano.solved ? MedalFinished : MedalHanging;
        pickupRestore(point.flags);
        statueRestore(point.flags);
    }
    bool inRoom = hasRoom;
    int previousImage = -1;
    bool enteringRoom = false;
    unsigned inspectionFade = 0, inspectionFadeLength = 0;
    systemCounterSetup();
    uint32_t previousTime = systemCounterGetTicks(), debt = 0, pressed = 0;
    // MAP1_S01 keyboard updates once per NTSC-U VBlank in the PS1 reference.
    const unsigned stepUs = 16683;
    const uint32_t step = systemCounterUsecsToTicks(stepUs);
    for (;;) {
        swiWaitForVBlank();
        if (enteringRoom) {
            setBrightness(1, 0);
            enteringRoom = false;
        } else if (inspectionFade) {
            setBrightness(1, -(int)(16*--inspectionFade/inspectionFadeLength));
        }
        scanKeys();
        pressed |= keysDown();
        unsigned held = keysHeld();
        uint32_t now = systemCounterGetTicks();
        uint32_t elapsed = now - previousTime;
        previousTime = now;
        debt += elapsed > 4 * step ? 4 * step : elapsed;
        bool roomChanged = false;
        while (debt >= step) {
            int sound = -1;
            if (point.ticks != UINT32_MAX) point.ticks++;
            if (inRoom) {
                MedalPhase phase = medal.phase;
                roomChanged |= medallionTick(&medal, pressed & KEY_START);
                int turn = 0, walk = 0;
                bool wasActive = pickupActive() || statueActive();
                if (!wasActive && (medal.phase == MedalHanging || medal.phase == MedalFinished)) {
                    turn = !!(held & KEY_RIGHT) - !!(held & KEY_LEFT);
                    walk = !!(held & KEY_UP) - !!(held & KEY_DOWN);
                }
                if (statueTick(pressed)) {
                    inspectionFadeLength = inspectionFade = StatueRevealVBlanks;
                    roomChanged = true;
                }
                roomChanged |= playerTick(turn, walk, statueCamera() ? PlayerPour : pickupAction());
                PickupSound pickupSound = pickupTick(pressed);
                if (pickupSound != PickupQuiet) sound = pickupSounds[pickupSound];
                if (phase == MedalDrop && medal.phase == MedalImpact)
                    sound = 9;
                if (!wasActive && phase == medal.phase && (phase == MedalHanging || phase == MedalFinished)) {
                    RoomExit exit = playerDestination(pressed & KEY_A);
                    SchoolRoom next = exit.room;
                    if (next == SouthHall && roomCurrent() == FirstFloor && !latchUnlocked()) {
                        if (latchStart()) sound = DoorUnlock;
                    } else if (next != roomCurrent()) {
                        inspectionFade = 0;
                        changeRoom(exit);
                        enteringRoom = roomChanged = true;
                        previousTime = systemCounterGetTicks();
                        debt = step;
                    } else if ((pressed & KEY_SELECT) && statueStart()) {
                        inspectionFade = 0;
                        setBrightness(1, 0);
                        roomChanged = true;
                    } else if ((pressed & KEY_A) && playerFacingNotebook()) {
                        playerSavePose(&point);
                        point.flags = (point.flags & NotebookRead) | (piano.solved ? PianoSolved : 0) |
                                      pickupFlags() | statueFlags() | (latchUnlocked() ? LatchOpen : 0);
                        if (notebookMenu(assetDir, false, &point) == NotebookMissingArt)
                            stop("Missing notebook.dat");
                        roomChanged = true;
                        previousTime = systemCounterGetTicks();
                        debt = step;
                    } else if (pressed & KEY_A) {
                        TowerPoint tower = playerFacingTower();
                        bool hand = playerFacingHand() && !(roomCanDissolveHand() && statueOpened());
                        if (tower != NoTowerPoint || hand) {
                            inspectionFadeLength = inspectionFade = hand ? handInspect(assetDir) : towerInspect(assetDir, tower);
                            if (!inspectionFade) stop(hand ? "Missing or invalid hand.dat." : "Missing or invalid tower.dat.");
                            roomChanged = true;
                            previousTime = systemCounterGetTicks();
                            debt = step;
                        } else if (roomCurrent() == MusicRoom && !piano.solved && playerFacingPiano())
                            inRoom = false;
                        else if (roomCurrent() != MusicRoom || phase == MedalFinished)
                            pickupStart();
                    }
                }
            } else if (!piano.checking && (pressed & KEY_B) && hasRoom) {
                inRoom = true;
            } else {
                sound = pianoUpdate(&piano, &assets.keys,
                        !!(held & KEY_RIGHT) - !!(held & KEY_LEFT),
                        !!(held & KEY_DOWN) - !!(held & KEY_UP),
                        pressed & KEY_A, pressed & KEY_B, stepUs);
                if (piano.solved && hasRoom) {
                    inRoom = true;
                    playerWarpForMedal();
                    medallionStart(&medal);
                }
            }
            playTone(sound);
            pressed = 0;
            debt -= step;
        }

        int imageIndex = inRoom ? 2 : piano.checking;
        if (inRoom && (previousImage != imageIndex || roomChanged)) {
            roomDraw(&medal);
            bgShow(0);
        } else if (!inRoom && previousImage != imageIndex) {
            bgHide(0);
            bgShow(bg);
            const Background *image = &assets.images[piano.checking];
            memcpy(bgGetGfxPtr(bg), image->pixels, sizeof image->pixels);
            memcpy(BG_PALETTE, image->palette, sizeof image->palette);
        }
        if (inRoom) pickupDrawPrompt(bg);
        previousImage = imageIndex;
        oamSetXY(&oamMain, 0, ((piano.x >> 12) + 160) * 4 / 5,
                              ((piano.y >> 12) + 120) * 4 / 5);
        oamSetHidden(&oamMain, 0, !piano.checking || piano.waitUs != 0);
        oamUpdate(&oamMain);
    }
}

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
#include "otherworld.h"

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
       PromptMove, PromptCancel, PromptConfirm, InsertMedallion, TowerChime,
       BoilerSwitchSound, BoilerStartSound, BoilerDoorOpen, BoilerDoorClose, BoilerHum, TowerOpening,
       OtherworldDoorOpen, OtherworldDoorClose, SideHallOpen, SideHallClose, CardSlide, CardUnlock, ToneCount };
static const int pickupSounds[] = {
    [PickupMove] = PromptMove, [PickupCancel] = PromptCancel, [PickupConfirm] = PromptConfirm,
    [PickupBoilerSwitch] = BoilerSwitchSound, [PickupBoilerStart] = BoilerStartSound
};
static Tone tones[ToneCount];
static uint8_t samples[388 * 1024] __attribute__((aligned(32)));
static unsigned toneCount;
static uint16_t humLoop;
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
    case ClockTower: name = "clockroom"; break;
    case FarClockTower: name = "fartower"; break;
    case OtherworldCourtyard: name = "otherworld"; break;
    case OtherworldNorthHall: name = "otherworldnorth"; break;
    case OtherworldLobby: name = "otherworldlobby"; break;
    case OtherworldSideHall: name = "otherworldside"; break;
    case OtherworldEastHall: name = "otherworldeast"; break;
    case OtherworldSouthHall: name = "otherworldsouth"; break;
    case OtherworldReception: name = "reception"; break;
    case OtherworldWestHall: name = "otherworldwest"; break;
    case NorthClassroom: name = "classroomnorth"; break;
    case SouthClassroom: name = "classroomsouth"; break;
    case TowerPassage: name = "passage"; break;
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
    if (count > BoilerHum && (fread(&humLoop, sizeof humLoop, 1, file) != 1 ||
                               (humLoop & 1) || 2u*humLoop >= tones[BoilerHum].bytes)) return false;
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
    static const char versions[] = "123456789ABCD";
    bool valid = fread(&assets, 1, sizeof assets, file) == sizeof assets &&
                 !memcmp(assets.magic, "SHP", 3) && assets.magic[3];
    const char *version = valid ? strchr(versions, assets.magic[3]) : NULL;
    valid = version != NULL;
    if (valid && version != versions) {
        static const unsigned counts[] = {0, 9, 10, 12, 14, 15, InsertMedallion,
                                          BoilerSwitchSound, TowerOpening, OtherworldDoorOpen, SideHallOpen, CardSlide, ToneCount};
        toneCount = counts[version-versions];
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

static void playTowerSound(bool chime)
{
    playTone(chime ? TowerChime : InsertMedallion);
}

static void playSlotSound(bool unlocked)
{
    playTone(unlocked ? CardUnlock : CardSlide);
}

static void updateBoilerHum(void)
{
    static int channel = -1;
    static unsigned previousVolume;
    if (toneCount <= BoilerHum) return;
    unsigned volume = boilerVolume()*tones[BoilerHum].volume/127;
    if (!volume) {
        if (channel >= 0) soundKill(channel);
        channel = -1;
    } else if (channel < 0) {
        const Tone *tone = &tones[BoilerHum];
        channel = soundPlaySample(samples + tone->offset, SoundFormat_16Bit, tone->bytes-2u*humLoop,
                                  tone->rate, volume, tone->pan, true, humLoop/2);
    } else if (previousVolume != volume) {
        soundSetVolume(channel, volume);
    }
    previousVolume = volume;
}

static void changeRoom(RoomExit exit)
{
    SchoolRoom from = roomCurrent(), next = exit.room;
    bool east = next == EastCorridor || from == EastCorridor;
    bool otherworldIntro = from == FarClockTower && next == OtherworldCourtyard;
    bool otherworldHall = from == OtherworldNorthHall || next == OtherworldNorthHall ||
                          from == OtherworldSouthHall || next == OtherworldSouthHall;
    bool pair12 = from == OtherworldSideHall || next == OtherworldSideHall ||
                  from == NorthClassroom || next == NorthClassroom ||
                  from == SouthClassroom || next == SouthClassroom ||
                  from == OtherworldReception || next == OtherworldReception;
    bool hall = east || from == Courtyard || next == Courtyard || otherworldHall || pair12 ||
                (from == FirstFloor && next == SouthHall) || (from == SouthHall && next == FirstFloor);
    bool towerDoor = (next == ClockTower && from == Courtyard) || otherworldIntro;
    bool slotDoor = from == OtherworldWestHall || next == OtherworldWestHall;
    bool door = !exit.stairs && next != TowerPassage && from != TowerPassage;
    bool boilerDoor = toneCount > BoilerDoorClose && (next == BoilerRoom || from == BoilerRoom);
    int opening = hall ? HallOpen : DoorOpen, closing = hall ? HallClose : DoorClose;
    if (towerDoor) {
        opening = TowerOpening;
        closing = -1;
    } else if (boilerDoor) {
        opening = BoilerDoorOpen;
        closing = BoilerDoorClose;
    } else if (slotDoor) {
        opening = HallOpen;
        closing = HallClose;
    } else if (pair12 && toneCount > SideHallClose) {
        opening = SideHallOpen;
        closing = SideHallClose;
    } else if (otherworldHall && toneCount > OtherworldDoorClose) {
        opening = OtherworldDoorOpen;
        closing = OtherworldDoorClose;
    }
    FILE *file = openRoom(next);
    if (!file) stop("Missing room data.\nRun the asset converter.");
    if (door) playTone(opening);
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
    playerEnterRoom(from, exit);
    if (otherworldIntro) otherworldStartIntro();
    updateBoilerHum();
    // GameBoot_InGameStartup step 11 keeps the load screen for at least
    // one second, including time already spent on disc or card reads.
    while ((uint32_t)(systemCounterGetTicks()-start) < systemCounterUsecsToTicks(1000000))
        swiWaitForVBlank();
    if (door) playTone(closing);
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
        towerRestore(point.flags);
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
            if (inRoom && roomHasNormals())
                roomChanged |= lightingTick((pressed & KEY_Y) && !otherworldIntroActive() && !pickupActive() && !slotActive());
            if (inRoom && otherworldIntroActive()) {
                roomChanged |= otherworldTickIntro(pressed & KEY_START);
            } else if (inRoom && slotActive()) {
                roomChanged |= slotTick(pressed, playSlotSound);
                if (!slotActive()) inspectionFadeLength = inspectionFade = SlotRevealVBlanks;
            } else if (inRoom && playerLadderActive()) {
                SchoolRoom arrival = roomCurrent();
                roomChanged |= playerLadderTick(pressed & KEY_START, &arrival);
                if (arrival != roomCurrent()) {
                    changeRoom((RoomExit){.room = arrival});
                    previousTime = systemCounterGetTicks();
                    debt = step;
                }
                if (!playerLadderActive())
                    inspectionFadeLength = inspectionFade = LadderRevealVBlanks;
            } else if (inRoom) {
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
                if (playerTick(turn, walk, statueCamera() ? PlayerPour : pickupAction())) {
                    roomTick();
                    roomChanged = true;
                }
                PickupSound pickupSound = pickupTick(pressed);
                if (pickupSound != PickupQuiet) sound = pickupSounds[pickupSound];
                if (phase == MedalDrop && medal.phase == MedalImpact)
                    sound = 9;
                if (!wasActive && phase == medal.phase && (phase == MedalHanging || phase == MedalFinished)) {
                    RoomExit exit = playerDestination(pressed & KEY_A);
                    TowerPoint tower = pressed & (KEY_A | KEY_SELECT) ? playerFacingTower() : NoTowerPoint;
                    if ((pressed & KEY_A) && tower == TowerDoor && boilerPowered())
                        exit = (RoomExit){.room = ClockTower};
                    SchoolRoom next = exit.room;
                    if (next == SouthHall && roomCurrent() == FirstFloor && !latchUnlocked()) {
                        if (latchStart()) sound = DoorUnlock;
                    } else if (next != roomCurrent()) {
                        inspectionFade = 0;
                        changeRoom(exit);
                        enteringRoom = !playerLadderActive() && !otherworldIntroActive();
                        roomChanged = true;
                        previousTime = systemCounterGetTicks();
                        debt = step;
                    } else if ((pressed & KEY_A) && playerStartAscent()) {
                        inspectionFade = 0;
                        roomChanged = true;
                    } else if ((pressed & KEY_SELECT) && !slotUnlocked() && (pickupFlags() & PictureCardHeld) &&
                               playerFacingSlot()) {
                        if (!slotStart(assetDir)) stop("Missing or invalid cardslot.dat.");
                        inspectionFade = 0;
                        roomChanged = true;
                        previousTime = systemCounterGetTicks();
                        debt = step;
                    } else if ((pressed & KEY_SELECT) && statueStart()) {
                        inspectionFade = 0;
                        setBrightness(1, 0);
                        roomChanged = true;
                    } else if ((pressed & KEY_SELECT) && towerCanPlace(tower, pickupFlags())) {
                        inspectionFadeLength = inspectionFade = towerPlace(assetDir, tower, playTowerSound);
                        if (!inspectionFade) stop("Missing or invalid tower.dat.\nRun the asset converter.");
                        roomChanged = true;
                        previousTime = systemCounterGetTicks();
                        debt = step;
                    } else if ((pressed & KEY_A) && playerFacingNotebook()) {
                        playerSavePose(&point);
                        point.flags = (point.flags & NotebookRead) | (piano.solved ? PianoSolved : 0) |
                                      pickupFlags() | statueFlags() | towerFlags() |
                                      (latchUnlocked() ? LatchOpen : 0) | (boilerPowered() ? BoilerPowered : 0);
                        if (notebookMenu(assetDir, false, &point) == NotebookMissingArt)
                            stop("Missing notebook.dat");
                        roomChanged = true;
                        previousTime = systemCounterGetTicks();
                        debt = step;
                    } else if ((pressed & KEY_A) && !slotUnlocked() && playerFacingSlot()) {
                        lightingScreen(false, false);
                        inspectionFadeLength = inspectionFade = slotInspect(assetDir);
                        if (!inspectionFade) stop("Missing or invalid cardslot.dat.");
                        roomChanged = true;
                        previousTime = systemCounterGetTicks();
                        debt = step;
                    } else if (pressed & KEY_A) {
                        bool hand = playerFacingHand() && !(roomCanDissolveHand() && statueOpened());
                        if (tower != NoTowerPoint || hand) {
                            inspectionFadeLength = inspectionFade = hand ? handInspect(assetDir) : towerInspect(assetDir, tower);
                            if (!inspectionFade) stop(hand ? "Missing or invalid hand.dat." : "Missing or invalid tower.dat.");
                            roomChanged = true;
                            previousTime = systemCounterGetTicks();
                            debt = step;
                        } else if (playerFacingLobbyFan())
                            lobbyFanStart();
                        else if (playerFacingTowerExit())
                            towerDoorStart();
                        else if (playerFacingBoiler())
                            boilerStart();
                        else if (roomCurrent() == MusicRoom && !piano.solved && playerFacingPiano())
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

        lightingScreen(inRoom && roomHasNormals(), otherworldIntroActive() || slotActive());
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
        if (inRoom) {
            if (roomCurrent() == OtherworldCourtyard) otherworldDrawSubtitle(bg);
            else if (slotActive()) slotDrawPrompt(bg);
            else pickupDrawPrompt(bg);
            updateBoilerHum();
        }
        previousImage = imageIndex;
        oamSetXY(&oamMain, 0, ((piano.x >> 12) + 160) * 4 / 5,
                              ((piano.y >> 12) + 120) * 4 / 5);
        oamSetHidden(&oamMain, 0, !piano.checking || piano.waitUs != 0);
        oamUpdate(&oamMain);
    }
}

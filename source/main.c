#include <nds.h>
#include <stdio.h>

int main(void)
{
    lcdMainOnTop();
    videoSetModeSub(MODE_0_2D);
    setBrightness(2, -16);

    PrintConsole console;
    videoSetMode(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    consoleInit(&console, 0, BgType_Text4bpp, BgSize_T_256x256,
                31, 0, true, true);
    puts(isDSiMode() ? "Silent Hill\nDSi mode" : "DSi mode required");
    for (;;) {
        swiWaitForVBlank();
    }
}

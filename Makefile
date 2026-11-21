BLOCKSDS ?= /opt/blocksds/core
NAME := sh1dsi
GAME_TITLE := Silent Hill
GAME_SUBTITLE := Nintendo DSi port
GAME_AUTHOR := sh1dsi
SPECS := $(BLOCKSDS)/sys/crts/dsi_arm9.specs
ARM7ELF := $(BLOCKSDS)/sys/arm7/main_core/arm7_minimal.elf
NDSTOOL_ARGS := -uc 3
include $(BLOCKSDS)/sys/default_makefiles/rom_arm9/Makefile

.PHONY: sdcard
sdcard: $(ROM)
	@for data in piano room corridor east firstfloor south north uppernorth laboratory equipment courtyard infirmary notebook tower hand basement boiler clockroom passage fartower otherworld otherworldnorth otherworldlobby otherworldside otherworldeast classroomnorth classroomsouth; do \
		test -f dist/sh1dsi/$$data.dat || { echo "Missing $$data.dat. Run tools/prepare_assets.py with your CUE first."; exit 1; }; \
	done
	@mkdir -p dist
	cp $(ROM) dist/

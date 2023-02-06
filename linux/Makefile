TARGET = gnw

DEBUG = 1
#OPT = -Og -ggdb

ifeq ($(DEBUG), 1)
  OPT = -O0 -ggdb3 -fsanitize=address
else
  OPT = -O3
endif

HEADLESS = 1

BUILD_DIR = build

ifeq ($(HEADLESS), 1)
  C_SOURCES = main_headless.c
else
  C_SOURCES = main.c
endif

C_SOURCES += \
porting.c \
zelda_assets.c \
../zelda3/config.c \
../zelda3/zelda_rtl.c \
../zelda3/misc.c \
../zelda3/nmi.c \
../zelda3/poly.c \
../zelda3/attract.c \
../zelda3/snes/ppu.c \
../zelda3/snes/dma.c \
../zelda3/spc_player.c \
../zelda3/util.c \
../zelda3/audio.c \
../zelda3/overworld.c \
../zelda3/ending.c \
../zelda3/select_file.c \
../zelda3/dungeon.c \
../zelda3/messaging.c \
../zelda3/hud.c \
../zelda3/load_gfx.c \
../zelda3/ancilla.c \
../zelda3/player.c \
../zelda3/sprite.c \
../zelda3/player_oam.c \
../zelda3/snes/apu.c \
../zelda3/snes/dsp.c \
../zelda3/sprite_main.c \
../zelda3/tagalong.c \
../zelda3/third_party/opus-1.3.1-stripped/opus_decoder_amalgam.c \
../zelda3/tile_detect.c \
../zelda3/overlord.c \


PREFIX = 

CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

C_DEFS =  \
-DIS_LITTLE_ENDIAN

C_INCLUDES =  \
-I. \
-I.. \


ASFLAGS = $(AS_DEFS) $(AS_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
#CFLAGS  = $(C_DEFS) $(C_INCLUDES) `sdl2-config --cflags` $(OPT) -Wall -fdata-sections -ffunction-sections -fmax-errors=5
CFLAGS  = $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -fmax-errors=5
ifeq ($(HEADLESS), 0)
  CFLAGS += `sdl2-config --cflags`
else
  CFLAGS += -DHEADLESS
endif
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

#LIBS = -lm `sdl2-config --libs`
#LIBS = -lasan -lm `sdl2-config --libs`
LIBS = -lm
ifeq ($(DEBUG), 1)
  LIBS += -lasan
endif
ifeq ($(HEADLESS), 0)
  LIBS += `sdl2-config --libs`
endif
LDFLAGS = $(LIBS) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

all: $(BUILD_DIR)/$(TARGET).elf

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))


$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

viewer: viewer.c Makefile
	$(CC) viewer.c $(LDFLAGS) -o $@
	$(SZ) $@

$(BUILD_DIR):
	mkdir $@

clean:
	-rm -fR $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)

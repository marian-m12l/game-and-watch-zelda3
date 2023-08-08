# Port of reverse engineered zelda3 for the Game and Watch

All credits for reverse engineering goes to https://github.com/snesrev/zelda3

## Requirements

Minimum flash size of 2MB (the game uses around 1.5MB of extflash).

GCC >= 12

# Build instructions

- Clone repository with submodules
```sh
git clone --recurse-submodules https://github.com/marian-m12l/game-and-watch-zelda3.git
```

- Install python requirements using pip
```sh
pip3 install -r requirements.txt
pip3 install -r zelda3/requirements.txt
```

- Place your US ROM file named `zelda3.sfc` in `zelda3/tables`.

- Optionally, place your localized ROM file `zelda3_<lang>.sfc` in `zelda3/tables` (e.g. `zelda3_fr.sfc`).

- Compile and flash (e.g. on internal flash bank 2, leaving 1MB (out of 16) for stock firmware at the beginning of extflash, french language)
```sh
make INTFLASH_BANK=2 EXTFLASH_SIZE_MB=15 EXTFLASH_OFFSET=1048576 ADAPTER=jlink OPENOCD=/path/to/patched/openocd-git/bin/openocd GNW_TARGET=mario DIALOGUES_LANGUAGE=fr flash
```

## List of build flags

To control how the firmware is programmed:

| Build flag    | Description |
| ------------- | ------------- |
| `INTFLASH_BANK` | Which internal flash bank to program.<br>1 or 2. |
| `EXTFLASH_OFFSET` | Offset after which to program external flash. In bytes. |
| `EXTFLASH_SIZE_MB` | Allocated space in external flash. In MB. |
| `LARGE_FLASH` | Required for external flash chips > 16MB. Enables 32-bit addressing. |

To control game options:

| Build flag    | Description |
| ------------- | ------------- |
| `LIMIT_30FPS` | Limit to 30 fps for improved stability.<br>Enabled by default.<br>Disabling this flag will result in unsteady framerate and stuttering. |
| `OVERCLOCK` | Overclock level: 0 (no overclocking), 1 (intermediate overclocking), or 2 (max overclocking).<br>Default value: 2. |
| `RENDER_FPS` | Render performance metrics. Disabled by default. |
| `ENABLE_SAVESTATE` | Enable savestate support. This allocates 178kB of external flash.<br>Disabled by default. |
| `FASTER_UI` | Increase UI speed (item menu, etc.).<br>Enabled by default. |
| `BATTERY_INDICATOR` | Display battery indicator in item menu.<br>Enabled by default. |
| `EXTENDED_SCREEN` | Extended screensize (0 for default screen size 256x224, 1 for full-height 256x240, 2 for full screen 320x240).<br>Default value: 1. |

## (WIP) Building as a Retro-Go application (a.k.a. running in RAM)

**DISCLAIMER: This is work-in-progress**

Zelda3 can be run without flashing to intflash. The "intflash" binary can be stored in the Retro-Go filesystem and loaded into RAM for execution.

The Retro-Go filesystem must be large enough to fit the "intflash" binary on top of emulator savestates.

```
make [...] flash_extflash
make [...] build/gw_zelda3_intflash.bin
tamp compress build/gw_zelda3_intflash.bin -o build/gw_zelda3_intflash.bin.tamp
python3 gnwmanager.py push /apps/zelda3.bin.tamp build/gw_zelda3_intflash.bin.tamp
```

# Backing up and restoring save (SRAM)

```sh
ADAPTER=jlink OPENOCD=/path/to/patched/openocd-git/bin/openocd ./scripts/saves_backup.sh build/gw_zelda3.elf 
ADAPTER=jlink OPENOCD=/path/to/patched/openocd-git/bin/openocd ./scripts/saves_erase.sh build/gw_zelda3.elf 
ADAPTER=jlink OPENOCD=/path/to/patched/openocd-git/bin/openocd ./scripts/saves_restore.sh build/gw_zelda3.elf 
```

# Using savestate

To enable savestate (as opposed to SRAM saves), add `ENABLE_SAVESTATE=1` to your make command.

Save is **automatic on power off** (you can power off without saving by holding PAUSE/SET and pressing POWER).

You can save by pressing `GAME + A`.
You can load the savestate by pressing `GAME + B`.

## Backing up and restoring savestate

```sh
ADAPTER=jlink OPENOCD=/path/to/patched/openocd-git/bin/openocd ./scripts/savestate_backup.sh build/gw_zelda3.elf 
ADAPTER=jlink OPENOCD=/path/to/patched/openocd-git/bin/openocd ./scripts/savestate_erase.sh build/gw_zelda3.elf 
ADAPTER=jlink OPENOCD=/path/to/patched/openocd-git/bin/openocd ./scripts/savestate_restore.sh build/gw_zelda3.elf 
```

# Applying additional features

Edit the `FEATURE_*` values in `Makefile` to enable additional features and bug fixes to the original game.

## List of features

| Build flag    | Description |
| ------------- | ------------- |
| `FEATURE_SWITCH_LR` | Item switch on L/R. Also allows reordering of items in inventory by pressing Y+direction.<br>Hold X, L, or R inside of the item selection screen to assign items to those buttons.<br>If X is reassigned, Select opens the map. Push Select while paused to save or quit.<br>When L or R are assigned items, those buttons will no longer cycle items. |
| `FEATURE_TURN_WHILE_DASHING` | Allow turning while dashing. |
| `FEATURE_MIRROR_TO_DARK_WORLD` | Allow mirror to be used to warp to the Dark World. |
| `FEATURE_COLLECT_ITEMS_WITH_SWORD` | Collect items (like hearts) with sword instead of having to touch them. |
| `FEATURE_BREAK_POTS_WITH_SWORD` | Level 2-4 sword can be used to break pots. |
| `FEATURE_DISABLE_LOW_HEALTH_BEEP` | Disable the low health beep. |
| `FEATURE_SKIP_INTRO_ON_KEYPRESS` | Avoid waiting too much at the start.<br>Enabled by default. |
| `FEATURE_SHOW_MAX_ITEMS_IN_YELLOW` | Display max rupees/bombs/arrows with orange/yellow color. |
| `FEATURE_MORE_ACTIVE_BOMBS` | Allows up to four bombs active at a time instead of two. |
| `FEATURE_CARRY_MORE_RUPEES` | Can carry 9999 rupees instead of 999. |
| `FEATURE_MISC_BUG_FIXES` | Enable various zelda bug fixes. |
| `FEATURE_CANCEL_BIRD_TRAVEL` | Allow bird travel to be cancelled by hitting the X key. |
| `FEATURE_GAME_CHANGING_BUG_FIXES` | Enable some more advanced zelda bugfixes that change game behavior. |
| `FEATURE_SWITCH_LR_LIMIT` | Enable this to limit the ItemSwitchLR item cycling to the first 4 items. |

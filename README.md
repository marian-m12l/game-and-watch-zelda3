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
make INTFLASH_BANK=2 EXTFLASH_SIZE_MB=15 EXTFLASH_OFFSET=1048576 ADAPTER=jlink OPENOCD=/path/to/patched/openocd-git/bin/openocd GNW_TARGET=mario LANGUAGE=fr flash
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

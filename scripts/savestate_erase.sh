#!/bin/bash

. ./scripts/common.sh

if [[ $# -lt 1 ]]; then
    echo "Usage: $(basename $0) <currently_running_binary.elf>"
    echo "This will erase zelda savestate from the device"
    exit 1
fi

ELF="$1"

address=$(get_symbol __SAVEFLASH_START__)
address=$(($address + 8192))
size=$((4096*68))

echo "Erasing savestate data:"
printf "    save_address=0x%08x\n" $address
echo "    save_size=$size"

DUMMY_FILE=$(mktemp /tmp/retro_go_dummy.XXXXXX)
if [[ ! -e "${DUMMY_FILE}" ]]; then
    echo "Can't create tempfile!"
    exit 1
fi

# Create dummy file with 0xFF of the size saveflash_size
/usr/bin/env python3 -c "with open('${DUMMY_FILE}', 'wb') as f: f.write(b'\xFF'*${size})"

# Flash it to the saveflash_start
export USE_4K_ERASE_CMD=1   # Used in stm32h7x_spiflash.cfg
${OPENOCD} -f scripts/interface_${ADAPTER}.cfg -c "program ${DUMMY_FILE} ${address} verify reset exit"

# Reset the device and disable clocks from running when device is suspended
reset_and_disable_debug

# Clean up
rm -f "${DUMMY_FILE}"

echo ""
echo ""
echo "Savestate has been erased."
echo ""
echo ""

# TODO read zelda_assets.dat
#   - skip heaader
#   - read asset sizes
#   - keep only a selection of assets
#   - write zelda_assets_bis.dat (+ the zelda_assets_bis.[c|h]) with: number of assets, (index+size) for each asset, asset data

import struct

# Takes 337 KB :'( ...
assets_to_keep = [
        #64,     # Dialogue offset               (794 B)
        #65,     # Dialogue text                 (36362 B)
        66,     # Sprites graphics              (133481 B)
        67,     # Backgrounds graphics          (119901 B)
        68,     # Overworld Map graphics        (16384 B)
        #69,     # Light Overworld Tilemap       (4096 B)
        #70,     # Dark Overworld Tilemap        (1024 B)
        #71,     # Prodefined Tile Data          (12876 B)
        72,     # Font Data                     (4096 B)
        #99,     # Backgrounds Tilemap 0         (1115 B)
        #100,    # Backgrounds Tilemap 1         (1467 B)
        #101,    # Backgrounds Tilemap 2         (177 B)
        #102,    # Backgrounds Tilemap 3         (81 B)
        #103,    # Backgrounds Tilemap 4         (233 B)
        #104,    # Backgrounds Tilemap 5         (661 B)
        #109,    # Overworld Background Palettes (136 B)
        #159,    # Overworld Sprites offset      (864 B)
        #160,    # Overworld Sprites             (2797 B)
        #161,    # Overworld Sprites graphics    (256 B)
        #162,    # Overworld Sprites palettes    (256 B)
]
assets_data_addr = []
assets_data_length = []

with open("zelda3/tables/zelda3_assets.dat", mode="rb") as f:
        with open("Core/Inc/porting/zelda_assets_in_ram.dat", "wb") as output:
                output.write(struct.pack('<I', len(assets_to_keep)))
                f.seek(80)
                nbAssets, *_ = struct.unpack('<I', f.read(4))
                print(nbAssets)
                offset, *_ = struct.unpack('<I', f.read(4))
                print(offset)
                addr = 88 + nbAssets*4 + offset
                addr_out = 4 + len(assets_to_keep)*8
                f.seek(88)
                for asset in range(nbAssets):
                        length, *_ = struct.unpack('<I', f.read(4))
                        addr = (addr + 3) & ~3  # Alignment
                        if asset in assets_to_keep:
                                addr_out = (addr_out + 3) & ~3
                                assets_data_addr.append(addr)
                                assets_data_length.append(length)
                                print(f'Asset #{asset}: {length} bytes @ 0x{addr:05x} --> @ Ox{addr_out:05x}')
                                output.write(struct.pack('<II', asset, length))
                                addr_out += length
                        else:
                                print(f'Asset #{asset}: {length} bytes @ 0x{addr:05x} --> DROPPED')
                        addr += length
                addr_out = 4 + len(assets_to_keep)*8
                for asset in range(len(assets_to_keep)):
                        f.seek(assets_data_addr[asset])
                        addr_out = (addr_out + 3) & ~3
                        print(f'COPYING asset #{assets_to_keep[asset]}: {assets_data_length[asset]} bytes @ 0x{assets_data_addr[asset]:05x} --> @ Ox{addr_out:05x}')
                        output.seek(addr_out)
                        output.write(f.read(assets_data_length[asset]))
                        addr_out += assets_data_length[asset]

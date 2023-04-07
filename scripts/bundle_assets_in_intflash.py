# TODO read zelda_assets.dat
#   - skip heaader
#   - read asset sizes
#   - keep only a selection of assets
#   - write zelda_assets_bis.dat (+ the zelda_assets_bis.[c|h]) with: number of assets, (index+size) for each asset, asset data

import struct

assets_to_keep = [
        #0,      # Sound Bank Intro              (50066 B)
        #1,      # Sound Bank Indoor             (12756 B)
        #2,      # Sound Bank Ending             (8354 B)
        57,     # Link graphics                 (28672 B)
        58,     # Dungeon Sprites               (4965 B)
        59,     # Dungeon Sprite Offset         (640 B)
        82,     # Palette Dungeon BG Main       (3600 B)
        83,     # Palette Sprites Main          (240 B)
        84,     # Palette Armor and Gloves      (150 B)
        85,     # Palette Sword                 (24 B)
        86,     # Palette Shield                (24 B)
        87,     # Palette Sprites Aux 3         (168 B)
        88,     # Palette Misc Sprites Indoor   (154 B)
        89,     # Palette Sprites Aux 1         (336 B)
        90,     # Palette Overworld BG Main     (420 B)
        91,     # Palette Overworld BG Aux 1/2  (840 B)
        92,     # Palette Overworld BG Aux 3    (196 B)
        93,     # Palette Palace Map BG         (192 B)
        94,     # Palette Palace Map Sprites    (42 B)
        95,     # Palette Hud                   (128 B)
        97,     # Dungeon Map Floor Layout      (1531 B)
        98,     # Dungeon Map Tiles             (242 B)
        111,    # Overworld Music Sets          (256 B)
        112,    # Overworld Music Sets 2        (96 B)

        #64,     # Dialogue offset               (794 B)
        #65,     # Dialogue text                 (36362 B)
        ##66,     # Sprites graphics              (133481 B)
        ##67,     # Backgrounds graphics          (119901 B)
        ##68,     # Overworld Map graphics        (16384 B)
        69,     # Light Overworld Tilemap       (4096 B)
        70,     # Dark Overworld Tilemap        (1024 B)
        71,     # Prodefined Tile Data          (12876 B)
        ##72,     # Font Data                     (4096 B)
        99,     # Backgrounds Tilemap 0         (1115 B)
        100,    # Backgrounds Tilemap 1         (1467 B)
        101,    # Backgrounds Tilemap 2         (177 B)
        102,    # Backgrounds Tilemap 3         (81 B)
        103,    # Backgrounds Tilemap 4         (233 B)
        104,    # Backgrounds Tilemap 5         (661 B)
        109,    # Overworld Background Palettes (136 B)
        159,    # Overworld Sprites offset      (864 B)
        160,    # Overworld Sprites             (2797 B)
        161,    # Overworld Sprites graphics    (256 B)
        162,    # Overworld Sprites palettes    (256 B)
]
assets_data_addr = []
assets_data_length = []

with open("zelda3/tables/zelda3_assets.dat", mode="rb") as f:
        with open("Core/Inc/porting/zelda_assets_in_intflash.dat", "wb") as output:
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

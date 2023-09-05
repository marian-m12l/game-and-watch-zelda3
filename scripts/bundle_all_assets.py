import struct

assets_location = [
        #0,                      # Sound Bank Intro              (50066 B)
        #1,                      # Sound Bank Indoor             (12756 B)
        #2,                      # Sound Bank Ending             (8354 B)
#        (57,    'intflash'),     # Link graphics                 (28672 B)
#        (58,    'ram'),          # Dungeon Sprites               (4965 B)
#        (59,    'intflash'),     # Dungeon Sprite Offset         (640 B)
#        (64,    'ram'),          # Sprites graphics              (133481 B)
#        (65,    'ram'),          # Backgrounds graphics          (119901 B)
#        (66,    'intflash'),     # Overworld Map graphics        (16384 B)
#        (67,    'intflash'),     # Light Overworld Tilemap       (4096 B)
#        (68,    'intflash'),     # Dark Overworld Tilemap        (1024 B)
#        (69,    'intflash'),     # Predefined Tile Data          (12876 B)
#        (79,    'ram'),          # Palette Dungeon BG Main       (3600 B)
#        (80,    'intflash'),     # Palette Sprites Main          (240 B)
#        (81,    'intflash'),     # Palette Armor and Gloves      (150 B)
#        (82,    'intflash'),     # Palette Sword                 (24 B)
#        (83,    'intflash'),     # Palette Shield                (24 B)
#        (84,    'intflash'),     # Palette Sprites Aux 3         (168 B)
#        (85,    'intflash'),     # Palette Misc Sprites Indoor   (154 B)
#        (86,    'intflash'),     # Palette Sprites Aux 1         (336 B)
#        (87,    'intflash'),     # Palette Overworld BG Main     (420 B)
#        (88,    'intflash'),     # Palette Overworld BG Aux 1/2  (840 B)
#        (89,    'intflash'),     # Palette Overworld BG Aux 3    (196 B)
#        (90,    'intflash'),     # Palette Palace Map BG         (192 B)
#        (91,    'intflash'),     # Palette Palace Map Sprites    (42 B)
#        (92,    'intflash'),     # Palette Hud                   (128 B)
        #94,                     # Dialogue
        #95,                     # Dialogue font
        #96,                     # Dialogue map
#        (97,    'intflash'),     # Dungeon Map Floor Layout      (1531 B)
#        (98,    'intflash'),     # Dungeon Map Tiles             (242 B)
#        (99,    'intflash'),     # Backgrounds Tilemap 0         (1115 B)
#        (100,   'intflash'),     # Backgrounds Tilemap 1         (1467 B)
#        (101,   'intflash'),     # Backgrounds Tilemap 2         (177 B)
#        (102,   'intflash'),     # Backgrounds Tilemap 3         (81 B)
#        (103,   'intflash'),     # Backgrounds Tilemap 4         (233 B)
#        (104,   'intflash'),     # Backgrounds Tilemap 5         (661 B)
#        (109,   'intflash'),     # Overworld Background Palettes (136 B)
#        (111,   'intflash'),     # Overworld Music Sets          (256 B)
#        (112,   'intflash'),     # Overworld Music Sets 2        (96 B)
#        (159,   'intflash'),     # Overworld Sprites offset      (864 B)
#        (160,   'intflash'),     # Overworld Sprites             (2797 B)
#        (161,   'intflash'),     # Overworld Sprites graphics    (256 B)
#        (162,   'intflash'),     # Overworld Sprites palettes    (256 B)
]
assets_in_intflash = [index for (index, location) in assets_location if location == 'intflash']
assets_in_ram = [index for (index, location) in assets_location if location == 'ram']
assets_in_extflash = [index for index in range(165) if index not in assets_in_intflash and index not in assets_in_ram]


def bundle_assets(assets_to_keep, path):
        assets_data_addr = []
        assets_data_length = []
        with open("zelda3/tables/zelda3_assets.dat", mode="rb") as f:
                with open(path, "wb") as output:
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


print('Bundling intflash assets')
bundle_assets(assets_in_intflash, "scripts/zelda_assets_in_intflash.dat")
print('Bundling ram assets')
bundle_assets(assets_in_ram, "scripts/zelda_assets_in_ram.dat")
print('Bundling extflash assets')
bundle_assets(assets_in_extflash, "scripts/zelda_assets_in_extflash.dat")

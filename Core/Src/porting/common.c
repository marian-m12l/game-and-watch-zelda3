#include "common.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "main.h"
#include "buttons.h"
#include "lcd.h"
#include "gw_linker.h"
#include "font_basic.h"

#if ENABLE_SCREENSHOT
uint16_t framebuffer_capture[GW_LCD_WIDTH * GW_LCD_HEIGHT]  __attribute__((section (".fbflash"))) __attribute__((aligned(4096)));
#endif

const uint8_t backlightLevels[] = {128, 130, 133, 139, 149, 162, 178, 198, 222, 255};


__attribute__((section (".configflash"))) __attribute__((aligned(4096))) persistent_config_t persistent_config_flash;
persistent_config_t persistent_config_ram;

void settings_init()
{
    memcpy(&persistent_config_ram, &persistent_config_flash, sizeof(persistent_config_t));

    if (persistent_config_ram.magic != CONFIG_MAGIC) {
        printf("Config: Magic mismatch. Expected 0x%08x, got 0x%08lx\n", CONFIG_MAGIC, persistent_config_ram.magic);
        settings_reset();
        return;
    }

    if (persistent_config_ram.version != persistent_config_default.version) {
        printf("Config: New config version, resetting settings.\n");
        settings_reset();
        return;
    }

    // Calculate crc32 of the whole struct with the crc32 value set to 0
    persistent_config_ram.crc32 = 0;
    persistent_config_ram.crc32 = crc32_le(0, (unsigned char *) &persistent_config_ram, sizeof(persistent_config_t));

    if (persistent_config_ram.crc32 != persistent_config_flash.crc32) {
        printf("Config: CRC32 mismatch. Expected 0x%08lx, got 0x%08lx\n", persistent_config_ram.crc32, persistent_config_flash.crc32);
        settings_reset();
        return;
    }
}

void settings_commit()
{
    // Calculate crc32 of the whole struct with the crc32 value set to 0
    persistent_config_ram.crc32 = 0;
    persistent_config_ram.crc32 = crc32_le(0, (unsigned char *) &persistent_config_ram, sizeof(persistent_config_t));

    store_save((const uint8_t *) &persistent_config_flash, (const uint8_t *) &persistent_config_ram, sizeof(persistent_config_t));
}

void settings_reset()
{
    memcpy(&persistent_config_ram, &persistent_config_default, sizeof(persistent_config_t));
}

uint8_t settings_Volume_get()
{
    return persistent_config_ram.volume;
}
void settings_Volume_set(uint8_t value)
{
    persistent_config_ram.volume = value;
}

uint8_t settings_Backlight_get()
{
    return persistent_config_ram.backlight;
}
void settings_Backlight_set(uint8_t value)
{
    persistent_config_ram.backlight = value;
}



#define OVERLAY_COLOR_565 0xFFFF
#define BORDER_COLOR_565 0x1082  // Dark Dark Gray

#define BORDER_HEIGHT 240
#define BORDER_WIDTH 32

#define BORDER_Y_OFFSET (((GW_LCD_HEIGHT) - (BORDER_HEIGHT)) / 2)

/**
 * Was generated using tool https://github.com/BrianPugh/sheikah-banner with params:
 *     python main.py gamewatch --vertical --mode=1 --size=480 --padding=60 --resize 0.05
 * The image was then recropped to 240x32 and then converted using:
 *     https://github.com/sylverb/game-and-watch-retro-go/blob/msx_wsv_genesis/tools/binary_png_to_bitarray.py
 */
static const uint8_t IMG_BORDER[] = {
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  0xFF, 0xFF, 0x00, 0x01, 0xFF, 0xFF, 0x80,
    0x03,  0xFF, 0xFF, 0xC0, 0x07, 0xFF, 0xFF, 0xE0,
    0x0F,  0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0xF0,
    0x0F,  0x00, 0x00, 0xF0, 0x0F, 0x03, 0xC0, 0xF0,
    0x0F,  0x07, 0xE0, 0xF0, 0x0F, 0x0F, 0xF0, 0xF0,
    0x0F,  0x1F, 0xF8, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x07,  0xFC, 0x78, 0xF0, 0x03, 0xF8, 0x78, 0xF0,
    0x01,  0xF0, 0x78, 0xF0, 0x00, 0xE0, 0x78, 0xF0,
    0x00,  0x00, 0x78, 0xF0, 0x00, 0x00, 0x78, 0xF0,
    0x0F,  0xFF, 0xF8, 0xF0, 0x07, 0xFF, 0xF0, 0xE0,
    0x03,  0xFF, 0xE0, 0xC0, 0x01, 0xFF, 0xC0, 0x80,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01,  0x03, 0xFF, 0x80, 0x03, 0x07, 0xFF, 0xC0,
    0x07,  0x0F, 0xFF, 0xE0, 0x0F, 0x1F, 0xFF, 0xE0,
    0x0F,  0x1E, 0x00, 0xF0, 0x0F, 0x1E, 0x00, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x00, 0xF0, 0x0F, 0x1E, 0x00, 0xF0,
    0x0F,  0x1E, 0x00, 0xF0, 0x0F, 0x1F, 0xF8, 0xF0,
    0x0F,  0x0F, 0xF8, 0xF0, 0x0F, 0x07, 0xF8, 0xF0,
    0x0F,  0x03, 0xF8, 0xF0, 0x0F, 0x00, 0x00, 0xF0,
    0x0F,  0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0xF0,
    0x07,  0xFF, 0xFF, 0xE0, 0x03, 0xFF, 0xFF, 0xC0,
    0x01,  0xFF, 0xFF, 0x80, 0x00, 0xFF, 0xFF, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00,
    0x01,  0xFF, 0xFF, 0x80, 0x03, 0xFF, 0xFF, 0xC0,
    0x07,  0xFF, 0xFF, 0xE0, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0F,  0xE0, 0x07, 0xF0, 0x0F, 0xF0, 0x0F, 0xF0,
    0x0F,  0xF8, 0x1F, 0xF0, 0x0F, 0xFC, 0x3F, 0xF0,
    0x00,  0x1E, 0x78, 0x00, 0x00, 0x1E, 0x78, 0x00,
    0x00,  0x1E, 0x78, 0x00, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xE0,
    0x07,  0xFF, 0xFF, 0xE0, 0x03, 0xFF, 0xFF, 0xC0,
    0x01,  0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x0F, 0x03, 0xC0, 0xF0,
    0x0F,  0x07, 0xE0, 0xF0, 0x0F, 0x0F, 0xF0, 0xF0,
    0x0F,  0x1F, 0xF8, 0xF0, 0x00, 0x1E, 0x78, 0x00,
    0x00,  0x1E, 0x78, 0x00, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x00, 0x00, 0xF0,
    0x0F,  0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0xF0,
    0x07,  0xE0, 0x07, 0xE0, 0x03, 0xF0, 0x0F, 0xC0,
    0x01,  0xF8, 0x1F, 0x80, 0x00, 0xFC, 0x3F, 0x00,
    0x00,  0x1E, 0x78, 0x00, 0x00, 0x1E, 0x78, 0x00,
    0x00,  0x1E, 0x78, 0x00, 0x07, 0xFC, 0x3F, 0xE0,
    0x03,  0xF8, 0x1F, 0xC0, 0x01, 0xF0, 0x0F, 0x80,
    0x00,  0xE0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  0xE0, 0x07, 0x00, 0x01, 0xF0, 0x0F, 0x80,
    0x03,  0xF8, 0x1F, 0xC0, 0x07, 0xFC, 0x3F, 0xE0,
    0x00,  0x1E, 0x78, 0x00, 0x00, 0x1E, 0x78, 0x00,
    0x00,  0x1E, 0x78, 0x00, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x0F, 0xF0, 0xF0, 0x0F, 0x07, 0xE0, 0xF0,
    0x0F,  0x03, 0xC0, 0xF0, 0x0F, 0x01, 0x80, 0xF0,
    0x0F,  0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0xF0,
    0x07,  0xFF, 0xFF, 0xE0, 0x03, 0xFF, 0xFF, 0xC0,
    0x01,  0xFF, 0xFF, 0x80, 0x00, 0xFF, 0xFF, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01,  0x03, 0xFF, 0x80, 0x03, 0x07, 0xFF, 0xC0,
    0x07,  0x0F, 0xFF, 0xE0, 0x0F, 0x1F, 0xFF, 0xF0,
    0x0F,  0x1E, 0x00, 0xF0, 0x0F, 0x1E, 0x00, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x00, 0xF0, 0x0F, 0x1E, 0x00, 0xF0,
    0x0F,  0x1E, 0x00, 0xF0, 0x0F, 0x1F, 0xF8, 0xF0,
    0x0F,  0x0F, 0xF8, 0xF0, 0x0F, 0x07, 0xF8, 0xF0,
    0x0F,  0x03, 0xF8, 0xF0, 0x0F, 0x00, 0x00, 0xF0,
    0x0F,  0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0xF0,
    0x07,  0xFF, 0xFF, 0xE0, 0x03, 0xFF, 0xFF, 0xC0,
    0x01,  0xFF, 0xFF, 0x80, 0x00, 0xFF, 0xFF, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x1F, 0xF8, 0x00,
    0x01,  0x1F, 0xF8, 0x80, 0x03, 0x1F, 0xF8, 0xC0,
    0x07,  0x1F, 0xF8, 0xE0, 0x0F, 0x00, 0x00, 0xF0,
    0x0F,  0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0xF0,
    0x0F,  0x03, 0xC0, 0xF0, 0x0F, 0x07, 0xE0, 0xF0,
    0x0F,  0x0F, 0xF0, 0xF0, 0x0F, 0x1F, 0xF8, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0xFE, 0x7F, 0xF0, 0x07, 0xFC, 0x3F, 0xE0,
    0x03,  0xF8, 0x1F, 0xC0, 0x01, 0xF0, 0x0F, 0x80,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x07, 0xFF, 0xFF, 0xE0,
    0x03,  0xFF, 0xFF, 0xC0, 0x01, 0xFF, 0xFF, 0x80,
    0x00,  0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x01, 0xF0, 0x0F, 0x80,
    0x03,  0xF8, 0x1F, 0xC0, 0x07, 0xFC, 0x3F, 0xE0,
    0x0F,  0xFE, 0x7F, 0xF0, 0x00, 0x1E, 0x78, 0x00,
    0x00,  0x1E, 0x78, 0x00, 0x00, 0xFC, 0x3F, 0x00,
    0x01,  0xF8, 0x1F, 0x80, 0x03, 0xF0, 0x0F, 0xC0,
    0x07,  0xE0, 0x07, 0xE0, 0x0F, 0x00, 0x00, 0xF0,
    0x0F,  0x00, 0x00, 0xF0, 0x0F, 0x00, 0x00, 0xF0,
    0x07,  0xE0, 0x07, 0xE0, 0x03, 0xF0, 0x0F, 0xC0,
    0x01,  0xF8, 0x1F, 0x80, 0x00, 0xFC, 0x3F, 0x00,
    0x00,  0x1E, 0x78, 0x00, 0x00, 0x1E, 0x78, 0x00,
    0x00,  0x1E, 0x78, 0x00, 0x07, 0xFC, 0x3F, 0xE0,
    0x03,  0xF8, 0x1F, 0xC0, 0x01, 0xF0, 0x0F, 0x80,
    0x00,  0xE0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0x80,
    0x03,  0xFF, 0xFF, 0xC0, 0x07, 0xFF, 0xFF, 0xE0,
    0x0F,  0xFF, 0xFF, 0xE0, 0x0F, 0x00, 0x00, 0xF0,
    0x0F,  0x00, 0x00, 0xF0, 0x0F, 0x01, 0x80, 0xF0,
    0x0F,  0x03, 0xC0, 0xF0, 0x0F, 0x07, 0xE0, 0xF0,
    0x0F,  0x0F, 0xF0, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x0F,  0x1E, 0x78, 0xF0, 0x0F, 0x1E, 0x78, 0xF0,
    0x07,  0x1E, 0x78, 0xE0, 0x03, 0x1E, 0x78, 0xC0,
    0x01,  0x1E, 0x78, 0x80, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00,  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void draw_border(pixel_t * fb){
    uint32_t start, bit_index;
    start = 0;
    bit_index = 0;
    for(uint16_t i=0; i < BORDER_HEIGHT; i++){
        uint32_t offset = start + i * GW_LCD_WIDTH;
        for(uint8_t j=0; j < BORDER_WIDTH; j++){
            fb[offset + j] = 
                (IMG_BORDER[bit_index >> 3] << (bit_index & 0x07)) & 0x80 ? BORDER_COLOR_565 : 0x0000;
            bit_index++;
        }
    }
    start = 32 + 256;
    bit_index = 0;
    for(uint16_t i=0; i < BORDER_HEIGHT; i++){
        uint32_t offset = start + i * GW_LCD_WIDTH;
        for(uint8_t j=0; j < BORDER_WIDTH; j++){
            fb[offset + j] = 
                (IMG_BORDER[bit_index >> 3] << (bit_index & 0x07)) & 0x80 ? BORDER_COLOR_565 : 0x0000;
            bit_index++;
        }
    }

}

// These must be multiples of 8
#define IMG_H 24
#define IMG_W 24

static const uint8_t IMG_SPEAKER[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x04, 0x00, 0x30, 0x42, 0x00,
    0x70, 0x21, 0x00, 0xF0, 0x11, 0x7F, 0xF1, 0x09,
    0xFF, 0xF0, 0x89, 0xFF, 0xF0, 0x49, 0xFF, 0xF0,
    0x49, 0xFF, 0xF0, 0x49, 0xFF, 0xF0, 0x49, 0xFF,
    0xF0, 0x49, 0xFF, 0xF0, 0x49, 0x7F, 0xF0, 0x89,
    0x00, 0xF1, 0x09, 0x00, 0x70, 0x11, 0x00, 0x30,
    0x21, 0x00, 0x00, 0x42, 0x00, 0x00, 0x04, 0x00,
    0x00, 0x08, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
};


static const uint8_t IMG_SUN[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x00, 0x08, 0x00, 0x02,
    0x08, 0x20, 0x01, 0x00, 0x40, 0x00, 0x80, 0x80,
    0x00, 0x1C, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x7F,
    0x00, 0x0E, 0x7F, 0x38, 0x00, 0x7F, 0x00, 0x00,
    0x3E, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x80, 0x40,
    0x01, 0x00, 0x20, 0x02, 0x08, 0x10, 0x00, 0x08,
    0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t ROUND[] = {  // This is the top/left of a 8-pixel radius circle
    0b00000001,
    0b00000111,
    0b00001111,
    0b00011111,
    0b00111111,
    0b01111111,
    0b01111111,
    0b11111111,
};

__attribute__((optimize("unroll-loops")))
static void draw_img(pixel_t *fb, const uint8_t *img, uint16_t x, uint16_t y){
    uint16_t idx = 0;
    for(uint8_t i=0; i < IMG_H; i++) {
        for(uint8_t j=0; j < IMG_W; j++) {
            if(img[idx / 8] & (1 << (7 - idx % 8))){
                fb[x + j +  GW_LCD_WIDTH * (y + i)] = OVERLAY_COLOR_565;
            }
            idx++;
        }
    }
}

#define DARKEN_MASK_565 0x7BEF  // Mask off the MSb of each color
#define DARKEN_ADD_565 0x2104  // value of 4-red, 8-green, 4-blue to add back in a little gray, especially on black backgrounds
static inline void darken_pixel(pixel_t *p){
    // Quickly divide all colors by 2
    *p = ((*p >> 1) & DARKEN_MASK_565) + DARKEN_ADD_565;
}

__attribute__((optimize("unroll-loops")))
static void draw_rectangle(pixel_t *fb, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2){
    for(uint16_t i=y1; i < y2; i++){
        for(uint16_t j=x1; j < x2; j++){
            fb[j + GW_LCD_WIDTH * i] = OVERLAY_COLOR_565;
        }
    }
}

__attribute__((optimize("unroll-loops")))
static void draw_darken_rectangle(pixel_t *fb, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2){
    for(uint16_t i=y1; i < y2; i++){
        for(uint16_t j=x1; j < x2; j++){
            darken_pixel(&fb[j + GW_LCD_WIDTH * i]);
        }
    }
}

__attribute__((optimize("unroll-loops")))
static void draw_darken_rounded_rectangle(pixel_t *fb, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2){
    // *1 is inclusive, *2 is exclusive
    uint16_t h = y2 - y1;
    uint16_t w = x2 - x1;
    if (w < 16 || h < 16) {
        // Draw not rounded rectangle
        draw_darken_rectangle(fb, x1, y1, x2, y2);
        return;
    }

    // Draw upper left round
    for(uint8_t i=0; i < 8; i++) for(uint8_t j=0; j < 8; j++)
        if(ROUND[i] & (1 << (7 - j))) darken_pixel(&fb[x1 + j + GW_LCD_WIDTH * (y1 + i)]);

    // Draw upper right round
    for(uint8_t i=0; i < 8; i++) for(uint8_t j=0; j < 8; j++)
        if(ROUND[i] & (1 << (7 - j))) darken_pixel(&fb[x2 - j - 1 + GW_LCD_WIDTH * (y1 + i)]);

    // Draw lower left round
    for(uint8_t i=0; i < 8; i++) for(uint8_t j=0; j < 8; j++)
        if(ROUND[i] & (1 << (7 - j))) darken_pixel(&fb[x1 + j + GW_LCD_WIDTH * (y2 - i - 1)]);

    // Draw lower right round
    for(uint8_t i=0; i < 8; i++) for(uint8_t j=0; j < 8; j++)
        if(ROUND[i] & (1 <<  (7 - j))) darken_pixel(&fb[x2 - j - 1 + GW_LCD_WIDTH * (y2 - i - 1)]);

    // Draw upper rectangle
    for(uint16_t i=x1+8; i < x2 - 8; i++) for(uint8_t j=0; j < 8; j++)
        darken_pixel(&fb[ i + GW_LCD_WIDTH * (y1 + j)]);

    // Draw central rectangle
    for(uint16_t i=x1; i < x2; i++) for(uint16_t j=y1+8; j < y2-8; j++)
        darken_pixel(&fb[i+GW_LCD_WIDTH * j]);

    // Draw lower rectangle
    for(uint16_t i=x1+8; i < x2 - 8; i++) for(uint8_t j=0; j < 8; j++)
        darken_pixel(&fb[ i + GW_LCD_WIDTH * (y2 - j - 1)]);
}

#define INGAME_OVERLAY_X 265
#define INGAME_OVERLAY_Y 10
#define INGAME_OVERLAY_BARS_H 128
#define INGAME_OVERLAY_W 39
#define INGAME_OVERLAY_BORDER 4
#define INGAME_OVERLAY_BOX_GAP 2

#define INGAME_OVERLAY_BARS_W INGAME_OVERLAY_W
#define INGAME_OVERLAY_IMG_H  (IMG_H + 2 * INGAME_OVERLAY_BORDER)  // For when only an image is showing

#define INGAME_OVERLAY_BOX_W (INGAME_OVERLAY_BARS_W - (2 * INGAME_OVERLAY_BORDER) - 6)
#define INGAME_OVERLAY_BOX_X (INGAME_OVERLAY_X + ((INGAME_OVERLAY_BARS_W - INGAME_OVERLAY_BOX_W) / 2))
#define INGAME_OVERLAY_BOX_Y (INGAME_OVERLAY_Y + INGAME_OVERLAY_BORDER + 3)

// For when only an image is shown
#define INGAME_OVERLAY_IMG_X (INGAME_OVERLAY_X + ((INGAME_OVERLAY_BARS_W - IMG_W) / 2))
#define INGAME_OVERLAY_IMG_Y (INGAME_OVERLAY_Y + INGAME_OVERLAY_BORDER)

// Places the image at the bottom for bars-related overlay (volume, brightness)
#define INGAME_OVERLAY_BARS_IMG_X INGAME_OVERLAY_IMG_X
#define INGAME_OVERLAY_BARS_IMG_Y (INGAME_OVERLAY_Y + INGAME_OVERLAY_BARS_H - IMG_H - INGAME_OVERLAY_BORDER)

#define DARKEN_IMG_ONLY() draw_darken_rounded_rectangle(fb, \
                    INGAME_OVERLAY_X, \
                    INGAME_OVERLAY_Y, \
                    INGAME_OVERLAY_X + INGAME_OVERLAY_BARS_W, \
                    INGAME_OVERLAY_Y + INGAME_OVERLAY_IMG_H)


static uint8_t box_height(uint8_t n) {
    return ((INGAME_OVERLAY_BARS_IMG_Y - INGAME_OVERLAY_BOX_Y) / n) - INGAME_OVERLAY_BOX_GAP;
}

void draw_ingame_overlay(pixel_t *fb, ingame_overlay_t overlay){
    uint8_t bh;
    uint16_t by = INGAME_OVERLAY_BOX_Y;

    switch(overlay)
    {
        case INGAME_OVERLAY_NONE:
            break;
        case INGAME_OVERLAY_VOLUME:
            bh = box_height(AUDIO_VOLUME_MAX);

            draw_darken_rounded_rectangle(fb,
                    INGAME_OVERLAY_X,
                    INGAME_OVERLAY_Y,
                    INGAME_OVERLAY_X + INGAME_OVERLAY_BARS_W,
                    INGAME_OVERLAY_Y + INGAME_OVERLAY_BARS_H);
            draw_img(fb, IMG_SPEAKER, INGAME_OVERLAY_BARS_IMG_X, INGAME_OVERLAY_BARS_IMG_Y);

            uint8_t volume = settings_Volume_get();
            for(int8_t i=AUDIO_VOLUME_MAX; i > 0; i--){
                if(i <= volume)
                    draw_rectangle(fb,
                            INGAME_OVERLAY_BOX_X,
                            by,
                            INGAME_OVERLAY_BOX_X + INGAME_OVERLAY_BOX_W,
                            by + bh);
                else
                    draw_darken_rectangle(fb,
                            INGAME_OVERLAY_BOX_X,
                            by,
                            INGAME_OVERLAY_BOX_X + INGAME_OVERLAY_BOX_W,
                            by + bh);

                by += bh + INGAME_OVERLAY_BOX_GAP;
            }
            break;
        case INGAME_OVERLAY_BRIGHTNESS:
            bh = box_height(BRIGHTNESS_MAX);

            draw_darken_rounded_rectangle(fb,
                    INGAME_OVERLAY_X,
                    INGAME_OVERLAY_Y,
                    INGAME_OVERLAY_X + INGAME_OVERLAY_BARS_W,
                    INGAME_OVERLAY_Y + INGAME_OVERLAY_BARS_H);
            draw_img(fb, IMG_SUN, INGAME_OVERLAY_BARS_IMG_X, INGAME_OVERLAY_BARS_IMG_Y);

            uint8_t brightness = settings_Backlight_get();
            for(int8_t i=BRIGHTNESS_MAX; i > 0; i--){
                if(i <= brightness)
                    draw_rectangle(fb,
                            INGAME_OVERLAY_BOX_X,
                            by,
                            INGAME_OVERLAY_BOX_X + INGAME_OVERLAY_BOX_W,
                            by + bh);
                else
                    draw_darken_rectangle(fb,
                            INGAME_OVERLAY_BOX_X,
                            by,
                            INGAME_OVERLAY_BOX_X + INGAME_OVERLAY_BOX_W,
                            by + bh);

                by += bh + INGAME_OVERLAY_BOX_GAP;
            }
            break;
    }
}


/*void odroid_display_write_rect(short left, short top, short width, short height, short stride, const uint16_t* buffer)
{
    pixel_t *dest = lcd_get_active_buffer();

    for (short y = 0; y < height; y++) {
        pixel_t *dest_row = &dest[(y + top) * GW_LCD_WIDTH + left];
        memcpy(dest_row, &buffer[y * stride], width * sizeof(pixel_t));
    }
}*/

// Same as odroid_display_write_rect but stride is assumed to be width (for backwards compat)
/*void odroid_display_write(short left, short top, short width, short height, const uint16_t* buffer)
{
    odroid_display_write_rect(left, top, width, height, width, buffer);
}*/

int odroid_overlay_get_font_width()
{
    return 8;
}

int odroid_overlay_draw_text_line(pixel_t *fb, uint16_t x_pos, uint16_t y_pos, uint16_t width, const char *text, uint16_t color, uint16_t color_bg)
{
    int font_height = 8; //odroid_overlay_get_font_size();
    int font_width = 8; //odroid_overlay_get_font_width();
    int x_offset = x_pos;//0;
    float scale = 1; //(float)font_height / 8;
    int text_len = strlen(text);

    for (int i = 0; i < (width / font_width); i++)
    {
        const char *glyph = font8x8_basic[(i < text_len) ? text[i] : ' '];
        for (int y = 0; y < font_height; y++)
        {
            int offset = x_offset + (width * (y + y_pos));
            for (int x = 0; x < 8; x++)
            {
                /*overlay_buffer*/fb[offset + x] = (glyph[(int)(y/scale)] & (1 << x)) ? color : color_bg;
            }
        }
        x_offset += font_width;
    }

    //odroid_display_write(x_pos, y_pos, width, font_height, /*overlay_buffer*/fb);

    return font_height;
}

int odroid_overlay_draw_text(pixel_t *fb, uint16_t x_pos, uint16_t y_pos, uint16_t width, const char *text, uint16_t color, uint16_t color_bg)
{
    int text_len = 1;
    int height = 0;

    if (text == NULL || text[0] == 0) {
        text = " ";
    }

    text_len = strlen(text);

    if (width < 1) {
        width = text_len * odroid_overlay_get_font_width();
    }

    if (width > (320/*ODROID_SCREEN_WIDTH*/ - x_pos)) {
        width = (320/*ODROID_SCREEN_WIDTH*/ - x_pos);
    }

    int line_len = width / odroid_overlay_get_font_width();
    char buffer[320/*ODROID_SCREEN_WIDTH*/ / 8 + 1];

    for (int pos = 0; pos < text_len;)
    {
        sprintf(buffer, "%.*s", line_len, text + pos);
        if (strchr(buffer, '\n')) *(strchr(buffer, '\n')) = 0;
        height += odroid_overlay_draw_text_line(fb, x_pos, y_pos + height, width, buffer, color, color_bg);
        pos += strlen(buffer);
        if (*(text + pos) == 0 || *(text + pos) == '\n') pos++;
    }

    return height;
}

cpumon_stats_t cpumon_stats = {0};

//uint32_t audioBuffer[AUDIO_BUFFER_LENGTH];
uint32_t audio_mute;


int16_t pendingSamples = 0;
int16_t audiobuffer[AUDIO_BUFFER_LENGTH];
int16_t audiobuffer_dma[AUDIO_BUFFER_LENGTH * 2];

dma_transfer_state_t dma_state;
uint32_t dma_counter;

const uint8_t volume_tbl[AUDIO_VOLUME_MAX + 1] = {
    (uint8_t)(UINT8_MAX * 0.00f),
    (uint8_t)(UINT8_MAX * 0.06f),
    (uint8_t)(UINT8_MAX * 0.125f),
    (uint8_t)(UINT8_MAX * 0.187f),
    (uint8_t)(UINT8_MAX * 0.25f),
    (uint8_t)(UINT8_MAX * 0.35f),
    (uint8_t)(UINT8_MAX * 0.42f),
    (uint8_t)(UINT8_MAX * 0.60f),
    (uint8_t)(UINT8_MAX * 0.80f),
    (uint8_t)(UINT8_MAX * 1.00f),
};

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    dma_counter++;
    dma_state = DMA_TRANSFER_STATE_HF;
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    dma_counter++;
    dma_state = DMA_TRANSFER_STATE_TC;
}


// TODO: Move to own file
void odroid_audio_mute(bool mute)
{
    if (mute) {
        for (int i = 0; i < sizeof(audiobuffer_dma) / sizeof(audiobuffer_dma[0]); i++) {
            audiobuffer_dma[i] = 0;
        }
    }

    audio_mute = mute;
}

common_emu_state_t common_emu_state = {
    .frame_time_10us = (uint16_t)(100000 / 60 + 0.5f),  // Reasonable default of 60FPS if not explicitly configured.
};


static runtime_stats_t statistics;
static runtime_counters_t counters;
static uint32_t skip;


void odroid_system_tick(uint32_t skippedFrame, uint32_t fullFrame, uint32_t busyTime)
{
    if (skippedFrame) counters.skippedFrames++;
    else if (fullFrame) counters.fullFrames++;
    counters.totalFrames++;

    // Because the emulator may have a different time perception, let's just skip the first report.
    if (skip) {
        skip = 0;
    } else {
        counters.busyTime += busyTime;
    }

    statistics.lastTickTime = HAL_GetTick();
}

runtime_stats_t odroid_system_get_stats()
{
    float tickTime = (HAL_GetTick() - counters.resetTime);

    //statistics.battery = odroid_input_read_battery();
    statistics.busyPercent = counters.busyTime / tickTime * 100.f;
    statistics.skippedFPS = counters.skippedFrames / (tickTime / 1000.f);
    statistics.totalFPS = counters.totalFrames / (tickTime / 1000.f);

    skip = 1;
    counters.busyTime = 0;
    counters.totalFrames = 0;
    counters.skippedFrames = 0;
    counters.resetTime = HAL_GetTick();

    return statistics;
}

bool common_emu_frame_loop(void){
    //rg_app_desc_t *app = odroid_system_get_app();
    static int32_t frame_integrator = 0;
    int16_t frame_time_10us = common_emu_state.frame_time_10us;
    int16_t elapsed_10us = 100 * (HAL_GetTick() - common_emu_state.last_sync_time);
    bool draw_frame = common_emu_state.skip_frames < 2;

    if( !cpumon_stats.busy_ms ) cpumon_busy();
    odroid_system_tick(!draw_frame, 0, cpumon_stats.busy_ms);
    cpumon_reset();

    common_emu_state.pause_frames = 0;
    common_emu_state.skip_frames = 0;

    common_emu_state.last_sync_time = HAL_GetTick();

    if(common_emu_state.startup_frames < 3) {
        common_emu_state.startup_frames++;
        return true;
    }

    frame_integrator += (elapsed_10us - frame_time_10us);
    if(frame_integrator > frame_time_10us << 1) common_emu_state.skip_frames = 2;
    else if(frame_integrator > frame_time_10us) common_emu_state.skip_frames = 1;
    else if(frame_integrator < -frame_time_10us) common_emu_state.pause_frames = 1;
    common_emu_state.skipped_frames += common_emu_state.skip_frames;

    return draw_frame;
}


static void cpumon_common(bool sleep){
    uint32_t t0 = HAL_GetTick();    //get_elapsed_time();
    if(cpumon_stats.last_busy){
        cpumon_stats.busy_ms += t0 - cpumon_stats.last_busy;
    }
    else{
        cpumon_stats.busy_ms = 0;
    }
    if(sleep) __WFI();
    uint32_t t1 = HAL_GetTick();    //get_elapsed_time();
    cpumon_stats.last_busy = t1;
    cpumon_stats.sleep_ms += t1 - t0;
}


void cpumon_busy(void){
    cpumon_common(false);
}

void cpumon_sleep(void){
    cpumon_common(true);
}

void cpumon_reset(void){
    cpumon_stats.busy_ms = 0;
    cpumon_stats.sleep_ms = 0;
}

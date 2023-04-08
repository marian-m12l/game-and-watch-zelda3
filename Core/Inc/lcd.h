#ifndef _LCD_H_
#define _LCD_H_

#include "stm32h7xx_hal.h"
#include <stdint.h>

extern uint16_t framebuffer[320 * 240];

#define GFX_MAX_WIDTH 320

void lcd_init(SPI_HandleTypeDef *spi, LTDC_HandleTypeDef *ltdc);
void lcd_deinit(SPI_HandleTypeDef *spi);
void lcd_backlight_on();
void lcd_backlight_off();
void lcd_backlight_set(uint8_t brightness);
#endif

#ifndef _LCD_H_
#define _LCD_H_

#include "stm32h7xx_hal.h"
#include <stdint.h>

#define GW_LCD_WIDTH 320
#define GW_LCD_HEIGHT 240

extern uint16_t framebuffer[GW_LCD_WIDTH * GW_LCD_HEIGHT];

void lcd_init(SPI_HandleTypeDef *spi, LTDC_HandleTypeDef *ltdc);
void lcd_deinit(SPI_HandleTypeDef *spi);
void lcd_backlight_on();
void lcd_backlight_off();
void lcd_backlight_set(uint8_t brightness);
#endif

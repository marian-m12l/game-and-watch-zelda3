#include "lcd.h"
#include "xip_from_flash.h"

// Executed from ext flash
void xip_from_flash() {
  lcd_fill_framebuffer(0x1f, 0x00, 0x00); // Red
  HAL_Delay(250);
  lcd_fill_framebuffer(0x0f, 0x00, 0x00); // Lesser red
  HAL_Delay(250);
}

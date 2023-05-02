/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdint.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32h7xx_hal.h"
#include "buttons.h"  // FIXME replace with gw_buttons ???
#include "lcd.h"      // FIXME replace with gw_lcd ??? handle dual framebuffer ???

#include "gw_flash.h"

#include "gw_linker.h"

// FIXME ??? #include "porting.h"
#include "zelda_assets_in_intflash.h"
#include "zelda_assets_in_ram.h"
#include "zelda_assets_in_extflash.h"

#include "zelda3/assets.h"
#include "zelda3/config.h"
#include "zelda3/snes/ppu.h"
#include "zelda3/types.h"
#include "zelda3/zelda_rtl.h"

#include "common.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#if !defined(GNW_TARGET_ZELDA)
#define GNW_TARGET_ZELDA 0
#endif /* GNW_TARGET_ZELDA */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/


bool wdog_enabled = false;
DAC_HandleTypeDef hdac1;
DAC_HandleTypeDef hdac2;

LTDC_HandleTypeDef hltdc;

OSPI_HandleTypeDef hospi1;

SAI_HandleTypeDef hsai_BlockA1;
DMA_HandleTypeDef hdma_sai1_a;

TIM_HandleTypeDef htim1;

SPI_HandleTypeDef hspi2;

/* USER CODE BEGIN PV */


//char logbuf[1024 * 4] PERSISTENT __attribute__((aligned(4)));
//uint32_t log_idx PERSISTENT;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_DAC1_Init(void);
static void MX_DAC2_Init(void);
static void MX_LTDC_Init(void);
static void MX_SPI2_Init(void);
static void MX_OCTOSPI1_Init(void);
static void MX_SAI1_Init(void);
static void MX_TIM1_Init(void);
static void MX_NVIC_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
WWDG_HandleTypeDef hwwdg1;

ingame_overlay_t ingame_overlay = INGAME_OVERLAY_NONE;
uint32_t overlay_start_ms = 0;
#define OVERLAY_DURATION_MS 5000
#define POWER_BUTTON_DELAY_MS 300

const char *fault_list[] = {
  [BSOD_ABORT] = "Assert",
  [BSOD_HARDFAULT] = "Hardfault",
  [BSOD_MEMFAULT] = "Memfault",
  [BSOD_BUSFAULT] = "Busfault",
  [BSOD_USAGEFAULT] = "Usagefault",
  [BSOD_WATCHDOG] = "Watchdog",
  [BSOD_OTHER] = "Other",
};

/**
  * @brief WWDG1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_WWDG1_Init(void)
{

  /* USER CODE BEGIN WWDG1_Init 0 */

  /* USER CODE END WWDG1_Init 0 */

  /* USER CODE BEGIN WWDG1_Init 1 */

  /* USER CODE END WWDG1_Init 1 */
  hwwdg1.Instance = WWDG1;
  hwwdg1.Init.Prescaler = WWDG_PRESCALER_128;
  hwwdg1.Init.Window = 127;
  hwwdg1.Init.Counter = 127;
  hwwdg1.Init.EWIMode = WWDG_EWI_ENABLE;
  if (HAL_WWDG_Init(&hwwdg1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN WWDG1_Init 2 */

  /* USER CODE END WWDG1_Init 2 */

}
static void wdog_enable()
{
  MX_WWDG1_Init();
  wdog_enabled = true;
}

static void wdog_refresh()
{
  if (wdog_enabled) {
    HAL_WWDG_Refresh(&hwwdg1);
  }
}

__attribute__((optimize("-O0"))) void BSOD(BSOD_t fault, uint32_t pc, uint32_t lr)
{
  char msg[256];
  size_t i = 0;
  char *start;
  char *end;
  char *line;
  int y = 2*8;

  __disable_irq();

  snprintf(msg, sizeof(msg), "FATAL EXCEPTION: %s\nPC=0x%08lx LR=0x%08lx\n", fault_list[fault], pc, lr);
  
  lcd_fill_framebuffer(0x00, 0x00, 0x1f); // Blue

/*
  lcd_sync();
  lcd_reset_active_buffer();
  odroid_display_set_backlight(ODROID_BACKLIGHT_LEVEL6);

  odroid_overlay_draw_text(0, 0, GW_LCD_WIDTH, msg, C_RED, C_BLUE);

  // Print each line from the log in reverse
  end = &logbuf[strnlen(logbuf, sizeof(logbuf)) - 1];
  while (y < GW_LCD_HEIGHT) {
    // Max 28 lines
    if (i++ >= 28) {
      break;
    }

    // Find the last line start not beyond end (inefficient but simple solution)
    start = logbuf;
    while (start < end) {
      line = start;
      start = strnstr(start, "\n", end - start);
      if (start == NULL) {
        break;
      } else {
        // Move past \n
        start += 1;
      }
    }

    // Terminate the previous line
    end[0] = '\x00';

    end = line;

    y += odroid_overlay_draw_text(0, y, GW_LCD_WIDTH, line, C_WHITE, C_BLUE);

    if (line == logbuf) {
      // No more lines to print
      break;
    }
  }
*/

  // Wait for a button press (allows a user to hold and release a button when the BSOD occurs)
  uint32_t old_buttons = buttons_get();
  while ((buttons_get() == 0 || (buttons_get() == old_buttons))) {
    wdog_refresh();
  }

  HAL_NVIC_SystemReset();

  // Does not return
  while (1) {
    __NOP();
  }
}


// TODO Handle power off / deep sleep

void GW_EnterDeepSleep(void)
{
  // Stop SAI DMA (audio)
  HAL_SAI_DMAStop(&hsai_BlockA1);

  // Enable wakup by PIN1, the power button
  HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN1_LOW);

  lcd_backlight_off();

  // Deinit the LCD, save power.
  lcd_deinit(&hspi2);

  // Leave a trace in RAM that we entered standby mode
  //boot_magic = BOOT_MAGIC_STANDBY;

  // Delay 500ms to give us a chance to attach a debugger in case
  // we end up in a suspend-loop.
  for (int i = 0; i < 10; i++) {
      wdog_refresh();
      HAL_Delay(50);
  }

  HAL_PWR_EnterSTANDBYMode();

  // Execution stops here, this function will not return
  while(1) {
    // If we for some reason survive until here, let's just reboot
    HAL_NVIC_SystemReset();
  }

}


/*int _write(int file, char *ptr, int len)
{
  if (log_idx + len + 1 > sizeof(logbuf)) {
    log_idx = 0;
  }

  memcpy(&logbuf[log_idx], ptr, len);
  log_idx += len;
  logbuf[log_idx + 1] = '\0';

  return len;
}*/


// Workaround for being able to run with -D_FORTIFY_SOURCE=1
static void memcpy_no_check(uint32_t *dst, uint32_t *src, size_t len)
{
  assert((len & 0b11) == 0);

  uint32_t *end = dst + len / 4;
  while (dst != end) {
    *(dst++) = *(src++);
  }
}


static uint8 g_paused, g_turbo, g_replay_turbo = true, g_cursor = true;
static uint8 g_current_window_scale;
static uint8 g_gamepad_buttons;
static int g_input1_state;
static bool g_display_perf;
static int g_curr_fps;
static int g_ppu_render_flags = kPpuRenderFlags_NewRenderer | kPpuRenderFlags_Height240;
static int g_snes_width, g_snes_height;
//static int g_sdl_audio_mixer_volume = SDL_MIX_MAXVOLUME;
//static struct RendererFuncs g_renderer_funcs;
static uint32 g_gamepad_modifiers;
static uint16 g_gamepad_last_cmd[kGamepadBtn_Count];

static uint32 frameCtr = 0;
static uint32 renderedFrameCtr = 0;


#define AUDIO_SAMPLE_RATE   (16000)   // SAI Sample rate
#if LIMIT_30FPS != 0
#define FRAMERATE 30
#else
#define FRAMERATE 60
#endif /* LIMIT_30FPS */
#define AUDIO_BUFFER_LENGTH 534 // (AUDIO_SAMPLE_RATE / FRAMERATE)  // SNES is 60 fps FIXME limited to 30 fps
#define AUDIO_BUFFER_LENGTH_DMA (2 * AUDIO_BUFFER_LENGTH) // ((2 * AUDIO_SAMPLE_RATE) / FRAMERATE)  // DMA buffer contains 2 frames worth of audio samples in a ring buffer


void pcm_submit() {
    int32_t factor = volume_tbl[volume];
    size_t offset = (dma_state == DMA_TRANSFER_STATE_HF) ? 0 : AUDIO_BUFFER_LENGTH;

    // Handle mute
    if (volume == AUDIO_VOLUME_MIN) {
        for (int i = 0; i < AUDIO_BUFFER_LENGTH; i++) {
            audiobuffer_dma[i + offset] = 0;
        }
    } else {
        for (int i = 0; i < AUDIO_BUFFER_LENGTH; i++) {
            int32_t sample = audiobuffer[i];
            audiobuffer_dma[i + offset] = (sample * factor) >> 8;
        }
    }
}


void NORETURN Die(const char *error) {
//#if defined(NDEBUG) && defined(_WIN32)
//  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kWindowTitle, error, NULL);
//#endif
  printf(stderr, "Error: %s\n", error);
  //exit(1);
  Error_Handler();
}


/* set audio frequency  */
static void set_audio_frequency(uint32_t frequency)
{

    /** reconfig PLL2 and SAI */
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_SAI1;

    if (frequency == 16000)
    {

        PeriphClkInitStruct.PLL2.PLL2M = 25;
        PeriphClkInitStruct.PLL2.PLL2N = 128;
        PeriphClkInitStruct.PLL2.PLL2P = 10;
        PeriphClkInitStruct.PLL2.PLL2Q = 2;
        PeriphClkInitStruct.PLL2.PLL2R = 5;
        PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
        PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
        PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
    }
    /* Reconfigure on the fly PLL2 */
    /* config to get 32768Hz */
    /* The audio clock frequency is derived directly */
    /* SAI mode is MCKDIV mode */
    else if (frequency == 32768)
    {

        PeriphClkInitStruct.PLL2.PLL2M = 25;
        PeriphClkInitStruct.PLL2.PLL2N = 196;
        PeriphClkInitStruct.PLL2.PLL2P = 10;
        PeriphClkInitStruct.PLL2.PLL2Q = 2;
        PeriphClkInitStruct.PLL2.PLL2R = 5;
        PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
        PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
        PeriphClkInitStruct.PLL2.PLL2FRACN = 5000;

    /* config to get 48KHz and multiple */
    /* SAI mode is in standard frequency mode */
    }
    else
    {

        PeriphClkInitStruct.PLL2.PLL2M = 25;
        PeriphClkInitStruct.PLL2.PLL2N = 192;
        PeriphClkInitStruct.PLL2.PLL2P = 5;
        PeriphClkInitStruct.PLL2.PLL2Q = 2;
        PeriphClkInitStruct.PLL2.PLL2R = 5;
        PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
        PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
        PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
    }

    // keep PLL3 unchanged
    PeriphClkInitStruct.PLL3.PLL3M = 4;
    PeriphClkInitStruct.PLL3.PLL3N = 9;
    PeriphClkInitStruct.PLL3.PLL3P = 2;
    PeriphClkInitStruct.PLL3.PLL3Q = 2;
    PeriphClkInitStruct.PLL3.PLL3R = 24;
    PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
    PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
    PeriphClkInitStruct.PLL3.PLL3FRACN = 0;

    PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
    PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* remove the current configuration */
    HAL_SAI_DeInit(&hsai_BlockA1);

    /* Set Audio sample rate at 32768Hz using MCKDIV mode */
    if (frequency == 32768)
    {

        hsai_BlockA1.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV;
        hsai_BlockA1.Init.Mckdiv = 6;

        /* config to get 48KHz and other standard values */
        /*
    SAI_AUDIO_FREQUENCY_192K      192000U
    SAI_AUDIO_FREQUENCY_96K        96000U
    SAI_AUDIO_FREQUENCY_48K        48000U
    SAI_AUDIO_FREQUENCY_44K        44100U
    SAI_AUDIO_FREQUENCY_32K        32000U
    SAI_AUDIO_FREQUENCY_22K        22050U
    SAI_AUDIO_FREQUENCY_16K        16000U
    SAI_AUDIO_FREQUENCY_11K        11025U
    SAI_AUDIO_FREQUENCY_8K          8000U
    */

    /* Set Audio sample rate at various standard frequencies using AudioFrequency mode */
    } else {
        /* default value 48KHz */
        hsai_BlockA1.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;

        /* check from the different possible values */
        if ((frequency == SAI_AUDIO_FREQUENCY_192K) ||
            (frequency == SAI_AUDIO_FREQUENCY_96K) ||
            (frequency == SAI_AUDIO_FREQUENCY_48K) ||
            (frequency == SAI_AUDIO_FREQUENCY_44K) ||
            (frequency == SAI_AUDIO_FREQUENCY_32K) ||
            (frequency == SAI_AUDIO_FREQUENCY_22K) ||
            (frequency == SAI_AUDIO_FREQUENCY_16K) ||
            (frequency == SAI_AUDIO_FREQUENCY_11K) ||
            (frequency == SAI_AUDIO_FREQUENCY_8K))
            hsai_BlockA1.Init.AudioFrequency = frequency;
    }

    /* apply the new configuration */
    HAL_SAI_Init(&hsai_BlockA1);
}


const uint8 *g_asset_ptrs[kNumberOfAssets];
uint32 g_asset_sizes[kNumberOfAssets];

static void LoadAssetsChunk(size_t length, uint8* data) {
  uint32 number_of_assets = *(uint32 *)(data);
  uint32 offset = 4 + number_of_assets * 8;
  for (size_t i = 0; i < number_of_assets; i++) {
    uint32 index = *(uint32 *)(data + 4 + i * 8);
    uint32 size = *(uint32 *)(data + 4 + i * 8 + 4);
    offset = (offset + 3) & ~3;
    if ((uint64)offset + size > length)
      Die("Assets file corruption");
    g_asset_sizes[index] = size;
    g_asset_ptrs[index] = data + offset;
    offset += size;
  }
}

static void LoadAssets() {
  // Load some assets with assets in extflash
  LoadAssetsChunk(zelda_extflash_assets_length, zelda_extflash_assets);

  // Load some assets with assets in intflash
  LoadAssetsChunk(zelda_intflash_assets_length, zelda_intflash_assets);

  // Load some assets with assets in ram
  LoadAssetsChunk(zelda_ram_assets_length, zelda_ram_assets);

  // Make sure all assets were loaded
  for (size_t i = 0; i < kNumberOfAssets; i++) {
    if (g_asset_ptrs[i] == 0) {
      Die("Missing asset");
    }
  }

}


static void DrawPpuFrameWithPerf() {
  /*int render_scale = PpuGetCurrentRenderScale(g_zenv.ppu, g_ppu_render_flags);*/
  wdog_refresh();

  uint8 *pixel_buffer = framebuffer + 32;    // Start 32 pixels from left
  int pitch = 320 * 2; // FIXME WIDTH * BPP; // FIXME 0;

  //ZeldaDrawPpuFrame(pixel_buffer, pitch, g_ppu_render_flags); // FIXME SHOULD DRAW RGB565 !!!

  static runtime_stats_t stats;
  static bool statsInit = false;

  static float history[64], average;
  static int history_pos;
  uint32 before = HAL_GetTick();
  ZeldaDrawPpuFrame(pixel_buffer, pitch, g_ppu_render_flags | RENDER_STEP_FLAGS | ((renderedFrameCtr & 0xf) << 24));
  uint32 after = HAL_GetTick();

  // Draw borders
  draw_border(framebuffer);

  if(after - overlay_start_ms < OVERLAY_DURATION_MS){
    draw_ingame_overlay(framebuffer, ingame_overlay);
  }
  else{
    ingame_overlay = INGAME_OVERLAY_NONE;
  }
  /* PERFORMANCE STUFF */
  float v = (double)1000.0f / (after - before);
  average += v - history[history_pos];
  history[history_pos] = v;
  history_pos = (history_pos + 1) & 63;
  g_curr_fps = average * (1.0f / 64);

#if RENDER_FPS
  // Render fps with dots
  for (uint8_t y = 1; y<=60; y++) {
    framebuffer[y*2*320+300] = (y <= g_curr_fps ? 0x07e0 : 0xf800);
  }
  if (!statsInit || (frameCtr % FRAMERATE) == 0) {
    stats = odroid_system_get_stats();
    statsInit = true;
  }
  for (uint8_t y = 1; y<=FRAMERATE; y++) {
    framebuffer[y*2*320+302] = (y <= (stats.totalFPS - stats.skippedFPS) ? 0x07e0 : 0xf800);
  }
  for (uint8_t y = 1; y<=stats.skippedFPS; y++) {
    framebuffer[y*2*320+303] = 0xffff;
  }

  // Render audio volume
  for (uint8_t y = 1; y<=AUDIO_VOLUME_MAX; y++) {
    framebuffer[y*2*320+305] = (y <= volume ? 0x07e0 : 0x7bef);
  }

  // Render brightness level
  for (uint8_t y = 1; y<=BRIGHTNESS_MAX; y++) {
    framebuffer[y*2*320+310] = (y <= brightness ? 0x07e0 : 0x7bef);
  }

  // Render frame counter with dots
  /*memset(&framebuffer[235*320], 0, sizeof(uint16_t)*320*5);
  for (uint16_t x = 1; x<=(renderedFrameCtr%(160*5)); x++) {
    framebuffer[235*320+x*2] = 0x07e0;  // FIXME WIDTH
  }*/

  // Render overclocking level with dots
  for (uint16_t x = 0; x<=OVERCLOCK; x++) {
    framebuffer[2*320+5+x*2] = 0x07e0;
  }
#endif
}


void ZeldaApuLock() {
}

void ZeldaApuUnlock() {
}


static void HandleCommand(uint32 j, bool pressed) {
  if (j <= kKeys_Controls_Last) {
    static const uint8 kKbdRemap[] = { 0, 4, 5, 6, 7, 2, 3, 8, 0, 9, 1, 10, 11 };
    if (pressed)
      g_input1_state |= 1 << kKbdRemap[j];
    else
      g_input1_state &= ~(1 << kKbdRemap[j]);
    return;
  }

  if (j == kKeys_Turbo) {
    g_turbo = pressed;
    return;
  }
}


void store_erase(const uint8_t *flash_ptr, uint32_t size)
{
  // Only allow addresses in the areas meant for erasing and writing.
  assert(
    ((flash_ptr >= &__SAVEFLASH_START__)   && ((flash_ptr + size) <= &__SAVEFLASH_END__)) ||
    ((flash_ptr >= &__configflash_start__) && ((flash_ptr + size) <= &__configflash_end__)) ||
    ((flash_ptr >= &__fbflash_start__) && ((flash_ptr + size) <= &__fbflash_end__))
  );

  // Convert mem mapped pointer to flash address
  uint32_t save_address = flash_ptr - &__EXTFLASH_BASE__;

  // Only allow 4kB aligned pointers
  assert((save_address & (4*1024 - 1)) == 0);

  // Round size up to nearest 4K
  if ((size & 0xfff) != 0) {
    size += 0x1000 - (size & 0xfff);
  }

  OSPI_DisableMemoryMappedMode();
  OSPI_EraseSync(save_address, size);
  OSPI_EnableMemoryMappedMode();
}

void store_save(const uint8_t *flash_ptr, const uint8_t *data, size_t size)
{
  // Convert mem mapped pointer to flash address
  uint32_t save_address = flash_ptr - &__EXTFLASH_BASE__;

  // Only allow 4kB aligned pointers
  assert((save_address & (4*1024 - 1)) == 0);

  int diff = memcmp((void*)flash_ptr, data, size);
  if (diff == 0) {
    return;
  }

  store_erase(flash_ptr, size);

  OSPI_DisableMemoryMappedMode();
  OSPI_Program(save_address, data, size);
  OSPI_EnableMemoryMappedMode();
}

// TODO In header file??
uint8_t SAVE_SRAM_EXTFLASH[8192]  __attribute__((section (".saveflash"))) __attribute__((aligned(4096)));

uint8_t* readSramImpl() {
  return SAVE_SRAM_EXTFLASH;
}
void writeSramImpl(uint8_t* sram) {
  store_save(SAVE_SRAM_EXTFLASH, sram, 8192);
}



void app_main(void)
{
    wdog_enable();

    LoadAssets();
    
    ZeldaInitialize();

    g_wanted_zelda_features = (
        kFeatures0_SkipIntroOnKeypress  // Avoid waiting too much at the start
        | kFeatures0_DisableLowHealthBeep  // Disable the low health beep
      );

    ZeldaEnableMsu(false);
    
    ZeldaReadSram();

    bool running = true;
    //uint32 lastTick = HAL_GetTick();
    //uint32 curTick = 0;
    //uint32 frameCtr = 0;
    bool audiopaused = true;

    // Skip frames
    uint32 prevFrameTick = 0;
    uint32 prevTime = 0;
    //uint32 thisFrameTick = 0;

    common_emu_state.frame_time_10us = (uint16_t)(100000 / FRAMERATE + 0.5f);

    uint32_t prev_buttons = 0;
    uint32_t prev_power_ms = 0;
    bool prompting_to_save = false;
    while(running) {

        if (g_paused != audiopaused) {
        audiopaused = g_paused;
        }

        if (g_paused) {
        continue;
        }

        // Check inputs
        uint32_t buttons = buttons_get();

        // Handle power off / deep sleep
        if (buttons & B_POWER) {
          if(prompting_to_save){
            if((HAL_GetTick() - prev_power_ms) > POWER_BUTTON_DELAY_MS){
              //HAL_SAI_DMAStop(&hsai_BlockA1);
              GW_EnterDeepSleep();
            }
          }
          else{
            prev_power_ms = HAL_GetTick();
            prompting_to_save = true;
            buttons |= B_TIME;  // Simulate pressing SELECT
          }
        }
        else if (buttons){
          // If any button except POWER has been pressed
          prompting_to_save = false;
        }

        #if GNW_TARGET_ZELDA != 0
            HandleCommand(1, !(buttons & B_GAME) && (buttons & B_Up));
            HandleCommand(2, !(buttons & B_GAME) && (buttons & B_Down));
            HandleCommand(3, !(buttons & B_GAME) && (buttons & B_Left));
            HandleCommand(4, !(buttons & B_GAME) && (buttons & B_Right));

            HandleCommand(7, !(buttons & B_GAME) && (buttons & B_A));  // A (Pegasus Boots/Interacting)
            HandleCommand(8, !(buttons & B_GAME) && (buttons & B_B));  // B (Sword)
            
            HandleCommand(9, (buttons & B_PAUSE));    // X (Show Map)
            HandleCommand(10, (buttons & B_SELECT));  // Y (Use Item)
            
            HandleCommand(5, (buttons & B_TIME));   // Select (Save Screen)
            HandleCommand(6, (buttons & B_START));  // Start (Item Selection Screen)
            
            // L & R aren't used in Zelda3, but we could enable item quick-swapping.
            HandleCommand(11, (buttons & B_GAME) && (buttons & B_B)); // L
            HandleCommand(12, (buttons & B_GAME) && (buttons & B_A)); // R
        #else 
            HandleCommand(1, !(buttons & B_GAME) && (buttons & B_Up));
            HandleCommand(2, !(buttons & B_GAME) && (buttons & B_Down));
            HandleCommand(3, !(buttons & B_GAME) && (buttons & B_Left));
            HandleCommand(4, !(buttons & B_GAME) && (buttons & B_Right));
            HandleCommand(7, !(buttons & B_GAME) && (buttons & B_A));
            HandleCommand(8, !(buttons & B_GAME) && (buttons & B_B));
            HandleCommand(9, !(buttons & B_GAME) && (buttons & B_TIME));    // X
            HandleCommand(10, !(buttons & B_GAME) && (buttons & B_PAUSE));  // Y
            
            HandleCommand(5, (buttons & B_GAME) && (buttons & B_TIME));   // Select
            HandleCommand(6, (buttons & B_GAME) && (buttons & B_PAUSE));  // Start
            HandleCommand(11, (buttons & B_GAME) && (buttons & B_B)); // L
            HandleCommand(12, (buttons & B_GAME) && (buttons & B_A)); // R
        #endif /* GNW_TARGET_ZELDA */

        #define B_MACRO_CHECK(x, y) ((buttons & x) && (buttons & y) && prev_buttons != buttons)
        
        if (B_MACRO_CHECK(B_GAME, B_Left)){
          if (volume > AUDIO_VOLUME_MIN) {
            volume--;
          }
          ingame_overlay = INGAME_OVERLAY_VOLUME;
          overlay_start_ms = HAL_GetTick();
        }
        if (B_MACRO_CHECK(B_GAME, B_Right)){
          if (volume < AUDIO_VOLUME_MAX) {
            volume++;
          }
          ingame_overlay = INGAME_OVERLAY_VOLUME;
          overlay_start_ms = HAL_GetTick();
        }
        if (B_MACRO_CHECK(B_GAME, B_Down)){
          if (brightness > BRIGHTNESS_MIN) {
            brightness--;
            lcd_backlight_set(backlightLevels[brightness]);
          }
          ingame_overlay = INGAME_OVERLAY_BRIGHTNESS;
          overlay_start_ms = HAL_GetTick();
        }
        if (B_MACRO_CHECK(B_GAME, B_Up)){
          if (brightness < BRIGHTNESS_MAX) {
            brightness++;
            lcd_backlight_set(backlightLevels[brightness]);
          }
          ingame_overlay = INGAME_OVERLAY_BRIGHTNESS;
          overlay_start_ms = HAL_GetTick();
        }
        
        
        // Clear gamepad inputs when joypad directional inputs to avoid wonkiness
        int inputs = g_input1_state;
        if (g_input1_state & 0xf0)
          g_gamepad_buttons = 0;
        inputs |= g_gamepad_buttons;

        


        bool drawFrame = common_emu_frame_loop();



        bool is_replay = ZeldaRunFrame(inputs);

        frameCtr++;

        /*if ((g_turbo ^ (is_replay & g_replay_turbo)) && (frameCtr & (g_turbo ? 0xf : 0x7f)) != 0) {
        continue;
        }*/
        
        
        #if LIMIT_30FPS != 0
        // Render audio to DMA buffer
        ZeldaRenderAudio(audiobuffer, AUDIO_BUFFER_LENGTH / 2, 1);
        // FIXME Render two frames worth of gameplay / audio for each screen render
        ZeldaRunFrame(inputs);
        ZeldaRenderAudio(audiobuffer + (AUDIO_BUFFER_LENGTH / 2), AUDIO_BUFFER_LENGTH / 2, 1);
        #else
        ZeldaRenderAudio(audiobuffer, AUDIO_BUFFER_LENGTH, 1);
        #endif /* LIMIT_30FPS*/

        if (drawFrame) {
        
          pcm_submit();
          ZeldaDiscardUnusedAudioFrames();

          // TODO Cap framerate to 60fps

          // TODO Render two frames + display only one to achieve consistent 30fps ???

          // Skip frames
          //thisFrameTick = HAL_GetTick();
          /*if (prevTime > 34) {
            prevTime -= 17;
            continue;
          }*/

          prevFrameTick = HAL_GetTick();
          renderedFrameCtr++;
          DrawPpuFrameWithPerf();
          prevTime = HAL_GetTick() - prevFrameTick;
        }

        // FIXME if no frame skip
        //if (prevTime < 17) {
        if(!common_emu_state.skip_frames)
        {
            // odroid_audio_submit(pcm.buf, pcm.pos >> 1);
            // handled in pcm_submit instead.
            static dma_transfer_state_t last_dma_state = DMA_TRANSFER_STATE_HF;
            // FIXME pause frame ???
            for(uint8_t p = 0; p < common_emu_state.pause_frames + 1; p++) {
                // TODO only if audio data ???
                while (dma_state == last_dma_state) {
                    cpumon_sleep();
                }
                last_dma_state = dma_state;
            }
        }
        //}
        prev_buttons = buttons;

    }

    return 0;
    
    
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
 

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DAC1_Init();
  MX_DAC2_Init();
  MX_LTDC_Init();
  MX_SPI2_Init();
  MX_OCTOSPI1_Init();
  MX_SAI1_Init();
  MX_TIM1_Init();

  /* Initialize interrupts */
  MX_NVIC_Init();
  /* USER CODE BEGIN 2 */

  // Initialize the external flash

  //SCB_EnableICache();
  //SCB_EnableDCache();

  OSPI_Init(&hospi1);

  lcd_init(&hspi2, &hltdc);
  lcd_fill_framebuffer(0x00, 0x00, 0x00);
  lcd_backlight_set(backlightLevels[brightness]);

  // Copy instructions and data from extflash to axiram
  static uint32_t copy_areas[4] __attribute__((used));
  copy_areas[0] = &_siramdata;  // 0x90000000
  copy_areas[1] = &__ram_exec_start__;  // 0x24000000
  copy_areas[2] = &__ram_exec_end__;  // 0x24000000 + length
  copy_areas[3] = copy_areas[2] - copy_areas[1];
  memcpy_no_check(copy_areas[1], copy_areas[0], copy_areas[3]);


  // Copy ITCRAM HOT section
  copy_areas[0] = (uint32_t) &_sitcram_hot;
  copy_areas[1] = (uint32_t) &__itcram_hot_start__;
  copy_areas[2] = (uint32_t) &__itcram_hot_end__;
  copy_areas[3] = copy_areas[2] - copy_areas[1];
  memcpy_no_check((uint32_t *) copy_areas[1], (uint32_t *) copy_areas[0], copy_areas[3]);

  // Also copy DTCMRAM HOT section
  copy_areas[0] = (uint32_t) &_sdtcram_hot;
  copy_areas[1] = (uint32_t) &__dtcram_hot_start__;
  copy_areas[2] = (uint32_t) &__dtcram_hot_end__;
  copy_areas[3] = copy_areas[2] - copy_areas[1];
  memcpy_no_check((uint32_t *) copy_areas[1], (uint32_t *) copy_areas[0], copy_areas[3]);

  // Also copy AHBRAM HOT section
  copy_areas[0] = (uint32_t) &_sahbram_hot;
  copy_areas[1] = (uint32_t) &__ahbram_hot_start__;
  copy_areas[2] = (uint32_t) &__ahbram_hot_end__;
  copy_areas[3] = copy_areas[2] - copy_areas[1];
  memcpy_no_check((uint32_t *) copy_areas[1], (uint32_t *) copy_areas[0], copy_areas[3]);

  // Init audio buffers and SAI DMA
  set_audio_frequency(AUDIO_SAMPLE_RATE);
  memset(audiobuffer, 0, sizeof(audiobuffer));
  memset(audiobuffer_dma, 0, sizeof(audiobuffer_dma));
  HAL_SAI_Transmit_DMA(&hsai_BlockA1, (uint8_t *) audiobuffer_dma, AUDIO_BUFFER_LENGTH_DMA);  // uint8_t ????
    


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  
  // TODO Make sure QPSI is in memory-mapped mode to enable XIP at 0x90100000
  // FIXME Done in OSPI_Init: OSPI_EnableMemoryMappedMode();
  //flash_memory_map(&hospi1);

  // Sanity check, sometimes this is triggered
  uint32_t add = 0x90000000;
  uint32_t* ptr = (uint32_t*)add;
  if(*ptr == 0x88888888) {
    Error_Handler();
  }

  app_main();

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSI);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  // BOOST 1: PLLM = 16 PLLN=156 PLLP=2 PLLQ=6 PLLR=2 CLOCKPLL >> 312MHz CoreClock and OSPI 104MHz
  // BOOST 2: PLLM = 38 PLLN=420 PLLP=2 PLLQ=7 PLLR=2 CLOCKPLL >> 3..MHz CoreClock and OSPI 100MHz
  #if OVERCLOCK == 1
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 156;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 6;
  RCC_OscInitStruct.PLL.PLLR = 2;
  #elif OVERCLOCK == 2
  RCC_OscInitStruct.PLL.PLLM = 38;
  RCC_OscInitStruct.PLL.PLLN = 420;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  RCC_OscInitStruct.PLL.PLLR = 2;
  #else
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 140;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  #endif
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_SPI2
                              |RCC_PERIPHCLK_SAI1|RCC_PERIPHCLK_OSPI
                              |RCC_PERIPHCLK_CKPER;
  PeriphClkInitStruct.PLL2.PLL2M = 25;
  PeriphClkInitStruct.PLL2.PLL2N = 192;
  PeriphClkInitStruct.PLL2.PLL2P = 5;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 5;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.PLL3.PLL3M = 4;
  PeriphClkInitStruct.PLL3.PLL3N = 9;
  PeriphClkInitStruct.PLL3.PLL3P = 2;
  PeriphClkInitStruct.PLL3.PLL3Q = 2;
  PeriphClkInitStruct.PLL3.PLL3R = 24;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_3;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.OspiClockSelection = RCC_OSPICLKSOURCE_PLL; //RCC_OSPICLKSOURCE_CLKP;
  PeriphClkInitStruct.CkperClockSelection = RCC_CLKPSOURCE_HSI;
  PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_CLKP;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief NVIC Configuration.
  * @retval None
  */
static void MX_NVIC_Init(void)
{
  /* OCTOSPI1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(OCTOSPI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(OCTOSPI1_IRQn);
}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */
  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT1 config
  */
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT2 config
  */
  sConfig.DAC_ConnectOnChipPeripheral = DAC_SAMPLEANDHOLD_DISABLE;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

}

/**
  * @brief DAC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC2_Init(void)
{

  /* USER CODE BEGIN DAC2_Init 0 */

  /* USER CODE END DAC2_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC2_Init 1 */

  /* USER CODE END DAC2_Init 1 */
  /** DAC Initialization
  */
  hdac2.Instance = DAC2;
  if (HAL_DAC_Init(&hdac2) != HAL_OK)
  {
    Error_Handler();
  }
  /** DAC channel OUT1 config
  */
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac2, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC2_Init 2 */

  /* USER CODE END DAC2_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};
  LTDC_LayerCfgTypeDef pLayerCfg1 = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IIPC;
  hltdc.Init.HorizontalSync = 9;
  hltdc.Init.VerticalSync = 1;
  hltdc.Init.AccumulatedHBP = 60;
  hltdc.Init.AccumulatedVBP = 7;
  hltdc.Init.AccumulatedActiveW = 380;
  hltdc.Init.AccumulatedActiveH = 247;
  hltdc.Init.TotalWidth = 392;
  hltdc.Init.TotalHeigh = 255;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 320;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 240;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 255;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg.FBStartAdress = 0x24000000;
  pLayerCfg.ImageWidth = 320;
  pLayerCfg.ImageHeight = 240;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 255;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg1.WindowX0 = 0;
  pLayerCfg1.WindowX1 = 0;
  pLayerCfg1.WindowY0 = 0;
  pLayerCfg1.WindowY1 = 0;
  pLayerCfg1.Alpha = 0;
  pLayerCfg1.Alpha0 = 0;
  pLayerCfg1.BlendingFactor1 = LTDC_BLENDING_FACTOR1_CA;
  pLayerCfg1.BlendingFactor2 = LTDC_BLENDING_FACTOR2_CA;
  pLayerCfg1.FBStartAdress = GFXMMU_VIRTUAL_BUFFER0_BASE;
  pLayerCfg1.ImageWidth = 0;
  pLayerCfg1.ImageHeight = 0;
  pLayerCfg1.Backcolor.Blue = 0;
  pLayerCfg1.Backcolor.Green = 0;
  pLayerCfg1.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg1, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief OCTOSPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_OCTOSPI1_Init(void)
{

  /* USER CODE BEGIN OCTOSPI1_Init 0 */

  /* USER CODE END OCTOSPI1_Init 0 */

  OSPIM_CfgTypeDef sOspiManagerCfg = {0};

  /* USER CODE BEGIN OCTOSPI1_Init 1 */

  /* USER CODE END OCTOSPI1_Init 1 */
  /* OCTOSPI1 parameter configuration*/
  hospi1.Instance = OCTOSPI1;
  hospi1.Init.FifoThreshold = 4;
  hospi1.Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
  hospi1.Init.MemoryType = HAL_OSPI_MEMTYPE_MACRONIX;
  hospi1.Init.DeviceSize = 24;  // 16MB
  hospi1.Init.ChipSelectHighTime = 2;
  hospi1.Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
  hospi1.Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
  hospi1.Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
  hospi1.Init.ClockPrescaler = 1;
  hospi1.Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
  hospi1.Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
  hospi1.Init.ChipSelectBoundary = 0;
  hospi1.Init.ClkChipSelectHighTime = 0;
  hospi1.Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_BYPASSED;
  hospi1.Init.MaxTran = 0;
  hospi1.Init.Refresh = 0;
  if (HAL_OSPI_Init(&hospi1) != HAL_OK)
  {
    Error_Handler();
  }
  sOspiManagerCfg.ClkPort = 1;
  sOspiManagerCfg.NCSPort = 1;
  sOspiManagerCfg.IOLowPort = HAL_OSPIM_IOPORT_1_LOW;
  if (HAL_OSPIM_Config(&hospi1, &sOspiManagerCfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN OCTOSPI1_Init 2 */

  /* USER CODE END OCTOSPI1_Init 2 */

}

/**
  * @brief SAI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SAI1_Init(void)
{

  /* USER CODE BEGIN SAI1_Init 0 */

  /* USER CODE END SAI1_Init 0 */

  /* USER CODE BEGIN SAI1_Init 1 */

  /* USER CODE END SAI1_Init 1 */
  hsai_BlockA1.Instance = SAI1_Block_A;
  hsai_BlockA1.Init.AudioMode = SAI_MODEMASTER_TX;
  hsai_BlockA1.Init.Synchro = SAI_ASYNCHRONOUS;
  hsai_BlockA1.Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLE;
  hsai_BlockA1.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
  hsai_BlockA1.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_FULL;
  hsai_BlockA1.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_16K;
  hsai_BlockA1.Init.SynchroExt = SAI_SYNCEXT_DISABLE;
  hsai_BlockA1.Init.MonoStereoMode = SAI_MONOMODE;
  hsai_BlockA1.Init.CompandingMode = SAI_NOCOMPANDING;
  hsai_BlockA1.Init.TriState = SAI_OUTPUT_NOTRELEASED;
  if (HAL_SAI_InitProtocol(&hsai_BlockA1, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BIT, 2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SAI1_Init 2 */

  /* USER CODE END SAI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES_TXONLY;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 0x0;
  hspi2.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi2.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi2.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi2.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi2.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi2.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi2.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi2.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi2.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 14000;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 20000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIO_Speaker_enable_GPIO_Port, GPIO_Speaker_enable_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  //HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8|GPIO_PIN_4, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin : GPIO_Speaker_enable_Pin */
  GPIO_InitStruct.Pin = GPIO_Speaker_enable_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIO_Speaker_enable_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_PAUSE_Pin BTN_GAME_Pin BTN_TIME_Pin */
  GPIO_InitStruct.Pin = BTN_PAUSE_Pin|BTN_GAME_Pin|BTN_TIME_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : BTN_PWR_Pin */
  GPIO_InitStruct.Pin = BTN_PWR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BTN_PWR_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_START_Pin BTN_SELECT_Pin */
  GPIO_InitStruct.Pin = BTN_START_Pin|BTN_SELECT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA5 PA6 */
  /*GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);*/

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PD8 PD1 PD4 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_1|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : BTN_A_Pin BTN_Left_Pin BTN_Down_Pin BTN_Right_Pin
                           BTN_Up_Pin BTN_B_Pin */
  GPIO_InitStruct.Pin = BTN_A_Pin|BTN_Left_Pin|BTN_Down_Pin|BTN_Right_Pin
                          |BTN_Up_Pin|BTN_B_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1) {
    // Blink display to indicate failure
    lcd_backlight_off();
    HAL_Delay(500);
    lcd_backlight_on();
    HAL_Delay(500);
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

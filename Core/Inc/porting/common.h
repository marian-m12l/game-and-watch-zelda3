#pragma once

#include "main.h"

extern SAI_HandleTypeDef hsai_BlockA1;
extern DMA_HandleTypeDef hdma_sai1_a;

#define WIDTH  320
#define HEIGHT 240
#define BPP      4

#define AUDIO_VOLUME_MIN 0
#define AUDIO_VOLUME_MAX 9

#define AUDIO_SAMPLE_RATE   (44100)    //FIXME (48000)
#define AUDIO_BUFFER_LENGTH (AUDIO_SAMPLE_RATE / 60)



typedef enum {
    DMA_TRANSFER_STATE_HF = 0x00,
    DMA_TRANSFER_STATE_TC = 0x01,
} dma_transfer_state_t;
extern dma_transfer_state_t dma_state;
extern uint32_t dma_counter;

//extern uint32_t audioBuffer[AUDIO_BUFFER_LENGTH];
extern uint32_t audio_mute;


extern int16_t pendingSamples;
extern int16_t audiobuffer[AUDIO_BUFFER_LENGTH];
extern int16_t audiobuffer_dma[AUDIO_BUFFER_LENGTH * 2];

extern const uint8_t volume_tbl[AUDIO_VOLUME_MAX + 1];

typedef struct {
    uint32_t last_busy;
    uint32_t busy_ms;
    uint32_t sleep_ms;
} cpumon_stats_t;
extern cpumon_stats_t cpumon_stats;

/**
 * Just calls `__WFI()` and measures time spent sleeping.
 */
void cpumon_sleep(void);
void cpumon_busy(void);
void cpumon_reset(void);

/**
 * Holds common higher-level emu options that need to be used at not-neat
 * locations in each emulator.
 *
 * There should only be one of these objects instantiated.
 */


typedef struct
{
     uint32_t totalFrames;
     uint32_t skippedFrames;
     uint32_t fullFrames;
     uint32_t busyTime;
     uint32_t realTime;
     uint32_t resetTime;
} runtime_counters_t;

typedef struct
{
     //odroid_battery_state_t battery;
     float partialFPS;
     float skippedFPS;
     float totalFPS;
     float emulatedSpeed;
     float busyPercent;
     uint32_t lastTickTime;
     uint32_t freeMemoryInt;
     uint32_t freeMemoryExt;
     uint32_t freeBlockInt;
     uint32_t freeBlockExt;
     uint32_t idleTimeCPU0;
     uint32_t idleTimeCPU1;
} runtime_stats_t;

typedef struct {
    uint32_t last_sync_time;
    uint32_t last_overlay_time;
    uint16_t skipped_frames;
    int16_t frame_time_10us;
    uint8_t skip_frames:2;
    uint8_t pause_frames:1;
    uint8_t pause_after_frames:3;
    uint8_t startup_frames:2;
    uint8_t overlay:3;
} common_emu_state_t;

typedef uint16_t pixel_t;

extern common_emu_state_t common_emu_state;

extern const uint8_t backlightLevels[];
extern uint8_t volume;
extern uint8_t brightness;

#define BRIGHTNESS_MIN 1
#define BRIGHTNESS_MAX 9

#define AUDIO_VOLUME_MIN 0
#define AUDIO_VOLUME_MAX 9

enum {
    INGAME_OVERLAY_NONE,
    INGAME_OVERLAY_VOLUME,
    INGAME_OVERLAY_BRIGHTNESS,
};
typedef uint8_t ingame_overlay_t;


void odroid_system_tick(uint32_t skippedFrame, uint32_t fullFrame, uint32_t busyTime);
runtime_stats_t odroid_system_get_stats();
void draw_ingame_overlay(pixel_t *fb, ingame_overlay_t overlay);
void draw_border(pixel_t * fb);

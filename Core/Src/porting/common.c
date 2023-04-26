#include "common.h"

#include <stdbool.h>
#include <string.h>
#include "main.h"
#include "buttons.h"
#include "lcd.h"
#include "gw_linker.h"

#if ENABLE_SCREENSHOT
uint16_t framebuffer_capture[GW_LCD_WIDTH * GW_LCD_HEIGHT]  __attribute__((section (".fbflash"))) __attribute__((aligned(4096)));
#endif

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
    .frame_time_10us = (uint16_t)(100000 / 30 + 0.5f),  // Reasonable default of 60FPS if not explicitly configured. FIXME limited to 30 fps
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

    /*
    switch(app->speedupEnabled){
        case SPEEDUP_0_5x:
            frame_time_10us *= 2;
            break;
        case SPEEDUP_0_75x:
            frame_time_10us *= 5;
            frame_time_10us /= 4;
            break;
        case SPEEDUP_1_25x:
            frame_time_10us *= 4;
            frame_time_10us /= 5;
            break;
        case SPEEDUP_1_5x:
            frame_time_10us *= 2;
            frame_time_10us /= 3;
            break;
        case SPEEDUP_2x:
            frame_time_10us /= 2;
            break;
        case SPEEDUP_3x:
            frame_time_10us /= 3;
            break;
    }
    */
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

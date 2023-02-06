#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <string.h>

#include "porting.h"
#include "zelda_assets.h"

#include "zelda3/assets.h"
#include "zelda3/config.h"
#include "zelda3/types.h"
#include "zelda3/zelda_rtl.h"


#define WIDTH  256  // FIXME 320
#define HEIGHT 224  // FIXME 240
#define BPP      4
#define SCALE    1

#define NES_SCREEN_WIDTH     256
#define NES_SCREEN_HEIGHT    240


#define APP_ID 30

// Framebuffer
uint16_t fb_data[WIDTH * HEIGHT * BPP];

// NES Screen framebuffers
/*static uint8_t bitmap_data[2][sizeof(bitmap_t) + (sizeof(uint8 *) * NES_SCREEN_HEIGHT)];
static bitmap_t *framebuffers[2];
framebuffers[0] = (bitmap_t*)bitmap_data[0];
framebuffers[1] = (bitmap_t*)bitmap_data[1];*/


static uint8 g_paused, g_turbo, g_replay_turbo = true, g_cursor = true;
static uint8 g_current_window_scale;
static uint8 g_gamepad_buttons;
static int g_input1_state;
static bool g_display_perf;
static int g_curr_fps;
static int g_ppu_render_flags = 0;
static int g_snes_width, g_snes_height;
//static int g_sdl_audio_mixer_volume = SDL_MIX_MAXVOLUME;
//static struct RendererFuncs g_renderer_funcs;
static uint32 g_gamepad_modifiers;
static uint16 g_gamepad_last_cmd[kGamepadBtn_Count];

void NORETURN Die(const char *error) {
//#if defined(NDEBUG) && defined(_WIN32)
//  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kWindowTitle, error, NULL);
//#endif
  fprintf(stderr, "Error: %s\n", error);
  exit(1);
}

const uint8 *g_asset_ptrs[kNumberOfAssets];
uint32 g_asset_sizes[kNumberOfAssets];

static void LoadAssets() {
  // TODO static allocation --> direct flash access???
  size_t length = zelda_assets_length;
  uint8 *data = zelda_assets;
  
  static const char kAssetsSig[] = { kAssets_Sig };

  if (length < 16 + 32 + 32 + 8 + kNumberOfAssets * 4 ||
      memcmp(data, kAssetsSig, 48) != 0 ||
      *(uint32*)(data + 80) != kNumberOfAssets)
    Die("Invalid assets file");

  uint32 offset = 88 + kNumberOfAssets * 4 + *(uint32 *)(data + 84);

  for (size_t i = 0; i < kNumberOfAssets; i++) {
    uint32 size = *(uint32 *)(data + 88 + i * 4);
    offset = (offset + 3) & ~3;
    if ((uint64)offset + size > length)
      Die("Assets file corruption");
    g_asset_sizes[i] = size;
    g_asset_ptrs[i] = data + offset;
    offset += size;
  }
}


static void DrawPpuFrameWithPerf() {
  /*int render_scale = PpuGetCurrentRenderScale(g_zenv.ppu, g_ppu_render_flags);*/
  uint8 *pixel_buffer = fb_data;    //0;
  int pitch = WIDTH * BPP; // FIXME 0;

  ZeldaDrawPpuFrame(pixel_buffer, pitch, g_ppu_render_flags);
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


int main(int argc, char *argv[])
{
    printf("GNW Headless\n");

    printf("To access in-memory framebuffer:\n");
    printf("  sudo ./viewer  %d  %lx  %lu\n", getpid(), (long unsigned int) fb_data, sizeof(fb_data));

    LoadAssets();
    ZeldaInitialize();

    bool running = true;
    uint32 lastTick = HAL_GetTick();
    uint32 curTick = 0;
    uint32 frameCtr = 0;
    bool audiopaused = true;

    while(running) {

        if (g_paused != audiopaused) {
        audiopaused = g_paused;
        }

        if (g_paused) {
        continue;
        }

        // Clear gamepad inputs when joypad directional inputs to avoid wonkiness
        int inputs = g_input1_state;
        if (g_input1_state & 0xf0)
        g_gamepad_buttons = 0;
        inputs |= g_gamepad_buttons;

        bool is_replay = ZeldaRunFrame(inputs);

        frameCtr++;

        if ((g_turbo ^ (is_replay & g_replay_turbo)) && (frameCtr & (g_turbo ? 0xf : 0x7f)) != 0) {
        continue;
        }

        DrawPpuFrameWithPerf();
        
        // if vsync isn't working, delay manually
        curTick = HAL_GetTick();

        if (!g_config.disable_frame_delay) {
        static const uint8 delays[3] = { 17, 17, 16 }; // 60 fps
        lastTick += delays[frameCtr % 3];

        if (lastTick > curTick) {
            uint32 delta = lastTick - curTick;
            if (delta > 500) {
            lastTick = curTick - 500;
            delta = 500;
            }
    //        printf("Sleeping %d\n", delta);
        } else if (curTick - lastTick > 500) {
            lastTick = curTick;
        }
        }
    }

    return 0;
}













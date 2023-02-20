#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include <string.h>

#include "porting.h"
#include "zelda_assets.h"

#include "zelda3/assets.h"
#include "zelda3/config.h"
#include "zelda3/types.h"
#include "zelda3/zelda_rtl.h"


#define WIDTH  256  // FIXME 320
#define HEIGHT 224  // FIXME 240
#define BPP      2
#define SCALE    1

#define NES_SCREEN_WIDTH     256
#define NES_SCREEN_HEIGHT    240


#define APP_ID 30

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *fb_texture;

// SDL framebuffer
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
static int g_sdl_audio_mixer_volume = SDL_MIX_MAXVOLUME;
//static struct RendererFuncs g_renderer_funcs;
static uint32 g_gamepad_modifiers;
static uint16 g_gamepad_last_cmd[kGamepadBtn_Count];

void NORETURN Die(const char *error) {
#if defined(NDEBUG) && defined(_WIN32)
  SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, kWindowTitle, error, NULL);
#endif
  fprintf(stderr, "Error: %s\n", error);
  exit(1);
}


int init_window(int width, int height)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return 0;

    window = SDL_CreateWindow("GNW",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width * SCALE, height * SCALE,
        0);
    if (!window)
        return 0;

    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
        return 0;

    fb_texture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
        //SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        width, height);
    if (!fb_texture)
        return 0;
    
    return 0;
}


void blitscreen(/*bitmap_t *bmp*/)
{
    static uint32_t lastFPSTime = 0;
    static uint32_t lastTime = 0;
    static uint32_t frames = 0;

    frames++;
    //printf("frames=%d\n", frames);
    uint32_t currentTime = SDL_GetTicks();
    //printf("currentTime=%d\n", currentTime);
    float delta = currentTime - lastFPSTime;
    //printf("delta=%f\n", delta);
    if (delta >= 1000) {
        //FIXME printf("FPS: %f\n", ((float)frames / (delta / 1000.0f)));
        frames = 0;
        lastFPSTime = currentTime;
    }

    // we want 60 Hz for NTSC
    int wantedTime = 1000 / 60;
    //printf("wantedTime=%d\n", wantedTime);
    SDL_Delay(wantedTime); // rendering takes basically "0ms"
    lastTime = currentTime;

/*
    // LCD is 320 wide, framebuffer is only 256
    const int hpad = (WIDTH - NES_SCREEN_WIDTH) / 2;

    // printf("%d x %d\n", bmp->width, bmp->height);
    for (int line = 0; line < bmp->height; line++) {
        uint8_t *row = bmp->line[line];
        for (int x = 0; x < bmp->width; x++) {

            // This doesn't look good, but why? There is a palette and the
            // data seems to be stored with LUT indexes
            
            // this will read oob
            // fb_data[(2*line    ) * WIDTH + x + hpad] = myPalette[row[x]];
            // fb_data[(2*line + 1) * WIDTH + x + hpad] = myPalette[row[x]];

            fb_data[(2*line    ) * WIDTH + x + hpad] = myPalette[row[x] & 0b111111];
            fb_data[(2*line + 1) * WIDTH + x + hpad] = myPalette[row[x] & 0b111111];

            // fb_data[(2*line) * WIDTH + x] = row[x];
            // fb_data[(2*line + 1) * WIDTH + x] = row[x];
        }
    }
*/

    SDL_UpdateTexture(fb_texture, NULL, fb_data, WIDTH * BPP);
    SDL_RenderCopy(renderer, fb_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


void getinput(void)
{
    SDL_Event event;
    if (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            exit(1);
        } else if (event.type == SDL_KEYDOWN) {
            // printf("Press %d\n", event.key.keysym.sym);
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                exit(2);
                break;
            default:
                break;
            }
        } else if (event.type == SDL_KEYUP) {
            // printf("Release %d\n", event.key.keysym.sym);
            switch (event.key.keysym.sym) {
            default:
                break;
            }
        }
    }
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

  /*g_renderer_funcs.BeginDraw(g_snes_width * render_scale,
                             g_snes_height * render_scale,
                             &pixel_buffer, &pitch);
  if (g_display_perf || g_config.display_perf_title) {
    static float history[64], average;
    static int history_pos;
    uint64 before = SDL_GetPerformanceCounter();
    ZeldaDrawPpuFrame(pixel_buffer, pitch, g_ppu_render_flags);
    uint64 after = SDL_GetPerformanceCounter();
    float v = (double)SDL_GetPerformanceFrequency() / (after - before);
    average += v - history[history_pos];
    history[history_pos] = v;
    history_pos = (history_pos + 1) & 63;
    g_curr_fps = average * (1.0f / 64);
  } else {*/
    ZeldaDrawPpuFrame(pixel_buffer, pitch, g_ppu_render_flags);
    /* TODO Should do --> otherwise need to adapt ZeldaDrawPpuFrame
                fb_data[2*i*WIDTH+j] = (value << 8) | value;
                fb_data[2*(i+1)*WIDTH+j] = (value << 8);
    */
  /*}
  if (g_display_perf)
    RenderNumber(pixel_buffer + pitch * render_scale, pitch, g_curr_fps, render_scale == 4);
  g_renderer_funcs.EndDraw();*/
}


void ZeldaApuLock() {
  //SDL_LockMutex(g_audio_mutex);
}

void ZeldaApuUnlock() {
  //SDL_UnlockMutex(g_audio_mutex);
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

  // Everything that might access audio state
  // (like SaveLoad and Reset) must have the lock.
  //SDL_LockMutex(g_audio_mutex);
  //HandleCommand_Locked(j, pressed);
  //SDL_UnlockMutex(g_audio_mutex);
}

static void HandleInput(int keyCode, int keyMod, bool pressed) {
  int j = 0;//FindCmdForSdlKey(keyCode, keyMod);
  switch(keyCode) {
    case SDLK_UP:
        j = 1; break;
    case SDLK_DOWN:
        j = 2; break;
    case SDLK_LEFT:
        j = 3; break;
    case SDLK_RIGHT:
        j = 4; break;
    case SDLK_RSHIFT:
        j = 5; break;
    case SDLK_RETURN:
        j = 6; break;
    case SDLK_x:
        j = 7; break;
    case SDLK_z:
        j = 8; break;
    case SDLK_s:
        j = 9; break;
    case SDLK_a:
        j = 10; break;
    case SDLK_c:
        j = 11; break;
    case SDLK_v:
        j = 12; break;
    case SDLK_ESCAPE:
        exit(2);
        break;
  }
  if (j != 0)
    HandleCommand(j, pressed);
}


int main(int argc, char *argv[])
{
    init_window(WIDTH, HEIGHT);
    printf("GNW\n");

    printf("To access in-memory framebuffer:\n");
    printf("  sudo ./viewer  %d  %lx  %lu\n", getpid(), (long unsigned int) fb_data, sizeof(fb_data));

    // TODO Init zelda3 + run loop
    // TODO Will need to replace any SDL, file read, malloc, etc.
    // TODO + optimize memory usage

    /*argc--, argv++;
    const char *config_file = NULL;
    if (argc >= 2 && strcmp(argv[0], "--config") == 0) {
        config_file = argv[1];
        argc -= 2, argv += 2;
    } else {
        SwitchDirectory();
    }
    ParseConfigFile(config_file);*/
    LoadAssets();
    //LoadLinkGraphics();

    ZeldaInitialize();
    //g_zenv.ppu->extraLeftRight = UintMin(g_config.extended_aspect_ratio, kPpuExtraLeftRight);
    //g_snes_width = (g_config.extended_aspect_ratio * 2 + 256);
    //g_snes_height = (g_config.extend_y ? 240 : 224);


    // Delay actually setting those features in ram until any snapshots finish playing.
    //g_wanted_zelda_features = g_config.features0;

    /*g_ppu_render_flags = g_config.new_renderer * kPpuRenderFlags_NewRenderer |
                        g_config.enhanced_mode7 * kPpuRenderFlags_4x4Mode7 |
                        g_config.extend_y * kPpuRenderFlags_Height240 |
                        g_config.no_sprite_limits * kPpuRenderFlags_NoSpriteLimits;*/
    //ZeldaEnableMsu(true/*g_config.enable_msu*/);

    /*if (g_config.fullscreen == 1)
        g_win_flags ^= SDL_WINDOW_FULLSCREEN_DESKTOP;
    else if (g_config.fullscreen == 2)
        g_win_flags ^= SDL_WINDOW_FULLSCREEN;*/

    // Window scale (1=100%, 2=200%, 3=300%, etc.)
    //g_current_window_scale = (g_config.window_scale == 0) ? 2 : IntMin(g_config.window_scale, kMaxWindowScale);

    // audio_freq: Use common sampling rates (see user config file. values higher than 48000 are not supported.)
    /*if (g_config.audio_freq < 11025 || g_config.audio_freq > 48000)
        g_config.audio_freq = kDefaultFreq;*/

    // Currently, the SPC/DSP implementation only supports up to stereo.
    /*if (g_config.audio_channels < 1 || g_config.audio_channels > 2)
        g_config.audio_channels = kDefaultChannels;*/

    // audio_samples: power of 2
    /*if (g_config.audio_samples <= 0 || ((g_config.audio_samples & (g_config.audio_samples - 1)) != 0))
        g_config.audio_samples = kDefaultSamples;*/

    // set up SDL
    /*if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Failed to init SDL: %s\n", SDL_GetError());
        return 1;
    }*/

    /*bool custom_size  = g_config.window_width != 0 && g_config.window_height != 0;
    int window_width  = custom_size ? g_config.window_width  : g_current_window_scale * g_snes_width;
    int window_height = custom_size ? g_config.window_height : g_current_window_scale * g_snes_height;*/

    /*if (g_config.output_method == kOutputMethod_OpenGL) {
        g_win_flags |= SDL_WINDOW_OPENGL;
        OpenGLRenderer_Create(&g_renderer_funcs);
    } else {
        g_renderer_funcs = kSdlRendererFuncs;
    }*/

    /*SDL_Window* window = SDL_CreateWindow(kWindowTitle, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, g_win_flags);
    if(window == NULL) {
        printf("Failed to create window: %s\n", SDL_GetError());
        return 1;
    }
    g_window = window;
    SDL_SetWindowHitTest(window, HitTestCallback, NULL);

    if (!g_renderer_funcs.Initialize(window))
        return 1;

    SDL_AudioDeviceID device = 0;
    SDL_AudioSpec want = { 0 }, have;
    g_audio_mutex = SDL_CreateMutex();
    if (!g_audio_mutex) Die("No mutex");

    if (g_config.enable_audio) {
        want.freq = g_config.audio_freq;
        want.format = AUDIO_S16;
        want.channels = g_config.audio_channels;
        want.samples = g_config.audio_samples;
        want.callback = &AudioCallback;
        device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if (device == 0) {
        printf("Failed to open audio device: %s\n", SDL_GetError());
        return 1;
        }
        g_audio_channels = have.channels;
        g_frames_per_block = (534 * have.freq) / 32000;
        g_audiobuffer = malloc(g_frames_per_block * have.channels * sizeof(int16));
    }*/

    /*if (argc >= 1 && !g_run_without_emu)
        LoadRom(argv[0]);*/

    /*#if defined(_WIN32)
    _mkdir("saves");
    #else
    mkdir("saves", 0755);
    #endif*/

    // TODO ZeldaReadSram();

    /*for (int i = 0; i < SDL_NumJoysticks(); i++)
        OpenOneGamepad(i);*/

    bool running = true;
    SDL_Event event;
    uint32 lastTick = HAL_GetTick();
    uint32 curTick = 0;
    uint32 frameCtr = 0;
    bool audiopaused = true;

    /*if (g_config.autosave)
        HandleCommand(kKeys_Load + 0, true);*/

    while(running) {

        //getinput();
        //uint8_t value = SDL_GetTicks() % 255;
        //printf("value=%d\n", value);
        /*for (uint16_t i=0; i<HEIGHT; i++) {
            for (uint16_t j=0; j<WIDTH; j++) {
                fb_data[2*i*WIDTH+j] = (value << 8) | value;
                fb_data[2*(i+1)*WIDTH+j] = (value << 8);
            }
        }
        blitscreen();*/



        while(SDL_PollEvent(&event)) {
        switch(event.type) {
        /*case SDL_CONTROLLERDEVICEADDED:
            OpenOneGamepad(event.cdevice.which);
            break;
        case SDL_CONTROLLERAXISMOTION:
            HandleGamepadAxisInput(event.caxis.which, event.caxis.axis, event.caxis.value);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            int b = RemapSdlButton(event.cbutton.button);
            if (b >= 0)
            HandleGamepadInput(b, event.type == SDL_CONTROLLERBUTTONDOWN);
            break;
        }
        case SDL_MOUSEWHEEL:
            if (SDL_GetModState() & KMOD_CTRL && event.wheel.y != 0)
            ChangeWindowScale(event.wheel.y > 0 ? 1 : -1);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT && event.button.state == SDL_PRESSED && event.button.clicks == 2) {
            if ((g_win_flags & SDL_WINDOW_FULLSCREEN_DESKTOP) == 0 && (g_win_flags & SDL_WINDOW_FULLSCREEN) == 0 && SDL_GetModState() & KMOD_SHIFT) {
                g_win_flags ^= SDL_WINDOW_BORDERLESS;
                SDL_SetWindowBordered(g_window, (g_win_flags & SDL_WINDOW_BORDERLESS) == 0);
            }
            }
            break;*/
        case SDL_KEYDOWN:
            HandleInput(event.key.keysym.sym, event.key.keysym.mod, true);
            break;
        case SDL_KEYUP:
            HandleInput(event.key.keysym.sym, event.key.keysym.mod, false);
            break;
        case SDL_QUIT:
            running = false;
            break;
        }
        }

        if (g_paused != audiopaused) {
        audiopaused = g_paused;
        /*if (device)
            SDL_PauseAudioDevice(device, audiopaused);*/
        }

        if (g_paused) {
        //SDL_Delay(16);
        continue;
        }

        // Clear gamepad inputs when joypad directional inputs to avoid wonkiness
        int inputs = g_input1_state;
        if (g_input1_state & 0xf0)
        g_gamepad_buttons = 0;
        inputs |= g_gamepad_buttons;

        //SDL_LockMutex(g_audio_mutex);
        bool is_replay = ZeldaRunFrame(inputs);
        //SDL_UnlockMutex(g_audio_mutex);

        frameCtr++;

        if ((g_turbo ^ (is_replay & g_replay_turbo)) && (frameCtr & (g_turbo ? 0xf : 0x7f)) != 0) {
        continue;
        }

        DrawPpuFrameWithPerf();

        // TODO blitscreen with pixel buffer !!!
        blitscreen();

        /*if (g_config.display_perf_title) {
        char title[60];
        snprintf(title, sizeof(title), "%s | FPS: %d", kWindowTitle, g_curr_fps);
        SDL_SetWindowTitle(g_window, title);
        }*/

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
            //SDL_Delay(delta);
        } else if (curTick - lastTick > 500) {
            lastTick = curTick;
        }
        }
    }
    /*if (g_config.autosave)
        HandleCommand(kKeys_Save + 0, true);*/

    // clean sdl
    /*if (g_config.enable_audio) {
        SDL_PauseAudioDevice(device, 1);
        SDL_CloseAudioDevice(device);
    }

    SDL_DestroyMutex(g_audio_mutex);
    free(g_audiobuffer);

    g_renderer_funcs.Destroy();*/

    SDL_DestroyWindow(window);
    SDL_Quit();
    //SaveConfigFile();
    return 0;



    // TODO Run main loop
    for (;;) {
        getinput();
        uint8_t value = SDL_GetTicks() % 255;
        //printf("value=%d\n", value);
        for (uint16_t i=0; i<HEIGHT; i++) {
            for (uint16_t j=0; j<WIDTH; j++) {
                fb_data[2*i*WIDTH+j] = (value << 8) | value;
                fb_data[2*(i+1)*WIDTH+j] = (value << 8);
            }
        }
        blitscreen();
    }
    
    SDL_Quit();

    return 0;
}













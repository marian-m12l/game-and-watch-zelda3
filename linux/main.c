#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <SDL2/SDL.h>

#include <string.h>


#define WIDTH  320
#define HEIGHT 240
#define BPP      4
#define SCALE    4

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
        printf("FPS: %f\n", ((float)frames / (delta / 1000.0f)));
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


int main(int argc, char *argv[])
{
    init_window(WIDTH, HEIGHT);
    printf("GNW\n");

    // TODO Init zelda3 + run loop
    // TODO Will need to replace any SDL, file read, malloc, etc.
    // TODO + optimize memory usage

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













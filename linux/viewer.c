#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include <string.h>


#define WIDTH  256  // FIXME 320
#define HEIGHT 224  // FIXME 240
#define BPP      4
#define SCALE    1

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *fb_texture;

// TODO Access and display framebuffer from zelda process

// SDL framebuffer
//uint16_t fb_data[WIDTH * HEIGHT * BPP];


int init_window(int width, int height)
{
    //printf("init_window\n");
    //printf("SDL_Init\n");
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return 0;

    //printf("SDL_CreateWindow: %d x %d\n", (width* SCALE), (height * SCALE));
    window = SDL_CreateWindow("GNW VIEWER",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        width * SCALE, height * SCALE,
        0);
    if (!window)
        return 0;

    //printf("SDL_CreateRenderer\n");
    renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_PRESENTVSYNC);
    if (!renderer)
        return 0;

    //printf("SDL_CreateTexture\n");
    fb_texture = SDL_CreateTexture(renderer,
        // FIXME SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        width, height);
    if (!fb_texture)
        return 0;
    
    return 0;
}


void blitscreen(/*bitmap_t *bmp*/uint16_t* fb_data)
{
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
    printf("GNW VIEWER\n");
    init_window(WIDTH, HEIGHT);

    if (argc != 4) {
        printf("proc-2  pid  addr  length\n");
        exit(1);
    }

    int           pid  = strtol (argv[1], NULL, 10);
    unsigned long addr = strtoul(argv[2], NULL, 16);
    int           len  = strtol (argv[3], NULL, 10);

    char* proc_mem = malloc(50);
    sprintf(proc_mem, "/proc/%d/mem", pid);

    printf("opening %s, address is %ld\n", proc_mem, addr);
    int fd_proc_mem = open(proc_mem, O_RDWR);
    if (fd_proc_mem == -1) {
        printf("Could not open %s\n", proc_mem);
        exit(1);
    }

    uint16_t* buf = malloc(len);

    
    printf("MAIN LOOP\n");
    // TODO Run main loop
    for (;;) {
        getinput();
        lseek(fd_proc_mem, addr, SEEK_SET);
        read (fd_proc_mem, buf , len     );
        blitscreen((uint16_t*) buf);
        //blitscreen(fb_data);
        SDL_Delay(16);
    }
    
    SDL_Quit();

    return 0;
}













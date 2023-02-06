#include "porting.h"
#ifndef HEADLESS
#include <SDL2/SDL.h>
#endif

uint32_t HAL_GetTick(void)
{
    #ifndef HEADLESS
    return SDL_GetTicks();
    #else
    return 0;   // FIXME sys ticks ???
    #endif
}

void wdog_refresh(void)
{

}

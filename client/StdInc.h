#pragma once

#include "../Global.h"

#include <SDL_version.h>

#if (SDL_MAJOR_VERSION == 2)
#define VCMI_SDL2

#include <SDL_keycode.h>
typedef int SDLX_Coord;
typedef int SDLX_Size;

typedef SDL_Keycode SDLKey;

#define SDL_SRCCOLORKEY SDL_TRUE

#define SDL_FULLSCREEN SDL_WINDOW_FULLSCREEN

#elif (SDL_MAJOR_VERSION == 1) 
#define VCMI_SDL1
//SDL 1.x
typedef Sint16 SDLX_Coord;
typedef Uint16 SDLX_Size;
#else
#error "unkown or unsupported SDL version"
#endif

// This header should be treated as a pre compiled header file(PCH) in the compiler building settings.

// Here you can add specific libraries and macros which are specific to this project.

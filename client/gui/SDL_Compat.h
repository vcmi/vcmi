#pragma once

/*
 * SDL_Compat.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
#include <SDL_version.h>

#if (SDL_MAJOR_VERSION == 2)

#include <SDL_keycode.h>
typedef int SDLX_Coord;
typedef int SDLX_Size;

typedef SDL_Keycode SDLKey;

#define SDL_SRCCOLORKEY SDL_TRUE

#define SDL_FULLSCREEN SDL_WINDOW_FULLSCREEN

#else
#error "unknown or unsupported SDL version"
#endif
 


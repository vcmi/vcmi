#pragma once

#ifndef VCMI_SDL1
#include <SDL_render.h>

extern SDL_Texture * screenTexture;

extern SDL_Window * mainWindow;
extern SDL_Renderer * mainRenderer;

#endif // VCMI_SDL2

extern SDL_Surface *screen;      // main screen surface
extern SDL_Surface *screen2;     // and hlp surface (used to store not-active interfaces layer)
extern SDL_Surface *screenBuf; // points to screen (if only advmapint is present) or screen2 (else) - should be used when updating controls which are not regularly redrawed


extern bool gNoGUI; //if true there is no client window and game is silently played between AIs

void handleQuit();

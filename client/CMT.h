#pragma once
#include <SDL_render.h>
#include "../lib/CondSh.h"

extern SDL_Texture * screenTexture;

extern SDL_Window * mainWindow;
extern SDL_Renderer * mainRenderer;

extern SDL_Surface *screen;      // main screen surface
extern SDL_Surface *screen2;     // and hlp surface (used to store not-active interfaces layer)
extern SDL_Surface *screenBuf; // points to screen (if only advmapint is present) or screen2 (else) - should be used when updating controls which are not regularly redrawed


extern bool gNoGUI; //if true there is no client window and game is silently played between AIs

extern CondSh<bool> serverAlive; //used to prevent game start from executing if server is already running

void handleQuit(bool ask = true);

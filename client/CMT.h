#pragma once

extern SDL_Surface *screen;      // main screen surface
extern SDL_Surface *screen2;     // and hlp surface (used to store not-active interfaces layer)
extern SDL_Surface *screenBuf; // points to screen (if only advmapint is present) or screen2 (else) - should be used when updating controls which are not regularly redrawed

extern bool gNoGUI; //if true there is no client window and game is silently played between AIs

void handleQuit();
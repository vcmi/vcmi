/*
 * CMT.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include <SDL_render.h>

extern SDL_Texture * screenTexture;

extern SDL_Window * mainWindow;
extern SDL_Renderer * mainRenderer;

extern SDL_Surface *screen;      // main screen surface
extern SDL_Surface *screen2;     // and hlp surface (used to store not-active interfaces layer)
extern SDL_Surface *screenBuf; // points to screen (if only advmapint is present) or screen2 (else) - should be used when updating controls which are not regularly redrawed

void removeGUI();
void handleQuit(bool ask = true);

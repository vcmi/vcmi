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

struct SDL_Texture;
struct SDL_Renderer;
struct SDL_Surface;

extern SDL_Texture * screenTexture;
extern SDL_Renderer * mainRenderer;

extern SDL_Surface *screen;

/// Notify user about encountered fatal error and terminate the game
/// Defined in clientapp EntryPoint
/// TODO: decide on better location for this method
[[noreturn]] void handleFatalError(const std::string & message, bool terminate);
void handleQuit(bool ask = true);

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

struct SDL_Renderer;
extern SDL_Renderer * mainRenderer;

/// Bumped whenever mainRenderer is destroyed. Textures remember the generation they were
/// created in, so that they can tell "still mine" from "already freed with the renderer".
extern uint32_t mainRendererGeneration;

struct SDL_Texture;

/// Queues a texture for destruction on the rendering thread. Textures may only be created
/// and destroyed there, so owners released on other threads hand them over instead.
void destroyTextureDeferred(SDL_Texture * texture);

/// Notify user about encountered fatal error and terminate the game
/// Defined in clientapp EntryPoint
/// TODO: decide on better location for this method
[[noreturn]] void handleFatalError(const std::string & message, bool terminate);

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

/// Notify user about encountered fatal error and terminate the game
/// Defined in clientapp EntryPoint
/// TODO: decide on better location for this method
[[noreturn]] void handleFatalError(const std::string & message, bool terminate);

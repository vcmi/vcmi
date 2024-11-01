/*
 * sdlHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

class SdlHandler
{
	std::map<int, SDL_GameController*> controller;
	std::function<void(SDL_GameControllerButton)> gameControllerButtonCallback;

	void mapGameControllerKey(SDL_GameControllerButton button);

public:
	void eventHandler();
	void setGameControllerButtonCallback(std::function<void(SDL_GameControllerButton)> cb);

	static QStringList getAvailableRenderingDrivers();
	static QVector<QSize> findAvailableResolutions(int displayIndex);

	SdlHandler();
	~SdlHandler();
};

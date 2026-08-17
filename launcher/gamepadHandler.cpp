/*
 * gamepadHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "gamepadHandler.h"

#include <SDL2/SDL.h>

static constexpr int POLL_INTERVAL_MS = 100;

GamepadHandler::GamepadHandler(QObject * parent)
	: QObject(parent)
{
	if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
	{
		logGlobal->warn("Failed to initialize game controller support: %s", SDL_GetError());
		return;
	}

	auto * timer = new QTimer(this);
	connect(timer, &QTimer::timeout, this, &GamepadHandler::poll);
	timer->start(POLL_INTERVAL_MS);
}

GamepadHandler::~GamepadHandler()
{
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}

void GamepadHandler::poll()
{
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			// controllers connected before initialization are reported here as well
			case SDL_CONTROLLERDEVICEADDED:
				SDL_GameControllerOpen(ev.cdevice.which);
				break;
			case SDL_CONTROLLERDEVICEREMOVED:
				if(auto * controller = SDL_GameControllerFromInstanceID(ev.cdevice.which))
					SDL_GameControllerClose(controller);
				break;
			case SDL_CONTROLLERBUTTONDOWN:
				if(ev.cbutton.button == SDL_CONTROLLER_BUTTON_A || ev.cbutton.button == SDL_CONTROLLER_BUTTON_START)
					emit startGameRequested();
				break;
		}
	}
}

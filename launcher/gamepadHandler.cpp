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

#ifdef VCMI_SDL3
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_init.h>
#else
#include <SDL2/SDL.h>
#endif

static constexpr int POLL_INTERVAL_MS = 100;

GamepadHandler::GamepadHandler(QObject * parent)
	: QObject(parent)
{
#ifdef VCMI_SDL3
	// SDL3 renamed the subsystem and reports success as a boolean rather than a zero result
	if(!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
#else
	if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
#endif
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
#ifdef VCMI_SDL3
	SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
#else
	SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
#endif
}

void GamepadHandler::poll()
{
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		switch(ev.type)
		{
			// controllers connected before initialization are reported here as well
#ifdef VCMI_SDL3
			// SDL3 calls them gamepads, addresses them by instance id and named the face
			// buttons after their position instead of their label
			case SDL_EVENT_GAMEPAD_ADDED:
				SDL_OpenGamepad(ev.gdevice.which);
				break;
			case SDL_EVENT_GAMEPAD_REMOVED:
				if(auto * controller = SDL_GetGamepadFromID(ev.gdevice.which))
					SDL_CloseGamepad(controller);
				break;
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
				if(ev.gbutton.button == SDL_GAMEPAD_BUTTON_SOUTH || ev.gbutton.button == SDL_GAMEPAD_BUTTON_START)
					emit startGameRequested();
				break;
#else
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
#endif
		}
	}
}

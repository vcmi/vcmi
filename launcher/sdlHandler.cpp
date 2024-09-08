/*
 * sdlHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "sdlHandler.h"

#include <SDL2/SDL_main.h>

SdlHandler::SdlHandler() : gameControllerButtonCallback(nullptr)
{
	SDL_SetMainReady();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
}

SdlHandler::~SdlHandler()
{
	SDL_Quit();
}

void SdlHandler::eventHandler()
{
	SDL_Event e;
	while(SDL_PollEvent(&e))
	{
		switch(e.type) {
			case SDL_CONTROLLERDEVICEADDED:
				controller.insert_or_assign(e.cdevice.which, SDL_GameControllerOpen(e.cdevice.which));
				break;
			case SDL_CONTROLLERDEVICEREMOVED:
				for(auto & c : controller)
					if (c.second && e.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c.second))) {
						SDL_GameControllerClose(c.second);
					}
				break;
			case SDL_CONTROLLERBUTTONDOWN:
				for(auto & c : controller)
					if (c.second && e.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c.second))) {
						if(gameControllerButtonCallback)
							gameControllerButtonCallback(static_cast<SDL_GameControllerButton>(e.cbutton.button));
						mapGameControllerKey(static_cast<SDL_GameControllerButton>(e.cbutton.button));
					}
				break;
		}
	}
}

void SdlHandler::setGameControllerButtonCallback(std::function<void(SDL_GameControllerButton)> cb)
{
	gameControllerButtonCallback = cb;
}

QStringList SdlHandler::getAvailableRenderingDrivers()
{
	QStringList result;

	result += QString(); // empty value for autoselection

	int driversCount = SDL_GetNumRenderDrivers();

	for(int it = 0; it < driversCount; it++)
	{
		SDL_RendererInfo info;
		if (SDL_GetRenderDriverInfo(it, &info) == 0)
			result += QString::fromLatin1(info.name);
	}

	return result;
}

QVector<QSize> SdlHandler::findAvailableResolutions(int displayIndex)
{
	// Ugly workaround since we don't actually need SDL in Launcher
	// However Qt at the moment provides no way to query list of available resolutions
	QVector<QSize> result;

	int modesCount = SDL_GetNumDisplayModes(displayIndex);

	for (int i =0; i < modesCount; ++i)
	{
		SDL_DisplayMode mode;
		if (SDL_GetDisplayMode(displayIndex, i, &mode) != 0)
			continue;

		QSize resolution(mode.w, mode.h);

		result.push_back(resolution);
	}

	boost::range::sort(result, [](const auto & left, const auto & right)
	{
		return left.height() * left.width() < right.height() * right.width();
	});

	result.erase(boost::unique(result).end(), result.end());

	return result;
}

void SdlHandler::mapGameControllerKey(SDL_GameControllerButton button)
{
	auto sendKey = [](Qt::Key key){
		auto focusObj = QGuiApplication::focusObject();
		if(focusObj)
		{
			QString keyStr(QKeySequence(key).toString());
			QKeyEvent pressEvent = QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier, keyStr);
			QKeyEvent releaseEvent = QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier);
			QCoreApplication::sendEvent(focusObj, &pressEvent);
			QCoreApplication::sendEvent(focusObj, &releaseEvent);
		}
	};

	switch(button) {
		case SDL_CONTROLLER_BUTTON_DPAD_UP:
			sendKey(Qt::Key_Up);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
			sendKey(Qt::Key_Down);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
			sendKey(Qt::Key_Left);
			break;
		case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
			sendKey(Qt::Key_Right);
			break;
		case SDL_CONTROLLER_BUTTON_X:
			sendKey(Qt::Key_Tab);
			break;
		case SDL_CONTROLLER_BUTTON_Y:
			sendKey(Qt::Key_Enter);
			break;
		case SDL_CONTROLLER_BUTTON_A:
			sendKey(Qt::Key_Space);
			break;
	}	
}

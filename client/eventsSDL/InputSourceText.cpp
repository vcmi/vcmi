/*
* InputSourceText.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "InputSourceText.h"

#include "../GameEngine.h"
#include "../gui/EventDispatcher.h"
#include "../render/IScreenHandler.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../lib/Rect.h"

#include <SDL_events.h>

InputSourceText::InputSourceText()
{
	// For whatever reason, in SDL text input is considered to be active by default at least on desktop platforms
	// Apparently fixed in SDL3, but until then we need a workaround
	SDL_StopTextInput();
}

void InputSourceText::handleEventTextInput(const SDL_TextInputEvent & text)
{
	ENGINE->events().dispatchTextInput(text.text);
}

void InputSourceText::handleEventTextEditing(const SDL_TextEditingEvent & text)
{
	ENGINE->events().dispatchTextEditing(text.text);
}

void InputSourceText::startTextInput(const Rect & whereInput)
{
	ENGINE->dispatchMainThread([whereInput]()
	{
		Rect rectInScreenCoordinates = ENGINE->screenHandler().convertLogicalPointsToWindow(whereInput);
		SDL_Rect textInputRect = CSDL_Ext::toSDL(rectInScreenCoordinates);

		SDL_SetTextInputRect(&textInputRect);

		if (SDL_IsTextInputActive() == SDL_FALSE)
		{
			SDL_StartTextInput();
		}
	});
}

void InputSourceText::stopTextInput()
{
	ENGINE->dispatchMainThread([]()
	{
		if (SDL_IsTextInputActive() == SDL_TRUE)
		{
			SDL_StopTextInput();
		}
	});
}

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

#include "GameEngine.h"
#include "gui/EventDispatcher.h"
#include "../render/IScreenHandler.h"
#include "../render/SDL_Extensions.h"

#include "lib/Rect.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_properties.h>

/// SDL3 tracks text input state per window instead of globally
static SDL_Window * textInputWindow()
{
	return SDL_GetKeyboardFocus();
}

InputSourceText::InputSourceText()
{
	// SDL3 no longer starts text input on its own, so nothing has to be stopped here
}

void InputSourceText::handleEventTextInput(const SDL_TextInputEvent & text)
{
	ENGINE->events().dispatchTextInput(text.text);
}

void InputSourceText::handleEventTextEditing(const SDL_TextEditingEvent & text)
{
	ENGINE->events().dispatchTextEditing(text.text);
}

void InputSourceText::startTextInput(const Rect & whereInput, bool numbersOnly)
{
	ENGINE->dispatchMainThread([whereInput, numbersOnly]()
	{
		Rect rectInScreenCoordinates = ENGINE->screenHandler().convertLogicalPointsToWindow(whereInput);
		SDL_Rect textInputRect = CSDL_Ext::toSDL(rectInScreenCoordinates);

		SDL_Window * window = textInputWindow();
		if (window == nullptr)
			return;

		SDL_SetTextInputArea(window, &textInputRect, 0);

		if (SDL_TextInputActive(window))
			SDL_StopTextInput(window);

		if (numbersOnly)
		{
			SDL_PropertiesID properties = SDL_CreateProperties();
			SDL_SetNumberProperty(properties, SDL_PROP_TEXTINPUT_TYPE_NUMBER, SDL_TEXTINPUT_TYPE_NUMBER);
			SDL_StartTextInputWithProperties(window, properties);
			SDL_DestroyProperties(properties);
		}
		else
		{
			SDL_StartTextInput(window);
		}
	});
}

void InputSourceText::stopTextInput()
{
	ENGINE->dispatchMainThread([]()
	{
		SDL_Window * window = textInputWindow();

		if (window != nullptr && SDL_TextInputActive(window))
		{
			SDL_StopTextInput(window);
		}
	});
}

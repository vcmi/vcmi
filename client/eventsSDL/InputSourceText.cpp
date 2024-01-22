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

#include "../CMT.h"
#include "../gui/CGuiHandler.h"
#include "../gui/EventDispatcher.h"
#include "../render/IScreenHandler.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../lib/Rect.h"

#include <SDL_events.h>

void InputSourceText::handleEventTextInput(const SDL_TextInputEvent & text)
{
	GH.events().dispatchTextInput(text.text);
}

void InputSourceText::handleEventTextEditing(const SDL_TextEditingEvent & text)
{
	GH.events().dispatchTextEditing(text.text);
}

void InputSourceText::startTextInput(const Rect & whereInput)
{
	GH.dispatchMainThread([whereInput]()
	{
		Rect rectInScreenCoordinates = GH.screenHandler().convertLogicalPointsToWindow(whereInput);
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
	GH.dispatchMainThread([]()
	{
		if (SDL_IsTextInputActive() == SDL_TRUE)
		{
			SDL_StopTextInput();
		}
	});
}

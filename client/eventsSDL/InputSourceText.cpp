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

#ifdef VCMI_APPLE
#	include <dispatch/dispatch.h>
#endif

#ifdef VCMI_IOS
#	include "ios/utils.h"
#endif

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

#ifdef VCMI_APPLE
	dispatch_async(dispatch_get_main_queue(), ^{
#endif

	Rect rectInScreenCoordinates = GH.screenHandler().convertLogicalPointsToWindow(whereInput);
	SDL_Rect textInputRect = CSDL_Ext::toSDL(rectInScreenCoordinates);

	SDL_SetTextInputRect(&textInputRect);

	if (SDL_IsTextInputActive() == SDL_FALSE)
	{
		SDL_StartTextInput();
	}

#ifdef VCMI_APPLE
	});
#endif
}

void InputSourceText::stopTextInput()
{
#ifdef VCMI_APPLE
	dispatch_async(dispatch_get_main_queue(), ^{
#endif

	if (SDL_IsTextInputActive() == SDL_TRUE)
	{
		SDL_StopTextInput();
	}

#ifdef VCMI_APPLE
	});
#endif
}

/*
* InputSourceText.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class Rect;
VCMI_LIB_NAMESPACE_END

struct SDL_TextEditingEvent;
struct SDL_TextInputEvent;

class InputSourceText
{
public:
	void handleEventTextInput(const SDL_TextInputEvent & current);
	void handleEventTextEditing(const SDL_TextEditingEvent & current);

	void startTextInput(const Rect & where);
	void stopTextInput();
};

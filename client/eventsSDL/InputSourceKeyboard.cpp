/*
* InputSourceKeyboard.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "InputSourceKeyboard.h"

#include "../../lib/CConfigHandler.h"
#include "../GameEngine.h"
#include "../GameEngineUser.h"
#include "../gui/EventDispatcher.h"
#include "../gui/Shortcut.h"
#include "../gui/ShortcutHandler.h"

#include <SDL_clipboard.h>
#include <SDL_events.h>
#include <SDL_hints.h>

InputSourceKeyboard::InputSourceKeyboard()
: handleBackRightMouseButton(settings["input"]["handleBackRightMouseButton"].Bool())
{
#ifdef VCMI_MAC
	// Ctrl+click should be treated as a right click on Mac OS X
	SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
#endif
	SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, handleBackRightMouseButton ? "1" : "0");
}

std::string InputSourceKeyboard::getKeyNameWithModifiers(const std::string & keyName, bool keyUp)
{
	std::string result;

	if(!keyUp)
	{
		wasKeyboardCtrlDown = isKeyboardCtrlDown();
		wasKeyboardAltDown = isKeyboardAltDown();
		wasKeyboardShiftDown = isKeyboardShiftDown();
	}

	if (wasKeyboardCtrlDown)
		result += "Ctrl+";
	if (wasKeyboardAltDown)
		result += "Alt+";
	if (wasKeyboardShiftDown)
		result += "Shift+";
	result += keyName;

	return result;
}

void InputSourceKeyboard::handleEventKeyDown(const SDL_KeyboardEvent & key)
{
	std::string keyName = getKeyNameWithModifiers(SDL_GetScancodeName(key.keysym.scancode), false);
	logGlobal->trace("keyboard: key '%s' pressed", keyName);
	assert(key.state == SDL_PRESSED);

	if (SDL_IsTextInputActive() == SDL_TRUE)
	{
		if(key.keysym.sym == SDLK_v && isKeyboardCtrlDown()) 
		{
			char * clipboardBuffer = SDL_GetClipboardText();
			std::string clipboardContent = clipboardBuffer;
			boost::erase_all(clipboardContent, "\r");
			boost::erase_all(clipboardContent, "\n");
			ENGINE->events().dispatchTextInput(clipboardContent);
			SDL_free(clipboardBuffer);
			return;
	 	}

		if (key.keysym.sym >= ' ' && key.keysym.sym < 0x80)
			return; // printable character - will be handled as text input
	} else {
		if(key.repeat != 0)
			return; // ignore periodic event resends
	}

	if(handleBackRightMouseButton && key.keysym.scancode ==  SDL_SCANCODE_AC_BACK) // on some android devices right mouse button is "back"
	{
		ENGINE->events().dispatchShowPopup(ENGINE->getCursorPosition(), settings["input"]["mouseToleranceDistance"].Integer());
		return;
	}

	auto shortcutsVector = ENGINE->shortcuts().translateKeycode(keyName);

	ENGINE->events().dispatchKeyPressed(keyName);

	if (vstd::contains(shortcutsVector, EShortcut::MAIN_MENU_LOBBY))
		ENGINE->user().onGlobalLobbyInterfaceActivated();

	if (vstd::contains(shortcutsVector, EShortcut::GLOBAL_FULLSCREEN))
	{
		Settings full = settings.write["video"]["fullscreen"];
		full->Bool() = !full->Bool();
		ENGINE->onScreenResize(true);
	}

	if (vstd::contains(shortcutsVector, EShortcut::SPECTATE_TRACK_HERO))
	{
		Settings s = settings.write["session"];
		s["spectate-ignore-hero"].Bool() = !settings["session"]["spectate-ignore-hero"].Bool();
	}

	if (vstd::contains(shortcutsVector, EShortcut::SPECTATE_SKIP_BATTLE))
	{
		Settings s = settings.write["session"];
		s["spectate-skip-battle"].Bool() = !settings["session"]["spectate-skip-battle"].Bool();
	}

	if (vstd::contains(shortcutsVector, EShortcut::SPECTATE_SKIP_BATTLE_RESULT))
	{
		Settings s = settings.write["session"];
		s["spectate-skip-battle-result"].Bool() = !settings["session"]["spectate-skip-battle-result"].Bool();
	}

	ENGINE->events().dispatchShortcutPressed(shortcutsVector);
}

void InputSourceKeyboard::handleEventKeyUp(const SDL_KeyboardEvent & key)
{
	if(key.repeat != 0)
		return; // ignore periodic event resends

	if(handleBackRightMouseButton && key.keysym.scancode ==  SDL_SCANCODE_AC_BACK) // on some android devices right mouse button is "back"
	{
		ENGINE->events().dispatchClosePopup(ENGINE->getCursorPosition());
		return;
	}

	std::string keyName = getKeyNameWithModifiers(SDL_GetScancodeName(key.keysym.scancode), true);
	logGlobal->trace("keyboard: key '%s' released", keyName);

	if (SDL_IsTextInputActive() == SDL_TRUE)
	{
		if (key.keysym.sym >= ' ' && key.keysym.sym < 0x80)
			return; // printable character - will be handled as text input
	}

	assert(key.state == SDL_RELEASED);

	auto shortcutsVector = ENGINE->shortcuts().translateKeycode(keyName);
	
	ENGINE->events().dispatchKeyReleased(keyName);

	ENGINE->events().dispatchShortcutReleased(shortcutsVector);
}

bool InputSourceKeyboard::isKeyboardCmdDown() const
{
#ifdef VCMI_APPLE
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LGUI] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RGUI];
#else
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RCTRL];
#endif
}

bool InputSourceKeyboard::isKeyboardCtrlDown() const
{
#ifdef VCMI_APPLE
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RCTRL] ||
		   SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LGUI] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RGUI];
#else
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LCTRL] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RCTRL];
#endif
}

bool InputSourceKeyboard::isKeyboardAltDown() const
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LALT] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RALT];
}

bool InputSourceKeyboard::isKeyboardShiftDown() const
{
	return SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_LSHIFT] || SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_RSHIFT];
}

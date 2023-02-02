/*
 * CInGameConsole.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

class CInGameConsole : public CIntObject
{
private:
	std::list< std::pair< std::string, uint32_t > > texts; //list<text to show, time of add>
	boost::mutex texts_mx;		// protects texts
	std::vector< std::string > previouslyEntered; //previously entered texts, for up/down arrows to work
	int prevEntDisp; //displayed entry from previouslyEntered - if none it's -1
	int defaultTimeout; //timeout for new texts (in ms)
	int maxDisplayedTexts; //hiw many texts can be displayed simultaneously

	std::weak_ptr<IStatusBar> currentStatusBar;
public:
	std::string enteredText;
	void show(SDL_Surface * to) override;
	void print(const std::string &txt);
	void keyDown(const SDL_Keycode & key) override;

	void textInputed(const SDL_TextInputEvent & event) override;
	void textEdited(const SDL_TextEditingEvent & event) override;

	void startEnteringText();
	void endEnteringText(bool processEnteredText);
	void refreshEnteredText();

	CInGameConsole();
};

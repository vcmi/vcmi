#pragma once

#include "../UIFramework/CIntObject.h"

struct SDL_Surface;

/*
 * CBattleConsole.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Class which shows the console at the bottom of the battle screen and manages the text of the console
class CBattleConsole : public CIntObject
{
private:
	std::vector< std::string > texts; //a place where texts are stored
	int lastShown; //last shown line of text
public:
	std::string alterTxt; //if it's not empty, this text is displayed
	std::string ingcAlter; //alternative text set by in-game console - very important!
	int whoSetAlter; //who set alter text; 0 - battle interface or none, 1 - button
	CBattleConsole(); //c-tor
	~CBattleConsole(); //d-tor
	void show(SDL_Surface *to = 0);
	bool addText(const std::string &text); //adds text at the last position; returns false if failed (e.g. text longer than 70 characters)
	void eraseText(ui32 pos); //erases added text at position pos
	void changeTextAt(const std::string &text, ui32 pos); //if we have more than pos texts, pos-th is changed to given one
	void scrollUp(ui32 by = 1); //scrolls console up by 'by' positions
	void scrollDown(ui32 by = 1); //scrolls console up by 'by' positions
};
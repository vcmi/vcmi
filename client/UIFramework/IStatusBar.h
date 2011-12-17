#pragma once

struct SDL_Surface;

/*
 * IStatusBar.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

// Status bar interface
class IStatusBar
{
public:
	virtual ~IStatusBar(){}; //d-tor
	virtual void print(const std::string & text)=0; //prints text and refreshes statusbar
	virtual void clear()=0;//clears statusbar and refreshes
	virtual void show(SDL_Surface * to)=0; //shows statusbar (with current text)
	virtual std::string getCurrent()=0; //returns currently displayed text
};
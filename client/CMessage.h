#pragma once

#include "Graphics.h"
#include "gui/Geometries.h"


/*
 * CMessage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
struct SDL_Surface;
enum EWindowType {infoOnly, infoOK, yesOrNO};
class CInfoWindow;
class CDefHandler;
class CComponent;
class CSelWindow;
class ComponentResolved;

/// Class which draws formatted text messages and generates chat windows
class CMessage
{
public:
	//Function usd only in CMessage.cpp
	static std::pair<int,int> getMaxSizes(std::vector<std::vector<SDL_Surface*> > * txtg, int fontHeight);
	static SDL_Surface * blitTextOnSur(std::vector<std::vector<SDL_Surface*> > * txtg, int fontHeight, int & curh, SDL_Surface * ret, int xCenterPos=-1); //xPos==-1 works as if ret->w/2

	/// Draw border on exiting surface
	static void drawBorder(PlayerColor playerColor, SDL_Surface * ret, int w, int h, int x=0, int y=0);

	/// Draw simple dialog box (borders and background only)
	static SDL_Surface * drawDialogBox(int w, int h, PlayerColor playerColor = PlayerColor(1));

	static void drawIWindow(CInfoWindow * ret, std::string text, PlayerColor player);

	/// split text in lines
	static std::vector<std::string> breakText(std::string text, size_t maxLineWidth, EFonts font);

	/// constructor
	static void init();
	/// destructor
	static void dispose();
};

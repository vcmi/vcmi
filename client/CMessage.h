#pragma once

#include "FontBase.h"
#include "UIFramework/Geometries.h"


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
	static std::vector<std::vector<SDL_Surface*> > * drawText(std::vector<std::string> * brtext, int &fontHeigh, EFonts font = FONT_MEDIUM);
	static SDL_Surface * blitTextOnSur(std::vector<std::vector<SDL_Surface*> > * txtg, int fontHeight, int & curh, SDL_Surface * ret, int xCenterPos=-1); //xPos==-1 works as if ret->w/2

	/// Draw border on exiting surface
	static void drawBorder(int playerColor, SDL_Surface * ret, int w, int h, int x=0, int y=0);

	/// Draw simple dialog box (borders and background only)
	static SDL_Surface * drawDialogBox(int w, int h, int playerColor=1);

	/// Draw simple dialog box and blit bitmap with text on it
	static SDL_Surface * drawBoxTextBitmapSub(int player, std::string text, SDL_Surface* bitmap, std::string sub, int charPerline=30, int imgToBmp=55);


	static void drawIWindow(CInfoWindow * ret, std::string text, int player);

	/// split text in lines
	static std::vector<std::string> breakText(std::string text, size_t maxLineSize=30, const boost::function<int(char)> &charMetric = boost::function<int(char)>(), bool allowLeadingWhitespace = false);
	static std::vector<std::string> breakText(std::string text, size_t maxLineWidth, EFonts font);

	/// constructor
	static void init();
	/// destructor
	static void dispose();
};

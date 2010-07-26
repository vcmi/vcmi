#ifndef __CMESSAGE_H__
#define __CMESSAGE_H__

#include "FontBase.h"
#include "../global.h"
#include <SDL.h>
#include <boost/function.hpp>

/*
 * CMessage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

enum EWindowType {infoOnly, infoOK, yesOrNO};
class CPreGame;
class MapSel;
class CSimpleWindow;
class CInfoWindow;
class CDefHandler;
class SComponent;
class CSelWindow;
class CSelectableComponent;
namespace NMessage
{
	extern CDefHandler * ok, *cancel;
	extern std::vector<std::vector<SDL_Surface*> > piecesOfBox; //in colors of all players
	extern SDL_Surface * background ;
}

struct ComponentResolved
{
	SComponent *comp;

	SDL_Surface *img;
	std::vector<std::vector<SDL_Surface*> > * txt;
	int txtFontHeight;

	ComponentResolved(); //c-tor
	ComponentResolved(SComponent *Comp); //c-tor
	~ComponentResolved(); //d-tor
};

struct ComponentsToBlit
{
	std::vector< std::vector<ComponentResolved*> > comps;
	int w, h;

	void blitCompsOnSur(SDL_Surface * _or, int inter, int &curh, SDL_Surface *ret);
	ComponentsToBlit(std::vector<SComponent*> & SComps, int maxw, SDL_Surface* _or); //c-tor
	~ComponentsToBlit(); //d-tor
};

class CMessage
{
public:

	static std::pair<int,int> getMaxSizes(std::vector<std::vector<SDL_Surface*> > * txtg, int fontHeight);
	static std::vector<std::vector<SDL_Surface*> > * drawText(std::vector<std::string> * brtext, int &fontHeigh, EFonts font = FONT_MEDIUM);
	static SDL_Surface * blitTextOnSur(std::vector<std::vector<SDL_Surface*> > * txtg, int fontHeight, int & curh, SDL_Surface * ret, int xCenterPos=-1); //xPos==-1 works as if ret->w/2
	static void drawIWindow(CInfoWindow * ret, std::string text, int player);
	static CSimpleWindow * genWindow(std::string text, int player, bool centerOnMouse=false, int Lmar=35, int Rmar=35, int Tmar=35, int Bmar=35);//supports h3 text formatting; player sets color of window, Lmar/Rmar/Tmar/Bmar are Left/Right/Top/Bottom margins
	static SDL_Surface * drawBox1(int w, int h, int playerColor=1);
	static void drawBorder(int playerColor, SDL_Surface * ret, int w, int h, int x=0, int y=0);
	static SDL_Surface * drawBoxTextBitmapSub(int player, std::string text, SDL_Surface* bitmap, std::string sub, int charperline=30, int imgToBmp=55);
	static std::vector<std::string> breakText(std::string text, size_t maxLineSize=30, const boost::function<int(char)> &charMetric = boost::function<int(char)>(), bool allowLeadingWhitespace = false);
	static std::vector<std::string> breakText(std::string text, size_t maxLineWidth, EFonts font);
	static void init();
	static void dispose();
};
//



#endif // __CMESSAGE_H__

#ifndef CMESSAGE_H
#define CMESSAGE_H

#include "global.h"
#include <SDL_ttf.h>
#include "SDL.h"
#include "CPreGame.h"

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

class CMessage
{
public:

	static std::pair<int,int> getMaxSizes(std::vector<std::vector<SDL_Surface*> > * txtg);
	static std::pair<int, int> getMaxSizes(std::vector< std::vector<SComponent*> > * komp);
	static std::vector<std::vector<SDL_Surface*> > * drawText(std::vector<std::string> * brtext);
	static SDL_Surface * blitTextOnSur(std::vector<std::vector<SDL_Surface*> > * txtg, int & curh, SDL_Surface * ret);
	static SDL_Surface * blitCompsOnSur(std::vector<SComponent*> & comps, int maxw, int inter, int & curh, SDL_Surface * ret);
	static SDL_Surface * blitCompsOnSur(SDL_Surface *_or, std::vector< std::vector<SComponent*> > *komp, int inter, int &curh, SDL_Surface *ret);
	static void drawIWindow(CInfoWindow * ret, std::string text, int player, int charperline);
	static std::vector< std::vector<SComponent*> > * breakComps(std::vector<SComponent*> &comps, int maxw, SDL_Surface* _or=NULL);
	static CSelWindow * genSelWindow(std::string text, int player, int charperline, std::vector<CSelectableComponent*> & comps, int owner);
	static CSimpleWindow * genWindow(std::string text, int player, int Lmar=35, int Rmar=35, int Tmar=35, int Bmar=35);//supports h3 text formatting; player sets color of window, Lmar/Rmar/Tmar/Bmar are Left/Right/Top/Bottom margins
	static SDL_Surface * genMessage(std::string title, std::string text, EWindowType type=infoOnly,
								std::vector<CDefHandler*> *addPics=NULL, void * cb=NULL);
	static SDL_Surface * drawBox1(int w, int h, int playerColor=1);
	static void drawBorder(int playerColor, SDL_Surface * ret, int w, int h, int x=0, int y=0);
	static SDL_Surface * drawBoxTextBitmapSub(int player, std::string text, SDL_Surface* bitmap, std::string sub, int charperline=30, int imgToBmp=55);
	static std::vector<std::string> * breakText(std::string text, int line=30, bool userBreak=true, bool ifor=true); //line - chars per line
	CMessage();
	static void init();
	static void dispose();
};
//


#endif //CMESSAGE_H

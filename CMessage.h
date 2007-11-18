#ifndef CMESSAGE_H
#define CMESSAGE_H

#include "global.h"
#include "SDL_TTF.h"
#include "SDL.h"
enum EWindowType {infoOnly, infoOK, yesOrNO};
class CPreGame;
class MapSel;
class CSimpleWindow;
class CInfoWindow;
class CDefHandler;
struct SComponent;
namespace NMessage
{
	extern std::vector<std::vector<SDL_Surface*> > piecesOfBox; //in colors of all players
	extern SDL_Surface * background ;
}

class CMessage
{
public:
	
	static std::pair<int,int> getMaxSizes(std::vector<std::vector<SDL_Surface*> > * txtg);
	static std::vector<std::vector<SDL_Surface*> > * drawText(std::vector<std::string> * brtext);
	static CInfoWindow * genIWindow(std::string text, int player, int charperline, std::vector<SComponent*> & comps);
	static CSimpleWindow * genWindow(std::string text, int player, int Lmar=35, int Rmar=35, int Tmar=35, int Bmar=35);//supports h3 text formatting; player sets color of window, Lmar/Rmar/Tmar/Bmar are Left/Right/Top/Bottom margins
	static SDL_Surface * genMessage(std::string title, std::string text, EWindowType type=infoOnly, 
								std::vector<CDefHandler*> *addPics=NULL, void * cb=NULL);
	static SDL_Surface * drawBox1(int w, int h, int playerColor=1);
	static std::vector<std::string> * breakText(std::string text, int line=30, bool userBreak=true, bool ifor=true); //line - chars per line
	CMessage();
	static void init();
	static void dispose();
};
//

#endif //CMESSAGE_H
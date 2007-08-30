#ifndef CMESSAGE_H
#define CMESSAGE_H

#include "global.h"
#include "SDL_TTF.h"
#include "SDL.h"
#include "hch\CSemiDefHandler.h"
#include "hch\CDefHandler.h"
#include "CGameInterface.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
enum EWindowType {infoOnly, infoOK, yesOrNO};
class CPreGame;
class MapSel;

namespace NMessage
{
	extern std::vector<std::vector<SDL_Surface*> > piecesOfBox; //in colors of all players
	extern SDL_Surface * background ;
}

class CMessage
{
public:
	static SDL_Surface * genMessage(std::string title, std::string text, EWindowType type=infoOnly, 
								std::vector<CDefHandler*> *addPics=NULL, void * cb=NULL);
	static SDL_Surface * drawBox1(int w, int h, int playerColor=1);
	static std::vector<std::string> * breakText(std::string text, int line=30, bool userBreak=true); //line - chars per line
	CMessage();
	static void init();
	static void dispose();
};
//

#endif //CMESSAGE_H
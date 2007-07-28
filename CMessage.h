#ifndef CMESSAGE_H
#define CMESSAGE_H

#include "SDL_TTF.h"
#include "CSemiDefHandler.h"
#include "CDefHandler.h"
#include "CGameInterface.h"
enum EWindowType {infoOnly, infoOK, yesOrNO};
class CPreGame;
class MapSel;

class CMessage
{
public:
	static std::vector<std::string> * breakText(std::string text, int line=30, bool userBreak=true); //line - chars per line
	CDefHandler * piecesOfBox;
	SDL_Surface * background;
	SDL_Surface * genMessage(std::string title, std::string text, EWindowType type=infoOnly, 
								std::vector<CDefHandler*> *addPics=NULL, void * cb=NULL);
	SDL_Surface * drawBox1(int w, int h, int playerColor=1);
	CMessage();
	~CMessage();
};

//

#endif //CMESSAGE_H
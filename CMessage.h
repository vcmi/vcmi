#include "SDL_TTF.h"
#include "CSemiDefHandler.h"
enum EWindowType {infoOnly, infoOK, yesOrNO};
class CMessage
{
	SDL_Color tytulowy ;
	SDL_Color tlo;
	SDL_Color zwykly ;
public:
	std::vector<std::string> * breakText(std::string text);
	CSemiDefHandler * piecesOfBox;
	SDL_Surface * background;
	SDL_Surface * genMessage(std::string title, std::string text, EWindowType type=infoOnly);
	SDL_Surface * drawBox1(int w, int h);
	CMessage();
};
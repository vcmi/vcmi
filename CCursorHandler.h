#ifndef CCURSORHANDLER_H
#define CCURSORHANDLER_H
#include "global.h"
#include <vector>
struct SDL_Thread;
class CDefHandler;
struct SDL_Surface;

class CCursorHandler //handles cursor
{
public:
	int mode, number;
	SDL_Surface * help;

	std::vector<CDefHandler*> cursors;
	int xpos, ypos; //position of cursor
	void initCursor(); //inits cursorHandler
	void cursorMove(int x, int y); //change cursor's positions to (x, y)
	void changeGraphic(int type, int no); //changes cursor graphic for type type (0 - adventure, 1 - combat, 2 - default, 3 - spellbook) and frame no (not used for type 3)
	void draw1();
	void draw2();
};


#endif //CCURSORHANDLER_H

#ifndef __CCURSORHANDLER_H__
#define __CCURSORHANDLER_H__
#include "../global.h"
#include <vector>
struct SDL_Thread;
class CDefHandler;
struct SDL_Surface;

/*
 * CCursorhandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CCursorHandler //handles cursor
{
public:
	int mode, number;
	SDL_Surface * help;
	bool Show;

	std::vector<CDefHandler*> cursors;
	int xpos, ypos; //position of cursor
	void initCursor(); //inits cursorHandler
	void cursorMove(const int & x, const int & y); //change cursor's positions to (x, y)
	void changeGraphic(const int & type, const int & no); //changes cursor graphic for type type (0 - adventure, 1 - combat, 2 - default, 3 - spellbook) and frame no (not used for type 3)
	void draw1();
	void draw2();
	void hide(){Show=0;};
	void show(){Show=1;};
};



#endif // __CCURSORHANDLER_H__

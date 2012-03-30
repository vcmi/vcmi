#pragma once



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

namespace ECursor
{
	enum ECursorTypes { ADVENTURE, COMBAT, DEFAULT, SPELLBOOK };

	enum EBattleCursors { COMBAT_BLOCKED, COMBAT_MOVE, COMBAT_FLY, COMBAT_SHOOT,
						COMBAT_HERO, COMBAT_QUERY, COMBAT_POINTER, 
						//various attack frames
						COMBAT_SHOOT_PENALTY = 15, COMBAT_SHOOT_CATAPULT, COMBAT_HEAL,
						COMBAT_SACRIFICE, COMBAT_TELEPORT};
}

/// handles mouse cursor
class CCursorHandler 
{
public:
	int mode, number;
	SDL_Surface * help;
	SDL_Surface * dndImage;
	bool Show;

	std::vector<CDefHandler*> cursors;
	int xpos, ypos; //position of cursor
	void initCursor(); //inits cursorHandler - run only once, it's not memleak-proof (rev 1333)
	void cursorMove(const int & x, const int & y); //change cursor's positions to (x, y)
	void changeGraphic(const int & type, const int & no); //changes cursor graphic for type type (0 - adventure, 1 - combat, 2 - default, 3 - spellbook) and frame no (not used for type 3)
	void dragAndDropCursor (SDL_Surface* image); // Replace cursor with a custom image.
	void draw1();
	void draw(SDL_Surface *to);

	void shiftPos( int &x, int &y );
	void draw2();
	void hide() { Show=0; };
	void show() { Show=1; };
	void centerCursor();
	~CCursorHandler();
};

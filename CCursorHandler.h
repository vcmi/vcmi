#ifndef CCURSORHANDLER_H
#define CCURSORHANDLER_H

struct SDL_Thread;
struct CDefHandler;
struct SDL_Surface;

class CCursorHandler //handles cursor
{
public:
	SDL_Thread * myThread; //thread that updates cursor
	bool curVisible; //true if cursor is visible
	int mode, number;
	SDL_Surface * behindCur;
	int xbef, ybef; //position of cursor after last move (to restore background)

	CDefHandler * adventure, * combat, * deflt, * spell; //read - only
	int xpos, ypos; //position of cursor - read only
	void initCursor(); //inits cursorHandler
	void showGraphicCursor(); //shows default graphic cursor
	void cursorMove(int x, int y); //change cursor's positions to (x, y)
	void changeGraphic(int type, int no); //changes cursor graphic for type type (0 - adventure, 1 - combat, 2 - default, 3 - spellbook) and frame no (not used for type 3)
	void hideCursor(); //no cursor will be visible
	void hardwareCursor(); // returns to hardware cursor mode
	friend int cursorHandlerFunc(void * cursorHandler);
};


#endif //CCURSORHANDLER_H
#include "stdafx.h"
#include "CCursorHandler.h"
#include "SDL.h"
#include "SDL_thread.h"
#include "CGameInfo.h"
#include "SDL_framerate.h"
#include "hch\CLodHandler.h"

extern SDL_Surface * screen;

/* Creates a new mouse cursor from an XPM */


/* XPM */
static const char *arrow[] = { //no cursor mode
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "0,0"
};

/* XPM */
static const char *arrow2[] = { //normal cursor
  /* width height num_colors chars_per_pixel */
  "    32    32        3            1",
  /* colors */
  "X c #000000",
  ". c #ffffff",
  "  c None",
  /* pixels */
  "X                               ",
  "XX                              ",
  "X.X                             ",
  "X..X                            ",
  "X...X                           ",
  "X....X                          ",
  "X.....X                         ",
  "X......X                        ",
  "X.......X                       ",
  "X........X                      ",
  "X.....XXXXX                     ",
  "X..X..X                         ",
  "X.X X..X                        ",
  "XX  X..X                        ",
  "X    X..X                       ",
  "     X..X                       ",
  "      X..X                      ",
  "      X..X                      ",
  "       XX                       ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "                                ",
  "0,0"
};

static SDL_Cursor *init_system_cursor(const char *image[])
{
  int i, row, col;
  Uint8 data[4*32];
  Uint8 mask[4*32];
  int hot_x, hot_y;

  i = -1;
  for ( row=0; row<32; ++row ) {
    for ( col=0; col<32; ++col ) {
      if ( col % 8 ) {
        data[i] <<= 1;
        mask[i] <<= 1;
      } else {
        ++i;
        data[i] = mask[i] = 0;
      }
      switch (image[4+row][col]) {
        case 'X':
          data[i] |= 0x01;
          //k[i] |= 0x01;
          break;
        case '.':
          mask[i] |= 0x01;
          break;
        case ' ':
          break;
      }
    }
  }
  sscanf(image[4+row], "%d,%d", &hot_x, &hot_y);
  return SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
}

//int cursorHandlerFunc(void * cursorHandler)
//{
//	FPSmanager * cursorFramerateKeeper = new FPSmanager;
//	SDL_initFramerate(cursorFramerateKeeper);
//	SDL_setFramerate(cursorFramerateKeeper, 200);
//
//	CCursorHandler * ch = (CCursorHandler *) cursorHandler;
//	while(true)
//	{
//		if(ch->xbef!=-1 && ch->ybef!=-1) //restore surface under cursor
//		{
//			blitAtWR(ch->behindCur, ch->xbef, ch->ybef);
//		}
//		ch->xbef = ch->xpos;
//		ch->ybef = ch->ypos;
//		//prepare part of surface to restore
//		SDL_BlitSurface(screen, &genRect(32, 32, ch->xpos, ch->ypos), ch->behindCur, NULL);
//		CSDL_Ext::update(ch->behindCur);
//
//		//blit cursor
//		if(ch->curVisible)
//		{
//			switch(ch->mode)
//			{
//			case 0:
//				{
//					break;
//				}
//			case 1:
//				{
//					break;
//				}
//			case 2:
//				{
//					blitAtWR(ch->deflt->ourImages[ch->number].bitmap, ch->xpos, ch->ypos);
//					break;
//				}
//			}
//		}
//		SDL_framerateDelay(cursorFramerateKeeper);
//		//SDL_Delay(5); //to avoid great usage of CPU
//	}
//	return 0;
//}

void CCursorHandler::initCursor()
{
//#if SDL_BYTEORDER == SDL_BIG_ENDIAN
//    int rmask = 0xff000000;
//    int gmask = 0x00ff0000;
//    int bmask = 0x0000ff00;
//    int amask = 0x000000ff;
//#else
//    int rmask = 0x000000ff;
//    int gmask = 0x0000ff00;
//    int bmask = 0x00ff0000;
//    int amask = 0xff000000;
//#endif
//	curVisible = true;
//	xpos = ypos = 0;
//	behindCur = SDL_CreateRGBSurface(SDL_SWSURFACE, 32, 32, 32, rmask, gmask, bmask, amask);
//	xbef = ybef = 0;
//	adventure = CGI->spriteh->giveDef("CRADVNTR.DEF");
//	combat = CGI->spriteh->giveDef("CRCOMBAT.DEF");
//	deflt = CGI->spriteh->giveDef("CRDEFLT.DEF");
//	spell = CGI->spriteh->giveDef("CRSPELL.DEF");
//	//SDL_SetCursor(init_system_cursor(arrow));
//	//SDL_Thread * myth = SDL_CreateThread(&cursorHandlerFunc, this);
}

void CCursorHandler::changeGraphic(int type, int no)
{
	//mode = type;
	//number = no;
}

void CCursorHandler::cursorMove(int x, int y)
{
	//xbef = xpos;
	//ybef = ypos;
	//xpos = x;
	//ypos = y;
}

void CCursorHandler::hardwareCursor()
{
	//curVisible = false;
	//SDL_SetCursor(init_system_cursor(arrow2));
}

void CCursorHandler::hideCursor()
{
	//curVisible = false;
	//SDL_SetCursor(init_system_cursor(arrow));
}

void CCursorHandler::showGraphicCursor()
{
	//curVisible = true;
	//SDL_SetCursor(init_system_cursor(arrow));
	//changeGraphic(0, 0);
}
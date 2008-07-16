#include "stdafx.h"
#include "CScreenHandler.h"
#include "SDL.h"
#include "SDL_thread.h"
#include "SDL_framerate.h"
#include "SDL_Extensions.h"
#include "CCursorHandler.h"
#include "CGameInfo.h"
#include "hch/CDefHandler.h"

extern SDL_Surface * screen, * screen2;

void CScreenHandler::initScreen()
{
	//myth = SDL_CreateThread(&internalScreenFunc, this);
}

void CScreenHandler::updateScreen()
{
	/*blitAt(screen, 0, 0, screen2);
	switch(CGI->curh->mode)
	{
	case 0:
		{
			blitAt(CGI->curh->adventure->ourImages[CGI->curh->number].bitmap, CGI->curh->xpos, CGI->curh->ypos, screen2);
			break;
		}
	case 1:
		{
			blitAt(CGI->curh->combat->ourImages[CGI->curh->number].bitmap, CGI->curh->xpos, CGI->curh->ypos, screen2);
			break;
		}
	case 2:
		{
			blitAt(CGI->curh->deflt->ourImages[CGI->curh->number].bitmap, CGI->curh->xpos, CGI->curh->ypos, screen2);
			break;
		}
	case 3:
		{
			blitAt(CGI->curh->spell->ourImages[CGI->curh->number].bitmap, CGI->curh->xpos, CGI->curh->ypos, screen2);
			break;
		}
	}
	CSDL_Ext::update(screen2);*/
}

#include "StdInc.h"
#include "CPicture.h"

#include "../CBitmapHandler.h"
#include "../SDL_Extensions.h"
#include "../Graphics.h"


CPicture::CPicture( SDL_Surface *BG, int x, int y, bool Free )
{
	init();
	bg = BG; 
	freeSurf = Free;
	pos.x += x;
	pos.y += y;
	pos.w = BG->w;
	pos.h = BG->h;
}

CPicture::CPicture( const std::string &bmpname, int x, int y )
{
	init();
	bg = BitmapHandler::loadBitmap(bmpname); 
	freeSurf = true;;
	pos.x += x;
	pos.y += y;
	if(bg)
	{
		pos.w = bg->w;
		pos.h = bg->h;
	}
	else
	{
		pos.w = pos.h = 0;
	}
}

CPicture::CPicture(const SRect &r, const SDL_Color &color, bool screenFormat /*= false*/)
{
	init();
	createSimpleRect(r, screenFormat, SDL_MapRGB(bg->format, color.r, color.g,color.b));
}

CPicture::CPicture(const SRect &r, ui32 color, bool screenFormat /*= false*/)
{
	init();
	createSimpleRect(r, screenFormat, color);
}

CPicture::CPicture(SDL_Surface *BG, const SRect &SrcRect, int x /*= 0*/, int y /*= 0*/, bool free /*= false*/)
{
	needRefresh = false;
	srcRect = new SRect(SrcRect);
	pos.x += x;
	pos.y += y;
	pos.w = srcRect->w;
	pos.h = srcRect->h;
	bg = BG;
	freeSurf = free;
}

CPicture::~CPicture()
{
	if(freeSurf)
		SDL_FreeSurface(bg);
	delete srcRect;
}

void CPicture::init()
{
	needRefresh = false;
	srcRect = NULL;
}

void CPicture::show( SDL_Surface * to )
{
	if (needRefresh)
		showAll(to);
}

void CPicture::showAll( SDL_Surface * to )
{
	if(bg)
	{
		if(srcRect)
		{
			SDL_Rect srcRectCpy = *srcRect;
			SDL_Rect dstRect = srcRectCpy;
			dstRect.x = pos.x;
			dstRect.y = pos.y;

			CSDL_Ext::blitSurface(bg, &srcRectCpy, to, &dstRect);
		}
		else
			blitAt(bg, pos, to);
	}
}

void CPicture::convertToScreenBPP()
{
	SDL_Surface *hlp = bg;
	bg = SDL_ConvertSurface(hlp,screen->format,0);
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	SDL_FreeSurface(hlp);
}

void CPicture::createSimpleRect(const SRect &r, bool screenFormat, ui32 color)
{
	pos += r;
	pos.w = r.w;
	pos.h = r.h;
	if(screenFormat)
		bg = CSDL_Ext::newSurface(r.w, r.h);
	else
		bg = SDL_CreateRGBSurface(SDL_SWSURFACE, r.w, r.h, 8, 0, 0, 0, 0);

	SDL_FillRect(bg, NULL, color);
	freeSurf = true;
}

void CPicture::colorizeAndConvert(int player)
{
	assert(bg);
	colorize(player);
	convertToScreenBPP();
}

void CPicture::colorize(int player)
{
	assert(bg);
	assert(bg->format->BitsPerPixel == 8);
	graphics->blueToPlayersAdv(bg, player);
}
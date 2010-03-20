#include "../stdafx.h"
#include "SDL_Extensions.h"
#include "SDL_ttf.h"
#include "CGameInfo.h"
#include <iostream>
#include <utility>
#include <algorithm>
#include "CMessage.h"
#include <boost/algorithm/string.hpp>
#include "../hch/CDefHandler.h"
#include <map>
#include "Graphics.h"

/*
 * SDL_Extensions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

SDL_Surface * CSDL_Ext::newSurface(int w, int h, SDL_Surface * mod) //creates new surface, with flags/format same as in surface given
{
	return SDL_CreateRGBSurface(mod->flags,w,h,mod->format->BitsPerPixel,mod->format->Rmask,mod->format->Gmask,mod->format->Bmask,mod->format->Amask);
}

SDL_Surface * CSDL_Ext::copySurface(SDL_Surface * mod) //returns copy of given surface
{
	//return SDL_DisplayFormat(mod);
	return SDL_ConvertSurface(mod, mod->format, mod->flags);
}

bool isItIn(const SDL_Rect * rect, int x, int y)
{
	if ((x>rect->x && x<rect->x+rect->w) && (y>rect->y && y<rect->y+rect->h))
		return true;
	else return false;
}

void blitAtWR(SDL_Surface * src, int x, int y, SDL_Surface * dst)
{
	SDL_Rect pom = genRect(src->h,src->w,x,y);
	SDL_BlitSurface(src,NULL,dst,&pom);
	SDL_UpdateRect(dst,x,y,src->w,src->h);
}

void blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst)
{
	if(!dst) dst = screen;
	SDL_Rect pom = genRect(src->h,src->w,x,y);
	SDL_BlitSurface(src,NULL,dst,&pom);
}

void blitAtWR(SDL_Surface * src, const SDL_Rect & pos, SDL_Surface * dst)
{
	blitAtWR(src,pos.x,pos.y,dst);
}

void blitAt(SDL_Surface * src, const SDL_Rect & pos, SDL_Surface * dst)
{
	blitAt(src,pos.x,pos.y,dst);
}

SDL_Color genRGB(int r, int g, int b, int a=0)
{
	SDL_Color ret;
	ret.b=b;
	ret.g=g;
	ret.r=r;
	ret.unused=a;
	return ret;
}

void updateRect (SDL_Rect * rect, SDL_Surface * scr)
{
	SDL_UpdateRect(scr,rect->x,rect->y,rect->w,rect->h);
}

void printAtMiddleWB(const std::string & text, int x, int y, TTF_Font * font, int charpr, SDL_Color kolor, SDL_Surface * dst)
{
	std::vector<std::string> * ws = CMessage::breakText(text,charpr);
	std::vector<SDL_Surface*> wesu;
	wesu.resize(ws->size());
	for (size_t i=0; i < wesu.size(); ++i)
	{
		wesu[i]=TTF_RenderText_Blended(font,(*ws)[i].c_str(),kolor);
	}

	int tox=0, toy=0;
	for (size_t i=0; i < wesu.size(); ++i)
	{
		toy+=wesu[i]->h;
		if (tox < wesu[i]->w)
			tox=wesu[i]->w;
	}
	int evx, evy = y - (toy/2);
	for (size_t i=0; i < wesu.size(); ++i)
	{
		evx = (x - (tox/2)) + ((tox-wesu[i]->w)/2);
		blitAt(wesu[i],evx,evy,dst);
		evy+=wesu[i]->h;
	}


	for (size_t i=0; i < wesu.size(); ++i)
		SDL_FreeSurface(wesu[i]);
	delete ws;
}

void printAtWB(const std::string & text, int x, int y, TTF_Font * font, int charpr, SDL_Color kolor, SDL_Surface * dst)
{
	std::vector<std::string> * ws = CMessage::breakText(text,charpr);
	std::vector<SDL_Surface*> wesu;
	wesu.resize(ws->size());
	for (size_t i=0; i < wesu.size(); ++i)
		wesu[i]=TTF_RenderText_Blended(font,(*ws)[i].c_str(),kolor);

	int evy = y;
	for (size_t i=0; i < wesu.size(); ++i)
	{
		blitAt(wesu[i],x,evy,dst);
		evy+=wesu[i]->h;
	}

	for (size_t i=0; i < wesu.size(); ++i)
		SDL_FreeSurface(wesu[i]);
	delete ws;
}

void CSDL_Ext::printAtWB(const std::string & text, int x, int y, EFonts font, int charpr, SDL_Color kolor, SDL_Surface * dst, bool refresh)
{
	if (graphics->fontsTrueType[font])
	{
		printAtWB(text,x, y, graphics->fontsTrueType[font], charpr, kolor, dst);
		return;
	}
	const Font *f = graphics->fonts[font];
	std::vector<std::string> * ws = CMessage::breakText(text,charpr);

	int cury = y;
	for (size_t i=0; i < ws->size(); ++i)
	{
		printAt((*ws)[i], x, cury, font, kolor, dst, refresh);
		cury += f->height;
	}
	delete ws;
}


void CSDL_Ext::printAtMiddleWB( const std::string & text, int x, int y, EFonts font, int charpr, SDL_Color kolor/*=tytulowy*/, SDL_Surface * dst/*=screen*/, bool refrsh /*= false*/ )
{
	if (graphics->fontsTrueType[font])
	{
		printAtMiddleWB(text,x, y, graphics->fontsTrueType[font], charpr, kolor, dst);
		return;
	}

	const Font *f = graphics->fonts[font];
	std::vector<std::string> * ws = CMessage::breakText(text,charpr);
	int totalHeight = ws->size() * f->height;

	int cury = y - totalHeight/2;
	for (size_t i=0; i < ws->size(); ++i)
	{
		printAt((*ws)[i], x - f->getWidth((*ws)[i].c_str())/2, cury, font, kolor, dst, refrsh);
		cury += f->height;
	}
	delete ws;
}

void printAtMiddle(const std::string & text, int x, int y, TTF_Font * font, SDL_Color kolor, SDL_Surface * dst, unsigned char quality=2, bool refresh=false)
{
	if(text.length()==0) return;
	SDL_Surface * temp;
	switch (quality)
	{
	case 0:
		temp = TTF_RenderText_Solid(font,text.c_str(),kolor);
		break;
	case 1:
		SDL_Color tem;
		tem.b = 0xff-kolor.b;
		tem.g = 0xff-kolor.g;
		tem.r = 0xff-kolor.r;
		tem.unused = 0xff-kolor.unused;
		temp = TTF_RenderText_Shaded(font,text.c_str(),kolor,tem);
		break;
	case 2:
		temp = TTF_RenderText_Blended(font,text.c_str(),kolor);
		break;
	default:
		temp = TTF_RenderText_Blended(font,text.c_str(),kolor);
		break;
	}
	SDL_BlitSurface(temp,NULL,dst,&genRect(temp->h,temp->w,x-(temp->w/2),y-(temp->h/2)));
	if(refresh)
		SDL_UpdateRect(dst,x-(temp->w/2),y-(temp->h/2),temp->w,temp->h);
	SDL_FreeSurface(temp);
}

void CSDL_Ext::printAtMiddle( const std::string & text, int x, int y, EFonts font, SDL_Color kolor/*=zwykly*/, SDL_Surface * dst/*=screen*/, bool refresh /*= false*/ )
{
	if (graphics->fontsTrueType[font])
	{
		printAtMiddle(text,x, y, graphics->fontsTrueType[font], kolor, dst);
		return;
	}
	const Font *f = graphics->fonts[font];
	int nx = x - f->getWidth(text.c_str())/2,
		ny = y - f->height/2;

	printAt(text, nx, ny, font, kolor, dst, refresh);
}

void printAt(const std::string & text, int x, int y, TTF_Font * font, SDL_Color kolor, SDL_Surface * dst, unsigned char quality=2, bool refresh=false)
{
	if (text.length()==0)
		return;
	SDL_Surface * temp;
	switch (quality)
	{
	case 0:
		temp = TTF_RenderText_Solid(font,text.c_str(),kolor);
		break;
	case 1:
		SDL_Color tem;
		tem.b = 0xff-kolor.b;
		tem.g = 0xff-kolor.g;
		tem.r = 0xff-kolor.r;
		tem.unused = 0xff-kolor.unused;
		temp = TTF_RenderText_Shaded(font,text.c_str(),kolor,tem);
		break;
	case 2:
		temp = TTF_RenderText_Blended(font,text.c_str(),kolor);
		break;
	default:
		temp = TTF_RenderText_Blended(font,text.c_str(),kolor);
		break;
	}
	SDL_BlitSurface(temp,NULL,dst,&genRect(temp->h,temp->w,x,y));
	if(refresh)
		SDL_UpdateRect(dst,x,y,temp->w,temp->h);
	SDL_FreeSurface(temp);
}

void CSDL_Ext::printAt( const std::string & text, int x, int y, EFonts font, SDL_Color kolor/*=zwykly*/, SDL_Surface * dst/*=screen*/, bool refresh /*= false*/ )
{
	if(!text.size())
		return;
	if (graphics->fontsTrueType[font])
	{
		printAt(text,x, y, graphics->fontsTrueType[font], kolor, dst);
		return;
	}

	assert(dst);
	assert(font < Graphics::FONTS_NUMBER);
	assert(dst->format->BytesPerPixel == 3   ||   dst->format->BytesPerPixel == 4); //  24/32 bpp dst only

	const Font *f = graphics->fonts[font];
	const Uint8 bpp = dst->format->BytesPerPixel;
	Uint8 *px = NULL;
	Uint8 *src = NULL;

	//if text is in {} braces, we'll ommit them
	const int first = (text[0] == '{' ? 1 : 0);
	const int beyondEnd = (text[text.size()-1] == '}' ? text.size()-1 : text.size());

	for(int txti = first; txti < beyondEnd; txti++)
	{
		const unsigned char c = text[txti];
		x += f->chars[c].unknown1;

		for(int i = 0; i < f->height  &&  (y + i) < (dst->h - 1); i++)
		{
			px = (Uint8*)dst->pixels;
			px +=  (y+i) * dst->pitch  +  x * bpp;
	 		src = f->chars[c].pixels;
			src += i * f->chars[c].width;//if we have reached end of surface in previous line

			for(int j = 0; j < f->chars[c].width  &&  (j + x) < (dst->w - 1); j++)
			{
				switch(*src)
				{
				case 1: //black "shadow"
					memset(px, 0, bpp);
					break;
				case 255: //text colour
					px[0] = kolor.b;
					px[1] = kolor.g;
					px[2] = kolor.r;
					break;
				}
				src++;
				px += bpp;
			}
		}

		x += f->chars[c].width;
		x += f->chars[c].unknown2;
	}

	if(refresh)
	{
		SDL_UpdateRect(dst, x, y, f->getWidth(text.c_str()), f->height);
	}
}

void printTo(const std::string & text, int x, int y, TTF_Font * font, SDL_Color kolor, SDL_Surface * dst, unsigned char quality=2)
{
	if (text.length()==0)
		return;
	SDL_Surface * temp;
	switch (quality)
	{
	case 0:
		temp = TTF_RenderText_Solid(font,text.c_str(),kolor);
		break;
	case 1:
		SDL_Color tem;
		tem.b = 0xff-kolor.b;
		tem.g = 0xff-kolor.g;
		tem.r = 0xff-kolor.r;
		tem.unused = 0xff-kolor.unused;
		temp = TTF_RenderText_Shaded(font,text.c_str(),kolor,tem);
		break;
	case 2:
		temp = TTF_RenderText_Blended(font,text.c_str(),kolor);
		break;
	default:
		temp = TTF_RenderText_Blended(font,text.c_str(),kolor);
		break;
	}
	SDL_BlitSurface(temp,NULL,dst,&genRect(temp->h,temp->w,x-temp->w,y-temp->h));
	SDL_UpdateRect(dst,x-temp->w,y-temp->h,temp->w,temp->h);
	SDL_FreeSurface(temp);
}

void CSDL_Ext::printTo( const std::string & text, int x, int y, EFonts font, SDL_Color kolor/*=zwykly*/, SDL_Surface * dst/*=screen*/, bool refresh /*= false*/ )
{
	if (graphics->fontsTrueType[font])
	{
		printTo(text,x, y, graphics->fontsTrueType[font], kolor, dst);
		return;
	}
	const Font *f = graphics->fonts[font];
	printAt(text, x - f->getWidth(text.c_str()), y - f->height, font, kolor, dst, refresh);
}

void printToWR(const std::string & text, int x, int y, TTF_Font * font, SDL_Color kolor, SDL_Surface * dst, unsigned char quality=2)
{
	if (text.length()==0)
		return;
	SDL_Surface * temp;
	switch (quality)
	{
	case 0:
		temp = TTF_RenderText_Solid(font,text.c_str(),kolor);
		break;
	case 1:
		SDL_Color tem;
		tem.b = 0xff-kolor.b;
		tem.g = 0xff-kolor.g;
		tem.r = 0xff-kolor.r;
		tem.unused = 0xff-kolor.unused;
		temp = TTF_RenderText_Shaded(font,text.c_str(),kolor,tem);
		break;
	case 2:
		temp = TTF_RenderText_Blended(font,text.c_str(),kolor);
		break;
	default:
		temp = TTF_RenderText_Blended(font,text.c_str(),kolor);
		break;
	}
	SDL_BlitSurface(temp,NULL,dst,&genRect(temp->h,temp->w,x-temp->w,y-temp->h));
	SDL_FreeSurface(temp);
}

void CSDL_Ext::SDL_PutPixel(SDL_Surface *ekran, const int & x, const int & y, const Uint8 & R, const Uint8 & G, const Uint8 & B, Uint8 A)
{
	Uint8 *p = (Uint8 *)ekran->pixels + y * ekran->pitch + x * ekran->format->BytesPerPixel;

	p[0] = B;
	p[1] = G;
	p[2] = R;
	if(ekran->format->BytesPerPixel==4)
		p[3] = A;

	SDL_UpdateRect(ekran, x, y, 1, 1);
}

// Vertical flip
SDL_Surface * CSDL_Ext::rotate01(SDL_Surface * toRot)
{
	SDL_Surface * ret = SDL_ConvertSurface(toRot, toRot->format, toRot->flags);
	const int bpl = ret->pitch;
	const int bpp = ret->format->BytesPerPixel;

	for(int i=0; i<ret->h; i++) {
		char *src = (char *)toRot->pixels + i*bpl;
		char *dst = (char *)ret->pixels + i*bpl;
		for(int j=0; j<ret->w; j++) {
			for (int k=0; k<bpp; k++) {
				dst[j*bpp + k] = src[(ret->w-j-1)*bpp + k];
			}
		}
	}

	return ret;
}

// Horizontal flip
SDL_Surface * CSDL_Ext::hFlip(SDL_Surface * toRot)
{
	SDL_Surface * ret = SDL_ConvertSurface(toRot, toRot->format, toRot->flags);
	int bpl = ret->pitch;

	for(int i=0; i<ret->h; i++) {
		memcpy((char *)ret->pixels + i*bpl, (char *)toRot->pixels + (ret->h-i-1)*bpl, bpl);
	}

	return ret;
};

///**************/
///Rotates toRot surface by 90 degrees left
///**************/
SDL_Surface * CSDL_Ext::rotate02(SDL_Surface * toRot)
{
	SDL_Surface * ret = SDL_ConvertSurface(toRot, toRot->format, toRot->flags);
	//SDL_SetColorKey(ret, SDL_SRCCOLORKEY, toRot->format->colorkey);
	for(int i=0; i<ret->w; ++i)
	{
		for(int j=0; j<ret->h; ++j)
		{
			{
				Uint8 *p = (Uint8 *)toRot->pixels + i * toRot->pitch + j * toRot->format->BytesPerPixel;
/*
#if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
					SDL_PutPixelWithoutRefresh(ret, i, j, p[0], p[1], p[2]);
#else
*/
					SDL_PutPixelWithoutRefresh(ret, i, j, p[2], p[1], p[0]);
//#endif
			}
		}
	}
	return ret;
}

///*************/
///Rotates toRot surface by 180 degrees
///*************/
SDL_Surface * CSDL_Ext::rotate03(SDL_Surface * toRot)
{
	SDL_Surface * ret = SDL_ConvertSurface(toRot, toRot->format, toRot->flags);
	//SDL_SetColorKey(ret, SDL_SRCCOLORKEY, toRot->format->colorkey);
	if(ret->format->BytesPerPixel!=1)
	{
		for(int i=0; i<ret->w; ++i)
		{
			for(int j=0; j<ret->h; ++j)
			{
				{
					Uint8 *p = (Uint8 *)toRot->pixels + (ret->h - j - 1) * toRot->pitch + (ret->w - i - 1) * toRot->format->BytesPerPixel+2;
/*
#if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
						SDL_PutPixelWithoutRefresh(ret, i, j, p[0], p[1], p[2], 0);
#else
*/
						SDL_PutPixelWithoutRefresh(ret, i, j, p[2], p[1], p[0], 0);
//#endif
				}
			}
		}
	}
	else
	{
		for(int i=0; i<ret->w; ++i)
		{
			for(int j=0; j<ret->h; ++j)
			{
				Uint8 *p = (Uint8 *)toRot->pixels + (ret->h - j - 1) * toRot->pitch + (ret->w - i - 1) * toRot->format->BytesPerPixel;
				(*((Uint8*)ret->pixels + j*ret->pitch + i*ret->format->BytesPerPixel)) = *p;
			}
		}
	}
	return ret;
}
Uint32 CSDL_Ext::SDL_GetPixel(SDL_Surface *surface, const int & x, const int & y, bool colorByte)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp)
	{
    case 1:
		if(colorByte)
		{
			return colorToUint32(surface->format->palette->colors+(*p));
		}
		else
			return *p;

    case 2:
        return *(Uint16 *)p;

    case 3:
/*
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
#else
*/
            return p[0] | p[1] << 8 | p[2] << 16;
//#endif

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       // shouldn't happen, but avoids warnings
    }
}

void CSDL_Ext::alphaTransform(SDL_Surface *src)
{
	assert(src->format->BitsPerPixel == 8);
	SDL_Color colors[] = {{0,0,0,255}, {0,0,0,214}, {0,0,0,164}, {0,0,0,82}, {0,0,0,128},
						{255,0,0,0}, {255,0,0,0}, {255,0,0,0}, {0,0,0,192}, {0,0,0,192}};

	SDL_SetColors(src, colors, 0, ARRAY_COUNT(colors));
	SDL_SetColorKey(src, SDL_SRCCOLORKEY, SDL_MapRGBA(src->format, 0, 0, 0, 255));
}
//	<=>

static void prepareOutRect(SDL_Rect *src, SDL_Rect *dst, const SDL_Rect & clip_rect)
{
	const int xoffset = std::max(clip_rect.x - dst->x, 0),
		yoffset = std::max(clip_rect.y - dst->y, 0);

	src->x += xoffset;
	src->y += yoffset;
	dst->x += xoffset;
	dst->y += yoffset;

	src->w = dst->w = std::max(0,std::min(dst->w - xoffset, clip_rect.x + clip_rect.w - dst->x));
	src->h = dst->h = std::max(0,std::min(dst->h - yoffset, clip_rect.y + clip_rect.h - dst->y));
}

void CSDL_Ext::blitWithRotateClip(SDL_Surface *src,SDL_Rect * srcRect, SDL_Surface * dst, SDL_Rect * dstRect, ui8 rotation)//srcRect is not used, works with 8bpp sources and 24bpp dests
{
	static void (*blitWithRotate[])(const SDL_Surface *, const SDL_Rect *, SDL_Surface *, const SDL_Rect *) = {blitWithRotate1, blitWithRotate2, blitWithRotate3};
	if(!rotation)
	{
		SDL_BlitSurface(src, srcRect, dst, dstRect);
	}
	else
	{
		prepareOutRect(srcRect, dstRect, dst->clip_rect);
		blitWithRotate[rotation-1](src, srcRect, dst, dstRect);
	}
}

void CSDL_Ext::blitWithRotateClipVal( SDL_Surface *src,SDL_Rect srcRect, SDL_Surface * dst, SDL_Rect dstRect, ui8 rotation )
{
	blitWithRotateClip(src, &srcRect, dst, &dstRect, rotation);
}

void CSDL_Ext::blitWithRotateClipWithAlpha(SDL_Surface *src,SDL_Rect * srcRect, SDL_Surface * dst, SDL_Rect * dstRect, ui8 rotation)//srcRect is not used, works with 8bpp sources and 24bpp dests
{
	static void (*blitWithRotate[])(const SDL_Surface *, const SDL_Rect *, SDL_Surface *, const SDL_Rect *) = {blitWithRotate1WithAlpha, blitWithRotate2WithAlpha, blitWithRotate3WithAlpha};
	if(!rotation)
	{
		blit8bppAlphaTo24bpp(src, srcRect, dst, dstRect);
	}
	else
	{
		prepareOutRect(srcRect, dstRect, dst->clip_rect);
		blitWithRotate[rotation-1](src, srcRect, dst, dstRect);
	}
}

void CSDL_Ext::blitWithRotateClipValWithAlpha( SDL_Surface *src,SDL_Rect srcRect, SDL_Surface * dst, SDL_Rect dstRect, ui8 rotation )
{
	blitWithRotateClipWithAlpha(src, &srcRect, dst, &dstRect, rotation);
}

void CSDL_Ext::blitWithRotate1(const SDL_Surface *src, const SDL_Rect * srcRect, SDL_Surface * dst, const SDL_Rect * dstRect)//srcRect is not used, works with 8bpp sources and 24/32 bpp dests
{
	Uint8 *sp = (Uint8 *)src->pixels + srcRect->y*src->pitch + (src->w - srcRect->w - srcRect->x);
	const int bpp = dst->format->BytesPerPixel;
	Uint8 *dporg = (Uint8 *)dst->pixels + dstRect->y*dst->pitch + (dstRect->x+dstRect->w)*bpp;
	const SDL_Color * const colors = src->format->palette->colors;

	for(int i=dstRect->h; i>0; i--, dporg += dst->pitch)
	{
		Uint8 *dp = dporg;
		for(int j=dstRect->w; j>0; j--, sp++)
		{
			if (bpp == 4)
				*(--dp) = 0;

			const SDL_Color color = colors[*sp];

			*(--dp) = color.r;
			*(--dp) = color.g;
			*(--dp) = color.b;
		}

		sp += src->w - dstRect->w;
	}
}

void CSDL_Ext::blitWithRotate2(const SDL_Surface *src, const SDL_Rect * srcRect, SDL_Surface * dst, const SDL_Rect * dstRect)//srcRect is not used, works with 8bpp sources and 24/32 bpp dests
{
	Uint8 *sp = (Uint8 *)src->pixels + (src->h - srcRect->h - srcRect->y)*src->pitch + srcRect->x;
	const int bpp = dst->format->BytesPerPixel;
	Uint8 *dporg = (Uint8 *)dst->pixels + (dstRect->y + dstRect->h - 1)*dst->pitch + dstRect->x*bpp;
	const SDL_Color * const colors = src->format->palette->colors;

	for(int i=dstRect->h; i>0; i--, dporg -= dst->pitch)
	{
		Uint8 *dp = dporg;

		for(int j=dstRect->w; j>0; j--, sp++)
		{
			const SDL_Color color = colors[*sp];

			*(dp++) = color.b;
			*(dp++) = color.g;
			*(dp++) = color.r;

			if (bpp == 4)
				*(dp++) = 0;
		}

		sp += src->w - dstRect->w;
	}
}

void CSDL_Ext::blitWithRotate3(const SDL_Surface *src, const SDL_Rect * srcRect, SDL_Surface * dst, const SDL_Rect * dstRect)//srcRect is not used, works with 8bpp sources and 24/32 bpp dests
{
 	Uint8 *sp = (Uint8 *)src->pixels + (src->h - srcRect->h - srcRect->y)*src->pitch + (src->w - srcRect->w - srcRect->x);
 	const int bpp = dst->format->BytesPerPixel;
 	Uint8 *dporg = (Uint8 *)dst->pixels +(dstRect->y + dstRect->h - 1)*dst->pitch + (dstRect->x+dstRect->w)*bpp;
 	const SDL_Color * const colors = src->format->palette->colors;
 
 	for(int i=dstRect->h; i>0; i--, dporg -= dst->pitch)
 	{
 		Uint8 *dp = dporg;
 		for(int j=dstRect->w; j>0; j--, sp++)
 		{
 			const SDL_Color color = colors[*sp];
 
 			if (bpp == 4)
 				*(--dp) = 0;
 
 			*(--dp) = color.r;
 			*(--dp) = color.g;
 			*(--dp) = color.b;
		}

		sp += src->w - dstRect->w;
 	}
}

void CSDL_Ext::blitWithRotate1WithAlpha(const SDL_Surface *src, const SDL_Rect * srcRect, SDL_Surface * dst, const SDL_Rect * dstRect)//srcRect is not used, works with 8bpp sources and 24/32 bpp dests
{
	Uint8 *sp = (Uint8 *)src->pixels + srcRect->y*src->pitch + (src->w - srcRect->w - srcRect->x);
	const int bpp = dst->format->BytesPerPixel;
	Uint8 *dporg = (Uint8 *)dst->pixels + dstRect->y*dst->pitch + (dstRect->x+dstRect->w)*bpp;
	const SDL_Color * const colors = src->format->palette->colors;

	for(int i=dstRect->h; i>0; i--, dporg += dst->pitch)
	{
		Uint8 *dp = dporg;
		for(int j=dstRect->w; j>0; j--, sp++)
		{
			if(*sp)
			{
				if (bpp == 4)
					*(--dp) = 0;

				const SDL_Color color = colors[*sp];

				*(--dp) = color.r;
				*(--dp) = color.g;
				*(--dp) = color.b;
			}
			else
			{
				dp -= bpp;
			}
		}

		sp += src->w - dstRect->w;
	}
}

void CSDL_Ext::blitWithRotate2WithAlpha(const SDL_Surface *src, const SDL_Rect * srcRect, SDL_Surface * dst, const SDL_Rect * dstRect)//srcRect is not used, works with 8bpp sources and 24/32 bpp dests
{
	Uint8 *sp = (Uint8 *)src->pixels + (src->h - srcRect->h - srcRect->y)*src->pitch + srcRect->x;
	const int bpp = dst->format->BytesPerPixel;
	Uint8 *dporg = (Uint8 *)dst->pixels + (dstRect->y + dstRect->h - 1)*dst->pitch + dstRect->x*bpp;
	const SDL_Color * const colors = src->format->palette->colors;

	for(int i=dstRect->h; i>0; i--, dporg -= dst->pitch)
	{
		Uint8 *dp = dporg;

		for(int j=dstRect->w; j>0; j--, sp++)
		{
			if(*sp)
			{
				const SDL_Color color = colors[*sp];

				*(dp++) = color.b;
				*(dp++) = color.g;
				*(dp++) = color.r;

				if (bpp == 4)
					*(dp++) = 0;
			}
			else
			{
				dp += bpp;
			}
		}

		sp += src->w - dstRect->w;
	}
}

void CSDL_Ext::blitWithRotate3WithAlpha(const SDL_Surface *src, const SDL_Rect * srcRect, SDL_Surface * dst, const SDL_Rect * dstRect)//srcRect is not used, works with 8bpp sources and 24/32 bpp dests
{
	Uint8 *sp = (Uint8 *)src->pixels + (src->h - srcRect->h - srcRect->y)*src->pitch + (src->w - srcRect->w - srcRect->x);
	const int bpp = dst->format->BytesPerPixel;
	Uint8 *dporg = (Uint8 *)dst->pixels +(dstRect->y + dstRect->h - 1)*dst->pitch + (dstRect->x+dstRect->w)*bpp;
	const SDL_Color * const colors = src->format->palette->colors;

	for(int i=dstRect->h; i>0; i--, dporg -= dst->pitch)
	{
		Uint8 *dp = dporg;
		for(int j=dstRect->w; j>0; j--, sp++)
		{
			if(*sp)
			{
				const SDL_Color color = colors[*sp];

				if (bpp == 4)
					*(--dp) = 0;

				*(--dp) = color.r;
				*(--dp) = color.g;
				*(--dp) = color.b;
			}
			else
			{
				dp -= bpp;
			}
		}

		sp += src->w - dstRect->w;
	}
}

int CSDL_Ext::blit8bppAlphaTo24bpp(const SDL_Surface * src, const SDL_Rect * srcRect, SDL_Surface * dst, SDL_Rect * dstRect)
{
	const int bpp = dst->format->BytesPerPixel;

	if (src && src->format->BytesPerPixel==1 && dst && (bpp==3 || bpp==4)) //everything's ok
	{
		SDL_Rect fulldst;
		int srcx, srcy, w, h;

		/* Make sure the surfaces aren't locked */
		if ( ! src || ! dst )
		{
			SDL_SetError("SDL_UpperBlit: passed a NULL surface");
			return -1;
		}
		if ( src->locked || dst->locked )
		{
			SDL_SetError("Surfaces must not be locked during blit");
			return -1;
		}

		/* If the destination rectangle is NULL, use the entire dest surface */
		if ( dstRect == NULL )
		{
			fulldst.x = fulldst.y = 0;
			dstRect = &fulldst;
		}

		/* clip the source rectangle to the source surface */
		if(srcRect)
		{
			int maxw, maxh;

			srcx = srcRect->x;
			w = srcRect->w;
			if(srcx < 0)
			{
				w += srcx;
				dstRect->x -= srcx;
				srcx = 0;
			}
			maxw = src->w - srcx;
			if(maxw < w)
				w = maxw;

			srcy = srcRect->y;
			h = srcRect->h;
			if(srcy < 0)
			{
					h += srcy;
				dstRect->y -= srcy;
				srcy = 0;
			}
			maxh = src->h - srcy;
			if(maxh < h)
				h = maxh;

		}
		else
		{
			srcx = srcy = 0;
			w = src->w;
			h = src->h;
		}

		/* clip the destination rectangle against the clip rectangle */
		{
				SDL_Rect *clip = &dst->clip_rect;
			int dx, dy;

			dx = clip->x - dstRect->x;
			if(dx > 0)
			{
				w -= dx;
				dstRect->x += dx;
				srcx += dx;
			}
			dx = dstRect->x + w - clip->x - clip->w;
			if(dx > 0)
				w -= dx;

			dy = clip->y - dstRect->y;
			if(dy > 0)
			{
				h -= dy;
				dstRect->y += dy;
				srcy += dy;
			}
			dy = dstRect->y + h - clip->y - clip->h;
			if(dy > 0)
				h -= dy;
		}

		if(w > 0 && h > 0)
		{
			dstRect->w = w;
			dstRect->h = h;

			if(SDL_LockSurface(dst))
				return -1; //if we cannot lock the surface

			const SDL_Color *colors = src->format->palette->colors;
			Uint8 *colory = (Uint8*)src->pixels + srcy*src->pitch + srcx;
			Uint8 *py = (Uint8*)dst->pixels + dstRect->y*dst->pitch + dstRect->x*bpp;

			if(dst->format->Rshift==16)	//such as screen
			{
				for(int y=h; y; y--, colory+=src->pitch, py+=dst->pitch)
				{
					Uint8 *color = colory;
					Uint8 *p = py;

					for(int x=w; x; x--, p += bpp)
					{
						const SDL_Color tbc = colors[*color++]; //color to blit

						switch (tbc.unused)
						{
							case 255:
								// ~59% of calls
								break;
							case 0:
								// ~37% of calls
								p[0] = tbc.b;
								p[1] = tbc.g;
								p[2] = tbc.r;
								break;
							case 128:  // optimized
								// ~3.5% of calls
								p[0] = ((Uint16)tbc.b + (Uint16)p[0]) >> 1;
								p[1] = ((Uint16)tbc.g + (Uint16)p[1]) >> 1;
								p[2] = ((Uint16)tbc.r + (Uint16)p[2]) >> 1;
								break;
							default:
								// ~0.5% of calls
								p[0] = ((((Uint32)p[0]-(Uint32)tbc.b)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.b);
								p[1] = ((((Uint32)p[1]-(Uint32)tbc.g)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.g);
								p[2] = ((((Uint32)p[2]-(Uint32)tbc.r)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.r);
								//p[2] = ((Uint32)tbc.unused*(Uint32)p[2] + (Uint32)tbc.r*(Uint32)(255-tbc.unused))>>8; //red
								//p[1] = ((Uint32)tbc.unused*(Uint32)p[1] + (Uint32)tbc.g*(Uint32)(255-tbc.unused))>>8; //green
								//p[0] = ((Uint32)tbc.unused*(Uint32)p[0] + (Uint32)tbc.b*(Uint32)(255-tbc.unused))>>8; //blue
								break;
						}
					}
				}
			}
			else if(dst->format->Rshift==0)	//like in most surfaces
			{
				for(int y=h; y; y--, colory+=src->pitch, py+=dst->pitch)
				{
					Uint8 *color = colory;
					Uint8 *p = py;

					for(int x=w; x; x--, p += bpp)
					{
						const SDL_Color tbc = colors[*color++]; //color to blit

						// According analyze, the values of tbc.unused are fixed,
						// and the approximate ratios are as following:
						//
						// tbc.unused	numbers
						// 192			    2679
						// 164			  326907
						// 82			  705590
						// 214			 1292625
						// 128			 4842923
						// 0			72138078
						// 255			77547326
						//
						// By making use of such characteristic, we may implement a
						// very fast algorithm for heroes3 without loose much quality.
						switch (tbc.unused)
						{
							case 255:
								break;
							case 0:
								p[0] = tbc.r;
								p[1] = tbc.g;
								p[2] = tbc.b;
								break;
							case 128:  // optimized
								p[0] = ((Uint16)tbc.r + (Uint16)p[0]) >> 1;
								p[1] = ((Uint16)tbc.g + (Uint16)p[1]) >> 1;
								p[2] = ((Uint16)tbc.b + (Uint16)p[2]) >> 1;
								break;
							default:
								p[0] = ((((Uint32)p[0]-(Uint32)tbc.r)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.r);
								p[1] = ((((Uint32)p[1]-(Uint32)tbc.g)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.g);
								p[2] = ((((Uint32)p[2]-(Uint32)tbc.b)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.b);
								//p[0] = ((Uint32)tbc.unused*(Uint32)p[0] + (Uint32)tbc.r*(Uint32)(255-tbc.unused))>>8; //red
								//p[1] = ((Uint32)tbc.unused*(Uint32)p[1] + (Uint32)tbc.g*(Uint32)(255-tbc.unused))>>8; //green
								//p[2] = ((Uint32)tbc.unused*(Uint32)p[2] + (Uint32)tbc.b*(Uint32)(255-tbc.unused))>>8; //blue
								break;
						}
					}
				}
			}
			SDL_UnlockSurface(dst);
		}
	}
	return 0;
}

Uint32 CSDL_Ext::colorToUint32(const SDL_Color * color)
{
	Uint32 ret = 0;
	ret+=color->unused;
	ret<<=8; //*=256
	ret+=color->b;
	ret<<=8; //*=256
	ret+=color->g;
	ret<<=8; //*=256
	ret+=color->r;
	return ret;
}

void CSDL_Ext::update(SDL_Surface * what)
{
	if(what)
		SDL_UpdateRect(what, 0, 0, what->w, what->h);
}
void CSDL_Ext::drawBorder(SDL_Surface * sur, int x, int y, int w, int h, const int3 &color)
{
	for(int i=0;i<w;i++)
	{
		SDL_PutPixelWithoutRefresh(sur,x+i,y,color.x,color.y,color.z);
		SDL_PutPixelWithoutRefresh(sur,x+i,y+h-1,color.x,color.y,color.z);
	}
	for(int i=0; i<h;i++)
	{
		SDL_PutPixelWithoutRefresh(sur,x,y+i,color.x,color.y,color.z);
		SDL_PutPixelWithoutRefresh(sur,x+w-1,y+i,color.x,color.y,color.z);
	}
}

void CSDL_Ext::drawBorder( SDL_Surface * sur, const SDL_Rect &r, const int3 &color )
{
	drawBorder(sur, r.x, r.y, r.w, r.h, color);
}

void CSDL_Ext::setPlayerColor(SDL_Surface * sur, unsigned char player)
{
	if(player==254)
		return;
	if(sur->format->BitsPerPixel==8)
	{
		SDL_Color *color = (player == 255 
							? graphics->neutralColor
							: &graphics->playerColors[player]);
		SDL_SetColors(sur, color, 5, 1);
	}
	else
		tlog3 << "Warning, setPlayerColor called on not 8bpp surface!\n";
}
int readNormalNr (std::istream &in, int bytCon)
{
	int ret=0;
	int amp=1;
	unsigned char byte;
	if (in.good())
	{
		for (int i=0; i<bytCon; i++)
		{
			in.read((char*)&byte,1);
			ret+=byte*amp;
			amp<<=8;
		}
	}
	else return -1;
	return ret;
}

std::string CSDL_Ext::processStr(std::string str, std::vector<std::string> & tor)
{
	for (size_t i=0; (i<tor.size())&&(boost::find_first(str,"%s")); ++i)
	{
		boost::replace_first(str,"%s",tor[i]);
	}
	return str;
}

bool CSDL_Ext::isTransparent( SDL_Surface * srf, int x, int y )
{
	if(srf->format->BytesPerPixel == 1)
	{
		return ((ui8*)srf->pixels)[x + srf->pitch * y]  == 0;
	}
	else
	{
		assert(!"isTransparent called with non-8bpp surface!");
	}
}

SDL_Surface * CSDL_Ext::std32bppSurface = NULL;

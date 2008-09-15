#include "stdafx.h"
#include "SDL_Extensions.h"
#include "SDL_ttf.h"
#include "CGameInfo.h"
#include <iostream>
#include <utility>
#include <algorithm>
#include "CMessage.h"
#include <boost/algorithm/string.hpp>
#include "hch/CDefHandler.h"
#include <map>
#include "client/Graphics.h"

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
inline SDL_Rect genRect(int hh, int ww, int xx, int yy)
{
	SDL_Rect ret;
	ret.h=hh;
	ret.w=ww;
	ret.x=xx;
	ret.y=yy;
	return ret;
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
void blitAtWR(SDL_Surface * src, SDL_Rect pos, SDL_Surface * dst)
{
	blitAtWR(src,pos.x,pos.y,dst);
}
void blitAt(SDL_Surface * src, SDL_Rect pos, SDL_Surface * dst)
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
void CSDL_Ext::printAtMiddleWB(std::string text, int x, int y, TTF_Font * font, int charpr, SDL_Color kolor, SDL_Surface * dst)
{
	std::vector<std::string> * ws = CMessage::breakText(text,charpr);
	std::vector<SDL_Surface*> wesu;
	wesu.resize(ws->size());
	for (int i=0;i<wesu.size();i++)
		wesu[i]=TTF_RenderText_Blended(font,(*ws)[i].c_str(),kolor);

	int tox=0, toy=0;
	for (int i=0;i<wesu.size();i++)
	{
		toy+=wesu[i]->h;
		if (tox < wesu[i]->w)
			tox=wesu[i]->w;
	}
	int evx, evy = y - (toy/2);
	for (int i=0;i<wesu.size();i++)
	{
		evx = (x - (tox/2)) + ((tox-wesu[i]->w)/2);
		blitAt(wesu[i],evx,evy,dst);
		evy+=wesu[i]->h;
	}


	for (int i=0;i<wesu.size();i++)
		SDL_FreeSurface(wesu[i]);
	delete ws;
}
void CSDL_Ext::printAtWB(std::string text, int x, int y, TTF_Font * font, int charpr, SDL_Color kolor, SDL_Surface * dst)
{
	std::vector<std::string> * ws = CMessage::breakText(text,charpr);
	std::vector<SDL_Surface*> wesu;
	wesu.resize(ws->size());
	for (int i=0;i<wesu.size();i++)
		wesu[i]=TTF_RenderText_Blended(font,(*ws)[i].c_str(),kolor);

	int evy = y;
	for (int i=0;i<wesu.size();i++)
	{
		blitAt(wesu[i],x,evy,dst);
		evy+=wesu[i]->h;
	}

	for (int i=0;i<wesu.size();i++)
		SDL_FreeSurface(wesu[i]);
	delete ws;
}
void CSDL_Ext::printAtMiddle(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor, SDL_Surface * dst, unsigned char quality)
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
	//SDL_UpdateRect(dst,x-(temp->w/2),y-(temp->h/2),temp->w,temp->h);
	SDL_FreeSurface(temp);
}
void CSDL_Ext::printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor, SDL_Surface * dst, unsigned char quality)
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
	//SDL_UpdateRect(dst,x,y,temp->w,temp->h);
	SDL_FreeSurface(temp);
}
void CSDL_Ext::printTo(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor, SDL_Surface * dst, unsigned char quality)
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
	//SDL_UpdateRect(dst,x-temp->w,y-temp->h,temp->w,temp->h);
	SDL_FreeSurface(temp);
}

void CSDL_Ext::printToWR(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor, SDL_Surface * dst, unsigned char quality)
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

void CSDL_Ext::SDL_PutPixel(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B, int myC, Uint8 A)
{
	Uint8 *p = (Uint8 *)ekran->pixels + y * ekran->pitch + x * ekran->format->BytesPerPixel-myC;
/*
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	p[0] = R;
	p[1] = G;
	p[2] = B;
#else
*/
	p[0] = B;
	p[1] = G;
	p[2] = R;
	if(ekran->format->BytesPerPixel==4)
		p[3] = A;
//#endif
	SDL_UpdateRect(ekran, x, y, 1, 1);
}

void CSDL_Ext::SDL_PutPixelWithoutRefresh(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B, int myC, Uint8 A)
{
     Uint8 *p = (Uint8 *)ekran->pixels + y * ekran->pitch + x * ekran->format->BytesPerPixel-myC;
/*
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	p[0] = B;
	p[1] = G;
	p[2] = R;
#else
*/
	p[0] = R;
	p[1] = G;
	p[2] = B;
	if(ekran->format->BytesPerPixel==4)
		p[3] = A;
//#endif
}

///**************/
///Reverses the toRot surface by the vertical axis
///**************/
SDL_Surface * CSDL_Ext::rotate01(SDL_Surface * toRot, int myC)
{
	SDL_Surface * ret = SDL_ConvertSurface(toRot, toRot->format, toRot->flags);
	//SDL_SetColorKey(ret, SDL_SRCCOLORKEY, toRot->format->colorkey);
	if(toRot->format->BytesPerPixel!=1)
	{
		for(int i=0; i<ret->w; ++i)
		{
			for(int j=0; j<ret->h; ++j)
			{
				{
					Uint8 *p = (Uint8 *)toRot->pixels + j * toRot->pitch + (ret->w - i - 1) * toRot->format->BytesPerPixel;
/*

#if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
						CSDL_Ext::SDL_PutPixel(ret, i, j, p[0], p[1], p[2], myC);
#else
*/
						CSDL_Ext::SDL_PutPixel(ret, i, j, p[2], p[1], p[0], myC);
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
				{
					Uint8 *p = (Uint8 *)toRot->pixels + j * toRot->pitch + (ret->w - i - 1) * toRot->format->BytesPerPixel;
					(*((Uint8*)ret->pixels + j*ret->pitch + i*ret->format->BytesPerPixel)) = *p;
				}
			}
		}
	}
	return ret;
}
SDL_Surface * CSDL_Ext::hFlip(SDL_Surface * toRot)
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
					Uint8 *p = (Uint8 *)toRot->pixels + (ret->h - j -1) * toRot->pitch + i * toRot->format->BytesPerPixel;
					//int k=2;
/*
#if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
						CSDL_Ext::SDL_PutPixel(ret, i, j, p[0], p[1], p[2]);
#else
*/
						CSDL_Ext::SDL_PutPixel(ret, i, j, p[2], p[1], p[0]);
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
				{
					Uint8 *p = (Uint8 *)toRot->pixels + (ret->h - j -1) * toRot->pitch + i * toRot->format->BytesPerPixel;
					(*((Uint8*)ret->pixels + j*ret->pitch + i*ret->format->BytesPerPixel)) = *p;
				}
			}
		}
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
					SDL_PutPixel(ret, i, j, p[0], p[1], p[2]);
#else
*/
					SDL_PutPixel(ret, i, j, p[2], p[1], p[0]);
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
						SDL_PutPixel(ret, i, j, p[0], p[1], p[2], 2);
#else
*/
						SDL_PutPixel(ret, i, j, p[2], p[1], p[0], 2);
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

    switch(bpp) {
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

SDL_Surface * CSDL_Ext::alphaTransform(SDL_Surface *src)
{
	Uint32 trans = SDL_MapRGBA(src->format, 0, 255, 255, 255);
	SDL_SetColorKey(src, 0, trans);
	src->flags|=SDL_SRCALPHA;

	SDL_Color transp;
	transp.b = transp.g = transp.r = 0;
	transp.unused = 255;

	if(src->format->BitsPerPixel == 8)
	{
		for(int yy=0; yy<src->format->palette->ncolors; ++yy)
		{
			SDL_Color cur = *(src->format->palette->colors+yy);
			//if(cur.r == 255 && cur.b == 255)
			if(yy==1 || yy==2 || yy==3 || yy==4 || yy==8 || yy==9)
			{
				SDL_Color shadow;
				shadow.b = shadow.g = shadow.r = 0;
				switch(cur.g) //change this values; make diffrerent for objects and shadows (?)
				{
				case 0:
					shadow.unused = 128;
					break;
				case 50:
					shadow.unused = 50+32;
					break;
				case 100:
					shadow.unused = 100+64;
					break;
				case 125:
					shadow.unused = 125+64;
					break;
				case 128:
					shadow.unused = 128+64;
					break;
				case 150:
					shadow.unused = 150+64;
					break;
				default:
					shadow.unused = 255;
					break;
				}
				SDL_SetColors(src, &shadow, yy, 1);
			}
			if(yy==0 || (cur.r == 255 && cur.g == 0 && cur.b == 0))
			{
				SDL_SetColors(src, &transp, yy, 1);
			}
		}
	}
	return src;
}

int CSDL_Ext::blit8bppAlphaTo24bpp(SDL_Surface * src, SDL_Rect * srcRect, SDL_Surface * dst, SDL_Rect * dstRect)
{
	static std::map<Uint8, int> st;
	if(src && src->format->BytesPerPixel==1 && dst && (dst->format->BytesPerPixel==3 || dst->format->BytesPerPixel==4)) //everything's ok
	{
		SDL_Rect fulldst;
		int srcx, srcy, w, h;

		/* Make sure the surfaces aren't locked */
		if ( ! src || ! dst ) {
			SDL_SetError("SDL_UpperBlit: passed a NULL surface");
			return(-1);
		}
		if ( src->locked || dst->locked ) {
			SDL_SetError("Surfaces must not be locked during blit");
			return(-1);
		}

		/* If the destination rectangle is NULL, use the entire dest surface */
		if ( dstRect == NULL ) {
				fulldst.x = fulldst.y = 0;
			dstRect = &fulldst;
		}

		/* clip the source rectangle to the source surface */
		if(srcRect) {
				int maxw, maxh;

			srcx = srcRect->x;
			w = srcRect->w;
			if(srcx < 0) {
					w += srcx;
				dstRect->x -= srcx;
				srcx = 0;
			}
			maxw = src->w - srcx;
			if(maxw < w)
				w = maxw;

			srcy = srcRect->y;
			h = srcRect->h;
			if(srcy < 0) {
					h += srcy;
				dstRect->y -= srcy;
				srcy = 0;
			}
			maxh = src->h - srcy;
			if(maxh < h)
				h = maxh;

		} else {
				srcx = srcy = 0;
			w = src->w;
			h = src->h;
		}

		/* clip the destination rectangle against the clip rectangle */
		{
				SDL_Rect *clip = &dst->clip_rect;
			int dx, dy;

			dx = clip->x - dstRect->x;
			if(dx > 0) {
				w -= dx;
				dstRect->x += dx;
				srcx += dx;
			}
			dx = dstRect->x + w - clip->x - clip->w;
			if(dx > 0)
				w -= dx;

			dy = clip->y - dstRect->y;
			if(dy > 0) {
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
			SDL_Rect sr;
			sr.x = srcx;
			sr.y = srcy;
			sr.w = dstRect->w = w;
			sr.h = dstRect->h = h;

			if(SDL_LockSurface(dst))
				return -1; //if we cannot lock the surface

			if(dst->format->Rshift==0) //like in most surfaces
			{
				for(Uint16 y=0; y<sr.h; ++y)
				{
					for(Uint16 x=0; x<sr.w; ++x)
					{
						SDL_Color & tbc = src->format->palette->colors[*((Uint8*)src->pixels + (y+sr.y)*src->pitch + x + sr.x)]; //color to blit
						Uint8 * p = (Uint8*)dst->pixels + (y+dstRect->y)*dst->pitch + (x+dstRect->x)*dst->format->BytesPerPixel; //place to blit at

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
						switch ((Uint32)tbc.unused)
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
								p[0] = ((((Uint32)p[0]-(Uint32)tbc.r)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.r) & 0xFF;
								p[1] = ((((Uint32)p[1]-(Uint32)tbc.g)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.g) & 0xFF;
								p[2] = ((((Uint32)p[2]-(Uint32)tbc.b)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.b) & 0xFF;
								//p[0] = ((Uint32)tbc.unused*(Uint32)p[0] + (Uint32)tbc.r*(Uint32)(255-tbc.unused))>>8; //red
								//p[1] = ((Uint32)tbc.unused*(Uint32)p[1] + (Uint32)tbc.g*(Uint32)(255-tbc.unused))>>8; //green
								//p[2] = ((Uint32)tbc.unused*(Uint32)p[2] + (Uint32)tbc.b*(Uint32)(255-tbc.unused))>>8; //blue
								break;
						}
					}
				}
			}
			else if(dst->format->Rshift==16) //such as screen
			{
				for(Uint16 y=0; y<sr.h; ++y)
				{
					for(Uint16 x=0; x<sr.w; ++x)
					{
						SDL_Color & tbc = src->format->palette->colors[*((Uint8*)src->pixels + (y+sr.y)*src->pitch + x + sr.x)]; //color to blit
						Uint8 * p = (Uint8*)dst->pixels + (y+dstRect->y)*dst->pitch + (x+dstRect->x)*dst->format->BytesPerPixel; //place to blit at

						switch ((Uint32)tbc.unused)
						{
							case 255:
								break;
							case 0:
								p[2] = tbc.r;
								p[1] = tbc.g;
								p[0] = tbc.b;
								break;
							case 128:  // optimized
								p[2] = ((Uint16)tbc.r + (Uint16)p[2]) >> 1;
								p[1] = ((Uint16)tbc.g + (Uint16)p[1]) >> 1;
								p[0] = ((Uint16)tbc.b + (Uint16)p[0]) >> 1;
								break;
							default:
								p[2] = ((((Uint32)p[2]-(Uint32)tbc.r)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.r) & 0xFF;
								p[1] = ((((Uint32)p[1]-(Uint32)tbc.g)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.g) & 0xFF;
								p[0] = ((((Uint32)p[0]-(Uint32)tbc.b)*(Uint32)tbc.unused) >> 8 + (Uint32)tbc.b) & 0xFF;
								//p[2] = ((Uint32)tbc.unused*(Uint32)p[2] + (Uint32)tbc.r*(Uint32)(255-tbc.unused))>>8; //red
								//p[1] = ((Uint32)tbc.unused*(Uint32)p[1] + (Uint32)tbc.g*(Uint32)(255-tbc.unused))>>8; //green
								//p[0] = ((Uint32)tbc.unused*(Uint32)p[0] + (Uint32)tbc.b*(Uint32)(255-tbc.unused))>>8; //blue
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
void CSDL_Ext::drawBorder(SDL_Surface * sur, int x, int y, int w, int h, int3 color)
{
	for(int i=0;i<w;i++)
	{
		SDL_PutPixel(sur,x+i,y,color.x,color.y,color.z);
		SDL_PutPixel(sur,x+i,y+h-1,color.x,color.y,color.z);
	}
	for(int i=0; i<h;i++)
	{
		SDL_PutPixel(sur,x,y+i,color.x,color.y,color.z);
		SDL_PutPixel(sur,x+w-1,y+i,color.x,color.y,color.z);
	}
}
void CSDL_Ext::setPlayerColor(SDL_Surface * sur, unsigned char player)
{
	if(player==254)
		return;
	if(sur->format->BitsPerPixel==8)
	{
		if(player != 255)
			*(sur->format->palette->colors+5) = graphics->playerColors[player];
		else
			*(sur->format->palette->colors+5) = *graphics->neutralColor;
	}
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
	for (int i=0;(i<tor.size())&&(boost::find_first(str,"%s"));i++)
	{
		boost::replace_first(str,"%s",tor[i]);
	}
	return str;
}

SDL_Surface * CSDL_Ext::std32bppSurface = NULL;

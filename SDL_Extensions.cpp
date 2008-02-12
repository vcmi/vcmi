#include "stdafx.h"
#include "SDL_Extensions.h"
#include "SDL_TTF.h"
#include "CGameInfo.h"
#include <iostream>
#include <utility>
#include <algorithm>
#include "CMessage.h"
#include <boost/algorithm/string.hpp>
#include "hch\CDefHandler.h"
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
	SDL_UpdateRect(dst,x-(temp->w/2),y-(temp->h/2),temp->w,temp->h);
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
	SDL_UpdateRect(dst,x,y,temp->w,temp->h);
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
	SDL_UpdateRect(dst,x-temp->w,y-temp->h,temp->w,temp->h);
	SDL_FreeSurface(temp);
}
void CSDL_Ext::SDL_PutPixel(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B, int myC, Uint8 A)
{
	Uint8 *p = (Uint8 *)ekran->pixels + y * ekran->pitch + x * ekran->format->BytesPerPixel-myC;
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	p[0] = R;
	p[1] = G;
	p[2] = B;
#else
	p[0] = B;
	p[1] = G;
	p[2] = R;
	if(ekran->format->BytesPerPixel==4)
		p[3] = A;
#endif
	SDL_UpdateRect(ekran, x, y, 1, 1);
}

void CSDL_Ext::SDL_PutPixelWithoutRefresh(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B, int myC, Uint8 A)
{
     Uint8 *p = (Uint8 *)ekran->pixels + y * ekran->pitch + x * ekran->format->BytesPerPixel-myC;
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
	p[0] = B;
	p[1] = G;
	p[2] = R;
#else
	p[0] = R;
	p[1] = G;
	p[2] = B;
	if(ekran->format->BytesPerPixel==4)
		p[3] = A;
#endif
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
#if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
						CSDL_Ext::SDL_PutPixel(ret, i, j, p[0], p[1], p[2], myC);
#else
						CSDL_Ext::SDL_PutPixel(ret, i, j, p[2], p[1], p[0], myC);
#endif
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
#if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
						CSDL_Ext::SDL_PutPixel(ret, i, j, p[0], p[1], p[2]);
#else
						CSDL_Ext::SDL_PutPixel(ret, i, j, p[2], p[1], p[0]);
#endif
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
#if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
					SDL_PutPixel(ret, i, j, p[0], p[1], p[2]);
#else
					SDL_PutPixel(ret, i, j, p[2], p[1], p[0]);
#endif
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
#if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
						SDL_PutPixel(ret, i, j, p[0], p[1], p[2], 2);
#else
						SDL_PutPixel(ret, i, j, p[2], p[1], p[0], 2);
#endif
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
 //converts surface to cursor
SDL_Cursor * CSDL_Ext::SurfaceToCursor(SDL_Surface *image, int hx, int hy)
{
	int             w, x, y;
	Uint8           *data, *mask, *d, *m, r, g, b;
	Uint32          color;
	SDL_Cursor      *cursor;

	w = (image->w + 7) / 8;
	data = (Uint8 *)alloca(w * image->h * 2);
	if (data == NULL)
		return NULL;
	memset(data, 0, w * image->h * 2);
	mask = data + w * image->h;
	if (SDL_MUSTLOCK(image))
		SDL_LockSurface(image);
	for (y = 0; y < image->h; y++)
	{
		d = data + y * w;
		m = mask + y * w;
		for (x = 0; x < image->w; x++)
		{
			color = CSDL_Ext::SDL_GetPixel(image, x, y);
			if ((image->flags & SDL_SRCCOLORKEY) == 0 || color != image->format->colorkey)
			{
				SDL_GetRGB(color, image->format, &r, &g, &b);
				color = (r + g + b) / 3;
				m[x / 8] |= 128 >> (x & 7);
				if (color < 128)
					d[x / 8] |= 128 >> (x & 7);
			}
		}
	}
	if (SDL_MUSTLOCK(image))
		SDL_UnlockSurface(image);
	cursor = SDL_CreateCursor(data, mask, w, image->h, hx, hy);
	return cursor;
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
#if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
#else
            return p[0] | p[1] << 8 | p[2] << 16;
#endif

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
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
	SDL_UpdateRect(src, 0, 0, src->w, src->h);
	return src;
}

SDL_Surface * CSDL_Ext::secondAlphaTransform(SDL_Surface * src, SDL_Surface * alpha)
{
	return copySurface(src);
}

int CSDL_Ext::blit8bppAlphaTo24bpp(SDL_Surface * src, SDL_Rect * srcRect, SDL_Surface * dst, SDL_Rect * dstRect)
{
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

			if(dst->format->Rshift==0) //like in most surfaces
			{
				for(int x=0; x<sr.w; ++x)
				{
					for(int y=0; y<sr.h; ++y)
					{
						SDL_Color tbc = src->format->palette->colors[*((Uint8*)src->pixels + (y+sr.y)*src->pitch + x + sr.x)]; //color to blit
						Uint8 * p = (Uint8*)dst->pixels + (y+dstRect->y)*dst->pitch + (x+dstRect->x)*dst->format->BytesPerPixel; //place to blit at
						p[0] = ((Uint32)tbc.unused*(Uint32)p[0] + (Uint32)tbc.r*(Uint32)(255-tbc.unused))>>8; //red
						p[1] = ((Uint32)tbc.unused*(Uint32)p[1] + (Uint32)tbc.g*(Uint32)(255-tbc.unused))>>8; //green
						p[2] = ((Uint32)tbc.unused*(Uint32)p[2] + (Uint32)tbc.b*(Uint32)(255-tbc.unused))>>8; //blue
					}
				}
			}
			else if(dst->format->Rshift==16) //such as screen
			{
				for(int x=0; x<sr.w; ++x)
				{
					for(int y=0; y<sr.h; ++y)
					{
						SDL_Color tbc = src->format->palette->colors[*((Uint8*)src->pixels + (y+sr.y)*src->pitch + x + sr.x)]; //color to blit
						Uint8 * p = (Uint8*)dst->pixels + (y+dstRect->y)*dst->pitch + (x+dstRect->x)*dst->format->BytesPerPixel; //place to blit at
						p[2] = ((Uint32)tbc.unused*(Uint32)p[2] + (Uint32)tbc.r*(Uint32)(255-tbc.unused))>>8; //red
						p[1] = ((Uint32)tbc.unused*(Uint32)p[1] + (Uint32)tbc.g*(Uint32)(255-tbc.unused))>>8; //green
						p[0] = ((Uint32)tbc.unused*(Uint32)p[0] + (Uint32)tbc.b*(Uint32)(255-tbc.unused))>>8; //blue
					}
				}
			}
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

void CSDL_Ext::blueToPlayers(SDL_Surface * sur, int player)
{
	if(sur->format->BitsPerPixel == 8)
	{
		for(int i=0; i<sur->format->palette->ncolors; ++i)
		{
			SDL_Color * cc = sur->format->palette->colors+i;
			if(cc->r==0 && cc->g==0 && cc->b==255)
			{
				cc->r = CGameInfo::mainObj->playerColors[player].r;
				cc->g = CGameInfo::mainObj->playerColors[player].g;
				cc->b = CGameInfo::mainObj->playerColors[player].b;
			}
		}
	}
	else if(sur->format->BitsPerPixel == 24)
	{
		for(int y=0; y<sur->h; ++y)
		{
			for(int x=0; x<sur->w; ++x)
			{
				Uint8* cp = (Uint8*)sur->pixels + y+sur->pitch + x*3;
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				{
					if(cp[0]==0 && cp[1]==0 && cp[2]==255)
					{
						cp[0] = CGameInfo::mainObj->playerColors[player].r;
						cp[1] = CGameInfo::mainObj->playerColors[player].g;
						cp[2] = CGameInfo::mainObj->playerColors[player].b;
					}
				}
				else
				{
					
					if(cp[0]==255 && cp[1]==0 && cp[2]==0)
					{
						cp[0] = CGameInfo::mainObj->playerColors[player].b;
						cp[1] = CGameInfo::mainObj->playerColors[player].g;
						cp[2] = CGameInfo::mainObj->playerColors[player].r;
					}
				}
			}
		}
	}
}

void CSDL_Ext::blueToPlayersAdv(SDL_Surface * sur, int player, int mode, void* additionalInfo)
{
	if(player==1) //it is actually blue...
		return;
	if(sur->format->BitsPerPixel == 8)
	{
		for(int i=0; i<sur->format->palette->ncolors; ++i) //message, button, avmap, resbar
		{
			SDL_Color * cc = sur->format->palette->colors+i;
			if(
				((mode==0) && (cc->b>cc->g) && (cc->b>cc->r)) ||
				((mode==1) && (cc->r<45) && (cc->b>80) && (cc->g<70) && ((cc->b-cc->r)>40)) ||
				((mode==2) && (cc->r<110) && (cc->b>63) && (cc->g<122) && ((cc->b-cc->r)>44) && ((cc->b-cc->g)>32))
			  )
			{
				if ((mode==2) && additionalInfo)
				{
					for (int vi=0; vi<((std::vector<SDL_Color>*)additionalInfo)->size(); vi++)
					{
						if 
						  (
							((*((std::vector<SDL_Color>*)additionalInfo))[vi].r==cc->r) &&
							((*((std::vector<SDL_Color>*)additionalInfo))[vi].g==cc->g) &&
							((*((std::vector<SDL_Color>*)additionalInfo))[vi].b==cc->b)
						  )	
							goto main8bitloopend;
					}
				}
				std::vector<long long int> sort1;
				sort1.push_back(cc->r);
				sort1.push_back(cc->g);
				sort1.push_back(cc->b);
				std::vector< std::pair<long long int, Uint8*> > sort2;
				sort2.push_back(std::make_pair(CGameInfo::mainObj->playerColors[player].r, &(cc->r)));
				sort2.push_back(std::make_pair(CGameInfo::mainObj->playerColors[player].g, &(cc->g)));
				sort2.push_back(std::make_pair(CGameInfo::mainObj->playerColors[player].b, &(cc->b)));
				std::sort(sort1.begin(), sort1.end());
				if(sort2[0].first>sort2[1].first)
					std::swap(sort2[0], sort2[1]);
				if(sort2[1].first>sort2[2].first)
					std::swap(sort2[1], sort2[2]);
				if(sort2[0].first>sort2[1].first)
					std::swap(sort2[0], sort2[1]);
				for(int hh=0; hh<3; ++hh)
				{
					(*sort2[hh].second) = (sort1[hh]*0.8 + sort2[hh].first)/2;
				}
			}
main8bitloopend:
			;
		}
	}
	else if(sur->format->BitsPerPixel == 24)
	{
		for(int y=0; y<sur->h; ++y)
		{
			for(int x=0; x<sur->w; ++x)
			{
				Uint8* cp = (Uint8*)sur->pixels + y*sur->pitch + x*3;
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				{
					if(cp[2]>cp[1] && cp[2]>cp[0])
					{
						std::vector<long long int> sort1;
						sort1.push_back(cp[0]);
						sort1.push_back(cp[1]);
						sort1.push_back(cp[2]);
						std::vector< std::pair<long long int, Uint8*> > sort2;
						sort2.push_back(std::make_pair(CGameInfo::mainObj->playerColors[player].r, &(cp[0])));
						sort2.push_back(std::make_pair(CGameInfo::mainObj->playerColors[player].g, &(cp[1])));
						sort2.push_back(std::make_pair(CGameInfo::mainObj->playerColors[player].b, &(cp[2])));
						std::sort(sort1.begin(), sort1.end());
						if(sort2[0].first>sort2[1].first)
							std::swap(sort2[0], sort2[1]);
						if(sort2[1].first>sort2[2].first)
							std::swap(sort2[1], sort2[2]);
						if(sort2[0].first>sort2[1].first)
							std::swap(sort2[0], sort2[1]);
						for(int hh=0; hh<3; ++hh)
						{
							(*sort2[hh].second) = (sort1[hh] + sort2[hh].first)/2.2;
						}
					}
				}
				else
				{
					if(
						((mode==0) && (cp[0]>cp[1]) && (cp[0]>cp[2])) ||
						((mode==1) && (cp[2]<45) && (cp[0]>80) && (cp[1]<70) && ((cp[0]-cp[1])>40))
					  )
					{
						std::vector<long long int> sort1;
						sort1.push_back(cp[2]);
						sort1.push_back(cp[1]);
						sort1.push_back(cp[0]);
						std::vector< std::pair<long long int, Uint8*> > sort2;
						sort2.push_back(std::make_pair(CGameInfo::mainObj->playerColors[player].r, &(cp[2])));
						sort2.push_back(std::make_pair(CGameInfo::mainObj->playerColors[player].g, &(cp[1])));
						sort2.push_back(std::make_pair(CGameInfo::mainObj->playerColors[player].b, &(cp[0])));
						std::sort(sort1.begin(), sort1.end());
						if(sort2[0].first>sort2[1].first)
							std::swap(sort2[0], sort2[1]);
						if(sort2[1].first>sort2[2].first)
							std::swap(sort2[1], sort2[2]);
						if(sort2[0].first>sort2[1].first)
							std::swap(sort2[0], sort2[1]);
						for(int hh=0; hh<3; ++hh)
						{
							(*sort2[hh].second) = (sort1[hh]*0.8 + sort2[hh].first)/2;
						}
					}
				}
			}
		}
	}
}

void CSDL_Ext::blueToPlayersNice(SDL_Surface * sur, int player) //incomplete, TODO: finish
{
	if(sur->format->BitsPerPixel==8)
	{
		for(int a=0; a<sur->format->palette->ncolors; ++a)
		{
			for(int s=0; s<CGI->playerColorInfo[0]->ourImages[1].bitmap->format->palette->ncolors; ++s)
			{
				if(abs((sur->format->palette->colors+a)->b - (CGI->playerColorInfo[0]->ourImages[1].bitmap->format->palette->colors+s)->b) < 5
					&& abs((sur->format->palette->colors+a)->g - (CGI->playerColorInfo[0]->ourImages[1].bitmap->format->palette->colors+s)->g) < 5
					&& abs((sur->format->palette->colors+a)->r - (CGI->playerColorInfo[0]->ourImages[1].bitmap->format->palette->colors+s)->r) < 5
					)
				{
					(sur->format->palette->colors+a)->b = (CGI->playerColorInfo[0]->ourImages[player].bitmap->format->palette->colors+s)->b;
					(sur->format->palette->colors+a)->r = (CGI->playerColorInfo[0]->ourImages[player].bitmap->format->palette->colors+s)->g;
					(sur->format->palette->colors+a)->g = (CGI->playerColorInfo[0]->ourImages[player].bitmap->format->palette->colors+s)->r;
					break;
				}
			}
		}
	}
}

void CSDL_Ext::setPlayerColor(SDL_Surface * sur, unsigned char player)
{
	if(player==254)
		return;
	if(sur->format->BitsPerPixel==8)
	{
		if(player != 255) 
			*(sur->format->palette->colors+5) = CGameInfo::mainObj->playerColors[player];
		else
			*(sur->format->palette->colors+5) = CGameInfo::mainObj->neutralColor;
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

//void CSDL_Ext::fullAlphaTransform(SDL_Surface *& src)
//{
//	src = alphaTransform(src);
//	//SDL_Surface * hlp2;
//	//hlp2 = secondAlphaTransform(src, std32bppSurface);
//	//SDL_FreeSurface(src);
//	//src = hlp2;
//}

std::string CSDL_Ext::processStr(std::string str, std::vector<std::string> & tor)
{
	for (int i=0;(i<tor.size())&&(boost::find_first(str,"%s"));i++)
	{
		boost::replace_first(str,"%s",tor[i]);
	}
	return str;
}

SDL_Surface * CSDL_Ext::std32bppSurface = NULL;

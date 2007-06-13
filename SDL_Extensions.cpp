#include "stdafx.h"
#include "SDL_Extensions.h"
extern SDL_Surface * ekran;
SDL_Rect genRect(int hh, int ww, int xx, int yy)
{
	SDL_Rect ret;
	ret.h=hh;
	ret.w=ww;
	ret.x=xx;
	ret.y=yy;
	return ret;
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
void updateRect (SDL_Rect * rect, SDL_Surface * scr = ekran)
{
	SDL_UpdateRect(scr,rect->x,rect->y,rect->w,rect->h);
}
void CSDL_Ext::SDL_PutPixel(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B, int myC)
{
     Uint8 *p = (Uint8 *)ekran->pixels + y * ekran->pitch + x * ekran->format->BytesPerPixel-myC;
     if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
     {
          p[0] = R;
          p[1] = G;
          p[2] = B;
     }
     else
     {
          p[0] = B;
          p[1] = G;
          p[2] = R;
     }
     SDL_UpdateRect(ekran, x, y, 1, 1);
}

///**************/
///Reverses the toRot surface by the vertical axis
///**************/
SDL_Surface * CSDL_Ext::rotate01(SDL_Surface * toRot)
{
	SDL_Surface * first = SDL_CreateRGBSurface(toRot->flags, toRot->w, toRot->h, toRot->format->BitsPerPixel, toRot->format->Rmask, toRot->format->Gmask, toRot->format->Bmask, toRot->format->Amask);
	SDL_Surface * ret = SDL_ConvertSurface(first, toRot->format, toRot->flags);
	for(int i=0; i<ret->w; ++i)
	{
		for(int j=0; j<ret->h; ++j)
		{
			{
				Uint8 *p = (Uint8 *)toRot->pixels + j * toRot->pitch + (ret->w - i - 1) * toRot->format->BytesPerPixel;
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				{
					CSDL_Ext::SDL_PutPixel(ret, i, j, p[0], p[1], p[2], 2);
				}
				else
				{
					CSDL_Ext::SDL_PutPixel(ret, i, j, p[2], p[1], p[0], 2);
				}
			}
		}
	}
	SDL_FreeSurface(first);
	return ret;
}
SDL_Surface * CSDL_Ext::hFlip(SDL_Surface * toRot)
{
	SDL_Surface * first = SDL_CreateRGBSurface(toRot->flags, toRot->w, toRot->h, toRot->format->BitsPerPixel, toRot->format->Rmask, toRot->format->Gmask, toRot->format->Bmask, toRot->format->Amask);
	SDL_Surface * ret = SDL_ConvertSurface(first, toRot->format, toRot->flags);
	for(int i=0; i<ret->w; ++i)
	{
		for(int j=0; j<ret->h; ++j)
		{
			{
				Uint8 *p = (Uint8 *)toRot->pixels + (ret->h - j -1) * toRot->pitch + i * toRot->format->BytesPerPixel-2;
				int k=2;
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				{
					CSDL_Ext::SDL_PutPixel(ret, i, j, p[0], p[1], p[2], k);
				}
				else
				{
					CSDL_Ext::SDL_PutPixel(ret, i, j, p[2], p[1], p[0], k);
				}
			}
		}
	}
	SDL_FreeSurface(first);
	return ret;
};

///**************/
///Rotates toRot surface by 90 degrees left
///**************/
SDL_Surface * CSDL_Ext::rotate02(SDL_Surface * toRot)
{
	SDL_Surface * first = SDL_CreateRGBSurface(toRot->flags, toRot->h, toRot->w, toRot->format->BitsPerPixel, toRot->format->Rmask, toRot->format->Gmask, toRot->format->Bmask, toRot->format->Amask);
	SDL_Surface * ret = SDL_ConvertSurface(first, toRot->format, toRot->flags);
	for(int i=0; i<ret->w; ++i)
	{
		for(int j=0; j<ret->h; ++j)
		{
			{
				Uint8 *p = (Uint8 *)toRot->pixels + i * toRot->pitch + j * toRot->format->BytesPerPixel;
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				{
					SDL_PutPixel(ret, i, j, p[0], p[1], p[2]);
				}
				else
				{
					SDL_PutPixel(ret, i, j, p[2], p[1], p[0]);
				}
			}
		}
	}
	SDL_FreeSurface(first);
	return ret;
}

///*************/
///Rotates toRot surface by 180 degrees
///*************/
SDL_Surface * CSDL_Ext::rotate03(SDL_Surface * toRot)
{
	SDL_Surface * first = SDL_CreateRGBSurface(toRot->flags, toRot->w, toRot->h, toRot->format->BitsPerPixel, toRot->format->Rmask, toRot->format->Gmask, toRot->format->Bmask, toRot->format->Amask);
	SDL_Surface * ret = SDL_ConvertSurface(first, toRot->format, toRot->flags);
	for(int i=0; i<ret->w; ++i)
	{
		for(int j=0; j<ret->h; ++j)
		{
			{
				Uint8 *p = (Uint8 *)toRot->pixels + (ret->h - j - 1) * toRot->pitch + (ret->w - i - 1) * toRot->format->BytesPerPixel+2;
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				{
					SDL_PutPixel(ret, i, j, p[0], p[1], p[2], 2);
				}
				else
				{
					SDL_PutPixel(ret, i, j, p[2], p[1], p[0], 2);
				}
			}
		}
	}
	SDL_FreeSurface(first);
	return ret;
}

#include "stdafx.h"
#include "SDL_Extensions.h"

void CSDL_Ext::SDL_PutPixel(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B)
{
     Uint8 *p = (Uint8 *)ekran->pixels + y * ekran->pitch + x * ekran->format->BytesPerPixel;
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
				Uint8 *p = (Uint8 *)toRot->pixels + (ret->h - j - 1) * toRot->pitch + (ret->w - i - 1) * toRot->format->BytesPerPixel;
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

#include "stdafx.h"
#include "SDL_Extensions.h"
#include "SDL_TTF.h"
#include <iostream>
extern SDL_Surface * ekran;
extern SDL_Color tytulowy, tlo, zwykly ;
bool isItIn(const SDL_Rect * rect, int x, int y)
{
	if ((x>rect->x && x<rect->x+rect->w) && (y>rect->y && y<rect->y+rect->h))
		return true;
	else return false;
}
SDL_Rect genRect(int hh, int ww, int xx, int yy)
{
	SDL_Rect ret;
	ret.h=hh;
	ret.w=ww;
	ret.x=xx;
	ret.y=yy;
	return ret;
}
void blitAt(SDL_Surface * src, int x, int y, SDL_Surface * dst=ekran)
{
	SDL_BlitSurface(src,NULL,dst,&genRect(src->h,src->w,x,y));
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
void printAt(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor=tytulowy, SDL_Surface * dst=ekran)
{
	SDL_Surface * temp = TTF_RenderText_Blended(font,text.c_str(),kolor);
	SDL_BlitSurface(temp,NULL,dst,&genRect(temp->h,temp->w,x,y));
	SDL_UpdateRect(dst,x,y,temp->w,temp->h);
	SDL_FreeSurface(temp);
}
void CSDL_Ext::SDL_PutPixel(SDL_Surface *ekran, int x, int y, Uint8 R, Uint8 G, Uint8 B, int myC, Uint8 A)
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
SDL_Surface * CSDL_Ext::rotate01(SDL_Surface * toRot, int myC)
{
	SDL_Surface * first = SDL_CreateRGBSurface(toRot->flags, toRot->w, toRot->h, toRot->format->BitsPerPixel, toRot->format->Rmask, toRot->format->Gmask, toRot->format->Bmask, toRot->format->Amask);
	SDL_Surface * ret = SDL_ConvertSurface(first, toRot->format, toRot->flags);
	//SDL_SetColorKey(ret, SDL_SRCCOLORKEY, toRot->format->colorkey);
	for(int i=0; i<ret->w; ++i)
	{
		for(int j=0; j<ret->h; ++j)
		{
			{
				Uint8 *p = (Uint8 *)toRot->pixels + j * toRot->pitch + (ret->w - i - 1) * toRot->format->BytesPerPixel;
				if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
				{
					CSDL_Ext::SDL_PutPixel(ret, i, j, p[0], p[1], p[2], myC);
				}
				else
				{
					CSDL_Ext::SDL_PutPixel(ret, i, j, p[2], p[1], p[0], myC);
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
	//SDL_SetColorKey(ret, SDL_SRCCOLORKEY, toRot->format->colorkey);
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
	//SDL_SetColorKey(ret, SDL_SRCCOLORKEY, toRot->format->colorkey);
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
	//SDL_SetColorKey(ret, SDL_SRCCOLORKEY, toRot->format->colorkey);
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

Uint32 CSDL_Ext::SDL_GetPixel(SDL_Surface *surface, int x, int y, bool colorByte)
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
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;

    case 4:
        return *(Uint32 *)p;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}

SDL_Surface * CSDL_Ext::alphaTransform(SDL_Surface *src)
{
	Uint32 trans = SDL_MapRGBA(src->format, 0, 255, 255, 255);
	//SDL_SetColorKey(src, SDL_SRCCOLORKEY, trans);
	/*SDL_SetColorKey(src, 0, trans);
	src = SDL_ConvertSurface(src, ekran->format, ekran->flags);
	for(int i=0; i<src->w; ++i)
	{
		for(int j=0; j<src->h; ++j)
		{
			Uint8 cr, cg, cb, ca;
			SDL_GetRGBA(SDL_GetPixel(src, i, j), src->format, &cr, &cg, &cb, &ca);
			if(cr == 255 && cb == 255)
			{
				Uint32 aaaa=src->format->Amask;
				Uint32 aaab=src->format->Bmask;
				Uint32 aaag=src->format->Gmask;
				Uint32 aaar=src->format->Rmask;
				Uint32 put = cg << 24 | cr << 16 | ca << 8 | cb;
				SDL_Rect rrr = genRect(1, 1, i, j);
				SDL_FillRect(src, &rrr, put);
			}
		}
	}*/
	//SDL_UpdateRect(src, 0, 0, src->w, src->h);
	SDL_SetColorKey(src, 0, trans);
	src->flags|=SDL_SRCALPHA;
	
	if(src->format->BitsPerPixel == 8)
	{
		for(int yy=0; yy<src->format->palette->ncolors; ++yy)
		{
			SDL_Color cur = *(src->format->palette->colors+yy);
			if(cur.r == 255 && cur.b == 255)
			{
				SDL_Color shadow;
				shadow.b = shadow.g = shadow.r = 0;
				shadow.unused = cur.g + 25; //25 is a scalable constans to make it nicer
				SDL_SetColors(src, &shadow, yy, 1);
			}
			if(cur.g == 255 && cur.b == 255)
			{
				SDL_Color transp;
				transp.b = transp.g = transp.r = 0;
				transp.unused = 255;
				SDL_SetColors(src, &transp, yy, 1);
			}
		}
	}
	SDL_UpdateRect(src, 0, 0, src->w, src->h);
	return src;
}

Uint32 CSDL_Ext::colorToUint32(const SDL_Color * color)
{
	Uint32 ret = 0;
	ret+=color->unused;
	ret*=256;
	ret+=color->b;
	ret*=256;
	ret+=color->g;
	ret*=256;
	ret+=color->r;
	return ret;
}

#include "stdafx.h"
#include "SDL_Extensions.h"
#include "SDL_TTF.h"
#include "CGameInfo.h"
#include <iostream>
#include <utility>
#include <algorithm>

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
void CSDL_Ext::printAtMiddle(std::string text, int x, int y, TTF_Font * font, SDL_Color kolor, SDL_Surface * dst, unsigned char quality)
{
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
	if(toRot->format->BytesPerPixel!=1)
	{
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
	SDL_FreeSurface(first);
	return ret;
}
SDL_Surface * CSDL_Ext::hFlip(SDL_Surface * toRot)
{
	SDL_Surface * first = SDL_CreateRGBSurface(toRot->flags, toRot->w, toRot->h, toRot->format->BitsPerPixel, toRot->format->Rmask, toRot->format->Gmask, toRot->format->Bmask, toRot->format->Amask);
	SDL_Surface * ret = SDL_ConvertSurface(first, toRot->format, toRot->flags);
	//SDL_SetColorKey(ret, SDL_SRCCOLORKEY, toRot->format->colorkey);
	if(ret->format->BytesPerPixel!=1)
	{
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
	if(ret->format->BytesPerPixel!=1)
	{
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
	SDL_SetColorKey(src, 0, trans);
	src->flags|=SDL_SRCALPHA;
	
	if(src->format->BitsPerPixel == 8)
	{
		for(int yy=0; yy<src->format->palette->ncolors; ++yy)
		{
			SDL_Color cur = *(src->format->palette->colors+yy);
			//if(cur.r == 255 && cur.b == 255)
			if(yy==1 || yy==2 || yy==3 || yy==4 || yy==8)
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

SDL_Surface * CSDL_Ext::secondAlphaTransform(SDL_Surface * src, SDL_Surface * alpha)
{
	
	Uint32 pompom[256][256];
	for(int i=0; i<src->w; ++i)
	{
		for(int j=0; j<src->h; ++j)
		{
			pompom[i][j] = 0xffffffff - (SDL_GetPixel(src, i, j, true) & 0xff000000);
		}
	}
	Uint32 pompom2[256][256];
	for(int i=0; i<src->w; ++i)
	{
		for(int j=0; j<src->h; ++j)
		{
			pompom2[i][j] = pompom[i][j]>>24;
		}
	}
	SDL_Surface * hide2 = SDL_ConvertSurface(src, alpha->format, SDL_SWSURFACE);
	for(int i=0; i<hide2->w; ++i)
	{
		for(int j=0; j<hide2->h; ++j)
		{
			Uint32 * place = (Uint32*)( (Uint8*)hide2->pixels + j * hide2->pitch + i * hide2->format->BytesPerPixel);
			(*place)&=pompom[i][j];
			int ffgg=0;
		}
	}
	return hide2;
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

void CSDL_Ext::update(SDL_Surface * what)
{
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

void CSDL_Ext::blueToPlayersAdv(SDL_Surface * sur, int player)
{
	if(sur->format->BitsPerPixel == 8)
	{
		for(int i=0; i<sur->format->palette->ncolors; ++i)
		{
			SDL_Color * cc = sur->format->palette->colors+i;
			if(cc->b>cc->g && cc->b>cc->r)
			{
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
					if(cp[0]>cp[1] && cp[0]>cp[2])
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

void CSDL_Ext::setPlayerColor(SDL_Surface * sur, int player)
{
	if(sur->format->BitsPerPixel==8)
	{
		if(player != -1) 
			*(sur->format->palette->colors+5) = CGameInfo::mainObj->playerColors[player];
		else
			*(sur->format->palette->colors+5) = CGameInfo::mainObj->neutralColor;
	}
}
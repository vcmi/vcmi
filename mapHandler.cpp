#include "stdafx.h"
#include "mapHandler.h"
#include "CSemiDefHandler.h"
#include "SDL_rotozoom.h"
#include "SDL_Extensions.h"
extern SDL_Surface * ekran;
void mapHandler::init()
{
	terrainBitmap = new SDL_Surface **[reader->map.width];
	for (int ii=0;ii<reader->map.width;ii++)
		terrainBitmap[ii] = new SDL_Surface*[reader->map.height]; // allocate memory 
	for (int i=0; i<reader->map.width; i++)
	{
		for (int j=0; j<reader->map.height;j++)
		{
			TerrainTile zz = reader->map.terrain[i][j];
			std::string name = CSemiDefHandler::nameFromType(reader->map.terrain[i][j].tertype);
			for (int k=0; k<reader->defs.size(); k++)
			{
				try
				{
					if (reader->defs[k]->defName != name)
						continue;
					else
					{
						SDL_Surface * n;
						int ktora = reader->map.terrain[i][j].terview;
						terrainBitmap[i][j] = reader->defs[k]->ourImages[ktora].bitmap;
						//TODO: odwracanie	
						switch ((reader->map.terrain[i][j].siodmyTajemniczyBajt)%4)
						{
						case 1:
							{
								terrainBitmap[i][j] = CSDL_Ext::rotate01(terrainBitmap[i][j]);
								break;
							}
						case 2:
							{
								terrainBitmap[i][j] = CSDL_Ext::hFlip(terrainBitmap[i][j]);
								break;
							}
						case 3:
							{
								terrainBitmap[i][j] = CSDL_Ext::rotate03(terrainBitmap[i][j]);
								break;
							}
						}
						//SDL_BlitSurface(terrainBitmap[i][j],NULL,ekran,NULL); SDL_Flip(ekran);SDL_Delay(50);

						break;
					}
				}
				catch (...)
				{	continue;	}
			}
		}
	}
}
SDL_Surface * mapHandler::terrainRect(int x, int y, int dx, int dy)
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    int rmask = 0xff000000;
    int gmask = 0x00ff0000;
    int bmask = 0x0000ff00;
    int amask = 0x000000ff;
#else
    int rmask = 0x000000ff;
    int gmask = 0x0000ff00;
    int bmask = 0x00ff0000;
    int amask = 0xff000000;
#endif
	SDL_Surface * su = SDL_CreateRGBSurface(SDL_SWSURFACE, dx*32, dy*32, 32,
                                   rmask, gmask, bmask, amask);
	if (((dx+x)>((reader->map.width)-1) || (dy+y)>((reader->map.height)-1)) || ((x<0)||(y<0) ) )
		throw new std::string("Poza zakresem");
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			SDL_Rect * sr = new SDL_Rect;
			sr->y=by*32;
			sr->x=bx*32;
			sr->h=sr->w=32;
			
			SDL_BlitSurface(terrainBitmap[bx+x][by+y],NULL,su,sr);

			//SDL_BlitSurface(su,NULL,ekran,NULL);SDL_Flip(ekran);
		}
	}
	return su;
}

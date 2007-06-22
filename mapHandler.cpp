#include "stdafx.h"
#include "mapHandler.h"
#include "CSemiDefHandler.h"
#include "SDL_rotozoom.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"

extern SDL_Surface * ekran;
void mapHandler::init()
{
	terrainBitmap = new SDL_Surface **[reader->map.width+8];
	for (int ii=0;ii<reader->map.width+8;ii++)
		terrainBitmap[ii] = new SDL_Surface*[reader->map.height+8]; // allocate memory 
	CSemiDefHandler * bord = CGameInfo::mainObj->sspriteh->giveDef("EDG.DEF");
	for (int i=0; i<reader->map.width+8; i++) //jest po szerokoœci
	{
		for (int j=0; j<reader->map.height+8;j++) //po wysokoœci
		{
			if(i < 4 || i > (reader->map.width+3) || j < 4  || j > (reader->map.height+3))
			{
				if(i==3 && j==3)
				{
					terrainBitmap[i][j] = bord->ourImages[16].bitmap;
					continue;
				}
				else if(i==3 && j==(reader->map.height+4))
				{
					terrainBitmap[i][j] = bord->ourImages[19].bitmap;
					continue;
				}
				else if(i==(reader->map.width+4) && j==3)
				{
					terrainBitmap[i][j] = bord->ourImages[17].bitmap;
					continue;
				}
				else if(i==(reader->map.width+4) && j==(reader->map.height+4))
				{
					terrainBitmap[i][j] = bord->ourImages[18].bitmap;
					continue;
				}
				else if(j == 3 && i > 3 && i < reader->map.height+4)
				{
					terrainBitmap[i][j] = bord->ourImages[22+rand()%2].bitmap;
					continue;
				}
				else if(i == 3 && j > 3 && j < reader->map.height+4)
				{
					terrainBitmap[i][j] = bord->ourImages[33+rand()%2].bitmap;
					continue;
				}
				else if(j == reader->map.height+4 && i > 3 && i < reader->map.width+4)
				{
					terrainBitmap[i][j] = bord->ourImages[29+rand()%2].bitmap;
					continue;
				}
				else if(i == reader->map.width+4 && j > 3 && j < reader->map.height+4)
				{
					terrainBitmap[i][j] = bord->ourImages[25+rand()%2].bitmap;
					continue;
				}
				else
				{
					terrainBitmap[i][j] = bord->ourImages[rand()%16].bitmap;
					continue;
				}
			}
			TerrainTile zz = reader->map.terrain[i-4][j-4];
			std::string name = CSemiDefHandler::nameFromType(reader->map.terrain[i-4][j-4].tertype);
			for (unsigned int k=0; k<reader->defs.size(); k++)
			{
				try
				{
					if (reader->defs[k]->defName != name)
						continue;
					else
					{
						SDL_Surface * n;
						int ktora = reader->map.terrain[i-4][j-4].terview;
						terrainBitmap[i][j] = reader->defs[k]->ourImages[ktora].bitmap;
						//TODO: odwracanie	
						switch ((reader->map.terrain[i-4][j-4].siodmyTajemniczyBajt)%4)
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
	if (reader->map.twoLevel)
	{
		undTerrainBitmap = new SDL_Surface **[reader->map.width+8];
		for (int ii=0;ii<reader->map.width+8;ii++)
			undTerrainBitmap[ii] = new SDL_Surface*[reader->map.height+8]; // allocate memory 
		for (int i=0; i<reader->map.width+8; i++)
		{
			for (int j=0; j<reader->map.height+8;j++)
			{
				if(i < 4 || i > (reader->map.width+3) || j < 4  || j > (reader->map.height+3))
				{
					if(i==3 && j==3)
					{
						undTerrainBitmap[i][j] = bord->ourImages[16].bitmap;
						continue;
					}
					else if(i==3 && j==(reader->map.height+4))
					{
						undTerrainBitmap[i][j] = bord->ourImages[19].bitmap;
						continue;
					}
					else if(i==(reader->map.width+4) && j==3)
					{
						undTerrainBitmap[i][j] = bord->ourImages[17].bitmap;
						continue;
					}
					else if(i==(reader->map.width+4) && j==(reader->map.height+4))
					{
						undTerrainBitmap[i][j] = bord->ourImages[18].bitmap;
						continue;
					}
					else if(j == 3 && i > 3 && i < reader->map.height+4)
					{
						undTerrainBitmap[i][j] = bord->ourImages[22+rand()%2].bitmap;
						continue;
					}
					else if(i == 3 && j > 3 && j < reader->map.height+4)
					{
						undTerrainBitmap[i][j] = bord->ourImages[33+rand()%2].bitmap;
						continue;
					}
					else if(j == reader->map.height+4 && i > 3 && i < reader->map.width+4)
					{
						undTerrainBitmap[i][j] = bord->ourImages[29+rand()%2].bitmap;
						continue;
					}
					else if(i == reader->map.width+4 && j > 3 && j < reader->map.height+4)
					{
						undTerrainBitmap[i][j] = bord->ourImages[25+rand()%2].bitmap;
						continue;
					}
					else
					{
						undTerrainBitmap[i][j] = bord->ourImages[rand()%16].bitmap;
						continue;
					}
				}
				TerrainTile zz = reader->map.undergroungTerrain[i-4][j-4];
				std::string name = CSemiDefHandler::nameFromType(reader->map.undergroungTerrain[i-4][j-4].tertype);
				for (unsigned int k=0; k<reader->defs.size(); k++)
				{
					try
					{
						if (reader->defs[k]->defName != name)
							continue;
						else
						{
							SDL_Surface * n;
							int ktora = reader->map.undergroungTerrain[i-4][j-4].terview;
							undTerrainBitmap[i][j] = reader->defs[k]->ourImages[ktora].bitmap;
							//TODO: odwracanie	
							switch ((reader->map.undergroungTerrain[i-4][j-4].siodmyTajemniczyBajt)%4)
							{
							case 1:
								{
									undTerrainBitmap[i][j] = CSDL_Ext::rotate01(undTerrainBitmap[i][j]);
									break;
								}
							case 2:
								{
									undTerrainBitmap[i][j] = CSDL_Ext::hFlip(undTerrainBitmap[i][j]);
									break;
								}
							case 3:
								{
									undTerrainBitmap[i][j] = CSDL_Ext::rotate03(undTerrainBitmap[i][j]);
									break;
								}
							}
							//SDL_BlitSurface(undTerrainBitmap[i][j],NULL,ekran,NULL); SDL_Flip(ekran);SDL_Delay(50);

							break;
						}
					}
					catch (...)
					{	continue;	}
				}
			}
		}
	}
}
SDL_Surface * mapHandler::terrainRect(int x, int y, int dx, int dy, int level)
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
	if (((dx+x)>((reader->map.width+8)) || (dy+y)>((reader->map.height+8))) || ((x<0)||(y<0) ) )
		throw new std::string("Poza zakresem");
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			SDL_Rect * sr = new SDL_Rect;
			sr->y=by*32;
			sr->x=bx*32;
			sr->h=sr->w=32;
			if (!level)
				SDL_BlitSurface(terrainBitmap[bx+x][by+y],NULL,su,sr);
			else 
				SDL_BlitSurface(undTerrainBitmap[bx+x][by+y],NULL,su,sr);
			delete sr;
			//SDL_BlitSurface(su,NULL,ekran,NULL);SDL_Flip(ekran);
		}
	}
	return su;
}

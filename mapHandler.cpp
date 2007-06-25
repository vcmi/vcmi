#include "stdafx.h"
#include "mapHandler.h"
#include "CSemiDefHandler.h"
#include "SDL_rotozoom.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include "stdlib.h"

extern SDL_Surface * ekran;
void CMapHandler::init()
{
	fullHide = CGameInfo::mainObj->sspriteh->giveDef("TSHRC.DEF", 2);
	partialHide = CGameInfo::mainObj->sspriteh->giveDef("TSHRE.DEF", 2);

	for(int i=0; i<partialHide->ourImages.size(); ++i)
	{
		//CSDL_Ext::alphaTransform(partialHide->ourImages[i].bitmap);
	}

	visibility.resize(reader->map.width+8);
	for(int gg=0; gg<reader->map.width+8; ++gg)
	{
		visibility[gg].resize(reader->map.height+8);
		for(int jj=0; jj<reader->map.height+8; ++jj)
			visibility[gg][jj] = true;
	}
	undVisibility.resize(reader->map.width+8);
	for(int gg=0; gg<reader->map.width+8; ++gg)
	{
		undVisibility[gg].resize(reader->map.height+8);
		for(int jj=0; jj<reader->map.height+8; ++jj)
			undVisibility[gg][jj] = true;
	}

	visibility[6][7] = false;
	undVisibility[5][7] = false;
	visibility[7][7] = false;
	visibility[6][8] = false;
	visibility[6][6] = false;
	//visibility[5][6] = false;
	//visibility[7][8] = false;
	visibility[5][8] = false;
	visibility[7][6] = false;
	visibility[6][9] = false;

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
			} //end of internal for
		} //end of external for
	} //end of if
}

SDL_Surface * CMapHandler::terrainRect(int x, int y, int dx, int dy, int level)
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
			{
				SDL_BlitSurface(terrainBitmap[bx+x][by+y],NULL,su,sr);
				if( bx+x>3 && by+y>3 && bx+x<visibility.size()-3 && by+y<visibility[0].size()-3 && !visibility[bx+x][by+y])
				{
					SDL_Surface * hide = CSDL_Ext::alphaTransform(getVisBitmap(bx+x, by+y, visibility));
					Uint32 pompom[32][32];
					for(int i=0; i<hide->w; ++i)
					{
						for(int j=0; j<hide->h; ++j)
						{
							pompom[i][j] = 0xffffffff - (CSDL_Ext::SDL_GetPixel(hide, i, j, true) & 0xff000000);
						}
					}
					hide = SDL_ConvertSurface(hide, su->format, SDL_SWSURFACE);
					for(int i=0; i<hide->w; ++i)
					{
						for(int j=0; j<hide->h; ++j)
						{
							Uint32 * place = (Uint32*)( (Uint8*)hide->pixels + j * hide->pitch + i * hide->format->BytesPerPixel);
							(*place)&=pompom[i][j];
						}
					}
					SDL_BlitSurface(hide, NULL, su, sr);
				}
			}
			else 
			{
				SDL_BlitSurface(undTerrainBitmap[bx+x][by+y],NULL,su,sr);
				if( bx+x>3 && by+y>3 && bx+x<undVisibility.size()-3 && by+y<undVisibility[0].size()-3 && !undVisibility[bx+x][by+y])
				{
					SDL_Surface * hide = CSDL_Ext::alphaTransform(getVisBitmap(bx+x, by+y, undVisibility));
					Uint32 pompom[32][32];
					for(int i=0; i<hide->w; ++i)
					{
						for(int j=0; j<hide->h; ++j)
						{
							pompom[i][j] = 0xffffffff - (CSDL_Ext::SDL_GetPixel(hide, i, j, true) & 0xff000000);
						}
					}
					hide = SDL_ConvertSurface(hide, su->format, SDL_SWSURFACE);
					for(int i=0; i<hide->w; ++i)
					{
						for(int j=0; j<hide->h; ++j)
						{
							Uint32 * place = (Uint32*)( (Uint8*)hide->pixels + j * hide->pitch + i * hide->format->BytesPerPixel);
							(*place)&=pompom[i][j];
						}
					}
					SDL_BlitSurface(hide, NULL, su, sr);
				}
			}
			delete sr;
			//SDL_BlitSurface(su,NULL,ekran,NULL);SDL_Flip(ekran);
		}
	}
	return su;
}

SDL_Surface * CMapHandler::terrBitmap(int x, int y)
{
	return terrainBitmap[x+4][y+4];
}

SDL_Surface * CMapHandler::undTerrBitmap(int x, int y)
{
	return undTerrainBitmap[x+4][y+4];
}

SDL_Surface * CMapHandler::getVisBitmap(int x, int y, std::vector< std::vector<bool> > & visibility)
{
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && !visibility[x+1][y+1] && !visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		return fullHide->ourImages[rand()%fullHide->ourImages.size()].bitmap; //fully hidden
	}
	else if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && visibility[x+1][y+1] && !visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		return partialHide->ourImages[22].bitmap; //visible right bottom corner
	}
	else if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && !visibility[x+1][y+1] && visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		return partialHide->ourImages[15].bitmap; //visible right top corner
	}
	else if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && !visibility[x+1][y+1] && !visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[22].bitmap); //visible left bottom corner
	}
	else if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && !visibility[x+1][y+1] && !visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[15].bitmap); //visible left top corner
	}
	else if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && visibility[x][y-1] && visibility[x-1][y-1] && !visibility[x+1][y+1] && visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		return partialHide->ourImages[rand()%2].bitmap; //visible top
	}
	else if(visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && visibility[x+1][y+1] && !visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return partialHide->ourImages[4+rand()%2].bitmap; //visble bottom
	}
	else if(!visibility[x][y+1] && !visibility[x+1][y] && visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && !visibility[x+1][y+1] && !visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[2+rand()%2].bitmap); //visible left
	}
	else if(!visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && visibility[x+1][y+1] && visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		return partialHide->ourImages[2+rand()%2].bitmap; //visible right
	}
	else if(visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1])
	{
		return partialHide->ourImages[12+2*(rand()%2)].bitmap; //visible bottom, right - bottom, right; left top corner hidden
	}
	else if(!visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && visibility[x][y-1] && !visibility[x-1][y+1])
	{
		return partialHide->ourImages[13].bitmap; //visible right, right - top; left bottom corner hidden
	}
	else if(!visibility[x][y+1] && !visibility[x+1][y] && visibility[x-1][y] && visibility[x][y-1] && !visibility[x+1][y+1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[13].bitmap); //visible top, top - left, left; right bottom corner hidden
	}
	else if(visibility[x][y+1] && !visibility[x+1][y] && visibility[x-1][y] && !visibility[x][y-1]  && !visibility[x+1][y-1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[12+2*(rand()%2)].bitmap); //visible left, left - bottom, bottom; right top corner hidden
	}
	else if(visibility[x][y+1] && visibility[x+1][y] && visibility[x-1][y] && visibility[x][y-1] && visibility[x-1][y-1] && visibility[x+1][y+1] && visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return partialHide->ourImages[10].bitmap; //visible left, right, bottom and top
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && visibility[x+1][y+1] && visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		return partialHide->ourImages[16].bitmap; //visible right corners
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && !visibility[x+1][y+1] && visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		return partialHide->ourImages[18].bitmap; //visible top corners
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && !visibility[x+1][y+1] && !visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[16].bitmap); //visible left corners
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && visibility[x+1][y+1] && !visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return CSDL_Ext::hFlip(partialHide->ourImages[18].bitmap); //visible bottom corners
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && !visibility[x+1][y+1] && visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return partialHide->ourImages[17].bitmap; //visible right - top and bottom - left corners
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && visibility[x+1][y+1] && !visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		return CSDL_Ext::hFlip(partialHide->ourImages[17].bitmap); //visible top - left and bottom - right corners
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && visibility[x+1][y+1] && visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return partialHide->ourImages[19].bitmap; //visible corners without left top
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && visibility[x+1][y+1] && visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		return partialHide->ourImages[20].bitmap; //visible corners without left bottom
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && !visibility[x+1][y+1] && visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[20].bitmap); //visible corners without right bottom
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && visibility[x+1][y+1] && !visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[19].bitmap); //visible corners without right top
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && visibility[x+1][y+1] && visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		return partialHide->ourImages[21].bitmap; //visible all corners only
	}
	if(visibility[x][y+1] && visibility[x+1][y] && visibility[x-1][y] && !visibility[x][y-1])
	{
		return partialHide->ourImages[6].bitmap; //hidden top
	}
	if(visibility[x][y+1] && !visibility[x+1][y] && visibility[x-1][y] && visibility[x][y-1])
	{
		return partialHide->ourImages[7].bitmap; //hidden right
	}
	if(!visibility[x][y+1] && visibility[x+1][y] && visibility[x-1][y] && visibility[x][y-1])
	{
		return partialHide->ourImages[8].bitmap; //hidden bottom
	}
	if(visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && visibility[x][y-1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[6].bitmap); //hidden left
	}
	if(!visibility[x][y+1] && visibility[x+1][y] && visibility[x-1][y] && visibility[x][y-1])
	{
		return partialHide->ourImages[9].bitmap; //hidden top and bottom
	}
	if(visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && visibility[x][y-1])
	{
		return partialHide->ourImages[29].bitmap;  //hidden left and right
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && visibility[x][y-1] && visibility[x+1][y+1] && !visibility[x-1][y+1])
	{
		return partialHide->ourImages[24].bitmap; //visible top and right bottom corner
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && visibility[x][y-1] && !visibility[x+1][y+1] && visibility[x-1][y+1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[24].bitmap); //visible top and left bottom corner
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && visibility[x][y-1] && visibility[x+1][y+1] && visibility[x-1][y+1])
	{
		return partialHide->ourImages[33].bitmap; //visible top and bottom corners
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && visibility[x-1][y] && !visibility[x][y-1] && !visibility[x+1][y+1] && visibility[x+1][y-1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[26].bitmap); //visible left and right top corner
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && visibility[x-1][y] && !visibility[x][y-1] && visibility[x+1][y+1] && !visibility[x+1][y-1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[25].bitmap); //visible left and right bottom corner
	}
	if(!visibility[x][y+1] && !visibility[x+1][y] && visibility[x-1][y] && !visibility[x][y-1] && visibility[x+1][y+1] && visibility[x+1][y-1])
	{
		return partialHide->ourImages[32].bitmap; //visible left and right corners
	}
	if(visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && !visibility[x+1][y-1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[30].bitmap); //visible bottom and left top corner
	}
	if(visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && visibility[x+1][y-1])
	{
		return partialHide->ourImages[30].bitmap; //visible bottom and right top corner
	}
	if(visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && visibility[x+1][y-1])
	{
		return partialHide->ourImages[31].bitmap; //visible bottom and top corners
	}
	if(!visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && visibility[x-1][y+1])
	{
		return partialHide->ourImages[25].bitmap; //visible right and left bottom corner
	}
	if(!visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && !visibility[x-1][y+1])
	{
		return partialHide->ourImages[26].bitmap; //visible right and left top corner
	}
	if(!visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && visibility[x-1][y+1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[32].bitmap); //visible right and left cornres
	}
	if(visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1])
	{
		return partialHide->ourImages[28].bitmap; //visible bottom, right - bottom, right; left top corner visible
	}
	else if(!visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && visibility[x][y-1] && visibility[x-1][y+1])
	{
		return partialHide->ourImages[27].bitmap; //visible right, right - top; left bottom corner visible
	}
	else if(!visibility[x][y+1] && !visibility[x+1][y] && visibility[x-1][y] && visibility[x][y-1] && visibility[x+1][y+1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[27].bitmap); //visible top, top - left, left; right bottom corner visible
	}
	else if(visibility[x][y+1] && !visibility[x+1][y] && visibility[x-1][y] && !visibility[x][y-1]  && visibility[x+1][y-1])
	{
		return CSDL_Ext::rotate01(partialHide->ourImages[28].bitmap); //visible left, left - bottom, bottom; right top corner visible
	}
	return fullHide->ourImages[0].bitmap; //this case should never happen, but it is better to hide too much than reveal it....
}

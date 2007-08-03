#include "stdafx.h"
#include "mapHandler.h"
#include "CSemiDefHandler.h"
#include "SDL_rotozoom.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include "stdlib.h"
#include <algorithm>

extern SDL_Surface * ekran;
//
//bool ObjSorter::operator <(const ObjSorter & por) const
//{
//	if(this->xpos>=por.xpos)
//		return false;
//	if(this->ypos>=por.ypos)
//		return false;
//	return true;
//};

class poX
{
public:
	bool operator()(const ObjSorter & por2, const ObjSorter & por) const
	{
		if(por2.xpos<=por.xpos)
			return false;
		return true;
	};
} pox;
class poY
{
public:
	bool operator ()(const ObjSorter & por2, const ObjSorter & por) const
	{
		if(por2.ypos>=por.ypos)
			return false;
		return true;
	};
} poy;

void CMapHandler::init()
{
	fullHide = CGameInfo::mainObj->spriteh->giveDef("TSHRC.DEF");
	partialHide = CGameInfo::mainObj->spriteh->giveDef("TSHRE.DEF");

	for(int i=0; i<partialHide->ourImages.size(); ++i)
	{
		CSDL_Ext::alphaTransform(partialHide->ourImages[i].bitmap);
	}

	visibility.resize(reader->map.width+2*Woff);
	for(int gg=0; gg<reader->map.width+2*Woff; ++gg)
	{
		visibility[gg].resize(reader->map.height+2*Hoff);
		for(int jj=0; jj<reader->map.height+2*Hoff; ++jj)
			visibility[gg][jj] = true;
	}
	undVisibility.resize(reader->map.width+2*Woff);
	for(int gg=0; gg<reader->map.width+2*Woff; ++gg)
	{
		undVisibility[gg].resize(reader->map.height+2*Woff);
		for(int jj=0; jj<reader->map.height+2*Woff; ++jj)
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


	//initializing road's and river's DefHandlers

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

	SDL_Surface * su = SDL_CreateRGBSurface(SDL_SWSURFACE, 32, 32, 32,
                                   rmask, gmask, bmask, amask);

	roadDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("dirtrd.def"));
	roadDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("gravrd.def"));
	roadDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("cobbrd.def"));
	staticRiverDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("clrrvr.def"));
	staticRiverDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("icyrvr.def"));
	staticRiverDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("mudrvr.def"));
	staticRiverDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("lavrvr.def"));

	roadBitmaps = new SDL_Surface **[reader->map.width+2*Woff];
	for (int ii=0;ii<reader->map.width+2*Woff;ii++)
		roadBitmaps[ii] = new SDL_Surface*[reader->map.height+2*Hoff]; // allocate memory 

	for (int i=0; i<reader->map.width+2*Woff; i++) //jest po szerokoœci
	{
		for (int j=0; j<reader->map.height+2*Hoff;j++) //po wysokoœci
		{
			if(i<Woff || i>reader->map.width+Woff-1 || j<Woff || j>reader->map.height+Hoff-1)
				roadBitmaps[i][j] = NULL;
			else
			{
				if(reader->map.terrain[i-Woff][j-Hoff].malle)
				{
					roadBitmaps[i][j] = roadDefs[reader->map.terrain[i-Woff][j-Hoff].malle-1]->ourImages[reader->map.terrain[i-Woff][j-Hoff].roadDir].bitmap;
					int cDir = reader->map.terrain[i-Woff][j-Hoff].roadDir;
					if(cDir==0 || cDir==1 || cDir==2 || cDir==3 || cDir==4 || cDir==5)
					{
						if(i-Woff+1<reader->map.width && j-Hoff-1>0 && reader->map.terrain[i-Woff+1][j-Hoff].malle && reader->map.terrain[i-Woff][j-Hoff-1].malle)
						{
							roadBitmaps[i][j] = CSDL_Ext::hFlip(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::alphaTransform(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(roadBitmaps[i][j], su);
						}
						if(i-Woff-1>0 && j-Hoff-1>0 && reader->map.terrain[i-Woff-1][j-Hoff].malle && reader->map.terrain[i-Woff][j-Hoff-1].malle)
						{
							roadBitmaps[i][j] = CSDL_Ext::rotate03(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::alphaTransform(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(roadBitmaps[i][j], su);
						}
						if(i-Woff-1>0 && j-Hoff+1<reader->map.height && reader->map.terrain[i-Woff-1][j-Hoff].malle && reader->map.terrain[i-Woff][j-Hoff+1].malle)
						{
							roadBitmaps[i][j] = CSDL_Ext::rotate01(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::alphaTransform(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(roadBitmaps[i][j], su);
						}
					}
					if(cDir==8 || cDir==9)
					{
						if((j-Hoff+1<reader->map.height && !(reader->map.terrain[i-Woff][j-Hoff+1].malle)) || j-Hoff+1==reader->map.height)
						{
							roadBitmaps[i][j] = CSDL_Ext::hFlip(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::alphaTransform(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(roadBitmaps[i][j], su);
						}
					}
					if(cDir==6 || cDir==7)
					{
						if(i-Woff+1<reader->map.width && !(reader->map.terrain[i-Woff+1][j-Hoff].malle))
						{
							roadBitmaps[i][j] = CSDL_Ext::rotate01(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::alphaTransform(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(roadBitmaps[i][j], su);
						}
					}
					if(cDir==0x0e)
					{
						if(j-Hoff+1<reader->map.height && !(reader->map.terrain[i-Woff][j-Hoff+1].malle))
						{
							roadBitmaps[i][j] = CSDL_Ext::hFlip(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::alphaTransform(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(roadBitmaps[i][j], su);
						}
					}
					if(cDir==0x0f)
					{
						if(i-Woff+1<reader->map.width && !(reader->map.terrain[i-Woff+1][j-Hoff].malle))
						{
							roadBitmaps[i][j] = CSDL_Ext::rotate01(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::alphaTransform(roadBitmaps[i][j]);
							roadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(roadBitmaps[i][j], su);
						}
					}
				}
				else
					roadBitmaps[i][j] = NULL;
			}
		}
	}

	undRoadBitmaps = new SDL_Surface **[reader->map.width+2*Woff];
	for (int ii=0;ii<reader->map.width+2*Woff;ii++)
		undRoadBitmaps[ii] = new SDL_Surface*[reader->map.height+2*Hoff]; // allocate memory 

	if(reader->map.twoLevel)
	{
		for (int i=0; i<reader->map.width+2*Woff; i++) //jest po szerokoœci
		{
			for (int j=0; j<reader->map.height+2*Hoff;j++) //po wysokoœci
			{
				if(i<Woff || i>reader->map.width+Woff-1 || j<Woff || j>reader->map.height+Hoff-1)
					undRoadBitmaps[i][j] = NULL;
				else
				{
					if(reader->map.undergroungTerrain[i-Woff][j-Hoff].malle)
					{
						undRoadBitmaps[i][j] = roadDefs[reader->map.undergroungTerrain[i-Woff][j-Hoff].malle-1]->ourImages[reader->map.undergroungTerrain[i-Woff][j-Hoff].roadDir].bitmap;
						int cDir = reader->map.undergroungTerrain[i-Woff][j-Hoff].roadDir;
						if(cDir==0 || cDir==1 || cDir==2 || cDir==3 || cDir==4 || cDir==5)
						{
							if(i-Woff+1<reader->map.width && j-Hoff-1>0 && reader->map.undergroungTerrain[i-Woff+1][j-Hoff].malle && reader->map.undergroungTerrain[i-Woff][j-Hoff-1].malle)
							{
								undRoadBitmaps[i][j] = CSDL_Ext::hFlip(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::alphaTransform(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undRoadBitmaps[i][j], su);
							}
							if(i-Woff-1>0 && j-Hoff-1>0 && reader->map.undergroungTerrain[i-Woff-1][j-Hoff].malle && reader->map.undergroungTerrain[i-Woff][j-Hoff-1].malle)
							{
								undRoadBitmaps[i][j] = CSDL_Ext::rotate03(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::alphaTransform(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undRoadBitmaps[i][j], su);
							}
							if(i-Woff-1>0 && j-Hoff+1<reader->map.height && reader->map.undergroungTerrain[i-Woff-1][j-Hoff].malle && reader->map.undergroungTerrain[i-Woff][j-Hoff+1].malle)
							{
								undRoadBitmaps[i][j] = CSDL_Ext::rotate01(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::alphaTransform(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undRoadBitmaps[i][j], su);
							}
						}
						if(cDir==8 || cDir==9)
						{
							if((j-Hoff+1<reader->map.height && !(reader->map.undergroungTerrain[i-Woff][j-Hoff+1].malle)) || j-Hoff+1==reader->map.height)
							{
								undRoadBitmaps[i][j] = CSDL_Ext::hFlip(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::alphaTransform(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undRoadBitmaps[i][j], su);
							}
						}
						if(cDir==6 || cDir==7)
						{
							if(i-Woff+1<reader->map.width && !(reader->map.undergroungTerrain[i-Woff+1][j-Hoff].malle))
							{
								undRoadBitmaps[i][j] = CSDL_Ext::rotate01(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::alphaTransform(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undRoadBitmaps[i][j], su);
							}
						}
						if(cDir==0x0e)
						{
							if(j-Hoff+1<reader->map.height && !(reader->map.undergroungTerrain[i-Woff][j-Hoff+1].malle))
							{
								undRoadBitmaps[i][j] = CSDL_Ext::hFlip(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::alphaTransform(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undRoadBitmaps[i][j], su);
							}
						}
						if(cDir==0x0f)
						{
							if(i-Woff+1<reader->map.width && !(reader->map.undergroungTerrain[i-Woff+1][j-Hoff].malle))
							{
								undRoadBitmaps[i][j] = CSDL_Ext::rotate01(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::alphaTransform(undRoadBitmaps[i][j]);
								undRoadBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undRoadBitmaps[i][j], su);
							}
						}
					}
					else
						undRoadBitmaps[i][j] = NULL;
				}
			}
		}
	}

	staticRiverBitmaps = new SDL_Surface **[reader->map.width+2*Woff];
	for (int ii=0;ii<reader->map.width+2*Woff;ii++)
		staticRiverBitmaps[ii] = new SDL_Surface*[reader->map.height+2*Hoff]; // allocate memory 

	for (int i=0; i<reader->map.width+2*Woff; i++) //jest po szerokoœci
	{
		for (int j=0; j<reader->map.height+2*Hoff;j++) //po wysokoœci
		{
			if(i<Woff || i>reader->map.width+Woff-1 || j<Woff || j>reader->map.height+Hoff-1)
				staticRiverBitmaps[i][j] = NULL;
			else
			{
				if(reader->map.terrain[i-Woff][j-Hoff].nuine)
				{
					staticRiverBitmaps[i][j] = staticRiverDefs[reader->map.terrain[i-Woff][j-Hoff].nuine-1]->ourImages[reader->map.terrain[i-Woff][j-Hoff].rivDir].bitmap;
					int cDir = reader->map.terrain[i-Woff][j-Hoff].rivDir;
					if(cDir==0 || cDir==1 || cDir==2 || cDir==3)
					{
						if(i-Woff+1<reader->map.width && j-Hoff-1>0 && reader->map.terrain[i-Woff+1][j-Hoff].nuine && reader->map.terrain[i-Woff][j-Hoff-1].nuine)
						{
							staticRiverBitmaps[i][j] = CSDL_Ext::hFlip(staticRiverBitmaps[i][j]);
							staticRiverBitmaps[i][j] = CSDL_Ext::alphaTransform(staticRiverBitmaps[i][j]);
							staticRiverBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(staticRiverBitmaps[i][j], su);
						}
						if(i-Woff-1>0 && j-Hoff-1>0 && reader->map.terrain[i-Woff-1][j-Hoff].nuine && reader->map.terrain[i-Woff][j-Hoff-1].nuine)
						{
							staticRiverBitmaps[i][j] = CSDL_Ext::rotate03(staticRiverBitmaps[i][j]);
							staticRiverBitmaps[i][j] = CSDL_Ext::alphaTransform(staticRiverBitmaps[i][j]);
							staticRiverBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(staticRiverBitmaps[i][j], su);
						}
						if(i-Woff-1>0 && j-Hoff+1<reader->map.height && reader->map.terrain[i-Woff-1][j-Hoff].nuine && reader->map.terrain[i-Woff][j-Hoff+1].nuine)
						{
							staticRiverBitmaps[i][j] = CSDL_Ext::rotate01(staticRiverBitmaps[i][j]);
							staticRiverBitmaps[i][j] = CSDL_Ext::alphaTransform(staticRiverBitmaps[i][j]);
							staticRiverBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(staticRiverBitmaps[i][j], su);
						}
					}
					if(cDir==5 || cDir==6)
					{
						if(j-Hoff+1<reader->map.height && !(reader->map.terrain[i-Woff][j-Hoff+1].nuine))
						{
							staticRiverBitmaps[i][j] = CSDL_Ext::hFlip(staticRiverBitmaps[i][j]);
							staticRiverBitmaps[i][j] = CSDL_Ext::alphaTransform(staticRiverBitmaps[i][j]);
							staticRiverBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(staticRiverBitmaps[i][j], su);
						}
					}
					if(cDir==7 || cDir==8)
					{
						if(i-Woff+1<reader->map.width && !(reader->map.terrain[i-Woff+1][j-Hoff].nuine))
						{
							staticRiverBitmaps[i][j] = CSDL_Ext::rotate01(staticRiverBitmaps[i][j]);
							staticRiverBitmaps[i][j] = CSDL_Ext::alphaTransform(staticRiverBitmaps[i][j]);
							staticRiverBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(staticRiverBitmaps[i][j], su);
						}
					}
				}
				else
					staticRiverBitmaps[i][j] = NULL;
			}
		}
	}

	undStaticRiverBitmaps = new SDL_Surface **[reader->map.width+2*Woff];
	for (int ii=0;ii<reader->map.width+2*Woff;ii++)
		undStaticRiverBitmaps[ii] = new SDL_Surface*[reader->map.height+2*Hoff]; // allocate memory 

	if(reader->map.twoLevel)
	{
		for (int i=0; i<reader->map.width+2*Woff; i++) //jest po szerokoœci
		{
			for (int j=0; j<reader->map.height+2*Hoff;j++) //po wysokoœci
			{
				if(i<Woff || i>reader->map.width+Woff-1 || j<Woff || j>reader->map.height+Hoff-1)
					undStaticRiverBitmaps[i][j] = NULL;
				else
				{
					if(reader->map.undergroungTerrain[i-Woff][j-Hoff].nuine)
					{
						undStaticRiverBitmaps[i][j] = staticRiverDefs[reader->map.undergroungTerrain[i-Woff][j-Hoff].nuine-1]->ourImages[reader->map.undergroungTerrain[i-Woff][j-Hoff].rivDir].bitmap;
						int cDir = reader->map.undergroungTerrain[i-Woff][j-Hoff].rivDir;
						if(cDir==0 || cDir==1 || cDir==2 || cDir==3)
						{
							if(i-Woff+1<reader->map.width && j-Hoff-1>0 && reader->map.undergroungTerrain[i-Woff+1][j-Hoff].nuine && reader->map.undergroungTerrain[i-Woff][j-Hoff-1].nuine)
							{
								undStaticRiverBitmaps[i][j] = CSDL_Ext::hFlip(undStaticRiverBitmaps[i][j]);
								undStaticRiverBitmaps[i][j] = CSDL_Ext::alphaTransform(undStaticRiverBitmaps[i][j]);
								undStaticRiverBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undStaticRiverBitmaps[i][j], su);
							}
							if(i-Woff-1>0 && j-Hoff-1>0 && reader->map.undergroungTerrain[i-Woff-1][j-Hoff].nuine && reader->map.undergroungTerrain[i-Woff][j-Hoff-1].nuine)
							{
								undStaticRiverBitmaps[i][j] = CSDL_Ext::rotate03(undStaticRiverBitmaps[i][j]);
								undStaticRiverBitmaps[i][j] = CSDL_Ext::alphaTransform(undStaticRiverBitmaps[i][j]);
								undStaticRiverBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undStaticRiverBitmaps[i][j], su);
							}
							if(i-Woff-1>0 && j-Hoff+1<reader->map.height && reader->map.undergroungTerrain[i-Woff-1][j-Hoff].nuine && reader->map.undergroungTerrain[i-Woff][j-Hoff+1].nuine)
							{
								undStaticRiverBitmaps[i][j] = CSDL_Ext::rotate01(undStaticRiverBitmaps[i][j]);
								undStaticRiverBitmaps[i][j] = CSDL_Ext::alphaTransform(undStaticRiverBitmaps[i][j]);
								undStaticRiverBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undStaticRiverBitmaps[i][j], su);
							}
						}
						if(cDir==5 || cDir==6)
						{
							if(j-Hoff+1<reader->map.height && !(reader->map.undergroungTerrain[i-Woff][j-Hoff+1].nuine))
							{
								undStaticRiverBitmaps[i][j] = CSDL_Ext::hFlip(undStaticRiverBitmaps[i][j]);
								undStaticRiverBitmaps[i][j] = CSDL_Ext::alphaTransform(undStaticRiverBitmaps[i][j]);
								undStaticRiverBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undStaticRiverBitmaps[i][j], su);
							}
						}
						if(cDir==7 || cDir==8)
						{
							if(i-Woff+1<reader->map.width && !(reader->map.undergroungTerrain[i-Woff+1][j-Hoff].nuine))
							{
								undStaticRiverBitmaps[i][j] = CSDL_Ext::rotate01(undStaticRiverBitmaps[i][j]);
								undStaticRiverBitmaps[i][j] = CSDL_Ext::alphaTransform(undStaticRiverBitmaps[i][j]);
								undStaticRiverBitmaps[i][j] = CSDL_Ext::secondAlphaTransform(undStaticRiverBitmaps[i][j], su);
							}
						}
					}
					else
						undStaticRiverBitmaps[i][j] = NULL;
				}
			}
		}
	}

	SDL_FreeSurface(su);

	//road's and river's DefHandlers initialized

	terrainBitmap = new SDL_Surface **[reader->map.width+2*Woff];
	for (int ii=0;ii<reader->map.width+2*Woff;ii++)
		terrainBitmap[ii] = new SDL_Surface*[reader->map.height+2*Hoff]; // allocate memory 
	CDefHandler * bord = CGameInfo::mainObj->spriteh->giveDef("EDG.DEF");
	for (int i=0; i<reader->map.width+2*Woff; i++) //jest po szerokoœci
	{
		for (int j=0; j<reader->map.height+2*Hoff;j++) //po wysokoœci
		{
			if(i < Woff || i > (reader->map.width+Woff-1) || j < Hoff  || j > (reader->map.height+Hoff-1))
			{
				if(i==Woff-1 && j==Hoff-1)
				{
					terrainBitmap[i][j] = bord->ourImages[16].bitmap;
					continue;
				}
				else if(i==Woff-1 && j==(reader->map.height+Hoff))
				{
					terrainBitmap[i][j] = bord->ourImages[19].bitmap;
					continue;
				}
				else if(i==(reader->map.width+Woff) && j==Hoff-1)
				{
					terrainBitmap[i][j] = bord->ourImages[17].bitmap;
					continue;
				}
				else if(i==(reader->map.width+Woff) && j==(reader->map.height+Hoff))
				{
					terrainBitmap[i][j] = bord->ourImages[18].bitmap;
					continue;
				}
				else if(j == Hoff-1 && i > Woff-1 && i < reader->map.height+Woff)
				{
					terrainBitmap[i][j] = bord->ourImages[22+rand()%2].bitmap;
					continue;
				}
				else if(i == Woff-1 && j > Hoff-1 && j < reader->map.height+Hoff)
				{
					terrainBitmap[i][j] = bord->ourImages[33+rand()%2].bitmap;
					continue;
				}
				else if(j == reader->map.height+Hoff && i > Woff-1 && i < reader->map.width+Woff)
				{
					terrainBitmap[i][j] = bord->ourImages[29+rand()%2].bitmap;
					continue;
				}
				else if(i == reader->map.width+Woff && j > Hoff-1 && j < reader->map.height+Hoff)
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
			TerrainTile zz = reader->map.terrain[i-Woff][j-Hoff];
			std::string name = CSemiDefHandler::nameFromType(reader->map.terrain[i-Woff][j-Hoff].tertype);
			for (unsigned int k=0; k<reader->defs.size(); k++)
			{
				try
				{
					if (reader->defs[k]->defName != name)
						continue;
					else
					{
						int ktora = reader->map.terrain[i-Woff][j-Hoff].terview;
						terrainBitmap[i][j] = reader->defs[k]->ourImages[ktora].bitmap;
						//TODO: odwracanie	
						switch ((reader->map.terrain[i-Woff][j-Hoff].siodmyTajemniczyBajt)%4)
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
		for (int i=0; i<reader->map.width+2*Woff; i++)
		{
			for (int j=0; j<reader->map.height+2*Hoff;j++)
			{
				if(i < Woff || i > (reader->map.width+Woff-1) || j < Hoff  || j > (reader->map.height+Hoff-1))
				{
					if(i==Woff-1 && j==Hoff-1)
					{
						undTerrainBitmap[i][j] = bord->ourImages[16].bitmap;
						continue;
					}
					else if(i==Woff-1 && j==(reader->map.height+Hoff))
					{
						undTerrainBitmap[i][j] = bord->ourImages[19].bitmap;
						continue;
					}
					else if(i==(reader->map.width+Woff) && j==Hoff-1)
					{
						undTerrainBitmap[i][j] = bord->ourImages[17].bitmap;
						continue;
					}
					else if(i==(reader->map.width+Woff) && j==(reader->map.height+Hoff))
					{
						undTerrainBitmap[i][j] = bord->ourImages[18].bitmap;
						continue;
					}
					else if(j == Hoff-1 && i > Woff-1 && i < reader->map.width+Woff)
					{
						undTerrainBitmap[i][j] = bord->ourImages[22+rand()%2].bitmap;
						continue;
					}
					else if(i == Woff-1 && j > Hoff-1 && j < reader->map.height+Hoff)
					{
						undTerrainBitmap[i][j] = bord->ourImages[33+rand()%2].bitmap;
						continue;
					}
					else if(j == reader->map.height+Hoff && i > Woff-1 && i < reader->map.width+Woff)
					{
						undTerrainBitmap[i][j] = bord->ourImages[29+rand()%2].bitmap;
						continue;
					}
					else if(i == reader->map.width+Woff && j > Hoff-1 && j < reader->map.height+Hoff)
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
				TerrainTile zz = reader->map.undergroungTerrain[i-Woff][j-Hoff];
				std::string name = CSemiDefHandler::nameFromType(reader->map.undergroungTerrain[i-Woff][j-Hoff].tertype);
				for (unsigned int k=0; k<reader->defs.size(); k++)
				{
					try
					{
						if (reader->defs[k]->defName != name)
							continue;
						else
						{
							int ktora = reader->map.undergroungTerrain[i-Woff][j-Hoff].terview;
							undTerrainBitmap[i][j] = reader->defs[k]->ourImages[ktora].bitmap;
							//TODO: odwracanie	
							switch ((reader->map.undergroungTerrain[i-Woff][j-Hoff].siodmyTajemniczyBajt)%4)
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

SDL_Surface * CMapHandler::terrainRect(int x, int y, int dx, int dy, int level, unsigned char anim)
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
	////printing terrain
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
			}
			else 
			{
				SDL_BlitSurface(undTerrainBitmap[bx+x][by+y],NULL,su,sr);
			}
			delete sr;
		}
	}
	////terrain printed
	////printing rivers
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
				if(staticRiverBitmaps[bx+x][by+y])
					SDL_BlitSurface(staticRiverBitmaps[bx+x][by+y],NULL,su,sr);
			}
			else 
			{
				if(undStaticRiverBitmaps[bx+x][by+y])
					SDL_BlitSurface(undStaticRiverBitmaps[bx+x][by+y],NULL,su,sr);
			}
			delete sr;
		}
	}
	////rivers printed
	////printing roads
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
				if(roadBitmaps[bx+x][by+y])
					SDL_BlitSurface(roadBitmaps[bx+x][by+y],NULL,su,sr);
			}
			else 
			{
				if(undRoadBitmaps[bx+x][by+y])
					SDL_BlitSurface(undRoadBitmaps[bx+x][by+y],NULL,su,sr);
			}
			delete sr;
		}
	}
	////roads printed
	////printing objects
	std::vector<ObjSorter> lowPrObjs;
	std::vector<ObjSorter> highPrObjs;
	std::vector<ObjSorter> highPrObjsVis;
	for(int gg=0; gg<CGameInfo::mainObj->objh->objInstances.size(); ++gg)
	{
		if(CGameInfo::mainObj->objh->objInstances[gg].pos.x >= x-Woff-4 && CGameInfo::mainObj->objh->objInstances[gg].pos.x < dx+x-Hoff+4 && CGameInfo::mainObj->objh->objInstances[gg].pos.y >= y-Hoff-4 && CGameInfo::mainObj->objh->objInstances[gg].pos.y < dy+y-Hoff+4 && CGameInfo::mainObj->objh->objInstances[gg].pos.z == level)
		{
			if(!CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].isOnDefList)
			{
				ObjSorter os;
				os.bitmap = CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].handler->ourImages[anim%CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].handler->ourImages.size()].bitmap;
				os.xpos = (CGameInfo::mainObj->objh->objInstances[gg].pos.x-x+Woff)*32;
				os.ypos = (CGameInfo::mainObj->objh->objInstances[gg].pos.y-y+Hoff)*32;
				highPrObjsVis.push_back(os);
			}
			else if(CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].printPriority==0)
			{
				ObjSorter os;

				int defyod = CGameInfo::mainObj->objh->objInstances[gg].defNumber;
				int ourimagesod = anim%CGameInfo::mainObj->ac->map.defy[defyod].handler->ourImages.size();

				os.bitmap = CGameInfo::mainObj->ac->map.defy[defyod].handler->ourImages[ourimagesod].bitmap;

				os.xpos = (CGameInfo::mainObj->objh->objInstances[gg].pos.x-x+Woff)*32;
				os.ypos = (CGameInfo::mainObj->objh->objInstances[gg].pos.y-y+Hoff)*32;
				if (CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].isVisitable())
					highPrObjsVis.push_back(os);
				else
					highPrObjs.push_back(os);
			}
			else
			{
				ObjSorter os;
				os.bitmap = CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].handler->ourImages[anim%CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].handler->ourImages.size()].bitmap;
				os.xpos = (CGameInfo::mainObj->objh->objInstances[gg].pos.x-x+Woff)*32;
				os.ypos = (CGameInfo::mainObj->objh->objInstances[gg].pos.y-y+Hoff)*32;
				lowPrObjs.push_back(os);
			}
		}
	}
#ifndef _DEBUG
	std::stable_sort(lowPrObjs.begin(), lowPrObjs.end(), pox);
	std::stable_sort(lowPrObjs.begin(), lowPrObjs.end(), poy);
	std::stable_sort(highPrObjs.begin(), highPrObjs.end(),pox);
	std::stable_sort(highPrObjs.begin(), highPrObjs.end(),poy);
	std::stable_sort(highPrObjsVis.begin(), highPrObjsVis.end(),pox);
	std::stable_sort(highPrObjsVis.begin(), highPrObjsVis.end(),poy);
#else
	std::sort(lowPrObjs.begin(), lowPrObjs.end(), pox);
	std::sort(lowPrObjs.begin(), lowPrObjs.end(), poy);
	std::sort(highPrObjs.begin(), highPrObjs.end(),pox);
	std::sort(highPrObjs.begin(), highPrObjs.end(),poy);
	std::sort(highPrObjsVis.begin(), highPrObjsVis.end(),pox);
	std::sort(highPrObjsVis.begin(), highPrObjsVis.end(),poy);
#endif
	for(int yy=0; yy<lowPrObjs.size(); ++yy)
	{
		SDL_Rect * sr = new SDL_Rect;
		sr->w = lowPrObjs[yy].bitmap->w;
		sr->h = lowPrObjs[yy].bitmap->h;
		sr->x = lowPrObjs[yy].xpos - lowPrObjs[yy].bitmap->w + 32;
		sr->y = lowPrObjs[yy].ypos - lowPrObjs[yy].bitmap->h + 32;
		SDL_BlitSurface(lowPrObjs[yy].bitmap, NULL, su, sr);
		delete sr;
	}
	for(int yy=0; yy<highPrObjs.size(); ++yy)
	{
		SDL_Rect * sr = new SDL_Rect;
		sr->w = highPrObjs[yy].bitmap->w;
		sr->h = highPrObjs[yy].bitmap->h;
		sr->x = highPrObjs[yy].xpos - highPrObjs[yy].bitmap->w + 32;
		sr->y = highPrObjs[yy].ypos - highPrObjs[yy].bitmap->h + 32;
		SDL_BlitSurface(highPrObjs[yy].bitmap, NULL, su, sr);
		delete sr;
	}
	for(int yy=0; yy<highPrObjsVis.size(); ++yy)
	{
		SDL_Rect * sr = new SDL_Rect;
		sr->w = highPrObjsVis[yy].bitmap->w;
		sr->h = highPrObjsVis[yy].bitmap->h;
		sr->x = highPrObjsVis[yy].xpos - highPrObjsVis[yy].bitmap->w + 32;
		sr->y = highPrObjsVis[yy].ypos - highPrObjsVis[yy].bitmap->h + 32;
		SDL_BlitSurface(highPrObjsVis[yy].bitmap, NULL, su, sr);
		delete sr;
	}
	/*for(int gg=0; gg<CGameInfo::mainObj->objh->objInstances.size(); ++gg)
	{
		if(CGameInfo::mainObj->objh->objInstances[gg].x >= x-3 && CGameInfo::mainObj->objh->objInstances[gg].x < dx+x-3 && CGameInfo::mainObj->objh->objInstances[gg].y >= y-3 && CGameInfo::mainObj->objh->objInstances[gg].y < dy+y-3 && CGameInfo::mainObj->objh->objInstances[gg].z == level)
		{
			SDL_Rect * sr = new SDL_Rect;
			sr->w = CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].handler->ourImages[0].bitmap->w;
			sr->h = CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].handler->ourImages[0].bitmap->h;
			sr->x = (CGameInfo::mainObj->objh->objInstances[gg].x-x+4)*32 - CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].handler->ourImages[0].bitmap->w + 32;
			sr->y = (CGameInfo::mainObj->objh->objInstances[gg].y-y+4)*32 - CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].handler->ourImages[0].bitmap->h + 32;
			SDL_BlitSurface(CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].handler->ourImages[anim%CGameInfo::mainObj->ac->map.defy[CGameInfo::mainObj->objh->objInstances[gg].defNumber].handler->ourImages.size()].bitmap, NULL, su, sr);
			delete sr;
		}
	}*/
	////objects printed, printing shadow
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
				
				if( bx+x>Woff-1 && by+y>Hoff-1 && bx+x<visibility.size()-(Woff-1) && by+y<visibility[0].size()-(Hoff-1) && !visibility[bx+x][by+y])
				{
					SDL_Surface * hide = getVisBitmap(bx+x, by+y, visibility);
					SDL_Surface * hide2 = CSDL_Ext::secondAlphaTransform(hide, su);
					SDL_BlitSurface(hide2, NULL, su, sr);
					SDL_FreeSurface(hide2);
				}
			}
			else
			{
				if( bx+x>Woff-1 && by+y>Hoff-1 && bx+x<undVisibility.size()-(Woff-1) && by+y<undVisibility[0].size()-(Hoff-1) && !undVisibility[bx+x][by+y])
				{
					SDL_Surface * hide = getVisBitmap(bx+x, by+y, undVisibility);
					SDL_Surface * hide2 = CSDL_Ext::secondAlphaTransform(hide, su);
					SDL_BlitSurface(hide2, NULL, su, sr);
					SDL_FreeSurface(hide2);
				}
			}
			delete sr;
		}
	}
	////shadow printed
	//printing borders
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			if(bx+x<Woff || by+y<Hoff || bx+x>reader->map.width+(Woff-1) || by+y>reader->map.height+(Hoff-1))
			{
			SDL_Rect * sr = new SDL_Rect;
			sr->y=by*32;
			sr->x=bx*32;
			sr->h=sr->w=32;
			if (!level)
			{
				SDL_BlitSurface(terrainBitmap[bx+x][by+y],NULL,su,sr);
			}
			else 
			{
				SDL_BlitSurface(undTerrainBitmap[bx+x][by+y],NULL,su,sr);
			}
			delete sr;
			}
		}
	}
	//borders printed
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

SDL_Surface * CMapHandler::getVisBitmap(int x, int y, std::vector< std::vector<char> > & visibility)
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
		//return partialHide->ourImages[rand()%2].bitmap; //visible top
		return partialHide->ourImages[0].bitmap; //visible top
	}
	else if(visibility[x][y+1] && !visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && visibility[x+1][y+1] && !visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		//return partialHide->ourImages[4+rand()%2].bitmap; //visble bottom
		return partialHide->ourImages[4].bitmap; //visble bottom
	}
	else if(!visibility[x][y+1] && !visibility[x+1][y] && visibility[x-1][y] && !visibility[x][y-1] && visibility[x-1][y-1] && !visibility[x+1][y+1] && !visibility[x+1][y-1] && visibility[x-1][y+1])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[2+rand()%2].bitmap); //visible left
		return CSDL_Ext::rotate01(partialHide->ourImages[2].bitmap); //visible left
	}
	else if(!visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1] && visibility[x+1][y+1] && visibility[x+1][y-1] && !visibility[x-1][y+1])
	{
		//return partialHide->ourImages[2+rand()%2].bitmap; //visible right
		return partialHide->ourImages[2].bitmap; //visible right
	}
	else if(visibility[x][y+1] && visibility[x+1][y] && !visibility[x-1][y] && !visibility[x][y-1] && !visibility[x-1][y-1])
	{
		//return partialHide->ourImages[12+2*(rand()%2)].bitmap; //visible bottom, right - bottom, right; left top corner hidden
		return partialHide->ourImages[12].bitmap; //visible bottom, right - bottom, right; left top corner hidden
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
		//return CSDL_Ext::rotate01(partialHide->ourImages[12+2*(rand()%2)].bitmap); //visible left, left - bottom, bottom; right top corner hidden
		return CSDL_Ext::rotate01(partialHide->ourImages[12].bitmap); //visible left, left - bottom, bottom; right top corner hidden
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

char & CMapHandler::visAccess(int x, int y)
{
	return visibility[x+Woff][y+Hoff];
}

char & CMapHandler::undVisAccess(int x, int y)
{
	return undVisibility[x+Woff][y+Hoff];
}

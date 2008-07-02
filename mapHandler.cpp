#include "stdafx.h"
#include "mapHandler.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include <cstdlib>
#include "hch/CLodHandler.h"
#include "hch/CDefObjInfoHandler.h"
#include <algorithm>
#include "CGameState.h"
#include "CLua.h"
#include "hch/CHeroHandler.h"
#include "hch/CTownHandler.h"
#include "client\Graphics.h"
#include <iomanip>
#include <sstream>
#include "hch/CObjectHandler.h"
#include "map.h"
extern SDL_Surface * screen;
std::string nameFromType (EterrainType typ)
{
	switch(typ)
	{
		case dirt:
		{
			return std::string("DIRTTL.DEF");
			break;
		}
		case sand:
		{
			return std::string("SANDTL.DEF");
			break;
		}
		case grass:
		{
			return std::string("GRASTL.DEF");
			break;
		}
		case snow:
		{
			return std::string("SNOWTL.DEF");
			break;
		}
		case swamp:
		{
			return std::string("SWMPTL.DEF");			
			break;
		}
		case rough:
		{
			return std::string("ROUGTL.DEF");		
			break;
		}
		case subterranean:
		{
			return std::string("SUBBTL.DEF");		
			break;
		}
		case lava:
		{
			return std::string("LAVATL.DEF");		
			break;
		}
		case water:
		{
			return std::string("WATRTL.DEF");
			break;
		}
		case rock:
		{
			return std::string("ROCKTL.DEF");		
			break;
		}
	}
	return std::string();
}
class OCM_HLP
{
public:
	bool operator ()(const std::pair<CGObjectInstance*, SDL_Rect> & a, const std::pair<CGObjectInstance*, SDL_Rect> & b)
	{
		return (*a.first)<(*b.first);
	}
} ocmptwo ;
void alphaTransformDef(CGDefInfo * defInfo)
{	
	SDL_Surface * alphaTransSurf = SDL_CreateRGBSurface(SDL_SWSURFACE, 12, 12, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
	for(int yy=0;yy<defInfo->handler->ourImages.size();yy++)
	{
		defInfo->handler->ourImages[yy].bitmap = CSDL_Ext::alphaTransform(defInfo->handler->ourImages[yy].bitmap);
		defInfo->handler->alphaTransformed = true;
	}
	SDL_FreeSurface(alphaTransSurf);
}
void CMapHandler::prepareFOWDefs()
{
	fullHide = CDefHandler::giveDef("TSHRC.DEF");
	partialHide = CDefHandler::giveDef("TSHRE.DEF");

	//adding necessary rotations
	Cimage nw = partialHide->ourImages[22]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[15]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[2]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[13]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[12]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[16]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[18]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[17]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[20]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[19]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[7]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[24]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[26]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[25]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[30]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[32]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[27]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	nw = partialHide->ourImages[28]; nw.bitmap = CSDL_Ext::rotate01(nw.bitmap);
	partialHide->ourImages.push_back(nw);
	//necessaary rotations added

	for(int i=0; i<partialHide->ourImages.size(); ++i)
	{
		CSDL_Ext::alphaTransform(partialHide->ourImages[i].bitmap);
	}
	//visibility.resize(map->width+2*Woff);
	//for(int gg=0; gg<map->width+2*Woff; ++gg)
	//{
	//	visibility[gg].resize(map->height+2*Hoff);
	//	for(int jj=0; jj<map->height+2*Hoff; ++jj)
	//		visibility[gg][jj] = true;
	//}

	//visibility.resize(CGI->mh->map->width, Woff);
	//for (int i=0-Woff;i<visibility.size()-Woff;i++)
	//{
	//	visibility[i].resize(CGI->mh->map->height,Hoff);
	//}
	//for (int i=0-Woff; i<visibility.size()-Woff; ++i)
	//{
	//	for (int j=0-Hoff; j<CGI->mh->map->height+Hoff; ++j)
	//	{
	//		visibility[i][j].resize(CGI->mh->map->twoLevel+1,0);
	//		for(int k=0; k<CGI->mh->map->twoLevel+1; ++k)
	//			visibility[i][j][k]=true;
	//	}
	//}

	hideBitmap.resize(CGI->mh->map->width);
	for (int i=0;i<hideBitmap.size();i++)
	{
		hideBitmap[i].resize(CGI->mh->map->height);
	}
	for (int i=0; i<hideBitmap.size(); ++i)
	{
		for (int j=0; j<CGI->mh->map->height; ++j)
		{
			hideBitmap[i][j].resize(CGI->mh->map->twoLevel+1);
			for(int k=0; k<CGI->mh->map->twoLevel+1; ++k)
				hideBitmap[i][j][k] = rand()%fullHide->ourImages.size();
		}
	}
}

void CMapHandler::roadsRiverTerrainInit()
{
	//initializing road's and river's DefHandlers

	roadDefs.push_back(CDefHandler::giveDef("dirtrd.def"));
	roadDefs.push_back(CDefHandler::giveDef("gravrd.def"));
	roadDefs.push_back(CDefHandler::giveDef("cobbrd.def"));
	staticRiverDefs.push_back(CDefHandler::giveDef("clrrvr.def"));
	staticRiverDefs.push_back(CDefHandler::giveDef("icyrvr.def"));
	staticRiverDefs.push_back(CDefHandler::giveDef("mudrvr.def"));
	staticRiverDefs.push_back(CDefHandler::giveDef("lavrvr.def"));
	for(int g=0; g<staticRiverDefs.size(); ++g)
	{
		for(int h=0; h<staticRiverDefs[g]->ourImages.size(); ++h)
		{
			CSDL_Ext::alphaTransform(staticRiverDefs[g]->ourImages[h].bitmap);
		}
	}
	for(int g=0; g<roadDefs.size(); ++g)
	{
		for(int h=0; h<roadDefs[g]->ourImages.size(); ++h)
		{
			CSDL_Ext::alphaTransform(roadDefs[g]->ourImages[h].bitmap);
		}
	}

	sizes.x = CGI->mh->map->width;
	sizes.y = CGI->mh->map->height;
	sizes.z = CGI->mh->map->twoLevel+1;
	ttiles.resize(CGI->mh->map->width,Woff);
	for (int i=0-Woff;i<ttiles.size()-Woff;i++)
	{
		ttiles[i].resize(CGI->mh->map->height,Hoff);
	}
	for (int i=0-Woff;i<ttiles.size()-Woff;i++)
	{
		for (int j=0-Hoff;j<CGI->mh->map->height+Hoff;j++)
			ttiles[i][j].resize(CGI->mh->map->twoLevel+1,0);
	}



	for (int i=0; i<map->width; i++) //jest po szerokoœci
	{
		for (int j=0; j<map->height;j++) //po wysokoœci
		{
			for (int k=0; k<=map->twoLevel; ++k)
			{
				TerrainTile** pomm = map->terrain; ;
				if (k==0)
					pomm = map->terrain;
				else
					pomm = map->undergroungTerrain;
				if(pomm[i][j].malle)
				{
					int cDir;
					bool rotV, rotH;
					if(k==0)
					{
						int roadpom = map->terrain[i][j].malle-1,
							impom = map->terrain[i][j].roadDir;
						SDL_Surface *pom1 = roadDefs[roadpom]->ourImages[impom].bitmap;
						ttiles[i][j][k].roadbitmap.push_back(pom1);
						cDir = map->terrain[i][j].roadDir;

						rotH = (map->terrain[i][j].siodmyTajemniczyBajt >> 5) & 1;
						rotV = (map->terrain[i][j].siodmyTajemniczyBajt >> 4) & 1;
					}
					else
					{
						int pom111 = map->undergroungTerrain[i][j].malle-1,
							pom777 = map->undergroungTerrain[i][j].roadDir;
						SDL_Surface *pom1 = roadDefs[pom111]->ourImages[pom777].bitmap;
						ttiles[i][j][k].roadbitmap.push_back(pom1);
						cDir = map->undergroungTerrain[i][j].roadDir;

						rotH = (map->undergroungTerrain[i][j].siodmyTajemniczyBajt >> 5) & 1;
						rotV = (map->undergroungTerrain[i][j].siodmyTajemniczyBajt >> 4) & 1;
					}
					if(rotH)
					{
						ttiles[i][j][k].roadbitmap[0] = CSDL_Ext::hFlip(ttiles[i][j][k].roadbitmap[0]);
					}
					if(rotV)
					{
						ttiles[i][j][k].roadbitmap[0] = CSDL_Ext::rotate01(ttiles[i][j][k].roadbitmap[0]);
					}
					if(rotH || rotV)
					{
						ttiles[i][j][k].roadbitmap[0] = CSDL_Ext::alphaTransform(ttiles[i][j][k].roadbitmap[0]);
					}
				}
			}
		}
	}

	//initializing simple values
	for (int i=0; i<CGI->mh->map->width; i++) //jest po szerokoœci
	{
		for (int j=0; j<CGI->mh->map->height;j++) //po wysokoœci
		{
			for(int k=0; k<ttiles[0][0].size(); ++k)
			{
				ttiles[i][j][k].pos = int3(i, j, k);
				ttiles[i][j][k].blocked = false;
				ttiles[i][j][k].visitable = false;
				if(i<0 || j<0 || i>=CGI->mh->map->width || j>=CGI->mh->map->height)
				{
					ttiles[i][j][k].blocked = true;
					continue;
				}
				ttiles[i][j][k].terType = (k==0 ? CGI->mh->map->terrain[i][j].tertype : CGI->mh->map->undergroungTerrain[i][j].tertype);
				ttiles[i][j][k].malle = (k==0 ? CGI->mh->map->terrain[i][j].malle : CGI->mh->map->undergroungTerrain[i][j].malle);
				ttiles[i][j][k].nuine = (k==0 ? CGI->mh->map->terrain[i][j].nuine : CGI->mh->map->undergroungTerrain[i][j].nuine);
				ttiles[i][j][k].rivdir = (k==0 ? CGI->mh->map->terrain[i][j].rivDir : CGI->mh->map->undergroungTerrain[i][j].rivDir);
				ttiles[i][j][k].roaddir = (k==0 ? CGI->mh->map->terrain[i][j].roadDir : CGI->mh->map->undergroungTerrain[i][j].roadDir);

			}
		}
	}
	//simple values initialized

	for (int i=0; i<map->width; i++) //jest po szerokoœci
	{
		for (int j=0; j<map->height;j++) //po wysokoœci
		{
			for(int k=0; k<=map->twoLevel; ++k)
			{
				TerrainTile** pomm = map->terrain;
				if(k==0)
				{
					pomm = map->terrain;
				}
				else
				{
					pomm = map->undergroungTerrain;
				}
				if(pomm[i][j].nuine)
				{
					int cDir;
					bool rotH, rotV;
					if(k==0)
					{
						ttiles[i][j][k].rivbitmap.push_back(staticRiverDefs[map->terrain[i][j].nuine-1]->ourImages[map->terrain[i][j].rivDir].bitmap);
						cDir = map->terrain[i][j].rivDir;
						rotH = (map->terrain[i][j].siodmyTajemniczyBajt >> 3) & 1;
						rotV = (map->terrain[i][j].siodmyTajemniczyBajt >> 2) & 1;
					}
					else
					{
						ttiles[i][j][k].rivbitmap.push_back(staticRiverDefs[map->undergroungTerrain[i][j].nuine-1]->ourImages[map->undergroungTerrain[i][j].rivDir].bitmap);
						cDir = map->undergroungTerrain[i][j].rivDir;
						rotH = (map->undergroungTerrain[i][j].siodmyTajemniczyBajt >> 3) & 1;
						rotV = (map->undergroungTerrain[i][j].siodmyTajemniczyBajt >> 2) & 1;
					}
					if(rotH)
					{
						ttiles[i][j][k].rivbitmap[0] = CSDL_Ext::hFlip(ttiles[i][j][k].rivbitmap[0]);
					}
					if(rotV)
					{
						ttiles[i][j][k].rivbitmap[0] = CSDL_Ext::rotate01(ttiles[i][j][k].rivbitmap[0]);
					}
					if(rotH || rotV)
					{
						ttiles[i][j][k].rivbitmap[0] = CSDL_Ext::alphaTransform(ttiles[i][j][k].rivbitmap[0]);
					}
				}
			}
		}
	}
}
void CMapHandler::borderAndTerrainBitmapInit()
{
	CDefHandler * bord = CDefHandler::giveDef("EDG.DEF");
	bord->notFreeImgs =  true;
	for (int i=0-Woff; i<map->width+Woff; i++) //jest po szerokoœci
	{
		for (int j=0-Hoff; j<map->height+Hoff;j++) //po wysokoœci
		{
			for(int k=0; k<=map->twoLevel; ++k)
			{
				if(i < 0 || i > (map->width-1) || j < 0  || j > (map->height-1))
				{
					if(i==-1 && j==-1)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[16].bitmap);
						continue;
					}
					else if(i==-1 && j==(map->height))
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[19].bitmap);
						continue;
					}
					else if(i==(map->width) && j==-1)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[17].bitmap);
						continue;
					}
					else if(i==(map->width) && j==(map->height))
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[18].bitmap);
						continue;
					}
					else if(j == -1 && i > -1 && i < map->height)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[22+rand()%2].bitmap);
						continue;
					}
					else if(i == -1 && j > -1 && j < map->height)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[33+rand()%2].bitmap);
						continue;
					}
					else if(j == map->height && i >-1 && i < map->width)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[29+rand()%2].bitmap);
						continue;
					}
					else if(i == map->width && j > -1 && j < map->height)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[25+rand()%2].bitmap);
						continue;
					}
					else
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[rand()%16].bitmap);
						continue;
					}
				}
				//TerrainTile zz = map->terrain[i-Woff][j-Hoff];
				std::string name;
				if (k>0)
					name = nameFromType(map->undergroungTerrain[i][j].tertype);
				else
					name = nameFromType(map->terrain[i][j].tertype);
				for (unsigned int m=0; m<defs.size(); m++)
				{
					try
					{
						if (defs[m]->defName != name)
							continue;
						else
						{
							int ktora;
							if (k==0)
								ktora = map->terrain[i][j].terview;
							else
								ktora = map->undergroungTerrain[i][j].terview;
							ttiles[i][j][k].terbitmap.push_back(defs[m]->ourImages[ktora].bitmap);
							int zz;
							if (k==0)
								zz = (map->terrain[i][j].siodmyTajemniczyBajt)%4;
							else
								zz = (map->undergroungTerrain[i][j].siodmyTajemniczyBajt)%4;
							switch (zz)
							{
							case 1:
								{
									ttiles[i][j][k].terbitmap[0] = CSDL_Ext::rotate01(ttiles[i][j][k].terbitmap[0]);
									break;
								}
							case 2:
								{
									ttiles[i][j][k].terbitmap[0] = CSDL_Ext::hFlip(ttiles[i][j][k].terbitmap[0]);
									break;
								}
							case 3:
								{
									ttiles[i][j][k].terbitmap[0] = CSDL_Ext::rotate03(ttiles[i][j][k].terbitmap[0]);
									break;
								}
							}

							break;
						}
					}
					catch (...)
					{
						continue;
					}
				}
			}
		}
	}
	delete bord;
}
void CMapHandler::initObjectRects()
{
	//initializing objects / rects
	for(int f=0; f<map->objects.size(); ++f)
	{
		/*map->objects[f]->pos.x+=1;
		map->objects[f]->pos.y+=1;*/
		if(!map->objects[f]->defInfo)
		{
			continue;
		}
		CDefHandler * curd = map->objects[f]->defInfo->handler;
		if(curd)
		{
			for(int fx=0; fx<curd->ourImages[0].bitmap->w>>5; ++fx) //curd->ourImages[0].bitmap->w/32
			{
				for(int fy=0; fy<curd->ourImages[0].bitmap->h>>5; ++fy) //curd->ourImages[0].bitmap->h/32
				{
					SDL_Rect cr;
					cr.w = 32;
					cr.h = 32;
					cr.x = fx<<5; //fx*32
					cr.y = fy<<5; //fy*32
					std::pair<CGObjectInstance*,SDL_Rect> toAdd = std::make_pair(map->objects[f],cr);
					
					if((map->objects[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)>=0 && (map->objects[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)<ttiles.size()-Woff && (map->objects[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)>=0 && (map->objects[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)<ttiles[0].size()-Hoff)
					{
						//TerrainTile2 & curt =
						//	ttiles
						//	[map->objects[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32]
						//[map->objects[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32]
						//[map->objects[f]->pos.z];
						ttiles[map->objects[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32+1][map->objects[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32+1][map->objects[f]->pos.z].objects.push_back(toAdd);
					}
				} // for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
			} //for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
		}//if curd
	} // for(int f=0; f<map->objects.size(); ++f)
	for(int ix=0; ix<ttiles.size()-Woff; ++ix)
	{
		for(int iy=0; iy<ttiles[0].size()-Hoff; ++iy)
		{
			for(int iz=0; iz<ttiles[0][0].size(); ++iz)
			{
				stable_sort(ttiles[ix][iy][iz].objects.begin(), ttiles[ix][iy][iz].objects.end(), ocmptwo);
			}
		}
	}
}
void CMapHandler::calculateBlockedPos()
{
	for(int f=0; f<map->objects.size(); ++f) //calculationg blocked / visitable positions
	{
		if(!map->objects[f]->defInfo)
			continue;
		CDefHandler * curd = map->objects[f]->defInfo->handler;
		for(int fx=0; fx<8; ++fx)
		{
			for(int fy=0; fy<6; ++fy)
			{
				int xVal = map->objects[f]->pos.x + fx - 7;
				int yVal = map->objects[f]->pos.y + fy - 5;
				int zVal = map->objects[f]->pos.z;
				if(xVal>=0 && xVal<ttiles.size()-Woff && yVal>=0 && yVal<ttiles[0].size()-Hoff)
				{
					TerrainTile2 & curt = ttiles[xVal][yVal][zVal];
					if(((map->objects[f]->defInfo->visitMap[fy] >> (7 - fx)) & 1))
						curt.visitable = true;
					if(!((map->objects[f]->defInfo->blockMap[fy] >> (7 - fx)) & 1))
						curt.blocked = true;
				}
			}
		}
	}
}
void processDef (CGDefInfo* def)
{
	def->handler=CDefHandler::giveDef(def->name);
	def->width = def->handler->ourImages[0].bitmap->w/32;
	def->height = def->handler->ourImages[0].bitmap->h/32;
	CGDefInfo* pom = CGI->dobjinfo->gobjs[def->id][def->subid];
	if(pom)
	{
		pom->handler = def->handler;
		pom->width = pom->handler->ourImages[0].bitmap->w/32;
		pom->height = pom->handler->ourImages[0].bitmap->h/32;
	}
	else
		std::cout << "\t\tMinor warning: lacking def info for " << def->id << " " << def->subid <<" " << def->name << std::endl;
	if(!def->handler->alphaTransformed)
	{
		for(int yy=0; yy<def->handler->ourImages.size(); ++yy)
		{
			def->handler->ourImages[yy].bitmap = CSDL_Ext::alphaTransform(def->handler->ourImages[yy].bitmap);
			def->handler->alphaTransformed = true;
		}
	}
}
void CMapHandler::init()
{
	timeHandler th;
	th.getDif();
	std::for_each(map->defy.begin(),map->defy.end(),processDef); //load h3m defs
	std::for_each(map->defs.begin(),map->defs.end(),processDef); //and non-h3m defs
	THC std::cout<<"\tUnpacking and handling defs: "<<th.getDif()<<std::endl;

	//loading castles' defs
	//std::ifstream ifs("config/townsDefs.txt");
	//int ccc;
	//ifs>>ccc;
	//for(int i=0;i<ccc*2;i++)
	//{
	//	//CGDefInfo * n = new CGDefInfo(*CGI->dobjinfo->castles[i%ccc]);
	//	ifs >> n->name;
	//	if (!(n->handler = CDefHandler::giveDef(n->name)))
	//		std::cout << "Cannot open "<<n->name<<std::endl;
	//	else
	//	{
	//		n->width = n->handler->ourImages[0].bitmap->w/32;
	//		n->height = n->handler->ourImages[0].bitmap->h/32;
	//	}
	//	if(i<ccc)
	//		villages[i]=n;
	//	else
	//		capitols[i%ccc]=n;
	//	alphaTransformDef(n);
	//} 

	for(int i=0;i<PLAYER_LIMIT;i++)
	{
		for(int j=0; j<map->players[i].heroesNames.size();j++)
		{
			usedHeroes.insert(map->players[i].heroesNames[j].heroID);
		}
	}
	std::cout<<"\tLoading town defs, picking random factions and heroes: "<<th.getDif()<<std::endl;



	for(int h=0; h<map->defy.size(); ++h) //initializing loaded def handler's info	{
		CGI->mh->loadedDefs.insert(std::make_pair(map->defy[h]->name, map->defy[h]->handler));
	std::cout<<"\tCollecting loaded def's handlers: "<<th.getDif()<<std::endl;

	prepareFOWDefs();
	roadsRiverTerrainInit();	//road's and river's DefHandlers; and simple values initialization
	borderAndTerrainBitmapInit();
	std::cout<<"\tPreparing FoW, roads, rivers,borders: "<<th.getDif()<<std::endl;

	//giving starting hero
	//for(int i=0;i<PLAYER_LIMIT;i++)
	//{
	//	if((map->players[i].generateHeroAtMainTown && map->players[i].hasMainTown) ||  (map->players[i].hasMainTown && map->version==RoE))
	//	{
	//		int3 hpos = map->players[i].posOfMainTown;
	//		hpos.x+=1;// hpos.y+=1;
	//		int j;
	//		for(j=0;j<CGI->state->scenarioOps->playerInfos.size();j++)
	//			if(CGI->state->scenarioOps->playerInfos[j].color==i)
	//				break;
	//		if(j==CGI->state->scenarioOps->playerInfos.size())
	//			continue;
	//		int h=pickHero(i);
	//		CGHeroInstance * nnn = (CGHeroInstance*)createObject(34,h,hpos,i);
	//		nnn->defInfo->handler = graphics->flags1[0];
	//		map->heroes.push_back(nnn);
	//		map->objects.push_back(nnn);
	//	}
	//}
	std::cout<<"\tGiving starting heroes: "<<th.getDif()<<std::endl;

	initObjectRects();
	std::cout<<"\tMaking object rects: "<<th.getDif()<<std::endl;
	calculateBlockedPos();
	std::cout<<"\tCalculating blockmap: "<<th.getDif()<<std::endl;
}

SDL_Surface * CMapHandler::terrainRect(int x, int y, int dx, int dy, int level, unsigned char anim, std::vector< std::vector< std::vector<unsigned char> > > * visibilityMap, bool otherHeroAnim, unsigned char heroAnim, SDL_Surface * extSurf, SDL_Rect * extRect)
{
	if(!otherHeroAnim)
		heroAnim = anim; //the same, as it should be

	//setting surface to blit at
	SDL_Surface * su = NULL; //blitting surface CSDL_Ext::newSurface(dx*32, dy*32, CSDL_Ext::std32bppSurface);
	if(extSurf)
	{
		su = extSurf;
	}
	else
	{
		 su = CSDL_Ext::newSurface(dx*32, dy*32, CSDL_Ext::std32bppSurface);
	}

	if (((dx+x)>((map->width+Woff)) || (dy+y)>((map->height+Hoff))) || ((x<-Woff)||(y<-Hoff) ) )
		throw new std::string("terrainRect: out of range");
	////printing terrain
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			SDL_Rect sr;
			sr.y=by*32;
			sr.x=bx*32;
			sr.h=sr.w=32;
			validateRectTerr(&sr, extRect);
			SDL_BlitSurface(ttiles[x+bx][y+by][level].terbitmap[anim%ttiles[x+bx][y+by][level].terbitmap.size()],&genRect(sr.h, sr.w, 0, 0),su,&sr);
		}
	}
	////terrain printed
	////printing rivers
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			SDL_Rect sr;
			sr.y=by*32;
			sr.x=bx*32;
			sr.h=sr.w=32;
			validateRectTerr(&sr, extRect);
			if(ttiles[x+bx][y+by][level].rivbitmap.size())
			{
				CSDL_Ext::blit8bppAlphaTo24bpp(ttiles[x+bx][y+by][level].rivbitmap[anim%ttiles[x+bx][y+by][level].rivbitmap.size()],&genRect(sr.h, sr.w, 0, 0),su,&sr);
			}
		}
	}
	////rivers printed
	////printing roads
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=-1; by<dy; by++)
		{
			if(y+by<=-4)
				continue;
			SDL_Rect sr;
			sr.y=by*32+16;
			sr.x=bx*32;
			sr.h=sr.w=32;
			validateRectTerr(&sr, extRect);
			if(ttiles[x+bx][y+by][level].roadbitmap.size())
			{
				CSDL_Ext::blit8bppAlphaTo24bpp(ttiles[x+bx][y+by][level].roadbitmap[anim%ttiles[x+bx][y+by][level].roadbitmap.size()], &genRect(sr.h, sr.w, 0, (by==-1 ? 16 : 0)),su,&sr);
			}
		}
	}
	////roads printed
	////printing objects

	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			for(int h=0; h<ttiles[x+bx][y+by][level].objects.size(); ++h)
			{
				SDL_Rect sr;
				sr.w = 32;
				sr.h = 32;
				sr.x = (bx)*32;
				sr.y = (by)*32;
				validateRectTerr(&sr, extRect);

				SDL_Rect pp = ttiles[x+bx][y+by][level].objects[h].second;
				pp.h = sr.h;
				pp.w = sr.w;
				CGHeroInstance * themp = (dynamic_cast<CGHeroInstance*>(ttiles[x+bx][y+by][level].objects[h].first));

				if(themp && themp->moveDir && !themp->isStanding && themp->ID!=62) //last condition - this is not prison
				{
					int imgVal = 8;
					SDL_Surface * tb;

					if(themp->type==NULL)
						continue;
					std::vector<Cimage> & iv = themp->type->heroClass->moveAnim->ourImages;

					int gg;
					for(gg=0; gg<iv.size(); ++gg)
					{
						if(iv[gg].groupNumber==getHeroFrameNum(themp->moveDir, !themp->isStanding))
						{
							tb = iv[gg+heroAnim%imgVal].bitmap;
							break;
						}
					}
					CSDL_Ext::blit8bppAlphaTo24bpp(tb,&pp,su,&sr);
					pp.y+=imgVal*2-32;
					sr.y-=16;
					SDL_BlitSurface(graphics->flags4[themp->getOwner()]->ourImages[gg+heroAnim%imgVal+35].bitmap, &pp, su, &sr);
				}
				else if(themp && themp->moveDir && themp->isStanding && themp->ID!=62) //last condition - this is not prison)
				{
					int imgVal = 8;
					SDL_Surface * tb;

					if(themp->type==NULL)
						continue;
					std::vector<Cimage> & iv = themp->type->heroClass->moveAnim->ourImages;

					int gg;
					for(gg=0; gg<iv.size(); ++gg)
					{
						if(iv[gg].groupNumber==getHeroFrameNum(themp->moveDir, !themp->isStanding))
						{
							tb = iv[gg].bitmap;
							break;
						}
					}
					CSDL_Ext::blit8bppAlphaTo24bpp(tb,&pp,su,&sr);
					if(themp->pos.x==x+bx && themp->pos.y==y+by)
					{
						SDL_Rect bufr = sr;
						bufr.x-=2*32;
						bufr.y-=1*32;
						bufr.h = 64;
						bufr.w = 96;
						if(bufr.x-extRect->x>-64)
							SDL_BlitSurface(graphics->flags4[themp->getOwner()]->ourImages[ getHeroFrameNum(themp->moveDir, !themp->isStanding) *8+(heroAnim/4)%imgVal].bitmap, NULL, su, &bufr);
						themp->flagPrinted = true;
					}
				}
				else
				{
					int imgVal = ttiles[x+bx][y+by][level].objects[h].first->defInfo->handler->ourImages.size();
					int phaseShift = ttiles[x+bx][y+by][level].objects[h].first->animPhaseShift;

					//setting appropriate flag color
					if((ttiles[x+bx][y+by][level].objects[h].first->tempOwner>=0 && ttiles[x+bx][y+by][level].objects[h].first->tempOwner<8) || ttiles[x+bx][y+by][level].objects[h].first->tempOwner==255)
						CSDL_Ext::setPlayerColor(ttiles[x+bx][y+by][level].objects[h].first->defInfo->handler->ourImages[(anim+phaseShift)%imgVal].bitmap, ttiles[x+bx][y+by][level].objects[h].first->tempOwner);
					
					CSDL_Ext::blit8bppAlphaTo24bpp(ttiles[x+bx][y+by][level].objects[h].first->defInfo->handler->ourImages[(anim+phaseShift)%imgVal].bitmap,&pp,su,&sr);
				}
			}
		}
	}

	////objects printed, printing shadow
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			SDL_Rect sr;
			sr.y=by*32;
			sr.x=bx*32;
			sr.h=sr.w=32;
			validateRectTerr(&sr, extRect);
			
			if(bx+x>=0 && by+y>=0 && bx+x<CGI->mh->map->width && by+y<CGI->mh->map->height && !(*visibilityMap)[bx+x][by+y][level])
			{
				SDL_Surface * hide = getVisBitmap(bx+x, by+y, *visibilityMap, level);
				CSDL_Ext::blit8bppAlphaTo24bpp(hide, &genRect(sr.h, sr.w, 0, 0), su, &sr);
			}
		}
	}
	////shadow printed
	//printing borders
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			if(bx+x<0 || by+y<0 || bx+x>map->width+(-1) || by+y>map->height+(-1))
			{
				SDL_Rect sr;
				sr.y=by*32;
				sr.x=bx*32;
				sr.h=sr.w=32;
				validateRectTerr(&sr, extRect);

				SDL_BlitSurface(ttiles[x+bx][y+by][level].terbitmap[anim%ttiles[x+bx][y+by][level].terbitmap.size()],&genRect(sr.h, sr.w, 0, 0),su,&sr);
			}
			else 
			{
				if(MARK_BLOCKED_POSITIONS &&  ttiles[x+bx][y+by][level].blocked) //temporary hiding blocked positions
				{
					SDL_Rect sr;
					sr.y=by*32;
					sr.x=bx*32;
					sr.h=sr.w=32;
					validateRectTerr(&sr, extRect);

					SDL_Surface * ns =  CSDL_Ext::newSurface(32, 32, CSDL_Ext::std32bppSurface);
					for(int f=0; f<ns->w*ns->h*4; ++f)
					{
						*((unsigned char*)(ns->pixels) + f) = 128;
					}

					SDL_BlitSurface(ns,&genRect(sr.h, sr.w, 0, 0),su,&sr);

					SDL_FreeSurface(ns);
				}
				if(MARK_VISITABLE_POSITIONS &&  ttiles[x+bx][y+by][level].visitable) //temporary hiding visitable positions
				{
					SDL_Rect sr;
					sr.y=by*32;
					sr.x=bx*32;
					sr.h=sr.w=32;
					validateRectTerr(&sr, extRect);

					SDL_Surface * ns = CSDL_Ext::newSurface(32, 32, CSDL_Ext::std32bppSurface);
					for(int f=0; f<ns->w*ns->h*4; ++f)
					{
						*((unsigned char*)(ns->pixels) + f) = 128;
					}

					SDL_BlitSurface(ns,&genRect(sr.h, sr.w, 0, 0),su,&sr);

					SDL_FreeSurface(ns);
				}
			}
		}
	}
	//borders printed
	return su;
}

SDL_Surface * CMapHandler::terrBitmap(int x, int y)
{
	return ttiles[x+Woff][y+Hoff][0].terbitmap[0];
}

SDL_Surface * CMapHandler::undTerrBitmap(int x, int y)
{
	return ttiles[x+Woff][y+Hoff][0].terbitmap[1];
}

SDL_Surface * CMapHandler::getVisBitmap(int x, int y, std::vector< std::vector< std::vector<unsigned char> > > & visibilityMap, int lvl)
{
	int size = visibilityMap.size()-1;							//is tile visible. arrangement: (like num keyboard)
	bool d7 = (x>0 && y>0) ? visibilityMap[x-1][y-1][lvl] : 0,	//789
		d8 = (y>0) ? visibilityMap[x][y-1][lvl] : 0,			//456
		d9 = (y>0 && x<size) ? visibilityMap[x+1][y-1][lvl] : 0,//123
		d4 = (x>0) ? visibilityMap[x-1][y][lvl] : 0,
		d5 = visibilityMap[x][y][lvl],
		d6 = (x<size) ? visibilityMap[x+1][y][lvl] : 0,
		d1 = (x>0 && y<size) ? visibilityMap[x-1][y+1][lvl] : 0,
		d2 = (y<size) ? visibilityMap[x][y+1][lvl] : 0,
		d3 = (x<size && y<size) ? visibilityMap[x+1][y+1][lvl] : 0;

	if(!d2 && !d6 && !d4 && !d8 && !d7 && !d3 && !d9 && !d1)
	{
		return fullHide->ourImages[hideBitmap[x][y][lvl]].bitmap; //fully hidden
	}
	else if(!d2 && !d6 && !d4 && !d8 && !d7 && d3 && !d9 && !d1)
	{
		return partialHide->ourImages[22].bitmap; //visible right bottom corner
	}
	else if(!d2 && !d6 && !d4 && !d8 && !d7 && !d3 && d9 && !d1)
	{
		return partialHide->ourImages[15].bitmap; //visible right top corner
	}
	else if(!d2 && !d6 && !d4 && !d8 && !d7 && !d3 && !d9 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[22].bitmap); //visible left bottom corner
		return partialHide->ourImages[34].bitmap; //visible left bottom corner
	}
	else if(!d2 && !d6 && !d4 && !d8 && d7 && !d3 && !d9 && !d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[15].bitmap); //visible left top corner
		return partialHide->ourImages[35].bitmap;
	}
	else if(!d2 && !d6 && !d4 && d8 && d7 && !d3 && d9 && !d1)
	{
		//return partialHide->ourImages[rand()%2].bitmap; //visible top
		return partialHide->ourImages[0].bitmap; //visible top
	}
	else if(d2 && !d6 && !d4 && !d8 && !d7 && d3 && !d9 && d1)
	{
		//return partialHide->ourImages[4+rand()%2].bitmap; //visble bottom
		return partialHide->ourImages[4].bitmap; //visble bottom
	}
	else if(!d2 && !d6 && d4 && !d8 && d7 && !d3 && !d9 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[2+rand()%2].bitmap); //visible left
		//return CSDL_Ext::rotate01(partialHide->ourImages[2].bitmap); //visible left
		return partialHide->ourImages[36].bitmap;
	}
	else if(!d2 && d6 && !d4 && !d8 && !d7 && d3 && d9 && !d1)
	{
		//return partialHide->ourImages[2+rand()%2].bitmap; //visible right
		return partialHide->ourImages[2].bitmap; //visible right
	}
	else if(d2 && d6 && !d4 && !d8 && !d7)
	{
		//return partialHide->ourImages[12+2*(rand()%2)].bitmap; //visible bottom, right - bottom, right; left top corner hidden
		return partialHide->ourImages[12].bitmap; //visible bottom, right - bottom, right; left top corner hidden
	}
	else if(!d2 && d6 && !d4 && d8 && !d1)
	{
		return partialHide->ourImages[13].bitmap; //visible right, right - top; left bottom corner hidden
	}
	else if(!d2 && !d6 && d4 && d8 && !d3)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[13].bitmap); //visible top, top - left, left; right bottom corner hidden
		return partialHide->ourImages[37].bitmap;
	}
	else if(d2 && !d6 && d4 && !d8 && !d9)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[12+2*(rand()%2)].bitmap); //visible left, left - bottom, bottom; right top corner hidden
		//return CSDL_Ext::rotate01(partialHide->ourImages[12].bitmap); //visible left, left - bottom, bottom; right top corner hidden
		return partialHide->ourImages[38].bitmap;
	}
	else if(d2 && d6 && d4 && d8)
	{
		return partialHide->ourImages[10].bitmap; //visible left, right, bottom and top
	}
	if(!d2 && !d6 && !d4 && !d8 && !d7 && d3 && d9 && !d1)
	{
		return partialHide->ourImages[16].bitmap; //visible right corners
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && !d3 && d9 && !d1)
	{
		return partialHide->ourImages[18].bitmap; //visible top corners
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && !d3 && !d9 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[16].bitmap); //visible left corners
		return partialHide->ourImages[39].bitmap;
	}
	if(!d2 && !d6 && !d4 && !d8 && !d7 && d3 && !d9 && d1)
	{
		//return CSDL_Ext::hFlip(partialHide->ourImages[18].bitmap); //visible bottom corners
		return partialHide->ourImages[40].bitmap;
	}
	if(!d2 && !d6 && !d4 && !d8 && !d7 && !d3 && d9 && d1)
	{
		return partialHide->ourImages[17].bitmap; //visible right - top and bottom - left corners
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && d3 && !d9 && !d1)
	{
		//return CSDL_Ext::hFlip(partialHide->ourImages[17].bitmap); //visible top - left and bottom - right corners
		return partialHide->ourImages[41].bitmap;
	}
	if(!d2 && !d6 && !d4 && !d8 && !d7 && d3 && d9 && d1)
	{
		return partialHide->ourImages[19].bitmap; //visible corners without left top
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && d3 && d9 && !d1)
	{
		return partialHide->ourImages[20].bitmap; //visible corners without left bottom
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && !d3 && d9 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[20].bitmap); //visible corners without right bottom
		return partialHide->ourImages[42].bitmap;
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && d3 && !d9 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[19].bitmap); //visible corners without right top
		return partialHide->ourImages[43].bitmap;
	}
	if(!d2 && !d6 && !d4 && !d8 && d7 && d3 && d9 && d1)
	{
		return partialHide->ourImages[21].bitmap; //visible all corners only
	}
	if(d2 && d6 && d4 && !d8)
	{
		return partialHide->ourImages[6].bitmap; //hidden top
	}
	if(d2 && !d6 && d4 && d8)
	{
		return partialHide->ourImages[7].bitmap; //hidden right
	}
	if(!d2 && d6 && d4 && d8)
	{
		return partialHide->ourImages[8].bitmap; //hidden bottom
	}
	if(d2 && d6 && !d4 && d8)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[7].bitmap); //hidden left
		return partialHide->ourImages[44].bitmap;
	}
	if(!d2 && d6 && d4 && !d8)
	{
		return partialHide->ourImages[9].bitmap; //hidden top and bottom
	}
	if(d2 && !d6 && !d4 && d8)
	{
		return partialHide->ourImages[29].bitmap;  //hidden left and right
	}
	if(!d2 && !d6 && !d4 && d8 && d3 && !d1)
	{
		return partialHide->ourImages[24].bitmap; //visible top and right bottom corner
	}
	if(!d2 && !d6 && !d4 && d8 && !d3 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[24].bitmap); //visible top and left bottom corner
		return partialHide->ourImages[45].bitmap;
	}
	if(!d2 && !d6 && !d4 && d8 && d3 && d1)
	{
		return partialHide->ourImages[33].bitmap; //visible top and bottom corners
	}
	if(!d2 && !d6 && d4 && !d8 && !d3 && d9)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[26].bitmap); //visible left and right top corner
		return partialHide->ourImages[46].bitmap;
	}
	if(!d2 && !d6 && d4 && !d8 && d3 && !d9)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[25].bitmap); //visible left and right bottom corner
		return partialHide->ourImages[47].bitmap;
	}
	if(!d2 && !d6 && d4 && !d8 && d3 && d9)
	{
		return partialHide->ourImages[32].bitmap; //visible left and right corners
	}
	if(d2 && !d6 && !d4 && !d8 && d7 && !d9)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[30].bitmap); //visible bottom and left top corner
		return partialHide->ourImages[48].bitmap;
	}
	if(d2 && !d6 && !d4 && !d8 && !d7 && d9)
	{
		return partialHide->ourImages[30].bitmap; //visible bottom and right top corner
	}
	if(d2 && !d6 && !d4 && !d8 && d7 && d9)
	{
		return partialHide->ourImages[31].bitmap; //visible bottom and top corners
	}
	if(!d2 && d6 && !d4 && !d8 && !d7 && d1)
	{
		return partialHide->ourImages[25].bitmap; //visible right and left bottom corner
	}
	if(!d2 && d6 && !d4 && !d8 && d7 && !d1)
	{
		return partialHide->ourImages[26].bitmap; //visible right and left top corner
	}
	if(!d2 && d6 && !d4 && !d8 && d7 && d1)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[32].bitmap); //visible right and left cornres
		return partialHide->ourImages[49].bitmap;
	}
	if(d2 && d6 && !d4 && !d8 && d7)
	{
		return partialHide->ourImages[28].bitmap; //visible bottom, right - bottom, right; left top corner visible
	}
	else if(!d2 && d6 && !d4 && d8 && d1)
	{
		return partialHide->ourImages[27].bitmap; //visible right, right - top; left bottom corner visible
	}
	else if(!d2 && !d6 && d4 && d8 && d3)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[27].bitmap); //visible top, top - left, left; right bottom corner visible
		return partialHide->ourImages[50].bitmap;
	}
	else if(d2 && !d6 && d4 && !d8 && d9)
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[28].bitmap); //visible left, left - bottom, bottom; right top corner visible
		return partialHide->ourImages[51].bitmap;
	}
	//newly added
	else if(!d2 && !d6 && !d4 && d8 && !d7 && !d3 && d9 && !d1) //visible t and tr
	{
		return partialHide->ourImages[0].bitmap;
	}
	else if(!d2 && !d6 && !d4 && d8 && d7 && !d3 && !d9 && !d1) //visible t and tl
	{
		return partialHide->ourImages[1].bitmap;
	}
	else if(d2 && !d6 && !d4 && !d8 && !d7 && d3 && !d9 && !d1) //visible b and br
	{
		return partialHide->ourImages[4].bitmap;
	}
	else if(d2 && !d6 && !d4 && !d8 && !d7 && !d3 && !d9 && d1) //visible b and bl
	{
		return partialHide->ourImages[5].bitmap;
	}
	else if(!d2 && !d6 && d4 && !d8 && d7 && !d3 && !d9 && !d1) //visible l and tl
	{
		return partialHide->ourImages[36].bitmap;
	}
	else if(!d2 && !d6 && d4 && !d8 && !d7 && !d3 && !d9 && d1) //visible l and bl
	{
		return partialHide->ourImages[36].bitmap;
	}
	else if(!d2 && d6 && !d4 && !d8 && !d7 && !d3 && d9 && !d1) //visible r and tr
	{
		return partialHide->ourImages[2].bitmap;
	}
	else if(!d2 && d6 && !d4 && !d8 && !d7 && d3 && !d9 && !d1) //visible r and br
	{
		return partialHide->ourImages[3].bitmap;
	}
	return fullHide->ourImages[0].bitmap; //this case should never happen, but it is better to hide too much than reveal it....
}

int CMapHandler::getCost(int3 &a, int3 &b, const CGHeroInstance *hero)
{
	int ret=-1;
	if(a.x>=CGI->mh->map->width && a.y>=CGI->mh->map->height)
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[CGI->mh->map->width-1][CGI->mh->map->width-1][a.z].malle];
	else if(a.x>=CGI->mh->map->width && a.y<CGI->mh->map->height)
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[CGI->mh->map->width-1][a.y][a.z].malle];
	else if(a.x<CGI->mh->map->width && a.y>=CGI->mh->map->height)
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[a.x][CGI->mh->map->width-1][a.z].malle];
	else
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[a.x][a.y][a.z].malle];
	if(!(a.x==b.x || a.y==b.y))
		ret*=1.41421;

	//TODO: use hero's pathfinding skill during calculating cost
	return ret;
}

std::vector < std::string > CMapHandler::getObjDescriptions(int3 pos)
{
	std::vector < std::pair<CGObjectInstance*,SDL_Rect > > objs = ttiles[pos.x][pos.y][pos.z].objects;
	std::vector<std::string> ret;
	for(int g=0; g<objs.size(); ++g)
	{
		if( (5-(objs[g].first->pos.y-pos.y)) >= 0 && (5-(objs[g].first->pos.y-pos.y)) < 6 && (objs[g].first->pos.x-pos.x) >= 0 && (objs[g].first->pos.x-pos.x)<7 && objs[g].first->defInfo &&
			(((objs[g].first->defInfo->blockMap[5-(objs[g].first->pos.y-pos.y)])>>((objs[g].first->pos.x-pos.x)))&1)==0
			) //checking position blocking
		{
			//unsigned char * blm = objs[g].first->defInfo->blockMap;
			if (objs[g].first->state)
				ret.push_back(objs[g].first->state->hoverText(objs[g].first));
			else
				ret.push_back(CGI->objh->objects[objs[g].first->ID].name);
		}
	}
	return ret;
}

std::vector < CGObjectInstance * > CMapHandler::getVisitableObjs(int3 pos)
{
	std::vector < CGObjectInstance * > ret;
	for(int h=0; h<ttiles[pos.x][pos.y][pos.z].objects.size(); ++h)
	{
		CGObjectInstance * curi = ttiles[pos.x][pos.y][pos.z].objects[h].first;
		if(curi->visitableAt(- curi->pos.x + pos.x + curi->getWidth() - 1, -curi->pos.y + pos.y + curi->getHeight() - 1))
			ret.push_back(curi);
	}
	return ret;
}

CGObjectInstance * CMapHandler::createObject(int id, int subid, int3 pos, int owner)
{
	CGObjectInstance * nobj;
	switch(id)
	{
	case 34: //hero
		{
			CGHeroInstance * nobj;
			nobj = new CGHeroInstance();
			nobj->pos = pos;
			nobj->tempOwner = owner;
			nobj->defInfo = new CGDefInfo();
			nobj->defInfo->id = 34;
			nobj->defInfo->subid = subid;
			nobj->defInfo->printPriority = 0;
			nobj->type = CGI->heroh->heroes[subid];
			for(int i=0;i<6;i++)
			{
				nobj->defInfo->blockMap[i]=255;
				nobj->defInfo->visitMap[i]=0;
			}
			nobj->ID = id;
			nobj->subID = subid;
			nobj->defInfo->handler=NULL;
			nobj->defInfo->blockMap[5] = 253;
			nobj->defInfo->visitMap[5] = 2;
			nobj->artifacts.resize(20);
			nobj->artifWorn[16] = 3;
			nobj->primSkills.resize(4);
			nobj->primSkills[0] = nobj->type->heroClass->initialAttack;
			nobj->primSkills[1] = nobj->type->heroClass->initialDefence;
			nobj->primSkills[2] = nobj->type->heroClass->initialPower;
			nobj->primSkills[3] = nobj->type->heroClass->initialKnowledge;
			nobj->mana = 10 * nobj->primSkills[3];
			return nobj;
		}
	case 98: //town
		nobj = new CGTownInstance;
		break;
	default: //rest of objects
		nobj = new CGObjectInstance;
		nobj->defInfo = CGI->dobjinfo->gobjs[id][subid];
		break;
	}
	nobj->ID = id;
	nobj->subID = subid;
	if(!nobj->defInfo)
		std::cout <<"No def declaration for " <<id <<" "<<subid<<std::endl;
	nobj->pos = pos;
	//nobj->state = NULL;//new CLuaObjectScript();
	nobj->tempOwner = owner;
	nobj->info = NULL;
	nobj->defInfo->id = id;
	nobj->defInfo->subid = subid;

	//assigning defhandler
	if(nobj->ID==34 || nobj->ID==98)
		return nobj;
	nobj->defInfo = CGI->dobjinfo->gobjs[id][subid];
	if(!nobj->defInfo->handler)
	{
		nobj->defInfo->handler = CDefHandler::giveDef(nobj->defInfo->name);
		nobj->defInfo->width = nobj->defInfo->handler->ourImages[0].bitmap->w/32;
		nobj->defInfo->height = nobj->defInfo->handler->ourImages[0].bitmap->h/32;
	}
	return nobj;
}

std::string CMapHandler::getDefName(int id, int subid)
{
	CGDefInfo* temp = CGI->dobjinfo->gobjs[id][subid];
	if(temp)
		return temp->name;
	throw new std::exception("Def not found.");
}

bool CMapHandler::printObject(CGObjectInstance *obj)
{
	CDefHandler * curd = obj->defInfo->handler;
	for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
	{
		for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
		{
			SDL_Rect cr;
			cr.w = 32;
			cr.h = 32;
			cr.x = fx*32;
			cr.y = fy*32;
			std::pair<CGObjectInstance*,SDL_Rect> toAdd = std::make_pair(obj, cr);
			if((obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)>=0 && (obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)<ttiles.size()-Woff && (obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)>=0 && (obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)<ttiles[0].size()-Hoff)
			{
				TerrainTile2 & curt = 
					ttiles
					  [obj->pos.x + fx - curd->ourImages[0].bitmap->w/32]
				      [obj->pos.y + fy - curd->ourImages[0].bitmap->h/32]
					  [obj->pos.z];


				ttiles[obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1][obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1][obj->pos.z].objects.push_back(toAdd);
			}

		} // for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
	} //for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
	return true;
}

bool CMapHandler::hideObject(CGObjectInstance *obj)
{
	CDefHandler * curd = obj->defInfo->handler;
	for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
	{
		for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
		{
			if((obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)>=0 && (obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)<ttiles.size()-Woff && (obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)>=0 && (obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)<ttiles[0].size()-Hoff)
			{
				std::vector < std::pair<CGObjectInstance*,SDL_Rect> > & ctile = ttiles[obj->pos.x + fx - curd->ourImages[0].bitmap->w/32+1][obj->pos.y + fy - curd->ourImages[0].bitmap->h/32+1][obj->pos.z].objects;
				for(int dd=0; dd<ctile.size(); ++dd)
				{
					if(ctile[dd].first->id==obj->id)
						ctile.erase(ctile.begin() + dd);
				}
			}

		} // for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
	} //for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
	return true;
}

std::string CMapHandler::getRandomizedDefName(CGDefInfo *di, CGObjectInstance * obj)
{
	return std::string();
}

bool CMapHandler::removeObject(CGObjectInstance *obj)
{
	hideObject(obj);
	std::vector<CGObjectInstance *>::iterator db = std::find(map->objects.begin(), map->objects.end(), obj);
	recalculateHideVisPosUnderObj(*db);
	delete *db;
	map->objects.erase(db);
	return true;
}

bool CMapHandler::recalculateHideVisPos(int3 &pos)
{
	ttiles[pos.x][pos.y][pos.z].visitable = false;
	ttiles[pos.x][pos.y][pos.z].blocked = false;
	for(int i=0; i<ttiles[pos.x][pos.y][pos.z].objects.size(); ++i)
	{
		CDefHandler * curd = ttiles[pos.x][pos.y][pos.z].objects[i].first->defInfo->handler;
		for(int fx=0; fx<8; ++fx)
		{
			for(int fy=0; fy<6; ++fy)
			{
				int xVal = ttiles[pos.x][pos.y][pos.z].objects[i].first->pos.x + fx - 7;
				int yVal = ttiles[pos.x][pos.y][pos.z].objects[i].first->pos.y + fy - 5;
				int zVal = ttiles[pos.x][pos.y][pos.z].objects[i].first->pos.z;
				if(xVal>=0 && xVal<ttiles.size()-Woff && yVal>=0 && yVal<ttiles[0].size()-Hoff)
				{
					TerrainTile2 & curt = ttiles[xVal][yVal][zVal];
					if(((ttiles[pos.x][pos.y][pos.z].objects[i].first->defInfo->visitMap[fy] >> (7 - fx)) & 1))
						curt.visitable = true;
					if(!((ttiles[pos.x][pos.y][pos.z].objects[i].first->defInfo->blockMap[fy] >> (7 - fx)) & 1))
						curt.blocked = true;
				}
			}
		}
	}
	return true;
}

bool CMapHandler::recalculateHideVisPosUnderObj(CGObjectInstance *obj, bool withBorder)
{
	if(withBorder)
	{
		for(int fx=-1; fx<=obj->defInfo->handler->ourImages[0].bitmap->w/32; ++fx)
		{
			for(int fy=-1; fy<=obj->defInfo->handler->ourImages[0].bitmap->h/32; ++fy)
			{
				if((obj->pos.x + fx - obj->defInfo->handler->ourImages[0].bitmap->w/32+1)>=0 && (obj->pos.x + fx - obj->defInfo->handler->ourImages[0].bitmap->w/32+1)<ttiles.size()-Woff && (obj->pos.y + fy - obj->defInfo->handler->ourImages[0].bitmap->h/32+1)>=0 && (obj->pos.y + fy - obj->defInfo->handler->ourImages[0].bitmap->h/32+1)<ttiles[0].size()-Hoff)
				{
					recalculateHideVisPos(int3(obj->pos.x + fx - obj->defInfo->handler->ourImages[0].bitmap->w/32 +1, obj->pos.y + fy - obj->defInfo->handler->ourImages[0].bitmap->h/32 + 1, obj->pos.z));
				}
			}
		}
	}
	else
	{
		for(int fx=0; fx<obj->defInfo->handler->ourImages[0].bitmap->w/32; ++fx)
		{
			for(int fy=0; fy<obj->defInfo->handler->ourImages[0].bitmap->h/32; ++fy)
			{
				if((obj->pos.x + fx - obj->defInfo->handler->ourImages[0].bitmap->w/32+1)>=0 && (obj->pos.x + fx - obj->defInfo->handler->ourImages[0].bitmap->w/32+1)<ttiles.size()-Woff && (obj->pos.y + fy - obj->defInfo->handler->ourImages[0].bitmap->h/32+1)>=0 && (obj->pos.y + fy - obj->defInfo->handler->ourImages[0].bitmap->h/32+1)<ttiles[0].size()-Hoff)
				{
					recalculateHideVisPos(int3(obj->pos.x + fx - obj->defInfo->handler->ourImages[0].bitmap->w/32 +1, obj->pos.y + fy - obj->defInfo->handler->ourImages[0].bitmap->h/32 + 1, obj->pos.z));
				}
			}
		}
	}
	return true;
}

unsigned char CMapHandler::getHeroFrameNum(const unsigned char &dir, const bool &isMoving) const
{
	if(isMoving)
	{
		switch(dir)
		{
		case 1:
			return 10;
		case 2:
			return 5;
		case 3:
			return 6;
		case 4:
			return 7;
		case 5:
			return 8;
		case 6:
			return 9;
		case 7:
			return 12;
		case 8:
			return 11;
		default:
			return -1; //should never happen
		}
	}
	else //if(isMoving)
	{
		switch(dir)
		{
		case 1:
			return 13;
		case 2:
			return 0;
		case 3:
			return 1;
		case 4:
			return 2;
		case 5:
			return 3;
		case 6:
			return 4;
		case 7:
			return 15;
		case 8:
			return 14;
		default:
			return -1; //should never happen
		}
	}
}

void CMapHandler::validateRectTerr(SDL_Rect * val, const SDL_Rect * ext)
{
	if(ext)
	{
		if(val->x<0)
		{
			val->w += val->x;
			val->x = ext->x;
		}
		else
		{
			val->x += ext->x;
		}
		if(val->y<0)
		{
			val->h += val->y;
			val->y = ext->y;
		}
		else
		{
			val->y += ext->y;
		}

		if(val->x+val->w > ext->x+ext->w)
		{
			val->w = ext->x+ext->w-val->x;
		}
		if(val->y+val->h > ext->y+ext->h)
		{
			val->h = ext->y+ext->h-val->y;
		}

		//for sign problems
		if(val->h > 20000 || val->w > 20000)
		{
			val->h = val->w = 0;
		}
	}
}

unsigned char CMapHandler::getDir(const int3 &a, const int3 &b)
{
	if(a.z!=b.z)
		return -1; //error!
	if(a.x==b.x+1 && a.y==b.y+1) //lt
	{
		return 0;
	}
	else if(a.x==b.x && a.y==b.y+1) //t
	{
		return 1;
	}
	else if(a.x==b.x-1 && a.y==b.y+1) //rt
	{
		return 2;
	}
	else if(a.x==b.x-1 && a.y==b.y) //r
	{
		return 3;
	}
	else if(a.x==b.x-1 && a.y==b.y-1) //rb
	{
		return 4;
	}
	else if(a.x==b.x && a.y==b.y-1) //b
	{
		return 5;
	}
	else if(a.x==b.x+1 && a.y==b.y-1) //lb
	{
		return 6;
	}
	else if(a.x==b.x+1 && a.y==b.y) //l
	{
		return 7;
	}
	return -2; //shouldn't happen
}


void CMapHandler::loadDefs()
{
	std::set<int> loadedTypes;
	for (int i=0; i<map->width; i++)
	{
		for (int j=0; j<map->width; j++)
		{
			if (loadedTypes.find(map->terrain[i][j].tertype)==loadedTypes.end())
			{
				CDefHandler  *sdh = CDefHandler::giveDef(nameFromType(map->terrain[i][j].tertype).c_str());
				loadedTypes.insert(map->terrain[i][j].tertype);
				defs.push_back(sdh);
			}
			if (map->twoLevel && loadedTypes.find(map->undergroungTerrain[i][j].tertype)==loadedTypes.end())
			{
				CDefHandler  *sdh = CDefHandler::giveDef(nameFromType(map->undergroungTerrain[i][j].tertype).c_str());
				loadedTypes.insert(map->undergroungTerrain[i][j].tertype);
				defs.push_back(sdh);
			}
		}
	}
}
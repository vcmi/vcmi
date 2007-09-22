#include "stdafx.h"
#include "mapHandler.h"
#include "hch\CSemiDefHandler.h"
#include "SDL_rotozoom.h"
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include "stdlib.h"
#include "hch\CLodHandler.h"
#include "hch\CDefObjInfoHandler.h"
#include <algorithm>

extern SDL_Surface * ekran;

class OCM_HLP
{
public:
	bool operator ()(const std::pair<CObjectInstance *, SDL_Rect> & a, const std::pair<CObjectInstance *, SDL_Rect> & b)
	{
		return (*a.first)<(*b.first);
	}
} ocmptwo ;

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

	//roadBitmaps = new SDL_Surface** [reader->map.width+2*Woff];
	//for (int ii=0;ii<reader->map.width+2*Woff;ii++)
	//	roadBitmaps[ii] = new SDL_Surface*[reader->map.height+2*Hoff]; // allocate memory 
	sizes.x = CGI->ac->map.width;
	sizes.y = CGI->ac->map.height;
	sizes.z = CGI->ac->map.twoLevel+1;
	ttiles.resize(CGI->ac->map.width,Woff);
	for (int i=0-Woff;i<ttiles.size()-Woff;i++)
	{
		ttiles[i].resize(CGI->ac->map.height,Hoff);
	}
	for (int i=0-Woff;i<ttiles.size()-Woff;i++)
	{
		for (int j=0-Hoff;j<CGI->ac->map.height+Hoff;j++)
			ttiles[i][j].resize(CGI->ac->map.twoLevel+1,0);
	}



	for (int i=0; i<reader->map.width; i++) //jest po szerokoœci
	{
		for (int j=0; j<reader->map.height;j++) //po wysokoœci
		{
			for (int k=0; k<=reader->map.twoLevel; ++k)
			{
				TerrainTile** pomm = reader->map.terrain; ;
				if (k==0)
					pomm = reader->map.terrain;
				else
					pomm = reader->map.undergroungTerrain;
				if(pomm[i][j].malle)
				{
					int cDir;
					bool rotV, rotH;
					if(k==0)
					{
						int roadpom = reader->map.terrain[i][j].malle-1,
							impom = reader->map.terrain[i][j].roadDir;
						SDL_Surface *pom1 = roadDefs[roadpom]->ourImages[impom].bitmap;
						ttiles[i][j][k].roadbitmap.push_back(pom1);
						cDir = reader->map.terrain[i][j].roadDir;
						
						rotH = (reader->map.terrain[i][j].siodmyTajemniczyBajt >> 5) & 1;
						rotV = (reader->map.terrain[i][j].siodmyTajemniczyBajt >> 4) & 1;
					}
					else
					{
						int pom111 = reader->map.undergroungTerrain[i][j].malle-1,
							pom777 = reader->map.undergroungTerrain[i][j].roadDir;
						SDL_Surface *pom1 = roadDefs[pom111]->ourImages[pom777].bitmap;
						ttiles[i][j][k].roadbitmap.push_back(pom1);
						cDir = reader->map.undergroungTerrain[i][j].roadDir;

						rotH = (reader->map.undergroungTerrain[i][j].siodmyTajemniczyBajt >> 5) & 1;
						rotV = (reader->map.undergroungTerrain[i][j].siodmyTajemniczyBajt >> 4) & 1;
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
						SDL_Surface * buf = CSDL_Ext::secondAlphaTransform(ttiles[i][j][k].roadbitmap[0], su);
						SDL_FreeSurface(ttiles[i][j][k].roadbitmap[0]);
						ttiles[i][j][k].roadbitmap[0] = buf;
					}
				}
			}
		}
	}

	//initializing simple values
	for (int i=0; i<CGI->ac->map.width; i++) //jest po szerokoœci
	{
		for (int j=0; j<CGI->ac->map.height;j++) //po wysokoœci
		{
			for(int k=0; k<ttiles[0][0].size(); ++k)
			{
				ttiles[i][j][k].pos = int3(i, j, k);
				ttiles[i][j][k].blocked = false;
				ttiles[i][j][k].visitable = false;
				if(i<0 || j<0 || i>=CGI->ac->map.width || j>=CGI->ac->map.height)
				{
					ttiles[i][j][k].blocked = true;
					continue;
				}
				ttiles[i][j][k].terType = (k==0 ? CGI->ac->map.terrain[i][j].tertype : CGI->ac->map.undergroungTerrain[i][j].tertype);
				ttiles[i][j][k].malle = (k==0 ? CGI->ac->map.terrain[i][j].malle : CGI->ac->map.undergroungTerrain[i][j].malle);
				ttiles[i][j][k].nuine = (k==0 ? CGI->ac->map.terrain[i][j].nuine : CGI->ac->map.undergroungTerrain[i][j].nuine);
				ttiles[i][j][k].rivdir = (k==0 ? CGI->ac->map.terrain[i][j].rivDir : CGI->ac->map.undergroungTerrain[i][j].rivDir);
				ttiles[i][j][k].roaddir = (k==0 ? CGI->ac->map.terrain[i][j].roadDir : CGI->ac->map.undergroungTerrain[i][j].roadDir);

			}
		}
	}
	//simple values initialized

	for (int i=0; i<reader->map.width; i++) //jest po szerokoœci
	{
		for (int j=0; j<reader->map.height;j++) //po wysokoœci
		{
			for(int k=0; k<=reader->map.twoLevel; ++k)
			{
				TerrainTile** pomm = reader->map.terrain;
				if(k==0)
				{
					pomm = reader->map.terrain;
				}
				else
				{
					pomm = reader->map.undergroungTerrain;
				}
				if(pomm[i][j].nuine)
				{
					int cDir;
					bool rotH, rotV;
					if(k==0)
					{
						ttiles[i][j][k].rivbitmap.push_back(staticRiverDefs[reader->map.terrain[i][j].nuine-1]->ourImages[reader->map.terrain[i][j].rivDir].bitmap);
						cDir = reader->map.terrain[i][j].rivDir;
						rotH = (reader->map.terrain[i][j].siodmyTajemniczyBajt >> 3) & 1;
						rotV = (reader->map.terrain[i][j].siodmyTajemniczyBajt >> 2) & 1;
					}
					else
					{
						ttiles[i][j][k].rivbitmap.push_back(staticRiverDefs[reader->map.undergroungTerrain[i][j].nuine-1]->ourImages[reader->map.undergroungTerrain[i][j].rivDir].bitmap);
						cDir = reader->map.undergroungTerrain[i][j].rivDir;
						rotH = (reader->map.undergroungTerrain[i][j].siodmyTajemniczyBajt >> 3) & 1;
						rotV = (reader->map.undergroungTerrain[i][j].siodmyTajemniczyBajt >> 2) & 1;
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
						SDL_Surface * buf = CSDL_Ext::secondAlphaTransform(ttiles[i][j][k].rivbitmap[0], su);
						SDL_FreeSurface(ttiles[i][j][k].rivbitmap[0]);
						ttiles[i][j][k].rivbitmap[0] = buf;
					}
				}
			}
		}
	}

	SDL_FreeSurface(su);

	//road's and river's DefHandlers initialized

	//terrainBitmap = new SDL_Surface **[reader->map.width+2*Woff];
	//for (int ii=0;ii<reader->map.width+2*Woff;ii++)
	//	terrainBitmap[ii] = new SDL_Surface*[reader->map.height+2*Hoff]; // allocate memory 

	CDefHandler * bord = CGameInfo::mainObj->spriteh->giveDef("EDG.DEF");
	for (int i=0-Woff; i<reader->map.width+Woff; i++) //jest po szerokoœci
	{
		for (int j=0-Hoff; j<reader->map.height+Hoff;j++) //po wysokoœci
		{
			for(int k=0; k<=reader->map.twoLevel; ++k)
			{
				if(i < 0 || i > (reader->map.width-1) || j < 0  || j > (reader->map.height-1))
				{
					if(i==-1 && j==-1)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[16].bitmap);
						continue;
					}
					else if(i==-1 && j==(reader->map.height))
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[19].bitmap);
						continue;
					}
					else if(i==(reader->map.width) && j==-1)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[17].bitmap);
						continue;
					}
					else if(i==(reader->map.width) && j==(reader->map.height))
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[18].bitmap);
						continue;
					}
					else if(j == -1 && i > -1 && i < reader->map.height)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[22+rand()%2].bitmap);
						continue;
					}
					else if(i == -1 && j > -1 && j < reader->map.height)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[33+rand()%2].bitmap);
						continue;
					}
					else if(j == reader->map.height && i >-1 && i < reader->map.width)
					{
						ttiles[i][j][k].terbitmap.push_back(bord->ourImages[29+rand()%2].bitmap);
						continue;
					}
					else if(i == reader->map.width && j > -1 && j < reader->map.height)
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
				//TerrainTile zz = reader->map.terrain[i-Woff][j-Hoff];
				std::string name;
				if (k>0)
					name = CSemiDefHandler::nameFromType(reader->map.undergroungTerrain[i][j].tertype);
				else
					name = CSemiDefHandler::nameFromType(reader->map.terrain[i][j].tertype);
				for (unsigned int m=0; m<reader->defs.size(); m++)
				{
					try
					{
						if (reader->defs[m]->defName != name)
							continue;
						else
						{
							int ktora;
							if (k==0)
								ktora = reader->map.terrain[i][j].terview;
							else
								ktora = reader->map.undergroungTerrain[i][j].terview;
							ttiles[i][j][k].terbitmap.push_back(reader->defs[m]->ourImages[ktora].bitmap);
							int zz;
							if (k==0)
								zz = (reader->map.terrain[i][j].siodmyTajemniczyBajt)%4;
							else 
								zz = (reader->map.undergroungTerrain[i][j].siodmyTajemniczyBajt)%4;
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
	//initializing objects / rects
	for(int f=0; f<CGI->objh->objInstances.size(); ++f)
	{	
		CGI->objh->objInstances[f]->pos.x+=1;
		CGI->objh->objInstances[f]->pos.y+=1;
		CDefHandler * curd = CGI->ac->map.defy[CGI->objh->objInstances[f]->defNumber].handler;
		for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
		{
			for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
			{
				SDL_Rect cr;
				cr.w = 32;
				cr.h = 32;
				cr.x = fx*32;
				cr.y = fy*32;
				std::pair<CObjectInstance *, SDL_Rect> toAdd = std::make_pair(CGI->objh->objInstances[f], cr);
				if((CGI->objh->objInstances[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32)>=0 && (CGI->objh->objInstances[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32)<ttiles.size()-Woff && (CGI->objh->objInstances[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32)>=0 && (CGI->objh->objInstances[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32)<ttiles[0].size()-Hoff)
				{
					TerrainTile2 & curt = 
						ttiles
						  [CGI->objh->objInstances[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32]
					      [CGI->objh->objInstances[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32]
						  [CGI->objh->objInstances[f]->pos.z];


					ttiles[CGI->objh->objInstances[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32][CGI->objh->objInstances[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32][CGI->objh->objInstances[f]->pos.z].objects.push_back(toAdd);
				}

			} // for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
		} //for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
	} // for(int f=0; f<CGI->objh->objInstances.size(); ++f)
	for(int f=0; f<CGI->objh->objInstances.size(); ++f) //calculationg blocked / visitable positions
	{	
		if(CGI->objh->objInstances[f]->defObjInfoNumber == -1)
			continue;
		CDefHandler * curd = CGI->ac->map.defy[CGI->objh->objInstances[f]->defNumber].handler;
		for(int fx=0; fx<8; ++fx)
		{
			for(int fy=0; fy<6; ++fy)
			{
				int xVal = CGI->objh->objInstances[f]->pos.x + fx - 8;
				int yVal = CGI->objh->objInstances[f]->pos.y + fy - 6;
				int zVal = CGI->objh->objInstances[f]->pos.z;
				if(xVal>=0 && xVal<ttiles.size()-Woff && yVal>=0 && yVal<ttiles[0].size()-Hoff)
				{
					TerrainTile2 & curt = ttiles[xVal][yVal][zVal];
					if(((CGI->dobjinfo->objs[CGI->objh->objInstances[f]->defObjInfoNumber].visitMap[fy] >> (7 - fx)) & 1))
						curt.visitable = true;
					if(!((CGI->dobjinfo->objs[CGI->objh->objInstances[f]->defObjInfoNumber].blockMap[fy] >> (7 - fx)) & 1))
						curt.blocked = true;
				}
			}
		}
	}
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
	if (((dx+x)>((reader->map.width+Woff)) || (dy+y)>((reader->map.height+Hoff))) || ((x<-Woff)||(y<-Hoff) ) )
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
			SDL_BlitSurface(ttiles[x+bx][y+by][level].terbitmap[anim%ttiles[x+bx][y+by][level].terbitmap.size()],NULL,su,&sr);
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
			if(ttiles[x+bx][y+by][level].rivbitmap.size())
				SDL_BlitSurface(ttiles[x+bx][y+by][level].rivbitmap[anim%ttiles[x+bx][y+by][level].rivbitmap.size()],NULL,su,&sr);
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
			if(ttiles[x+bx][y+by][level].roadbitmap.size())
				SDL_BlitSurface(ttiles[x+bx][y+by][level].roadbitmap[anim%ttiles[x+bx][y+by][level].roadbitmap.size()],NULL,su,&sr);
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

				SDL_Rect pp = ttiles[x+bx][y+by][level].objects[h].second;
				if(ttiles[x+bx][y+by][level].objects[h].first->isHero && ttiles[x+bx][y+by][level].objects[h].first->moveDir)
				{
					int imgVal = 8;
					SDL_Surface * tb;
					switch(ttiles[x+bx][y+by][level].objects[h].first->moveDir)
					{
					case 1:
						{
							std::vector<Cimage> & iv = ((CHeroObjInfo*)ttiles[x+bx][y+by][level].objects[h].first->info)->myInstance->type->heroClass->moveAnim->ourImages;
							for(int gg=0; gg<iv.size(); ++gg)
							{
								if(iv[gg].groupNumber==10)
								{
									tb = iv[gg+anim%imgVal].bitmap;
									break;
								}
							}
							SDL_BlitSurface(tb,&pp,su,&sr);
							break;
						}
					case 2:
						{
							std::vector<Cimage> & iv = ((CHeroObjInfo*)ttiles[x+bx][y+by][level].objects[h].first->info)->myInstance->type->heroClass->moveAnim->ourImages;
							for(int gg=0; gg<iv.size(); ++gg)
							{
								if(iv[gg].groupNumber==5)
								{
									tb = iv[gg+anim%imgVal].bitmap;
									break;
								}
							}
							SDL_BlitSurface(tb,&pp,su,&sr);
							break;
						}
					case 3:
						{
							std::vector<Cimage> & iv = ((CHeroObjInfo*)ttiles[x+bx][y+by][level].objects[h].first->info)->myInstance->type->heroClass->moveAnim->ourImages;
							for(int gg=0; gg<iv.size(); ++gg)
							{
								if(iv[gg].groupNumber==6)
								{
									tb = iv[gg+anim%imgVal].bitmap;
									break;
								}
							}
							SDL_BlitSurface(tb,&pp,su,&sr);
							break;
						}
					case 4:
						{
							std::vector<Cimage> & iv = ((CHeroObjInfo*)ttiles[x+bx][y+by][level].objects[h].first->info)->myInstance->type->heroClass->moveAnim->ourImages;
							for(int gg=0; gg<iv.size(); ++gg)
							{
								if(iv[gg].groupNumber==7)
								{
									tb = iv[gg+anim%imgVal].bitmap;
									break;
								}
							}
							SDL_BlitSurface(tb,&pp,su,&sr);
							break;
						}
					case 5:
						{
							std::vector<Cimage> & iv = ((CHeroObjInfo*)ttiles[x+bx][y+by][level].objects[h].first->info)->myInstance->type->heroClass->moveAnim->ourImages;
							for(int gg=0; gg<iv.size(); ++gg)
							{
								if(iv[gg].groupNumber==8)
								{
									tb = iv[gg+anim%imgVal].bitmap;
									break;
								}
							}
							SDL_BlitSurface(tb,&pp,su,&sr);
							break;
						}
					case 6:
						{
							std::vector<Cimage> & iv = ((CHeroObjInfo*)ttiles[x+bx][y+by][level].objects[h].first->info)->myInstance->type->heroClass->moveAnim->ourImages;
							for(int gg=0; gg<iv.size(); ++gg)
							{
								if(iv[gg].groupNumber==9)
								{
									tb = iv[gg+anim%imgVal].bitmap;
									break;
								}
							}
							SDL_BlitSurface(tb,&pp,su,&sr);
							break;
						}
					case 7:
						{
							std::vector<Cimage> & iv = ((CHeroObjInfo*)ttiles[x+bx][y+by][level].objects[h].first->info)->myInstance->type->heroClass->moveAnim->ourImages;
							for(int gg=0; gg<iv.size(); ++gg)
							{
								if(iv[gg].groupNumber==12)
								{
									tb = iv[gg+anim%imgVal].bitmap;
									break;
								}
							}
							SDL_BlitSurface(tb,&pp,su,&sr);
							break;
						}
					case 8:
						{
							std::vector<Cimage> & iv = ((CHeroObjInfo*)ttiles[x+bx][y+by][level].objects[h].first->info)->myInstance->type->heroClass->moveAnim->ourImages;
							for(int gg=0; gg<iv.size(); ++gg)
							{
								if(iv[gg].groupNumber==11)
								{
									tb = iv[gg+anim%imgVal].bitmap;
									break;
								}
							}
							SDL_BlitSurface(tb,&pp,su,&sr);
							break;
						}
					}
				}
				else
				{
					int imgVal = CGI->ac->map.defy[ttiles[x+bx][y+by][level].objects[h].first->defNumber].handler->ourImages.size();
					SDL_BlitSurface(CGI->ac->map.defy[ttiles[x+bx][y+by][level].objects[h].first->defNumber].handler->ourImages[anim%imgVal].bitmap,&pp,su,&sr);
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
			if (!level)
			{
				
				if( bx+x>-1 && by+y>-1 && bx+x<visibility.size()-(-1) && by+y<visibility[0].size()-(-1) && !visibility[bx+x][by+y])
				{
					SDL_Surface * hide = getVisBitmap(bx+x, by+y, visibility);
					SDL_Surface * hide2 = CSDL_Ext::secondAlphaTransform(hide, su);
					SDL_BlitSurface(hide2, NULL, su, &sr);
					SDL_FreeSurface(hide2);
				}
			}
			else
			{
				if( bx+x>-1 && by+y>-1 && bx+x<undVisibility.size()-(-1) && by+y<undVisibility[0].size()-(-1) && !undVisibility[bx+x][by+y])
				{
					SDL_Surface * hide = getVisBitmap(bx+x, by+y, undVisibility);
					SDL_Surface * hide2 = CSDL_Ext::secondAlphaTransform(hide, su);
					SDL_BlitSurface(hide2, NULL, su, &sr);
					SDL_FreeSurface(hide2);
				}
			}
		}
	}
	////shadow printed
	//printing borders
	for (int bx=0; bx<dx; bx++)
	{
		for (int by=0; by<dy; by++)
		{
			if(bx+x<0 || by+y<0 || bx+x>reader->map.width+(-1) || by+y>reader->map.height+(-1))
			{
				SDL_Rect sr;
				sr.y=by*32;
				sr.x=bx*32;
				sr.h=sr.w=32;

				SDL_BlitSurface(ttiles[x+bx][y+by][level].terbitmap[anim%ttiles[x+bx][y+by][level].terbitmap.size()],NULL,su,&sr);
			}
			else 
			{
				if(MARK_BLOCKED_POSITIONS &&  ttiles[x+bx][y+by][level].blocked) //temporary hiding blocked positions
				{
					SDL_Rect sr;
					sr.y=by*32;
					sr.x=bx*32;
					sr.h=sr.w=32;

					SDL_Surface * ns =  SDL_CreateRGBSurface(SDL_SWSURFACE, 32, 32, 32,
									   rmask, gmask, bmask, amask);
					for(int f=0; f<ns->w*ns->h*4; ++f)
					{
						*((unsigned char*)(ns->pixels) + f) = 128;
					}

					SDL_BlitSurface(ns,NULL,su,&sr);

					SDL_FreeSurface(ns);
				}
				if(MARK_VISITABLE_POSITIONS &&  ttiles[x+bx][y+by][level].visitable) //temporary hiding visitable positions
				{
					SDL_Rect sr;
					sr.y=by*32;
					sr.x=bx*32;
					sr.h=sr.w=32;

					SDL_Surface * ns =  SDL_CreateRGBSurface(SDL_SWSURFACE, 32, 32, 32,
									   rmask, gmask, bmask, amask);
					for(int f=0; f<ns->w*ns->h*4; ++f)
					{
						*((unsigned char*)(ns->pixels) + f) = 128;
					}

					SDL_BlitSurface(ns,NULL,su,&sr);

					SDL_FreeSurface(ns);
				}
			}
		}
	}
	CSDL_Ext::update(su);
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

int CMapHandler::getCost(int3 &a, int3 &b, const CHeroInstance *hero)
{
	int ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[a.x][a.y][a.z].malle];
	if(!(a.x==b.x || a.y==b.y))
		ret*=1.41421;

	//TODO: use hero's pathfinding skill during calculating cost
	return ret;
}

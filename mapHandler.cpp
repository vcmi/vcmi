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
#include "CGameState.h"
#include "CLua.h"
#include "hch\CCastleHandler.h"
#include "hch\CHeroHandler.h"
#include "hch\CTownHandler.h"
#include <iomanip>
#include <sstream>
extern SDL_Surface * ekran;

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
		//SDL_Surface * bufs = CSDL_Ext::secondAlphaTransform(defInfo->handler->ourImages[yy].bitmap, alphaTransSurf);
		//SDL_FreeSurface(defInfo->handler->ourImages[yy].bitmap);
		//defInfo->handler->ourImages[yy].bitmap = bufs;
		defInfo->handler->alphaTransformed = true;
	}
	SDL_FreeSurface(alphaTransSurf);
}
int CMapHandler::pickHero(int owner)
{
	int h;
	if(usedHeroes.find(h = CGI->scenarioOps.getIthPlayersSettings(owner).hero)==usedHeroes.end() && h>=0) //we haven't used selected hero
	{
		usedHeroes.insert(h);
		return h;
	}
	int f = CGI->scenarioOps.getIthPlayersSettings(owner).castle;
	int i=0;
	do //try to find free hero of our faction
	{
		i++;
		h = CGI->scenarioOps.getIthPlayersSettings(owner).castle*HEROES_PER_TYPE*2+(rand()%(HEROES_PER_TYPE*2));//cgi->scenarioOps.playerInfos[pru].hero = cgi->
	} while((usedHeroes.find(h)!=usedHeroes.end())  &&  i<175);
	if(i>174) //probably no free heroes - there's no point in further search, we'll take first free
	{
		for(int j=0; j<HEROES_PER_TYPE * 2 * F_NUMBER; j++)
			if(usedHeroes.find(j)==usedHeroes.end())
				h=j;
	}
	usedHeroes.insert(h);
	return h;
}
std::pair<int,int> CMapHandler::pickObject(CGObjectInstance *obj)
{
	switch(obj->ID)
	{
	case 65: //random artifact
		return std::pair<int,int>(5,(rand()%136)+7); //tylko sensowny zakres - na poczatku sa katapulty itp, na koncu specjalne i blanki
	case 66: //random treasure artifact
		return std::pair<int,int>(5,CGI->arth->treasures[rand()%CGI->arth->treasures.size()]->id);
	case 67: //random minor artifact
		return std::pair<int,int>(5,CGI->arth->minors[rand()%CGI->arth->minors.size()]->id);
	case 68: //random major artifact
		return std::pair<int,int>(5,CGI->arth->majors[rand()%CGI->arth->majors.size()]->id);
	case 69: //random relic artifact
		return std::pair<int,int>(5,CGI->arth->relics[rand()%CGI->arth->relics.size()]->id);
	case 70: //random hero
		{
			return std::pair<int,int>(34,pickHero(obj->tempOwner));
		}
	case 71: //random monster
		return std::pair<int,int>(54,rand()%(CGI->creh->creatures.size())); 
	case 72: //random monster lvl1
		return std::pair<int,int>(54,CGI->creh->levelCreatures[1][rand()%CGI->creh->levelCreatures[1].size()]->idNumber); 
	case 73: //random monster lvl2
		return std::pair<int,int>(54,CGI->creh->levelCreatures[2][rand()%CGI->creh->levelCreatures[2].size()]->idNumber);
	case 74: //random monster lvl3
		return std::pair<int,int>(54,CGI->creh->levelCreatures[3][rand()%CGI->creh->levelCreatures[3].size()]->idNumber);
	case 75: //random monster lvl4
		return std::pair<int,int>(54,CGI->creh->levelCreatures[4][rand()%CGI->creh->levelCreatures[4].size()]->idNumber);
	case 76: //random resource
		return std::pair<int,int>(79,rand()%7); //now it's OH3 style, use %8 for mithril 
	case 77: //random town
		{
			int align = ((CCastleObjInfo*)obj->info)->alignment,
				f;
			if(align>PLAYER_LIMIT-1)//same as owner / random
			{
				if(obj->tempOwner > PLAYER_LIMIT-1)
					f = -1; //random
				else
					f = CGI->scenarioOps.getIthPlayersSettings(obj->tempOwner).castle;
			}
			else
			{
				f = CGI->scenarioOps.getIthPlayersSettings(align).castle;
			}
			if(f<0) f = rand()%CGI->townh->towns.size();
			return std::pair<int,int>(98,f); 
		}
	case 162: //random monster lvl5
		return std::pair<int,int>(54,CGI->creh->levelCreatures[5][rand()%CGI->creh->levelCreatures[5].size()]->idNumber);
	case 163: //random monster lvl6
		return std::pair<int,int>(54,CGI->creh->levelCreatures[6][rand()%CGI->creh->levelCreatures[6].size()]->idNumber);
	case 164: //random monster lvl7
		return std::pair<int,int>(54,CGI->creh->levelCreatures[7][rand()%CGI->creh->levelCreatures[7].size()]->idNumber); 
	case 216: //random dwelling
		{
			int faction = rand()%F_NUMBER;
			CCreGen2ObjInfo* info =(CCreGen2ObjInfo*)obj->info;
			if (info->asCastle)
			{
				for(int i=0;i<CGI->objh->objInstances.size();i++)
				{
					if(CGI->objh->objInstances[i]->ID==77 && dynamic_cast<CGTownInstance*>(CGI->objh->objInstances[i])->identifier == info->identifier)
					{
						randomizeObject(CGI->objh->objInstances[i]); //we have to randomize the castle first
						faction = CGI->objh->objInstances[i]->subID;
						break;
					}
					else if(CGI->objh->objInstances[i]->ID==98 && dynamic_cast<CGTownInstance*>(CGI->objh->objInstances[i])->identifier == info->identifier)
					{
						faction = CGI->objh->objInstances[i]->subID;
						break;
					}
				}
			}
			else
			{
				while((!(info->castles[0]&(1<<faction))))
				{
					if((faction>7) && (info->castles[1]&(1<<(faction-8))))
						break;
					faction = rand()%F_NUMBER;
				}
			}
			int level = ((info->maxLevel-info->minLevel) ? (rand()%(info->maxLevel-info->minLevel)+info->minLevel) : (info->minLevel));
			int cid = CGI->townh->towns[faction].basicCreatures[level];
			for(int i=0;i<CGI->objh->cregens.size();i++)
				if(CGI->objh->cregens[i]==cid)
					return std::pair<int,int>(17,i); 
			std::cout << "Cannot find a dwelling for creature "<<cid <<std::endl;
			return std::pair<int,int>(17,0); 
		}
	case 217:
		{
			int faction = rand()%F_NUMBER;
			CCreGenObjInfo* info =(CCreGenObjInfo*)obj->info;
			if (info->asCastle)
			{
				for(int i=0;i<CGI->objh->objInstances.size();i++)
				{
					if(CGI->objh->objInstances[i]->ID==77 && dynamic_cast<CGTownInstance*>(CGI->objh->objInstances[i])->identifier == info->identifier)
					{
						randomizeObject(CGI->objh->objInstances[i]); //we have to randomize the castle first
						faction = CGI->objh->objInstances[i]->subID;
						break;
					}
					else if(CGI->objh->objInstances[i]->ID==98 && dynamic_cast<CGTownInstance*>(CGI->objh->objInstances[i])->identifier == info->identifier)
					{
						faction = CGI->objh->objInstances[i]->subID;
						break;
					}
				}
			}
			else
			{
				while((!(info->castles[0]&(1<<faction))))
				{
					if((faction>7) && (info->castles[1]&(1<<(faction-8))))
						break;
					faction = rand()%F_NUMBER;
				}
			}
			int cid = CGI->townh->towns[faction].basicCreatures[obj->subID];
			for(int i=0;i<CGI->objh->cregens.size();i++)
				if(CGI->objh->cregens[i]==cid)
					return std::pair<int,int>(17,i); 
			std::cout << "Cannot find a dwelling for creature "<<cid <<std::endl;
			return std::pair<int,int>(17,0); 
		}
	case 218:
		{
			CCreGen3ObjInfo* info =(CCreGen3ObjInfo*)obj->info;
			int level = ((info->maxLevel-info->minLevel) ? (rand()%(info->maxLevel-info->minLevel)+info->minLevel) : (info->minLevel));
			int cid = CGI->townh->towns[obj->subID].basicCreatures[level];
			for(int i=0;i<CGI->objh->cregens.size();i++)
				if(CGI->objh->cregens[i]==cid)
					return std::pair<int,int>(17,i); 
			std::cout << "Cannot find a dwelling for creature "<<cid <<std::endl;
			return std::pair<int,int>(17,0); 
		}
	}
	return std::pair<int,int>(-1,-1);
}
void CMapHandler::randomizeObject(CGObjectInstance *cur)
{		
	std::pair<int,int> ran = pickObject(cur);
	if(ran.first<0 || ran.second<0) //this is not a random object, or we couldn't find anything
		return;
	else if(ran.first==34)//special code for hero
	{
		CGHeroInstance *h = dynamic_cast<CGHeroInstance *>(cur);
		if(!h) {std::cout<<"Wrong random hero at "<<cur->pos<<std::endl; return;}
		cur->ID = ran.first;
		cur->subID = ran.second;
		h->type = CGI->heroh->heroes[ran.second];
		CGI->heroh->heroInstances.push_back(h);
		CGI->objh->objInstances.erase(std::find(CGI->objh->objInstances.begin(),CGI->objh->objInstances.end(),h));
		return; //TODO: maybe we should do something with definfo?
	}
	else if(ran.first==98)//special code for town
	{
		CGTownInstance *t = dynamic_cast<CGTownInstance*>(cur);
		if(!t) {std::cout<<"Wrong random town at "<<cur->pos<<std::endl; return;}
		cur->ID = ran.first;
		cur->subID = ran.second;
		t->town = &CGI->townh->towns[ran.second];
		if(t->hasCapitol())
			t->defInfo = capitols[t->subID];
		else if(t->hasFort())
			t->defInfo = CGI->dobjinfo->castles[t->subID];
		else
			t->defInfo = villages[t->subID]; 
		if(!t->defInfo->handler)
		{
			t->defInfo->handler = CGI->spriteh->giveDef(t->defInfo->name);
			alphaTransformDef(t->defInfo);
		}
		//CGI->townh->townInstances.push_back(t);
		return;
	}
	//we have to replace normal random object
	cur->ID = ran.first;
	cur->subID = ran.second;
	cur->defInfo = CGI->dobjinfo->gobjs[ran.first][ran.second];
	if(!cur->defInfo){std::cout<<"Missing def declaration for "<<cur->ID<<" "<<cur->subID<<std::endl;return;}
	if(!cur->defInfo->handler) //if we have to load def
	{
		cur->defInfo->handler = CGI->spriteh->giveDef(cur->defInfo->name);
		alphaTransformDef(cur->defInfo);
	}

}
void CMapHandler::randomizeObjects()
{
	CGObjectInstance * cur;
	for(int no=0; no<CGI->objh->objInstances.size(); ++no)
	{
		randomizeObject(CGI->objh->objInstances[no]);
		if(CGI->objh->objInstances[no]->ID==26)
			CGI->objh->objInstances[no]->defInfo->handler=NULL;
	}
}
void CMapHandler::prepareFOWDefs()
{
	fullHide = CGameInfo::mainObj->spriteh->giveDef("TSHRC.DEF");
	partialHide = CGameInfo::mainObj->spriteh->giveDef("TSHRE.DEF");

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
	//visibility.resize(reader->map.width+2*Woff);
	//for(int gg=0; gg<reader->map.width+2*Woff; ++gg)
	//{
	//	visibility[gg].resize(reader->map.height+2*Hoff);
	//	for(int jj=0; jj<reader->map.height+2*Hoff; ++jj)
	//		visibility[gg][jj] = true;
	//}

	visibility.resize(CGI->ac->map.width, Woff);
	for (int i=0-Woff;i<visibility.size()-Woff;i++)
	{
		visibility[i].resize(CGI->ac->map.height,Hoff);
	}
	for (int i=0-Woff; i<visibility.size()-Woff; ++i)
	{
		for (int j=0-Hoff; j<CGI->ac->map.height+Hoff; ++j)
		{
			visibility[i][j].resize(CGI->ac->map.twoLevel+1,0);
			for(int k=0; k<CGI->ac->map.twoLevel+1; ++k)
				visibility[i][j][k]=true;
		}
	}

	hideBitmap.resize(CGI->ac->map.width, Woff);
	for (int i=0-Woff;i<visibility.size()-Woff;i++)
	{
		hideBitmap[i].resize(CGI->ac->map.height,Hoff);
	}
	for (int i=0-Woff; i<hideBitmap.size()-Woff; ++i)
	{
		for (int j=0-Hoff; j<CGI->ac->map.height+Hoff; ++j)
		{
			hideBitmap[i][j].resize(CGI->ac->map.twoLevel+1,0);
			for(int k=0; k<CGI->ac->map.twoLevel+1; ++k)
				hideBitmap[i][j][k] = rand()%fullHide->ourImages.size();
		}
	}

	//visibility[6][7][1] = false;
	//visibility[7][7][1] = false;
	//visibility[6][8][1] = false;
	//visibility[6][6][1] = false;
	//visibility[5][8][1] = false;
	//visibility[7][6][1] = false;
	//visibility[6][9][1] = false;
}

void CMapHandler::roadsRiverTerrainInit()
{
	//initializing road's and river's DefHandlers

	roadDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("dirtrd.def"));
	roadDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("gravrd.def"));
	roadDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("cobbrd.def"));
	staticRiverDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("clrrvr.def"));
	staticRiverDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("icyrvr.def"));
	staticRiverDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("mudrvr.def"));
	staticRiverDefs.push_back(CGameInfo::mainObj->spriteh->giveDef("lavrvr.def"));
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
					}
				}
			}
		}
	}
}
void CMapHandler::borderAndTerrainBitmapInit()
{
	//terrainBitmap = new SDL_Surface **[reader->map.width+2*Woff];
	//for (int ii=0;ii<reader->map.width+2*Woff;ii++)
	//	terrainBitmap[ii] = new SDL_Surface*[reader->map.height+2*Hoff]; // allocate memory

	CDefHandler * bord = CGameInfo::mainObj->spriteh->giveDef("EDG.DEF");
	bord->notFreeImgs =  true;
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
	delete bord;
}
void CMapHandler::initObjectRects()
{
	//initializing objects / rects
	for(int f=0; f<CGI->objh->objInstances.size(); ++f)
	{
		/*CGI->objh->objInstances[f]->pos.x+=1;
		CGI->objh->objInstances[f]->pos.y+=1;*/
		if(!CGI->objh->objInstances[f]->defInfo)
		{
			continue;
		}
		CDefHandler * curd = CGI->objh->objInstances[f]->defInfo->handler;
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
					std::pair<CGObjectInstance*,SDL_Rect> toAdd = std::make_pair(CGI->objh->objInstances[f],cr);
					
					if((CGI->objh->objInstances[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)>=0 && (CGI->objh->objInstances[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32+1)<ttiles.size()-Woff && (CGI->objh->objInstances[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)>=0 && (CGI->objh->objInstances[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32+1)<ttiles[0].size()-Hoff)
					{
						//TerrainTile2 & curt =
						//	ttiles
						//	[CGI->objh->objInstances[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32]
						//[CGI->objh->objInstances[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32]
						//[CGI->objh->objInstances[f]->pos.z];
						ttiles[CGI->objh->objInstances[f]->pos.x + fx - curd->ourImages[0].bitmap->w/32+1][CGI->objh->objInstances[f]->pos.y + fy - curd->ourImages[0].bitmap->h/32+1][CGI->objh->objInstances[f]->pos.z].objects.push_back(toAdd);
					}
				} // for(int fy=0; fy<curd->ourImages[0].bitmap->h/32; ++fy)
			} //for(int fx=0; fx<curd->ourImages[0].bitmap->w/32; ++fx)
		}//if curd
	} // for(int f=0; f<CGI->objh->objInstances.size(); ++f)
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
	for(int f=0; f<CGI->objh->objInstances.size(); ++f) //calculationg blocked / visitable positions
	{
		if(!CGI->objh->objInstances[f]->defInfo)
			continue;
		CDefHandler * curd = CGI->objh->objInstances[f]->defInfo->handler;
		for(int fx=0; fx<8; ++fx)
		{
			for(int fy=0; fy<6; ++fy)
			{
				int xVal = CGI->objh->objInstances[f]->pos.x + fx - 7;
				int yVal = CGI->objh->objInstances[f]->pos.y + fy - 5;
				int zVal = CGI->objh->objInstances[f]->pos.z;
				if(xVal>=0 && xVal<ttiles.size()-Woff && yVal>=0 && yVal<ttiles[0].size()-Hoff)
				{
					TerrainTile2 & curt = ttiles[xVal][yVal][zVal];
					if(((CGI->objh->objInstances[f]->defInfo->visitMap[fy] >> (7 - fx)) & 1))
						curt.visitable = true;
					if(!((CGI->objh->objInstances[f]->defInfo->blockMap[fy] >> (7 - fx)) & 1))
						curt.blocked = true;
				}
			}
		}
	}
}
void CMapHandler::init()
{
	//loading castles' defs
	std::ifstream ifs("config/townsDefs.txt");
	int ccc;
	ifs>>ccc;
	for(int i=0;i<ccc*2;i++)
	{
		CGDefInfo * n = new CGDefInfo(*CGI->dobjinfo->castles[i%ccc]);
		ifs >> n->name;
		if (!(n->handler = CGI->spriteh->giveDef(n->name)))
			std::cout << "Cannot open "<<n->name<<std::endl;
		if(i<ccc)
			villages[i]=n;
		else
			capitols[i%ccc]=n;
		alphaTransformDef(n);
	} 

	for(int i=0;i<CGI->scenarioOps.playerInfos.size();i++)
	{
		if(CGI->scenarioOps.playerInfos[i].castle==-1)
		{
			int f;
			do
			{
				f = rand()%F_NUMBER;
			}while(!(reader->map.players[CGI->scenarioOps.playerInfos[i].color].allowedFactions  & 1<<f));
			CGI->scenarioOps.playerInfos[i].castle = f;
		}
	}
	for(int i=0;i<PLAYER_LIMIT;i++)
	{
		for(int j=0; j<reader->map.players[i].heroesNames.size();j++)
		{
			usedHeroes.insert(reader->map.players[i].heroesNames[j].heroID);
		}
	}


	timeHandler th;
	th.getDif();
	randomizeObjects();//randomizing objects on map
	std::cout<<"\tRandomizing objects: "<<th.getDif()<<std::endl;

	for(int h=0; h<reader->map.defy.size(); ++h) //initializing loaded def handler's info
	{
		//std::string hlp = reader->map.defy[h]->name;
		//std::transform(hlp.begin(), hlp.end(), hlp.begin(), (int(*)(int))toupper);
		CGI->mh->loadedDefs.insert(std::make_pair(reader->map.defy[h]->name, reader->map.defy[h]->handler));
	}
	std::cout<<"\tCollecting loaded def's handlers: "<<th.getDif()<<std::endl;

	prepareFOWDefs();
	roadsRiverTerrainInit();	//road's and river's DefHandlers; and simple values initialization
	borderAndTerrainBitmapInit();
	std::cout<<"\tPreparing FoW, roads, rivers,borders: "<<th.getDif()<<std::endl;

	//giving starting hero
	for(int i=0;i<PLAYER_LIMIT;i++)
	{
		if((reader->map.players[i].generateHeroAtMainTown && reader->map.players[i].hasMainTown) ||  (reader->map.players[i].hasMainTown && reader->map.version==RoE))
		{
			int3 hpos = reader->map.players[i].posOfMainTown;
			hpos.x+=1;// hpos.y+=1;
			int j;
			for(j=0;j<CGI->scenarioOps.playerInfos.size();j++)
				if(CGI->scenarioOps.playerInfos[j].color==i)
					break;
			if(j==CGI->scenarioOps.playerInfos.size())
				continue;
			int h; //= CGI->scenarioOps.playerInfos[j].hero;
			//if(h<0)
				h=pickHero(i);
			CGHeroInstance * nnn = (CGHeroInstance*)createObject(34,h,hpos,i);
			nnn->defInfo->handler = CGI->heroh->flags1[0];
			CGI->heroh->heroInstances.push_back(nnn);
			CGI->objh->objInstances.push_back(nnn);
		}
	}
	std::cout<<"\tGiving starting heroes: "<<th.getDif()<<std::endl;

	initObjectRects();
	std::cout<<"\tMaking object rects: "<<th.getDif()<<std::endl;
	calculateBlockedPos();
	std::cout<<"\tCalculating blockmap: "<<th.getDif()<<std::endl;
}

SDL_Surface * CMapHandler::terrainRect(int x, int y, int dx, int dy, int level, unsigned char anim, PseudoV< PseudoV< PseudoV<unsigned char> > > & visibilityMap, bool otherHeroAnim, unsigned char heroAnim, SDL_Surface * extSurf, SDL_Rect * extRect)
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
					CSDL_Ext::blit8bppAlphaTo24bpp(CGI->heroh->flags4[themp->getOwner()]->ourImages[gg+heroAnim%imgVal+35].bitmap, &pp, su, &sr);
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
						CSDL_Ext::blit8bppAlphaTo24bpp(CGI->heroh->flags4[themp->getOwner()]->ourImages[ getHeroFrameNum(themp->moveDir, !themp->isStanding) *8+heroAnim%imgVal].bitmap, NULL, su, &bufr);
						themp->flagPrinted = true;
					}
				}
				else
				{
					int imgVal = ttiles[x+bx][y+by][level].objects[h].first->defInfo->handler->ourImages.size();

					//setting appropriate flag color
					if((ttiles[x+bx][y+by][level].objects[h].first->tempOwner>=0 && ttiles[x+bx][y+by][level].objects[h].first->tempOwner<8) || ttiles[x+bx][y+by][level].objects[h].first->tempOwner==255)
						CSDL_Ext::setPlayerColor(ttiles[x+bx][y+by][level].objects[h].first->defInfo->handler->ourImages[anim%imgVal].bitmap, ttiles[x+bx][y+by][level].objects[h].first->tempOwner);
					
					CSDL_Ext::blit8bppAlphaTo24bpp(ttiles[x+bx][y+by][level].objects[h].first->defInfo->handler->ourImages[anim%imgVal].bitmap,&pp,su,&sr);
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
			
			if(bx+x>=0 && by+y>=0 && bx+x<CGI->mh->reader->map.width && by+y<CGI->mh->reader->map.height && !visibilityMap[bx+x][by+y][level])
			{
				SDL_Surface * hide = getVisBitmap(bx+x, by+y, visibilityMap, level);
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
			if(bx+x<0 || by+y<0 || bx+x>reader->map.width+(-1) || by+y>reader->map.height+(-1))
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

SDL_Surface * CMapHandler::getVisBitmap(int x, int y, PseudoV< PseudoV< PseudoV<unsigned char> > > & visibilityMap, int lvl)
{
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		return fullHide->ourImages[hideBitmap[x][y][lvl]].bitmap; //fully hidden
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[22].bitmap; //visible right bottom corner
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[15].bitmap; //visible right top corner
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[22].bitmap); //visible left bottom corner
		return partialHide->ourImages[34].bitmap; //visible left bottom corner
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[15].bitmap); //visible left top corner
		return partialHide->ourImages[35].bitmap;
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		//return partialHide->ourImages[rand()%2].bitmap; //visible top
		return partialHide->ourImages[0].bitmap; //visible top
	}
	else if(visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		//return partialHide->ourImages[4+rand()%2].bitmap; //visble bottom
		return partialHide->ourImages[4].bitmap; //visble bottom
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[2+rand()%2].bitmap); //visible left
		//return CSDL_Ext::rotate01(partialHide->ourImages[2].bitmap); //visible left
		return partialHide->ourImages[36].bitmap;
	}
	else if(!visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		//return partialHide->ourImages[2+rand()%2].bitmap; //visible right
		return partialHide->ourImages[2].bitmap; //visible right
	}
	else if(visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl])
	{
		//return partialHide->ourImages[12+2*(rand()%2)].bitmap; //visible bottom, right - bottom, right; left top corner hidden
		return partialHide->ourImages[12].bitmap; //visible bottom, right - bottom, right; left top corner hidden
	}
	else if(!visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[13].bitmap; //visible right, right - top; left bottom corner hidden
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && !visibilityMap[x+1][y+1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[13].bitmap); //visible top, top - left, left; right bottom corner hidden
		return partialHide->ourImages[37].bitmap;
	}
	else if(visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x+1][y-1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[12+2*(rand()%2)].bitmap); //visible left, left - bottom, bottom; right top corner hidden
		//return CSDL_Ext::rotate01(partialHide->ourImages[12].bitmap); //visible left, left - bottom, bottom; right top corner hidden
		return partialHide->ourImages[38].bitmap;
	}
	else if(visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[10].bitmap; //visible left, right, bottom and top
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[16].bitmap; //visible right corners
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[18].bitmap; //visible top corners
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[16].bitmap); //visible left corners
		return partialHide->ourImages[39].bitmap;
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		//return CSDL_Ext::hFlip(partialHide->ourImages[18].bitmap); //visible bottom corners
		return partialHide->ourImages[40].bitmap;
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[17].bitmap; //visible right - top and bottom - left corners
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		//return CSDL_Ext::hFlip(partialHide->ourImages[17].bitmap); //visible top - left and bottom - right corners
		return partialHide->ourImages[41].bitmap;
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[19].bitmap; //visible corners without left top
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[20].bitmap; //visible corners without left bottom
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[20].bitmap); //visible corners without right bottom
		return partialHide->ourImages[42].bitmap;
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[19].bitmap); //visible corners without right top
		return partialHide->ourImages[43].bitmap;
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[21].bitmap; //visible all corners only
	}
	if(visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl])
	{
		return partialHide->ourImages[6].bitmap; //hidden top
	}
	if(visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl])
	{
		return partialHide->ourImages[7].bitmap; //hidden right
	}
	if(!visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl])
	{
		return partialHide->ourImages[8].bitmap; //hidden bottom
	}
	if(visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[7].bitmap); //hidden left
		return partialHide->ourImages[44].bitmap;
	}
	if(!visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl])
	{
		return partialHide->ourImages[9].bitmap; //hidden top and bottom
	}
	if(visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl])
	{
		return partialHide->ourImages[29].bitmap;  //hidden left and right
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[24].bitmap; //visible top and right bottom corner
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[24].bitmap); //visible top and left bottom corner
		return partialHide->ourImages[45].bitmap;
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[33].bitmap; //visible top and bottom corners
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[26].bitmap); //visible left and right top corner
		return partialHide->ourImages[46].bitmap;
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[25].bitmap); //visible left and right bottom corner
		return partialHide->ourImages[47].bitmap;
	}
	if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl])
	{
		return partialHide->ourImages[32].bitmap; //visible left and right corners
	}
	if(visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y-1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[30].bitmap); //visible bottom and left top corner
		return partialHide->ourImages[48].bitmap;
	}
	if(visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y-1][lvl])
	{
		return partialHide->ourImages[30].bitmap; //visible bottom and right top corner
	}
	if(visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y-1][lvl])
	{
		return partialHide->ourImages[31].bitmap; //visible bottom and top corners
	}
	if(!visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[25].bitmap; //visible right and left bottom corner
	}
	if(!visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[26].bitmap; //visible right and left top corner
	}
	if(!visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[32].bitmap); //visible right and left cornres
		return partialHide->ourImages[49].bitmap;
	}
	if(visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl])
	{
		return partialHide->ourImages[28].bitmap; //visible bottom, right - bottom, right; left top corner visible
	}
	else if(!visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y+1][lvl])
	{
		return partialHide->ourImages[27].bitmap; //visible right, right - top; left bottom corner visible
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && visibilityMap[x+1][y+1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[27].bitmap); //visible top, top - left, left; right bottom corner visible
		return partialHide->ourImages[50].bitmap;
	}
	else if(visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x+1][y-1][lvl])
	{
		//return CSDL_Ext::rotate01(partialHide->ourImages[28].bitmap); //visible left, left - bottom, bottom; right top corner visible
		return partialHide->ourImages[51].bitmap;
	}
	//newly added
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl]) //visible t and tr
	{
		return partialHide->ourImages[0].bitmap;
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl]) //visible t and tl
	{
		return partialHide->ourImages[1].bitmap;
	}
	else if(visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl]) //visible b and br
	{
		return partialHide->ourImages[4].bitmap;
	}
	else if(visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl]) //visible b and bl
	{
		return partialHide->ourImages[5].bitmap;
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl]) //visible l and tl
	{
		return partialHide->ourImages[36].bitmap;
	}
	else if(!visibilityMap[x][y+1][lvl] && !visibilityMap[x+1][y][lvl] && visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && visibilityMap[x-1][y+1][lvl]) //visible l and bl
	{
		return partialHide->ourImages[36].bitmap;
	}
	else if(!visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && !visibilityMap[x+1][y+1][lvl] && visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl]) //visible r and tr
	{
		return partialHide->ourImages[2].bitmap;
	}
	else if(!visibilityMap[x][y+1][lvl] && visibilityMap[x+1][y][lvl] && !visibilityMap[x-1][y][lvl] && !visibilityMap[x][y-1][lvl] && !visibilityMap[x-1][y-1][lvl] && visibilityMap[x+1][y+1][lvl] && !visibilityMap[x+1][y-1][lvl] && !visibilityMap[x-1][y+1][lvl]) //visible r and br
	{
		return partialHide->ourImages[3].bitmap;
	}
	return fullHide->ourImages[0].bitmap; //this case should never happen, but it is better to hide too much than reveal it....
}

int CMapHandler::getCost(int3 &a, int3 &b, const CGHeroInstance *hero)
{
	int ret=-1;
	if(a.x>=CGI->mh->reader->map.width && a.y>=CGI->mh->reader->map.height)
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[CGI->mh->reader->map.width-1][CGI->mh->reader->map.width-1][a.z].malle];
	else if(a.x>=CGI->mh->reader->map.width && a.y<CGI->mh->reader->map.height)
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[CGI->mh->reader->map.width-1][a.y][a.z].malle];
	else if(a.x<CGI->mh->reader->map.width && a.y>=CGI->mh->reader->map.height)
		ret = hero->type->heroClass->terrCosts[CGI->mh->ttiles[a.x][CGI->mh->reader->map.width-1][a.z].malle];
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
			nobj->artifWorn.resize(20);
			nobj->artifacts.resize(20);
			nobj->artifWorn[16] = &CGI->arth->artifacts[3];
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
		nobj->defInfo->handler = CGI->spriteh->giveDef(nobj->defInfo->name);
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
	std::vector<CGObjectInstance *>::iterator db = std::find(CGI->objh->objInstances.begin(), CGI->objh->objInstances.end(), obj);
	recalculateHideVisPosUnderObj(*db);
	delete *db;
	CGI->objh->objInstances.erase(db);
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

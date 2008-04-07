#include "stdafx.h"
#include "CCastleInterface.h"
#include "hch/CObjectHandler.h"
#include "CGameInfo.h"
#include "hch/CLodHandler.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "hch/CTownHandler.h"
#include "AdventureMapButton.h"
#include "hch/CBuildingHandler.h"
#include <sstream>
#include "CMessage.h"
#include "hch/CGeneralTextHandler.h"
#include "CCallback.h"
extern TTF_Font * GEOR16;
CBuildingRect::CBuildingRect(Structure *Str)
:str(Str), moi(false)
{	
	def = CGI->spriteh->giveDef(Str->defName);
	pos.x = str->pos.x;
	pos.y = str->pos.y;
	pos.w = def->ourImages[0].bitmap->w;
	pos.h = def->ourImages[0].bitmap->h;
	if(Str->ID<0  || (Str->ID>=27 && Str->ID<=29))
	{
		area = border = NULL;
		return;
	}
	if (border = CGI->bitmaph->loadBitmap(str->borderName))
		SDL_SetColorKey(border,SDL_SRCCOLORKEY,SDL_MapRGB(border->format,0,255,255));
	else
		std::cout << "Warning: no border for "<<Str->ID<<std::endl;
	if (area = CGI->bitmaph->loadBitmap(str->areaName))
		;//SDL_SetColorKey(area,SDL_SRCCOLORKEY,SDL_MapRGB(area->format,0,255,255));
	else
		std::cout << "Warning: no area for "<<Str->ID<<std::endl;
}

CBuildingRect::~CBuildingRect()
{
	delete def;
	if(border)
		SDL_FreeSurface(border);
	if(area)
		SDL_FreeSurface(area);
}
void CBuildingRect::activate()
{
	Hoverable::activate();
	ClickableL::activate();
	ClickableR::activate();

}
void CBuildingRect::deactivate()
{
	Hoverable::deactivate();
	ClickableL::deactivate();
	ClickableR::deactivate();
	if(moi)
		MotionInterested::deactivate();
	moi=false;
}
bool CBuildingRect::operator<(const CBuildingRect & p2) const
{
	if(str->pos.z != p2.str->pos.z)
		return (str->pos.z) < (p2.str->pos.z);
	else
		return (str->ID) < (p2.str->ID);
}
void CBuildingRect::hover(bool on)
{
	Hoverable::hover(on);
	if(on)
	{
		if(!moi)
			MotionInterested::activate();
		moi = true;
	}
	else 
	{
		if(moi)
			MotionInterested::deactivate();
		moi = false;
		if(LOCPLINT->castleInt->hBuild == this)
		{
			LOCPLINT->castleInt->hBuild = NULL;
			LOCPLINT->statusbar->clear();
		}
	}
}
void CBuildingRect::clickLeft (tribool down)
{
	
	if(area && (LOCPLINT->castleInt->hBuild==this) && !(indeterminate(down)) && (CSDL_Ext::SDL_GetPixel(area,LOCPLINT->current->motion.x-pos.x,LOCPLINT->current->motion.y-pos.y) != 0)) //na polu
	{
		if(pressedL && !down)
			LOCPLINT->castleInt->buildingClicked(str->ID);
		ClickableL::clickLeft(down);
	}

	
	//todo - handle
}
void CBuildingRect::clickRight (tribool down)
{
	if((!area) || (!((bool)down)) || (this!=LOCPLINT->castleInt->hBuild))
		return;
	if((CSDL_Ext::SDL_GetPixel(area,LOCPLINT->current->motion.x-pos.x,LOCPLINT->current->motion.y-pos.y) != 0)) //na polu
	{
		CInfoPopup *vinya = new CInfoPopup();
		vinya->free = true;
		vinya->bitmap = CMessage::drawBoxTextBitmapSub
			(LOCPLINT->playerID,
			CGI->buildh->buildings[str->townID][str->ID]->description, 
			LOCPLINT->castleInt->bicons->ourImages[str->ID].bitmap, 
			CGI->buildh->buildings[str->townID][str->ID]->name);
		vinya->pos.x = ekran->w/2 - vinya->bitmap->w/2;
		vinya->pos.y = ekran->h/2 - vinya->bitmap->h/2;
		vinya->activate();
	}
}

void CBuildingRect::mouseMoved (SDL_MouseMotionEvent & sEvent)
{
	if(area)
	{
		if(CSDL_Ext::SDL_GetPixel(area,sEvent.x-pos.x,sEvent.y-pos.y) == 0) //hovered pixel is inside this building
		{
			if(LOCPLINT->castleInt->hBuild == this)
			{
				LOCPLINT->castleInt->hBuild = NULL;
				LOCPLINT->statusbar->clear();
			}
		}
		else //inside the area of this building
		{
			if(LOCPLINT->castleInt->hBuild) //a building is hovered
			{
				if((*LOCPLINT->castleInt->hBuild)<(*this)) //set if we are on top
				{
					LOCPLINT->castleInt->hBuild = this;
					if(CGI->buildh->buildings[str->townID][str->ID] && CGI->buildh->buildings[str->townID][str->ID]->name.length())
						LOCPLINT->statusbar->print(CGI->buildh->buildings[str->townID][str->ID]->name);
					else
						LOCPLINT->statusbar->print(str->name);
				}
			}
			else //no building hovered
			{
				LOCPLINT->castleInt->hBuild = this;
				if(CGI->buildh->buildings[str->townID][str->ID] && CGI->buildh->buildings[str->townID][str->ID]->name.length())
					LOCPLINT->statusbar->print(CGI->buildh->buildings[str->townID][str->ID]->name);
				else
					LOCPLINT->statusbar->print(str->name);
			}
		}
	}
	//if(border)
	//	blitAt(border,pos.x,pos.y);
}

std::string getBgName(int type) //TODO - co z tym zrobiæ?
{
	switch (type)
	{
	case 0:
		return "TBCSBACK.bmp";
	case 1:
		return "TBRMBACK.bmp";
	case 2:
		return "TBTWBACK.bmp";
	case 3:
		return "TBINBACK.bmp";
	case 4:
		return "TBNCBACK.bmp";
	case 5:
		return "TBDNBACK.bmp";
	case 6:
		return "TBSTBACK.bmp";
	case 7:
		return "TBFRBACK.bmp";
	case 8:
		return "TBELBACK.bmp";
	default:
		throw new std::exception("std::string getBgName(int type): invalid type");
	}
}
class SORTHELP
{
public:
	bool operator ()
		(const CBuildingRect *a ,
		 const CBuildingRect *b)
	{
		return (*a)<(*b);
	}
} srthlp ;

CCastleInterface::CCastleInterface(const CGTownInstance * Town, bool Activate)
{	
	hall = NULL;
	townInt = CGI->bitmaph->loadBitmap("TOWNSCRN.bmp");
	cityBg = CGI->bitmaph->loadBitmap(getBgName(Town->subID));
	hall = CGI->spriteh->giveDef("ITMTL.DEF");
	fort = CGI->spriteh->giveDef("ITMCL.DEF");
	flag =  CGI->spriteh->giveDef("CREST58.DEF");
	townlist = new CTownList<CCastleInterface>(3,&genRect(128,48,744,414),744,414,744,526);
	exit = new AdventureMapButton<CCastleInterface>
		(CGI->townh->tcommands[8],"",&CCastleInterface::close,744,544,"TSBTNS.DEF",this,false,NULL,false);
	split = new AdventureMapButton<CCastleInterface>
		(CGI->townh->tcommands[3],"",&CCastleInterface::splitF,744,382,"TSBTNS.DEF",this,false,NULL,false);
	statusbar = new CStatusBar(8,555,"TSTATBAR.bmp",732);

	townlist->owner = this;
	townlist->fun = &CCastleInterface::townChange;
	townlist->genList();
	townlist->selected = getIndexOf(townlist->items,Town);
	if((townlist->selected+1) > townlist->SIZE)
		townlist->from = townlist->selected -  townlist->SIZE + 1;
	hBuild = NULL;
	count=0;
	town = Town;

	CSDL_Ext::blueToPlayersAdv(townInt,LOCPLINT->playerID);
	exit->bitmapOffset = 4;


	//buildings
	recreateBuildings();


	//garrison
	garr = new CGarrisonInt(305,387,4,32,townInt,243,13,town,town->visitingHero);

	if(Activate)
	{
		LOCPLINT->objsToBlit.push_back(this);
		activate();
		showAll();
	}

	std::string defname;
	switch (town->subID)
	{
	case 0:
		defname = "HALLCSTL.DEF";
		break;
	case 1:
		defname = "HALLRAMP.DEF";
		break;
	case 2:
		defname = "HALLTOWR.DEF";
		break;
	case 3:
		defname = "HALLINFR.DEF";
		break;
	case 4:
		defname = "HALLNECR.DEF";
		break;
	case 5:
		defname = "HALLDUNG.DEF";
		break;
	case 6:
		defname = "HALLSTRN.DEF";
		break;
	case 7:
		defname = "HALLFORT.DEF";
		break;
	case 8:
		defname = "HALLELEM.DEF";
		break;
	default:
		throw new std::exception("Bad town subID");
	}
	bicons = CGI->spriteh->giveDefEss(defname);
	//blit buildings on bg
	//for(int i=0;i<buildings.size();i++)
	//{
	//	blitAt(buildings[i]->def->ourImages[0].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,cityBg);
	//}
}
CCastleInterface::~CCastleInterface()
{
	SDL_FreeSurface(townInt);
	SDL_FreeSurface(cityBg);
	delete exit;
	delete split;
	delete hall;
	delete fort;
	delete flag;
	delete garr;
	delete townlist;
	delete statusbar;
	for(int i=0;i<buildings.size();i++)
	{
		delete buildings[i];
	}
	delete bicons;

}
void CCastleInterface::close()
{
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
	deactivate();
	LOCPLINT->castleInt = NULL;
	LOCPLINT->adventureInt->activate();
	delete this;
}
void CCastleInterface::splitF()
{
}
void CCastleInterface::buildingClicked(int building)
{
	std::cout<<"You've clicked on "<<building<<std::endl;
	if(building >= 30)
	{
		if(building>36)
			building-=7;
		std::vector<std::pair<int,int > > crs;
		int amount = (const_cast<CGTownInstance*>(town))->strInfo.creatures[building-30]; //trzeba odconstowac, bo inaczej operator [] by sypal :(
		crs.push_back(std::make_pair(town->town->basicCreatures[building-30],amount));

		if(town->builtBuildings.find(building+7) != town->builtBuildings.end()) //check if there is an upgraded building
			crs.push_back(std::make_pair(town->town->upgradedCreatures[building-30],amount));
		CRecrutationWindow *rw = new CRecrutationWindow(crs);
		rw->activate();
	}
	switch(building)
	{
	case 10: case 11: case 12: case 13:
		enterHall();
		break;
	default:
		std::cout<<"This building isn't handled...\n";
	}
}
void CCastleInterface::enterHall()
{
	deactivate();
	hallInt = new CHallInterface(this);
	hallInt->activate();
	hallInt->show();
}
void CCastleInterface::showAll(SDL_Surface * to)
{	
	if (!to)
		to=ekran;
	blitAt(cityBg,0,0,to);
	blitAt(townInt,0,374,to);
	LOCPLINT->adventureInt->resdatabar.draw();
	townlist->draw();
	statusbar->show();

	garr->show(); 
	int pom;

	//draw fort icon
	if(town->builtBuildings.find(9)!=town->builtBuildings.end())
		pom = 2;
	else if(town->builtBuildings.find(8)!=town->builtBuildings.end())
		pom = 1;
	else if(town->builtBuildings.find(7)!=town->builtBuildings.end())
		pom = 0;
	else pom = 3;
	blitAt(fort->ourImages[pom].bitmap,122,413,to);

	//draw ((village/town/city) hall)/capitol icon
	if(town->builtBuildings.find(13)!=town->builtBuildings.end())
		pom = 3;
	else if(town->builtBuildings.find(12)!=town->builtBuildings.end())
		pom = 2;
	else if(town->builtBuildings.find(11)!=town->builtBuildings.end())
		pom = 1;
	else pom = 0;
	blitAt(hall->ourImages[pom].bitmap,80,413,to);

	//draw creatures icons and their growths
	for(int i=0;i<CREATURES_PER_TOWN;i++)
	{
		int cid = -1;
		if (town->builtBuildings.find(30+i)!=town->builtBuildings.end())
		{
			if (town->builtBuildings.find(30+CREATURES_PER_TOWN+i)!=town->builtBuildings.end())
				cid = town->town->upgradedCreatures[i];
			else
				cid = town->town->basicCreatures[i];
		}
		if (cid>=0)
		{
			int pomx, pomy;
			pomx = 22 + (55*((i>3)?(i-4):i));
			pomy = (i>3)?(507):(459);
			blitAt(CGI->creh->smallImgs[cid],pomx,pomy,to);
			std::ostringstream oss;
			oss << '+' << (CGI->creh->creatures[cid].growth + town->creatureIncome[i]);
			CSDL_Ext::printAtMiddle(oss.str(),pomx+16,pomy+37,GEOR13,zwykly,to);
		}
	}

	//print name and income
	CSDL_Ext::printAt(town->name,85,389,GEOR13,zwykly,to);
	char temp[10];
	itoa(town->dailyIncome(),temp,10);
	CSDL_Ext::printAtMiddle(temp,195,442,GEOR13,zwykly,to);

	//blit town icon
	pom = town->subID*2;
	if (!town->hasFort())
		pom += F_NUMBER*2;
	if(town->builded >= MAX_BUILDING_PER_TURN)
		pom++;
	blitAt(LOCPLINT->bigTownPic->ourImages[pom].bitmap,15,387,to);

	//flag
	if(town->getOwner()<PLAYER_LIMIT)
		blitAt(flag->ourImages[town->getOwner()].bitmap,241,387,to);

	show();
}
void CCastleInterface::townChange()
{
	const CGTownInstance * nt = townlist->items[townlist->selected];
	deactivate();
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
	delete this;
	LOCPLINT->castleInt = new CCastleInterface(nt,true);
}
void CCastleInterface::show(SDL_Surface * to)
{
	if(!showing)
		return;
	if (!to)
		to=ekran;
	count++;
	if(count==4)
	{
		count=0;
		animval++;
	}

	blitAt(cityBg,0,0,to);


	//blit buildings
	for(int i=0;i<buildings.size();i++)
	{
		if((animval)%(buildings[i]->def->ourImages.size()))
		{
			blitAt(buildings[i]->def->ourImages[0].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,to);
			blitAt(buildings[i]->def->ourImages[(animval)%(buildings[i]->def->ourImages.size())].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,to);
		}
		else 
			blitAt(buildings[i]->def->ourImages[(animval)%(buildings[i]->def->ourImages.size())].bitmap,buildings[i]->pos.x,buildings[i]->pos.y,to);
		//if(buildings[i]->hovered && buildings[i]->border)
		//	blitAt(buildings[i]->border,buildings[i]->pos.x,buildings[i]->pos.y);
		if(hBuild==buildings[i] && hBuild->border)
			blitAt(hBuild->border,hBuild->pos,to);
	}
	
}
void CCastleInterface::activate()
{
	showing = true;
	townlist->activate();
	garr->activate();
	LOCPLINT->curint = this;
	LOCPLINT->statusbar = statusbar;
	exit->activate();
	split->activate();
	for(int i=0;i<buildings.size();i++)
		buildings[i]->activate();
}
void CCastleInterface::deactivate()
{
	showing = false;
	townlist->deactivate();
	garr->deactivate();
	exit->deactivate();
	split->deactivate();
	for(int i=0;i<buildings.size();i++)
		buildings[i]->deactivate();
}

void CCastleInterface::addBuilding(int bid)
{
	//TODO: lepiej by bylo tylko dodawac/usuwac co trzeba pamietajac o grupach
	recreateBuildings();
}

void CCastleInterface::removeBuilding(int bid)
{
	//TODO: lepiej by bylo tylko dodawac/usuwac co trzeba pamietajac o grupach
	recreateBuildings();
}

void CCastleInterface::recreateBuildings()
{
	for(int i=0;i<buildings.size();i++)
	{
		if(showing)
			buildings[i]->deactivate();
		delete buildings[i];
	}
	buildings.clear();
	hBuild = NULL;

	std::set< std::pair<int,int> > s; //group - id


		for (std::set<int>::const_iterator i=town->builtBuildings.begin();i!=town->builtBuildings.end();i++)
	{
		if(CGI->townh->structures.find(town->subID) != CGI->townh->structures.end()) //we have info about structures in this town
		{
			if(CGI->townh->structures[town->subID].find(*i)!=CGI->townh->structures[town->subID].end()) //we have info about that structure
			{
				Structure * st = CGI->townh->structures[town->subID][*i];
				if(st->group<0) //no group - just add it
				{
					buildings.push_back(new CBuildingRect(st));
				}
				else
				{
					std::set< std::pair<int,int> >::iterator obecny=s.end();
					for(std::set< std::pair<int,int> >::iterator seti = s.begin(); seti!=s.end(); seti++) //check if we have already building from same group
					{
						if(seti->first == st->group)
						{
							obecny = seti; 
							break;
						}
					}
					if(obecny != s.end())
					{
						if((*(CGI->townh->structures[town->subID][obecny->second])) < (*(CGI->townh->structures[town->subID][st->ID]))) //we have to replace old building with current one
						{
							for(int itpb = 0; itpb<buildings.size(); itpb++)
							{
								if(buildings[itpb]->str->ID == obecny->second)
								{
									delete buildings[itpb];
									buildings.erase(buildings.begin() + itpb);
									obecny->second = st->ID;
									buildings.push_back(new CBuildingRect(st));
								}
							}
						}
					}
					else
					{
						buildings.push_back(new CBuildingRect(st));
						s.insert(std::pair<int,int>(st->group,st->ID));
					}
				}
			}
			else continue;
		}
		else
			break;
	}
	std::sort(buildings.begin(),buildings.end(),srthlp);
}

void CHallInterface::CResDataBar::show(SDL_Surface * to)
{
	blitAt(bg,pos.x,pos.y);
	char * buf = new char[15];
	for (int i=0;i<7;i++)
	{
		itoa(LOCPLINT->cb->getResourceAmount(i),buf,10);
		CSDL_Ext::printAtMiddle(buf,pos.x + 50 + 76*i,pos.y+pos.h/2,GEOR13,zwykly);
	}
	std::vector<std::string> temp;
	itoa(LOCPLINT->cb->getDate(3),buf,10); temp.push_back(std::string(buf));
	itoa(LOCPLINT->cb->getDate(2),buf,10); temp.push_back(buf);
	itoa(LOCPLINT->cb->getDate(1),buf,10); temp.push_back(buf);
	CSDL_Ext::printAtMiddle(CSDL_Ext::processStr(
		CGI->generaltexth->allTexts[62]
			+": %s, " 
			+ CGI->generaltexth->allTexts[63] 
			+ ": %s, " 
			+	CGI->generaltexth->allTexts[64] 
			+ ": %s",temp)
		,pos.x+545+(pos.w-545)/2,pos.y+pos.h/2,GEOR13,zwykly);
	temp.clear();
	//updateRect(&pos,ekran);
	delete[] buf;
}
CHallInterface::CResDataBar::CResDataBar()
{
	bg = CGI->bitmaph->loadBitmap("Z2ESBAR.bmp");
	SDL_SetColorKey(bg,SDL_SRCCOLORKEY,SDL_MapRGB(bg->format,0,255,255));
	CSDL_Ext::blueToPlayersAdv(bg,LOCPLINT->playerID);
	pos.x = 7;
	pos.y = 575;
	pos.w = bg->w;
	pos.h = bg->h;
}
CHallInterface::CResDataBar::~CResDataBar()
{
	SDL_FreeSurface(bg);
}

void CHallInterface::CBuildingBox::hover(bool on)
{
	Hoverable::hover(on);
	if(on)
	{
		std::string toPrint = CGI->townh->hcommands[state];
		std::vector<std::string> name;
		name.push_back(CGI->buildh->buildings[LOCPLINT->castleInt->town->subID][BID]->name);
		LOCPLINT->statusbar->print(CSDL_Ext::processStr(toPrint,name));
	}
	else
		LOCPLINT->statusbar->clear();
}
void CHallInterface::CBuildingBox::clickLeft (tribool down)
{
	if(pressedL && (!down))
	{
		LOCPLINT->castleInt->hallInt->deactivate();
		new CHallInterface::CBuildWindow(LOCPLINT->castleInt->town->subID,BID,state,0);
	}
	ClickableL::clickLeft(down);
}
void CHallInterface::CBuildingBox::clickRight (tribool down)
{
	if(down)
	{
		LOCPLINT->castleInt->hallInt->deactivate();
		new CHallInterface::CBuildWindow(LOCPLINT->castleInt->town->subID,BID,state,1);
	}
	ClickableR::clickRight(down);
}
void CHallInterface::CBuildingBox::show(SDL_Surface * to)
{
	blitAt(LOCPLINT->castleInt->bicons->ourImages[BID].bitmap,pos.x,pos.y);
	int pom, pom2=-1;
	switch (state)
	{
	case 4: 
		pom = 0;
		pom2 = 0;
		break;
	case 7:
		pom = 1;
		break;
	case 6:
		pom2 = 2;
		pom = 2;
		break;
	case 0: case 5: case 8:
		pom2 = 1;
		pom = 2;
		break;
	case 2: case 1: default:
		pom = 3;
		break;
	}
	blitAt(LOCPLINT->castleInt->hallInt->bars->ourImages[pom].bitmap,pos.x-1,pos.y+71);
	if(pom2>=0)
		blitAt(LOCPLINT->castleInt->hallInt->status->ourImages[pom2].bitmap,pos.x+135, pos.y+54);
	CSDL_Ext::printAtMiddle(CGI->buildh->buildings[LOCPLINT->castleInt->town->subID][BID]->name,pos.x-1+LOCPLINT->castleInt->hallInt->bars->ourImages[0].bitmap->w/2,pos.y+71+LOCPLINT->castleInt->hallInt->bars->ourImages[0].bitmap->h/2, GEOR13,zwykly);
}
void CHallInterface::CBuildingBox::activate()
{
	Hoverable::activate();
	ClickableL::activate();
	ClickableR::activate();
}
void CHallInterface::CBuildingBox::deactivate()
{
	Hoverable::deactivate();
	ClickableL::deactivate();
	ClickableR::deactivate();
}
CHallInterface::CBuildingBox::~CBuildingBox()
{
}
CHallInterface::CBuildingBox::CBuildingBox(int id)
	:BID(id)
{
	pos.w = 150;
	pos.h = 70;
}
CHallInterface::CBuildingBox::CBuildingBox(int id, int x, int y)
	:BID(id)
{
	pos.x = x;
	pos.y = y;
	pos.w = 150;
	pos.h = 70;
}


CHallInterface::CHallInterface(CCastleInterface * owner)
{
	bg = CGI->bitmaph->loadBitmap(CGI->buildh->hall[owner->town->subID].first);
	CSDL_Ext::blueToPlayersAdv(bg,LOCPLINT->playerID);
	bars = CGI->spriteh->giveDefEss("TPTHBAR.DEF");
	status = CGI->spriteh->giveDefEss("TPTHCHK.DEF");
	exit = new AdventureMapButton<CHallInterface>
		(CGI->townh->tcommands[8],"",&CHallInterface::close,748,556,"TPMAGE1.DEF",this,false,NULL,false);

	//preparing boxes with buildings//
	boxes.resize(5);
	for(int i=0;i<5;i++) //for each row
	{
		for(int j=0; j<CGI->buildh->hall[owner->town->subID].second[i].size();j++) //for each box
		{
			int k=0;
			for(;k<CGI->buildh->hall[owner->town->subID].second[i][j].size();k++)//we are looking for the first not build structure
			{
				if(
					(owner->town->builtBuildings.find(CGI->buildh->hall[owner->town->subID].second[i][j][k]))
					== 
					(owner->town->builtBuildings.end())						)
				{
					int x = 34 + 194*j,
						y = 37 + 104*i,
						ID = CGI->buildh->hall[owner->town->subID].second[i][j][k];
					if(CGI->buildh->hall[owner->town->subID].second[i].size() == 2) //only two boxes in this row
						x+=194;
					else if(CGI->buildh->hall[owner->town->subID].second[i].size() == 3) //only three boxes in this row
						x+=97;
					boxes[i].push_back(new CBuildingBox(CGI->buildh->hall[owner->town->subID].second[i][j][k],x,y));

					boxes[i][boxes[i].size()-1]->state = 7; //allowed by default

					//can we build it?
					if(owner->town->forbiddenBuildings.find(CGI->buildh->hall[owner->town->subID].second[i][j][k])!=owner->town->forbiddenBuildings.end())
						boxes[i][boxes[i].size()-1]->state = 2; //forbidden
					else if(owner->town->builded >= MAX_BUILDING_PER_TURN)
						boxes[i][boxes[i].size()-1]->state = 5; //building limit

					//checking resources
					CBuilding * pom = CGI->buildh->buildings[owner->town->subID][CGI->buildh->hall[owner->town->subID].second[i][j][k]];
					for(int res=0;res<7;res++) //TODO: support custom amount of resources
					{
						if(pom->resources[res]>LOCPLINT->cb->getResourceAmount(res))
							boxes[i][boxes[i].size()-1]->state = 6; //lack of res
					}

					//checking for requirements
					for( std::set<int>::iterator ri  =  CGI->townh->requirements[owner->town->subID][ID].begin();
						 ri != CGI->townh->requirements[owner->town->subID][ID].end();
						 ri++ )
					{
						if(owner->town->builtBuildings.find(*ri)==owner->town->builtBuildings.end())
							boxes[i][boxes[i].size()-1]->state = 8; //lack of requirements - cannot build
					}

					//TODO: check if capital is already built, check if there is water for shipyard
					



					break;
				}
			}
			if(k==CGI->buildh->hall[owner->town->subID].second[i][j].size()) //all buildings built - let's take the last one
			{
				int x = 34 + 194*j,
					y = 37 + 104*i;
				if(CGI->buildh->hall[owner->town->subID].second[i].size() == 2)
					x+=194;
				else if(CGI->buildh->hall[owner->town->subID].second[i].size() == 3)
					x+=97;
				boxes[i].push_back(new CBuildingBox(CGI->buildh->hall[owner->town->subID].second[i][j][k-1],x,y));
				boxes[i][boxes[i].size()-1]->state = 4; //already exists
			}
		}
	}
}
CHallInterface::~CHallInterface()
{
	delete bars;
	delete status;
	SDL_FreeSurface(bg);
	for(int i=0;i<boxes.size();i++)
		for(int j=0;j<boxes[i].size();j++)
			delete boxes[i][j];
	delete exit;
}
void CHallInterface::close()
{
	deactivate();
	delete this;
	LOCPLINT->castleInt->activate();
	LOCPLINT->castleInt->showAll();
}
void CHallInterface::show(SDL_Surface * to)
{
	blitAt(bg,0,0);
	resdatabar.show();
	exit->show();
	for(int i=0; i<5; i++)
	{
		for(int j=0;j<boxes[i].size();j++)
			boxes[i][j]->show();
	}
}
void CHallInterface::activate()
{
	for(int i=0;i<5;i++)
		for(int j=0;j<boxes[i].size();j++)
			boxes[i][j]->activate();
	exit->activate();
}
void CHallInterface::deactivate()
{
	for(int i=0;i<5;i++)
	{
		for(int j=0;j<boxes[i].size();j++)
		{
			boxes[i][j]->deactivate();
		}
	}
	exit->deactivate();
}

void CHallInterface::CBuildWindow::activate()
{
	LOCPLINT->objsToBlit.push_back(this);
	ClickableR::activate();
	if(mode)
		return;
	if(state==7)
		buy->activate();
	cancel->activate();
}
void CHallInterface::CBuildWindow::deactivate()
{
	LOCPLINT->objsToBlit.erase(std::find(LOCPLINT->objsToBlit.begin(),LOCPLINT->objsToBlit.end(),this));
	ClickableR::deactivate();
	if(mode)
		return;
	if(state==7)
		buy->deactivate();
	cancel->deactivate();
}
void CHallInterface::CBuildWindow::Buy()
{
	LOCPLINT->cb->buildBuilding(LOCPLINT->castleInt->town,bid);
	deactivate();
	delete this;
	delete LOCPLINT->castleInt->hallInt;
	LOCPLINT->castleInt->hallInt = NULL;
	LOCPLINT->castleInt->activate();
	LOCPLINT->castleInt->showAll();
}
void CHallInterface::CBuildWindow::close()
{
	deactivate();
	delete this;
	LOCPLINT->castleInt->hallInt->activate();
	LOCPLINT->castleInt->hallInt->show();
}
void CHallInterface::CBuildWindow::clickRight (tribool down)
{
	if((!down || indeterminate(down)) && mode)
		close();
}
void CHallInterface::CBuildWindow::show(SDL_Surface * to)
{
	SDL_Rect pom = genRect(bitmap->h-1,bitmap->w-1,pos.x,pos.y);
	SDL_Rect poms = pom; poms.x=0;poms.y=0;
	SDL_BlitSurface(bitmap,&poms,to?to:ekran,&pom);
	if(!mode)
	{
		buy->show();
		cancel->show();
	}
}
std::string CHallInterface::CBuildWindow::getTextForState(int state)
{
	std::string ret;
	if(state<7)
		ret =  CGI->townh->hcommands[state];
	switch (state)
	{
	case 4:
		ret.replace(ret.find_first_of("%s"),2,CGI->buildh->buildings[tid][bid]->name);
		break;
	case 7:
		return CGI->generaltexth->allTexts[219]; //all prereq. are met
	case 8:
		{
			ret = CGI->generaltexth->allTexts[52];
			std::set<int> used;
			used.insert(bid);
			std::set<int> reqs;
			
			for(std::set<int>::iterator i=CGI->townh->requirements[tid][bid].begin();i!=CGI->townh->requirements[tid][bid].end();i++)
				if (LOCPLINT->castleInt->town->builtBuildings.find(*i)   ==  LOCPLINT->castleInt->town->builtBuildings.end()) 
					reqs.insert(*i);
			while(true)
			{
				int czystych=0;
				for(std::set<int>::iterator i=reqs.begin();i!=reqs.end();i++)
				{
					if(used.find(*i)==used.end()) //we haven't added requirements for this building
					{
						used.insert(*i);
						for(
							std::set<int>::iterator j=CGI->townh->requirements[tid][*i].begin();
							j!=CGI->townh->requirements[tid][*i].end();
							j++
								)
						{
							if(LOCPLINT->castleInt->town->builtBuildings.find(*j)   ==   //this building is not built
													LOCPLINT->castleInt->town->builtBuildings.end()) 
							reqs.insert(*j);
						}
					}
					else
					{
						czystych++;
					}
				}
				if(czystych==reqs.size())
					break;
			}
			bool first=true;
			for(std::set<int>::iterator i=reqs.begin();i!=reqs.end();i++)
			{
				ret+=(((first)?(" "):(", ")) + CGI->buildh->buildings[tid][*i]->name);
				first = false;
			}
		}
	}
	return ret;
}
CHallInterface::CBuildWindow::CBuildWindow(int Tid, int Bid, int State, bool Mode)
:tid(Tid),bid(Bid),mode(Mode), state(State)
{
	SDL_Surface *hhlp = CGI->bitmaph->loadBitmap("TPUBUILD.bmp");
	bitmap = SDL_ConvertSurface(hhlp,ekran->format,0); //na 8bitowej mapie by sie psulo
	SDL_SetColorKey(hhlp,SDL_SRCCOLORKEY,SDL_MapRGB(hhlp->format,0,255,255));
	SDL_FreeSurface(hhlp);
	pos.x = ekran->w/2 - bitmap->w/2;
	pos.y = ekran->h/2 - bitmap->h/2;
	CSDL_Ext::blueToPlayersAdv(bitmap,LOCPLINT->playerID);
	blitAt(LOCPLINT->castleInt->bicons->ourImages[bid].bitmap,125,50,bitmap);
	std::vector<std::string> pom; pom.push_back(CGI->buildh->buildings[tid][bid]->name);
	CSDL_Ext::printAtMiddleWB(CGI->buildh->buildings[tid][bid]->description,197,168,GEOR16,40,zwykly,bitmap);
	CSDL_Ext::printAtMiddleWB(getTextForState(state),197,248,GEOR13,50,zwykly,bitmap);
	CSDL_Ext::printAtMiddle(CSDL_Ext::processStr(CGI->townh->hcommands[7],pom),197,30,GEOR16,tytulowy,bitmap);
	int resamount=0; for(int i=0;i<7;i++) if(CGI->buildh->buildings[tid][bid]->resources[i]) resamount++;
	int ah = (resamount>4) ? 304 : 341;
	int cn=-1, it=0;
	int row1w = std::min(resamount,4) * 32 + (std::min(resamount,4)-1) * 45,
		row2w = (resamount-4) * 32 + (resamount-5) * 45;
	char buf[15];
	while(++cn<7)
	{
		if(!CGI->buildh->buildings[tid][bid]->resources[cn])
			continue;
		itoa(CGI->buildh->buildings[tid][bid]->resources[cn],buf,10);
		if(it<4)
		{
			CSDL_Ext::printAtMiddle(buf,(bitmap->w/2-row1w/2)+77*it+16,ah+42,GEOR16,zwykly,bitmap);
			blitAt(CGI->townh->resources->ourImages[cn].bitmap,(bitmap->w/2-row1w/2)+77*it++,ah,bitmap);
		}
		else
		{
			CSDL_Ext::printAtMiddle(buf,(bitmap->w/2-row2w/2)+77*it+16-308,ah+42,GEOR16,zwykly,bitmap);
			blitAt(CGI->townh->resources->ourImages[cn].bitmap,(bitmap->w/2-row2w/2)+77*it++ - 308,ah,bitmap);
		}
		if(it==4)
			ah+=75;
	}
	if(!mode)
	{
		buy = new AdventureMapButton<CBuildWindow>
			("","",&CBuildWindow::Buy,pos.x+45,pos.y+446,"IBUY30.DEF",this,false,NULL,false);
		cancel = new AdventureMapButton<CBuildWindow>
			("","",&CBuildWindow::close,pos.x+290,pos.y+445,"ICANCEL.DEF",this,false,NULL,false);
		if(state!=7)
			buy->state=2;
	}
	activate();
}
CHallInterface::CBuildWindow::~CBuildWindow()
{
	SDL_FreeSurface(bitmap);
	if(!mode)
	{
		delete buy;
		delete cancel;
	}
}
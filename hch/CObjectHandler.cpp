#include "stdafx.h"
#include "CObjectHandler.h"
#include "../CGameInfo.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include "CAmbarCendamo.h"
#include "../mapHandler.h"
#include "CDefObjInfoHandler.h"
#include "../CLua.h"
void CObjectHandler::loadObjects()
{
	int ID=0;
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("OBJNAMES.TXT");
	int it=0;
	while (it<buf.length()-1)
	{
		CObject nobj;
		CGeneralTextHandler::loadToIt(nobj.name,buf,it,3);
		if(nobj.name.size() && (nobj.name[nobj.name.size()-1]==(char)10 || nobj.name[nobj.name.size()-1]==(char)13 || nobj.name[nobj.name.size()-1]==(char)9))
			nobj.name = nobj.name.substr(0, nobj.name.size()-1);
		objects.push_back(nobj);
	}
}

bool CObjectInstance::operator <(const CObjectInstance &cmp) const
{
	//if(CGI->ac->map.defy[this->defNumber].printPriority==1 && CGI->ac->map.defy[cmp.defNumber].printPriority==0)
	//	return true;
	//if(CGI->ac->map.defy[cmp.defNumber].printPriority==1 && CGI->ac->map.defy[this->defNumber].printPriority==0)
	//	return false;
	//if(this->pos.y<cmp.pos.y)
	//	return true;
	//if(this->pos.y>cmp.pos.y)
	//	return false;
	//if(CGI->ac->map.defy[this->defNumber].isOnDefList && !(CGI->ac->map.defy[cmp.defNumber].isOnDefList))
	//	return true;
	//if(CGI->ac->map.defy[cmp.defNumber].isOnDefList && !(CGI->ac->map.defy[this->defNumber].isOnDefList))
	//	return false;
	//if(!CGI->ac->map.defy[this->defNumber].isVisitable() && CGI->ac->map.defy[cmp.defNumber].isVisitable())
	//	return true;
	//if(!CGI->ac->map.defy[cmp.defNumber].isVisitable() && CGI->ac->map.defy[this->defNumber].isVisitable())
	//	return false;
	//if(this->pos.x<cmp.pos.x)
	//	return true;
	return false;
}

int CObjectInstance::getWidth() const
{
	return -1;//CGI->mh->reader->map.defy[defNumber].handler->ourImages[0].bitmap->w/32;
}

int CObjectInstance::getHeight() const
{
	return -1;//CGI->mh->reader->map.defy[defNumber].handler->ourImages[0].bitmap->h/32;
}

bool CObjectInstance::visitableAt(int x, int y) const
{
	//if(x<0 || y<0 || x>=getWidth() || y>=getHeight() || defObjInfoNumber<0)
	//	return false;
	//if((CGI->dobjinfo->objs[defObjInfoNumber].visitMap[y+6-getHeight()] >> (7-(8-getWidth()+x) )) & 1)
	//	return true;
	return false;
}


bool CGObjectInstance::isHero() const
{
	return false;
}
int CGObjectInstance::getOwner() const
{
	return state->getOwner();
}
int CGObjectInstance::getWidth() const//returns width of object graphic in tiles
{
	return defInfo->handler->ourImages[0].bitmap->w/32;
}
int CGObjectInstance::getHeight() const //returns height of object graphic in tiles
{
	return defInfo->handler->ourImages[0].bitmap->h/32;
}
bool CGObjectInstance::visitableAt(int x, int y) const //returns true if ibject is visitable at location (x, y) form left top tile of image (x, y in tiles)
{
	if(x<0 || y<0 || x>=getWidth() || y>=getHeight() || defInfo==NULL)
		return false;
	if((defInfo->visitMap[y+6-getHeight()] >> (7-(8-getWidth()+x) )) & 1)
		return true;
	return false;
}
bool CGObjectInstance::operator<(const CGObjectInstance & cmp) const  //screen printing priority comparing
{
	if(defInfo->printPriority==1 && defInfo->printPriority==0)
		return true;
	if(defInfo->printPriority==1 && defInfo->printPriority==0)
		return false;
	if(this->pos.y<cmp.pos.y)
		return true;
	if(this->pos.y>cmp.pos.y)
		return false;
	if(defInfo->isOnDefList && !(cmp.defInfo->isOnDefList))
		return true;
	if(cmp.defInfo->isOnDefList && !(defInfo->isOnDefList))
		return false;
	if(!defInfo->isVisitable() && cmp.defInfo->isVisitable())
		return true;
	if(!cmp.defInfo->isVisitable() && defInfo->isVisitable())
		return false;
	if(this->pos.x<cmp.pos.x)
		return true;
	return false;
}


bool CGDefInfo::isVisitable()
{
	for (int i=0; i<6; i++)
	{
		if (visitMap[i])
			return true;
	}
	return false;
}

	
bool CGHeroInstance::isHero() const
{
	return true;
}
unsigned int CGHeroInstance::getTileCost(EterrainType & ttype, Eroad & rdtype, Eriver & rvtype)
{
	unsigned int ret = type->heroClass->terrCosts[ttype];
	switch(rdtype)
	{
	case Eroad::dirtRoad:
		ret*=0.75;
		break;
	case Eroad::grazvelRoad:
		ret*=0.667;
		break;
	case Eroad::cobblestoneRoad:
		ret*=0.5;
		break;
	}
	return ret;
}
unsigned int CGHeroInstance::getLowestCreatureSpeed()
{
	unsigned int sl = 100;
	for(int h=0; h<army.slots.size(); ++h)
	{
		if(army.slots[h].first->speed<sl)
			sl = army.slots[h].first->speed;
	}
	return sl;
}
int3 CGHeroInstance::convertPosition(int3 src, bool toh3m) //toh3m=true: manifest->h3m; toh3m=false: h3m->manifest
{
	if (toh3m)
	{
		src.x+=1;
		return src;
	}
	else
	{
		src.x-=1;
		return src;
	}
}
int3 CGHeroInstance::getPosition(bool h3m) const //h3m=true - returns position of hero object; h3m=false - returns position of hero 'manifestation'
{
	if (h3m)
		return pos;
	else return convertPosition(pos,false);
}
int CGHeroInstance::getSightDistance() const //returns sight distance of this hero
{
	return 6;
}
void CGHeroInstance::setPosition(int3 Pos, bool h3m) //as above, but sets position
{
	if (h3m)
		pos = Pos;
	else
		pos = convertPosition(Pos,true);
}

bool CGHeroInstance::canWalkOnSea() const
{
	//TODO: write it - it should check if hero is flying, or something similiar
	return false;
}
int CGHeroInstance::getCurrentLuck() const
{
	//TODO: write it
	return 0;
}
int CGHeroInstance::getCurrentMorale() const
{
	//TODO: write it
	return 0;
}


int CGTownInstance::getSightDistance() const //returns sight distance
{
	return 10;
}
CGTownInstance::CGTownInstance()
{
	pos = int3(-1,-1,-1);
	builded=-1;
	destroyed=-1;
	garrisonHero=NULL;
	state->owner=-1;
	town=NULL;
}

CGObjectInstance::CGObjectInstance()
{
	//std::cout << "Tworze obiekt "<<this<<std::endl;
	state = new CLuaObjectScript();
	defObjInfoNumber = -1;
}
CGObjectInstance::~CGObjectInstance()
{
	//std::cout << "Usuwam obiekt "<<this<<std::endl;
	if (state)
		delete state;
	state=NULL;
}

CGHeroInstance::~CGHeroInstance()
{
}

CGTownInstance::~CGTownInstance()
{}
CGObjectInstance::CGObjectInstance(const CGObjectInstance & right)
{
	pos = right.pos;
	ID = right.ID;
	subID = right.subID;
	id	= right.id;
	defInfo = right.defInfo;
	info = right.info;
	defObjInfoNumber = right.defObjInfoNumber;
	state = new CLuaObjectScript();
	*state = *right.state;
}
CGObjectInstance& CGObjectInstance::operator=(const CGObjectInstance & right)
{
	pos = right.pos;
	ID = right.ID;
	subID = right.subID;
	id	= right.id;
	defInfo = right.defInfo;
	info = right.info;
	defObjInfoNumber = right.defObjInfoNumber;
	//state = new CLuaObjectScript();
	*state = *right.state;
	return *this;
}
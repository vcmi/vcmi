#include "../stdafx.h"
#include "CObjectHandler.h"
#include "../CGameInfo.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include "CAmbarCendamo.h"
#include "../mapHandler.h"
#include "CDefObjInfoHandler.h"
#include "../CLua.h"
#include "CHeroHandler.h"
#include <boost/algorithm/string/replace.hpp>
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

	buf = CGameInfo::mainObj->bitmaph->getTextFile("ADVEVENT.TXT");
	it=0;
	std::string temp;
	while (it<buf.length()-1)
	{
		CGeneralTextHandler::loadToIt(temp,buf,it,3);
		if (temp[0]=='\"')
			temp = temp.substr(1,temp.length()-2);
		boost::algorithm::replace_all(temp,"\"\"","\"");
		advobtxt.push_back(temp);
	}

	buf = CGameInfo::mainObj->bitmaph->getTextFile("XTRAINFO.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		CGeneralTextHandler::loadToIt(temp,buf,it,3);
		xtrainfo.push_back(temp);
	}

	buf = CGameInfo::mainObj->bitmaph->getTextFile("MINENAME.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		CGeneralTextHandler::loadToIt(temp,buf,it,3);
		mines.push_back(std::pair<std::string,std::string>(temp,""));
	}

	buf = CGameInfo::mainObj->bitmaph->getTextFile("MINEEVNT.TXT");
	it=0;
	int i=0;
	while (it<buf.length()-1)
	{
		CGeneralTextHandler::loadToIt(temp,buf,it,3);
		temp = temp.substr(1,temp.length()-2);
		mines[i++].second = temp;
	}

	buf = CGameInfo::mainObj->bitmaph->getTextFile("RESTYPES.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		CGeneralTextHandler::loadToIt(temp,buf,it,3);
		restypes.push_back(temp);
	}

	cregens.resize(110); //TODO: hardcoded value - change
	for(int i=0; i<cregens.size();i++)
		cregens[i]=-1;
	std::ifstream ifs("config/cregens.txt");
	while(!ifs.eof())
	{
		int dw, cr;
		ifs >> dw >> cr;
		cregens[dw]=cr;
	}
	ifs.close();
	ifs.clear();
	buf = CGameInfo::mainObj->bitmaph->getTextFile("ZCRGN1.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		CGeneralTextHandler::loadToIt(temp,buf,it,3);
		creGens.push_back(temp);
	}

}

bool CGObjectInstance::isHero() const
{
	return false;
}
int CGObjectInstance::getOwner() const
{
	//if (state)
	//	return state->owner;
	//else 
		return tempOwner; //won't have owner
}

void CGObjectInstance::setOwner(int ow)
{
	//if (state)
	//	state->owner = ow;
	//else
		tempOwner = ow;
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
	if(defInfo->printPriority==1 && cmp.defInfo->printPriority==0)
		return true;
	if(cmp.defInfo->printPriority==1 && defInfo->printPriority==0)
		return false;
	if(this->pos.y<cmp.pos.y)
		return true;
	if(this->pos.y>cmp.pos.y)
		return false;
	if(cmp.ID==34 && ID!=34)
		return true;
	if(cmp.ID!=34 && ID==34)
		return false;
	if(!defInfo->isVisitable() && cmp.defInfo->isVisitable())
		return true;
	if(!cmp.defInfo->isVisitable() && defInfo->isVisitable())
		return false;
	//if(defInfo->isOnDefList && !(cmp.defInfo->isOnDefList))
	//	return true;
	//if(cmp.defInfo->isOnDefList && !(defInfo->isOnDefList))
	//	return false;
	if(this->pos.x<cmp.pos.x)
		return true;
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
int CGHeroInstance::getSecSkillLevel(int ID) const
{
	for(int i=0;i<secSkills.size();i++)
		if(secSkills[i].first==ID)
			return secSkills[i].second;
	return -1;
}


int CGTownInstance::getSightDistance() const //returns sight distance
{
	return 10;
}
int CGTownInstance::fortLevel() const //0 - none, 1 - fort, 2 - citadel, 3 - castle
{
	if((builtBuildings.find(9))!=builtBuildings.end())
		return 3;
	if((builtBuildings.find(8))!=builtBuildings.end())
		return 2;
	if((builtBuildings.find(7))!=builtBuildings.end())
		return 1;
	return 0;
}
int CGTownInstance::hallLevel() const // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
{
	if ((builtBuildings.find(13))!=builtBuildings.end())
		return 3;
	if ((builtBuildings.find(12))!=builtBuildings.end())
		return 2;
	if ((builtBuildings.find(11))!=builtBuildings.end())
		return 1;
	if ((builtBuildings.find(10))!=builtBuildings.end())
		return 0;
	return -1;
}
int CGTownInstance::dailyIncome() const
{
	int ret = 0;
	if ((builtBuildings.find(26))!=builtBuildings.end())
		ret+=5000;
	if ((builtBuildings.find(13))!=builtBuildings.end())
		ret+=4000;
	else if ((builtBuildings.find(12))!=builtBuildings.end())
		ret+=2000;
	else if ((builtBuildings.find(11))!=builtBuildings.end())
		ret+=1000;
	else if ((builtBuildings.find(10))!=builtBuildings.end())
		ret+=500;
	return ret;
}
bool CGTownInstance::hasFort() const
{
	return (builtBuildings.find(7))!=builtBuildings.end();
}
bool CGTownInstance::hasCapitol() const
{
	return (builtBuildings.find(13))!=builtBuildings.end();
}
CGTownInstance::CGTownInstance()
{
	pos = int3(-1,-1,-1);
	builded=-1;
	destroyed=-1;
	garrisonHero=NULL;
	//state->owner=-1;
	town=NULL;
	income = 500;
	visitingHero = NULL;
}

CGObjectInstance::CGObjectInstance()
{
	//std::cout << "Tworze obiekt "<<this<<std::endl;
	//state = new CLuaObjectScript();
	//state = NULL;
	tempOwner = 254;
	blockVisit = false;
}
CGObjectInstance::~CGObjectInstance()
{
	//std::cout << "Usuwam obiekt "<<this<<std::endl;
	//if (state)
	//	delete state;
	//state=NULL;
}
CGHeroInstance::CGHeroInstance()
{
	level = exp = -1;
	isStanding = true;
	moveDir = 4;
	mana = 0;
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
	blockVisit = right.blockVisit;
	//state = new CLuaObjectScript(right.state->);
	//*state = *right.state;
	//state = right.state;
	tempOwner = right.tempOwner;
}
CGObjectInstance& CGObjectInstance::operator=(const CGObjectInstance & right)
{
	pos = right.pos;
	ID = right.ID;
	subID = right.subID;
	id	= right.id;
	defInfo = right.defInfo;
	info = right.info;
	blockVisit = right.blockVisit;
	//state = new CLuaObjectScript();
	//*state = *right.state;
	tempOwner = right.tempOwner;
	return *this;
}
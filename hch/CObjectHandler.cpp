#define VCMI_DLL
#include "../stdafx.h"
#include "CObjectHandler.h"
#include "CDefObjInfoHandler.h"
#include "CLodHandler.h"
#include "CDefObjInfoHandler.h"
#include "CHeroHandler.h"
#include <boost/algorithm/string/replace.hpp>
#include "CTownHandler.h"
#include "CArtHandler.h"
#include "../lib/VCMI_Lib.h"
DLL_EXPORT void loadToIt(std::string &dest, std::string &src, int &iter, int mode);
extern CLodHandler * bitmaph;
void CObjectHandler::loadObjects()
{
	VLC->objh = this;
	int ID=0;
	std::string buf = bitmaph->getTextFile("OBJNAMES.TXT");
	int it=0;
	while (it<buf.length()-1)
	{
		std::string nobj;
		loadToIt(nobj,buf,it,3);
		if(nobj.size() && (nobj[nobj.size()-1]==(char)10 || nobj[nobj.size()-1]==(char)13 || nobj[nobj.size()-1]==(char)9))
			nobj = nobj.substr(0, nobj.size()-1);
		names.push_back(nobj);
	}

	buf = bitmaph->getTextFile("ADVEVENT.TXT");
	it=0;
	std::string temp;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
		if (temp[0]=='\"')
			temp = temp.substr(1,temp.length()-2);
		boost::algorithm::replace_all(temp,"\"\"","\"");
		advobtxt.push_back(temp);
	}

	buf = bitmaph->getTextFile("XTRAINFO.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
		xtrainfo.push_back(temp);
	}

	buf = bitmaph->getTextFile("MINENAME.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
		mines.push_back(std::pair<std::string,std::string>(temp,""));
	}

	buf = bitmaph->getTextFile("MINEEVNT.TXT");
	it=0;
	int i=0;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
		temp = temp.substr(1,temp.length()-2);
		mines[i++].second = temp;
	}

	buf = bitmaph->getTextFile("RESTYPES.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
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
	buf = bitmaph->getTextFile("ZCRGN1.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
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
	return defInfo->width;
}
int CGObjectInstance::getHeight() const //returns height of object graphic in tiles
{
	return defInfo->width;
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
	if(this->pos.x<cmp.pos.x)
		return true;
	return false;
}


bool CGHeroInstance::isHero() const
{
	return true;
}
unsigned int CGHeroInstance::getTileCost(const EterrainType & ttype, const Eroad & rdtype, const Eriver & rvtype) const
{
	unsigned int ret = type->heroClass->terrCosts[ttype];
	switch(rdtype)
	{
	case dirtRoad:
		ret*=0.75;
		break;
	case grazvelRoad:
		ret*=0.667;
		break;
	case cobblestoneRoad:
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
		if(VLC->creh->creatures[army.slots[h].first].speed<sl)
			sl = VLC->creh->creatures[army.slots[h].first].speed;
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
int CGHeroInstance::getSecSkillLevel(const int & ID) const
{
	for(int i=0;i<secSkills.size();i++)
		if(secSkills[i].first==ID)
			return secSkills[i].second;
	return -1;
}
ui32 CGHeroInstance::getArtAtPos(ui16 pos) const
{
	if(pos<19)
		if(vstd::contains(artifWorn,pos))
			return artifWorn.find(pos)->second;
		else
			return -1;
	else
		if(pos-19 < artifacts.size())
			return artifacts[pos-19];
		else 
			return -1;
}
void CGHeroInstance::setArtAtPos(ui16 pos, int art)
{
	if(art<0)
	{
		if(pos<19)
			artifWorn.erase(pos);
		else
			artifacts -= artifacts[pos-19];
	}
	else
	{
		if(pos<19)
			artifWorn[pos] = art;
		else
			if(pos-19 < artifacts.size())
				artifacts[pos-19] = art;
			else
				artifacts.push_back(art);
	}
}
const CArtifact * CGHeroInstance::getArt(int pos)
{
	int id = getArtAtPos(pos);
	if(id>=0)
		return &VLC->arth->artifacts[id];
	else
		return NULL;
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
int CGTownInstance::mageGuildLevel() const
{
	if ((builtBuildings.find(4))!=builtBuildings.end())
		return 5;
	if ((builtBuildings.find(3))!=builtBuildings.end())
		return 4;
	if ((builtBuildings.find(2))!=builtBuildings.end())
		return 3;
	if ((builtBuildings.find(1))!=builtBuildings.end())
		return 2;
	if ((builtBuildings.find(0))!=builtBuildings.end())
		return 1;
	return 0;
}
bool CGTownInstance::creatureDwelling(const int & level, bool upgraded) const
{
	return builtBuildings.find(30+level+upgraded*7)!=builtBuildings.end();
}
int CGTownInstance::getHordeLevel(const int & HID)  const//HID - 0 or 1; returns creature level or -1 if that horde structure is not present
{
	return town->hordeLvl[HID];
}
int CGTownInstance::creatureGrowth(const int & level) const
{
	int ret = VLC->creh->creatures[town->basicCreatures[level]].growth;
	switch(fortLevel())
	{
	case 3:
		ret*=2;break;
	case 2:
		ret*=(1.5); break;
	}
	if(builtBuildings.find(26)!=builtBuildings.end()) //grail
		ret+=VLC->creh->creatures[town->basicCreatures[level]].growth;
	if(getHordeLevel(0)==level)
		if((builtBuildings.find(18)!=builtBuildings.end()) || (builtBuildings.find(19)!=builtBuildings.end()))
			ret+=VLC->creh->creatures[town->basicCreatures[level]].hordeGrowth;
	if(getHordeLevel(1)==level)
		if((builtBuildings.find(24)!=builtBuildings.end()) || (builtBuildings.find(25)!=builtBuildings.end()))
			ret+=VLC->creh->creatures[town->basicCreatures[level]].hordeGrowth;
	return ret;
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
	town=NULL;
	visitingHero = NULL;
}

CGObjectInstance::CGObjectInstance(): animPhaseShift(rand()%0xff)
{
	//std::cout << "Tworze obiekt "<<this<<std::endl;
	//state = new CLuaObjectScript();
	state = NULL;
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
	inTownGarrison = false;
	portrait = level = exp = -1;
	isStanding = true;
	moveDir = 4;
	mana = 0;
	visitedTown = NULL;
	type = NULL;
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

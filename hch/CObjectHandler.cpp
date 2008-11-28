#define VCMI_DLL
#include "../stdafx.h"
#include "CObjectHandler.h"
#include "CDefObjInfoHandler.h"
#include "CLodHandler.h"
#include "CGeneralTextHandler.h"
#include "CDefObjInfoHandler.h"
#include "CHeroHandler.h"
#include "CSpellHandler.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/random/linear_congruential.hpp>
#include "CTownHandler.h"
#include "CArtHandler.h"
#include "../lib/VCMI_Lib.h"
DLL_EXPORT void loadToIt(std::string &dest, std::string &src, int &iter, int mode);
extern CLodHandler * bitmaph;
extern boost::rand48 ran;

void CObjectHandler::loadObjects()
{
	VLC->objh = this;
	int ID=0;
	tlog5 << "\t\tReading OBJNAMES \n";
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

	tlog5 << "\t\tReading ADVEVENT \n";
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

	tlog5 << "\t\tReading XTRAINFO \n";
	buf = bitmaph->getTextFile("XTRAINFO.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
		xtrainfo.push_back(temp);
	}

	tlog5 << "\t\tReading MINENAME \n";
	buf = bitmaph->getTextFile("MINENAME.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
		mines.push_back(std::pair<std::string,std::string>(temp,""));
	}

	tlog5 << "\t\tReading MINEEVNT \n";
	buf = bitmaph->getTextFile("MINEEVNT.TXT");
	it=0;
	int i=0;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
		temp = temp.substr(1,temp.length()-2);
		mines[i++].second = temp;
	}

	tlog5 << "\t\tReading RESTYPES \n";
	buf = bitmaph->getTextFile("RESTYPES.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
		restypes.push_back(temp);
	}

	tlog5 << "\t\tReading cregens \n";
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

	tlog5 << "\t\tReading ZCRGN1 \n";
	buf = bitmaph->getTextFile("ZCRGN1.TXT");
	it=0;
	while (it<buf.length()-1)
	{
		loadToIt(temp,buf,it,3);
		creGens.push_back(temp);
	}
	tlog5 << "\t\tDone loading objects!\n";
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
bool CGObjectInstance::visitableAt(int x, int y) const //returns true if object is visitable at location (x, y) form left top tile of image (x, y in tiles)
{
	if(x<0 || y<0 || x>=getWidth() || y>=getHeight() || defInfo==NULL)
		return false;
	if((defInfo->visitMap[y+6-getHeight()] >> (7-(8-getWidth()+x) )) & 1)
		return true;
	return false;
}
bool CGObjectInstance::blockingAt(int x, int y) const
{
	if(x<0 || y<0 || x>=getWidth() || y>=getHeight() || defInfo==NULL)
		return false;
	if((defInfo->blockMap[y+6-getHeight()] >> (7-(8-getWidth()+x) )) & 1)
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
	//applying pathfinding skill
	switch(getSecSkillLevel(0))
	{
	case 1: //basic
		switch(ttype)
		{
		case rough:
			ret = 100;
			break;
		case sand: case snow:
			if(ret>125)
				ret = 125;
			break;
		case swamp:
			if(ret>150)
				ret = 150;
			break;
		}
		break;
	case 2: //advanced
		switch(ttype)
		{
		case rough: case sand: case snow:
			ret = 100;
			break;
		case swamp:
			if(ret>125)
				ret = 125;
			break;
		}
		break;
	case 3: //expert
		ret = 100;
		break;
	}

	//calculating road influence
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
	return 6 + getSecSkillLevel(3); //default + scouting
}

int CGHeroInstance::manaLimit() const
{
	double modifier = 1.0;
	switch(getSecSkillLevel(24)) //intelligence level
	{
	case 1:		modifier+=0.25;		break;
	case 2:		modifier+=0.5;		break;
	case 3:		modifier+=1.0;		break;
	}
	return 10*getPrimSkillLevel(3)*modifier;
}
//void CGHeroInstance::setPosition(int3 Pos, bool h3m) //as above, but sets position
//{
//	if (h3m)
//		pos = Pos;
//	else
//		pos = convertPosition(Pos,true);
//}

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
int CGHeroInstance::getPrimSkillLevel(int id) const
{
	return primSkills[id];
}
int CGHeroInstance::getSecSkillLevel(const int & ID) const
{
	for(int i=0;i<secSkills.size();i++)
		if(secSkills[i].first==ID)
			return secSkills[i].second;
	return 0;
}
int lowestSpeed(const CGHeroInstance * chi)
{
	if(!chi->army.slots.size())
	{
		tlog1 << "Error! Hero " << chi->id << " ("<<chi->name<<") has no army!\n";
		return 20;
	}
	std::map<si32,std::pair<ui32,si32> >::const_iterator i = chi->army.slots.begin();
	int ret = VLC->creh->creatures[(*i++).second.first].speed;
	for (;i!=chi->army.slots.end();i++)
	{
		ret = std::min(ret,VLC->creh->creatures[(*i).second.first].speed);
	}
	return ret;
}

int CGHeroInstance::maxMovePoints(bool onLand) const
{
	int ret = 1270+70*lowestSpeed(this);
	if (ret>2000) 
		ret=2000;

	if(onLand)
	{
		//logistics:
		switch(getSecSkillLevel(2))
		{
		case 1:
			ret *= 1.1f;
			break;
		case 2:
			ret *= 1.2f;
			break;
		case 3:
			ret *= 1.3f;
			break;
		}
	}
	else
	{
		//navigation:
		switch(getSecSkillLevel(2))
		{
		case 1:
			ret *= 1.5f;
			break;
		case 2:
			ret *= 2.0f;
			break;
		case 3:
			ret *= 2.5f;
			break;
		}
	}
	return ret;
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
const CArtifact * CGHeroInstance::getArt(int pos) const
{
	int id = getArtAtPos(pos);
	if(id>=0)
		return &VLC->arth->artifacts[id];
	else
		return NULL;
}

int CGHeroInstance::getSpellSecLevel(int spell) const
{
	int bestslvl = 0;
	if(VLC->spellh->spells[spell].air)
		if(getSecSkillLevel(15) >= bestslvl)
		{
			bestslvl = getSecSkillLevel(15);
		}
	if(VLC->spellh->spells[spell].fire)
		if(getSecSkillLevel(14) >= bestslvl)
		{
			bestslvl = getSecSkillLevel(14);
		}
	if(VLC->spellh->spells[spell].water)
		if(getSecSkillLevel(16) >= bestslvl)
		{
			bestslvl = getSecSkillLevel(16);
		}
	if(VLC->spellh->spells[spell].earth)
		if(getSecSkillLevel(17) >= bestslvl)
		{
			bestslvl = getSecSkillLevel(17);
		}
	return bestslvl;
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
	builded=-1;
	destroyed=-1;
	garrisonHero=NULL;
	town=NULL;
	visitingHero = NULL;
}

CGObjectInstance::CGObjectInstance(): animPhaseShift(rand()%0xff)
{
	pos = int3(-1,-1,-1);
	//std::cout << "Tworze obiekt "<<this<<std::endl;
	//state = new CLuaObjectScript();
	ID = subID = id = -1;
	defInfo = NULL;
	state = NULL;
	info = NULL;
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
	ID = 34;
	tacticFormationEnabled = inTownGarrison = false;
	mana = movement = portrait = level = -1;
	isStanding = true;
	moveDir = 4;
	exp = 0xffffffff;
	visitedTown = NULL;
	type = NULL;
	secSkills.push_back(std::make_pair(-1, -1));
}

void CGHeroInstance::initHero(int SUBID)
{
	subID = SUBID;
	initHero();
}

void CGHeroInstance::initHero()
{
	if(!defInfo)
	{
		defInfo = new CGDefInfo();
		defInfo->id = 34;
		defInfo->subid = subID;
		defInfo->printPriority = 0;
		defInfo->visitDir = 0xff;
	}
	if(!type)
		type = VLC->heroh->heroes[subID];
	for(int i=0;i<6;i++)
	{
		defInfo->blockMap[i]=255;
		defInfo->visitMap[i]=0;
	}
	defInfo->handler=NULL;
	defInfo->blockMap[5] = 253;
	defInfo->visitMap[5] = 2;

	artifWorn[16] = 3;
	if(type->heroType % 2 == 1) //it's a magical hero
	{
		artifWorn[17] = 0; //give him spellbook
	}

	if(portrait < 0 || portrait == 255)
		portrait = subID;
	if((!primSkills.size()) || (getPrimSkillLevel(0)<0))
	{
		primSkills.resize(4);
		primSkills[0] = type->heroClass->initialAttack;
		primSkills[1] = type->heroClass->initialDefence;
		primSkills[2] = type->heroClass->initialPower;
		primSkills[3] = type->heroClass->initialKnowledge;
	}
	if(secSkills.size() == 1 && secSkills[0] == std::pair<ui8,ui8>(-1, -1)) //set secondary skills to default
		secSkills = type->secSkillsInit;
	if(mana < 0)
		mana = manaLimit();
	if (!name.length())
		name = type->name;
	if (exp == 0xffffffff)
	{
		exp=40+  (ran())  % 50;
		level = 1;
	}
	else
	{
		level = VLC->heroh->level(exp);
	}
	
	if (!army.slots.size()) //standard army//initial army
	{
		int pom, pom2=0;
		for(int x=0;x<3;x++)
		{
			pom = (VLC->creh->nameToID[type->refTypeStack[x]]);
			if(pom>=145 && pom<=149) //war machine
			{
				pom2++;
				switch (pom)
				{
				case 145: //catapult
					artifWorn[16] = 3;
					break;
				default:
					artifWorn[9+CArtHandler::convertMachineID(pom,true)] = CArtHandler::convertMachineID(pom,true);
					break;
				}
				continue;
			}
			army.slots[x-pom2].first = pom;
			if((pom = (type->highStack[x]-type->lowStack[x])) > 0)
				army.slots[x-pom2].second = (ran()%pom)+type->lowStack[x];
			else 
				army.slots[x-pom2].second = +type->lowStack[x];
			army.formation = false;
		}
	}
}

CGHeroInstance::~CGHeroInstance()
{
}

bool CGHeroInstance::needsLastStack() const
{
	return true;
}

const std::string & CGHeroInstance::getBiography()
{
	if (biography.length())
		return biography;
	else
		return VLC->generaltexth->hTxts[subID].biography;		
}
CGTownInstance::~CGTownInstance()
{}

int CGTownInstance::spellsAtLevel(int level, bool checkGuild) const
{
	if(checkGuild && mageGuildLevel() < level)
		return 0;
	int ret = 6 - level; //how many spells are available at this level
	if(subID == 2   &&   vstd::contains(builtBuildings,22)) //magic library in Tower
		ret++; 
	return ret;
}

bool CGTownInstance::needsLastStack() const
{
	if(garrisonHero)
		return true;
	else return false;
}
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

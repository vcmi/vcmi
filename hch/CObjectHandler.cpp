#define VCMI_DLL
#include "../stdafx.h"
#include "CObjectHandler.h"
#include "CDefObjInfoHandler.h"
#include "CLodHandler.h"
#include "CGeneralTextHandler.h"
#include "CDefObjInfoHandler.h"
#include "CHeroHandler.h"
#include "CSpellHandler.h"
#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/random/linear_congruential.hpp>
#include "CTownHandler.h"
#include "CArtHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/IGameCallback.h"
#include "../CGameState.h"
#include "../lib/NetPacks.h"

std::map<int,std::map<int, std::vector<int> > > CGTeleport::objs;
IGameCallback * IObjectInterface::cb = NULL;
DLL_EXPORT void loadToIt(std::string &dest, std::string &src, int &iter, int mode);
extern CLodHandler * bitmaph;
extern boost::rand48 ran;


void IObjectInterface::onHeroVisit(const CGHeroInstance * h) const 
{};

void IObjectInterface::onHeroLeave(const CGHeroInstance * h) const 
{};

void IObjectInterface::newTurn () const
{};

IObjectInterface::~IObjectInterface()
{}

IObjectInterface::IObjectInterface()
{}

void IObjectInterface::initObj()
{}

void CObjectHandler::loadObjects()
{
	tlog5 << "\t\tReading cregens \n";
	cregens.resize(110); //TODO: hardcoded value - change
	for(size_t i=0; i < cregens.size(); ++i)
	{
		cregens[i]=-1;
	}
	std::ifstream ifs("config/cregens.txt");
	while(!ifs.eof())
	{
		int dw, cr;
		ifs >> dw >> cr;
		cregens[dw]=cr;
	}
	ifs.close();
	ifs.clear();
	tlog5 << "\t\tDone loading objects!\n";
}
int CGObjectInstance::getOwner() const
{
	//if (state)
	//	return state->owner;
	//else
		return tempOwner; //won't have owner
}

CGObjectInstance::CGObjectInstance(): animPhaseShift(rand()%0xff)
{
	pos = int3(-1,-1,-1);
	//std::cout << "Tworze obiekt "<<this<<std::endl;
	//state = new CLuaObjectScript();
	ID = subID = id = -1;
	defInfo = NULL;
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
//CGObjectInstance::CGObjectInstance(const CGObjectInstance & right)
//{
//	pos = right.pos;
//	ID = right.ID;
//	subID = right.subID;
//	id	= right.id;
//	defInfo = right.defInfo;
//	info = right.info;
//	blockVisit = right.blockVisit;
//	//state = new CLuaObjectScript(right.state->);
//	//*state = *right.state;
//	//state = right.state;
//	tempOwner = right.tempOwner;
//}
//CGObjectInstance& CGObjectInstance::operator=(const CGObjectInstance & right)
//{
//	pos = right.pos;
//	ID = right.ID;
//	subID = right.subID;
//	id	= right.id;
//	defInfo = right.defInfo;
//	info = right.info;
//	blockVisit = right.blockVisit;
//	//state = new CLuaObjectScript();
//	//*state = *right.state;
//	tempOwner = right.tempOwner;
//	return *this;
//}

const std::string & CGObjectInstance::getHoverText() const
{
	return hoverName;
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

void CGObjectInstance::initObj()
{
}

int lowestSpeed(const CGHeroInstance * chi)
{
	if(!chi->army.slots.size())
	{
		tlog1 << "Error! Hero " << chi->id << " ("<<chi->name<<") has no army!\n";
		return 20;
	}
	std::map<si32,std::pair<ui32,si32> >::const_iterator i = chi->army.slots.begin();
	ui32 ret = VLC->creh->creatures[(*i++).second.first].speed;
	for (;i!=chi->army.slots.end();i++)
	{
		ret = std::min(ret,VLC->creh->creatures[(*i).second.first].speed);
	}
	return ret;
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
        default:
            //TODO do something nasty here throw maybe? or some def value asing
            break;
		}
		break;
	case 2: //advanced
		switch(ttype)
		{
		case rough: 
                case sand:
                case snow:
			ret = 100;
			break;
		case swamp:
			if(ret>125)
				ret = 125;
			break;
        default:
            //TODO look up
            break;
		}
		break;
	case 3: //expert
		ret = 100;
		break;
    default:
        //TODO look up
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
    default:
        //TODO killllll me
        break;
	}
	return ret;
}
unsigned int CGHeroInstance::getLowestCreatureSpeed() const
{
	unsigned int sl = 100;
	for(size_t h=0; h < army.slots.size(); ++h)
	{
		if(VLC->creh->creatures[army.slots.find(h)->first].speed<sl)
			sl = VLC->creh->creatures[army.slots.find(h)->first].speed;
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
	{
		return pos;
	}
	else
	{
		return convertPosition(pos,false);
	}
}
int CGHeroInstance::getSightDistance() const //returns sight distance of this hero
{
	return 6 + getSecSkillLevel(3); //default + scouting
}

si32 CGHeroInstance::manaLimit() const
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
ui8 CGHeroInstance::getSecSkillLevel(const int & ID) const
{
	for(size_t i=0; i < secSkills.size(); ++i)
		if(secSkills[i].first==ID)
			return secSkills[i].second;
	return 0;
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
	hoverName = VLC->generaltexth->allTexts[15];
	boost::algorithm::replace_first(hoverName,"%s",name);
	boost::algorithm::replace_first(hoverName,"%s", type->heroClass->name);
}

CGHeroInstance::~CGHeroInstance()
{
}

bool CGHeroInstance::needsLastStack() const
{
	return true;
}
void CGHeroInstance::onHeroVisit(const CGHeroInstance * h) const
{
	//TODO: check for allies
	if(tempOwner == h->tempOwner) //our hero
	{
		//exchange
	}
	else
	{
		cb->startBattleI(
			&h->army,
			&army,
			h->pos,
			h,
			this,
			0);
	}
}

const std::string & CGHeroInstance::getBiography() const
{
	if (biography.length())
		return biography;
	else
		return VLC->generaltexth->hTxts[subID].biography;		
}
void CGHeroInstance::initObj()
{
	blockVisit = true;
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

void CGTownInstance::onHeroVisit(const CGHeroInstance * h) const
{
	if(getOwner() != h->getOwner())
	{
		return;
	}
	cb->heroVisitCastle(id,h->id);
}

void CGTownInstance::onHeroLeave(const CGHeroInstance * h) const
{
	cb->stopHeroVisitCastle(id,h->id);
}

void CGTownInstance::initObj()
{
	MetaString ms;
	ms << name << ", " << town->Name();
	hoverName = toString(ms);
}

void CGVisitableOPH::onHeroVisit( const CGHeroInstance * h ) const
{
	if(visitors.find(h->id)==visitors.end())
	{
		onNAHeroVisit(h->id, false);
		if(ID != 102  &&  ID!=4) //not tree nor arena
			cb->setObjProperty(id,4,h->id); //add to the visitors
	}
	else
	{
		onNAHeroVisit(h->id, true);
	}
}

void CGVisitableOPH::initObj()
{
	if(ID==102)
		ttype = ran()%3;
	else
		ttype = -1;
}

void CGVisitableOPH::treeSelected( int heroID, int resType, int resVal, int expVal, ui32 result ) const
{
	if(result==0) //player agreed to give res for exp
	{
		cb->giveResource(cb->getOwner(heroID),resType,-resVal); //take resource
		cb->changePrimSkill(heroID,4,expVal); //give exp
		cb->setObjProperty(id,4,heroID); //add to the visitors
	}
}
void CGVisitableOPH::onNAHeroVisit(int heroID, bool alreadyVisited) const
{
	int id=0, subid=0, ot=0, val=1;
	switch(ID)
	{
	case 4:
		ot = 0;
		break;
	case 51:
		subid=0;
		ot=80;
		break;
	case 23:
		subid=1;
		ot=39;
		break;
	case 61:
		subid=2;
		ot=100;
		break;
	case 32:
		subid=3;
		ot=59;
		break;
	case 100:
		id=5;
		ot=143;
		val=1000;
		break;
	case 102:
		id = 5;
		subid = 1;
		ot = 146;
		val = 1;
		break;
	}
	if (!alreadyVisited)
	{
		switch (ID)
		{
		case 4: //arena
			{
				SelectionDialog sd;
				sd.text << std::pair<ui8,ui32>(11,ot);
				sd.components.push_back(Component(0,0,2,0));
				sd.components.push_back(Component(0,1,2,0));
				sd.player = cb->getOwner(heroID);
				cb->showSelectionDialog(&sd,boost::bind(&CGVisitableOPH::arenaSelected,this,heroID,_1));
				return;
			}
		case 51:
		case 23:
		case 61:
		case 32:
			{
				cb->changePrimSkill(heroID,subid,val);
				InfoWindow iw;
				iw.components.push_back(Component(0,subid,val,0));
				iw.text << std::pair<ui8,ui32>(11,ot);
				iw.player = cb->getOwner(heroID);
				cb->showInfoDialog(&iw);
				break;
			}
		case 100: //give exp
			{
				InfoWindow iw;
				iw.components.push_back(Component(id,subid,val,0));
				iw.player = cb->getOwner(heroID);
				iw.text << std::pair<ui8,ui32>(11,ot);
				cb->showInfoDialog(&iw);
				cb->changePrimSkill(heroID,4,val);
				break;
			}
		case 102:
			{
				const CGHeroInstance *h = cb->getHero(heroID);
				val = VLC->heroh->reqExp(h->level+val) - VLC->heroh->reqExp(h->level);
				if(!ttype)
				{
					cb->setObjProperty(this->id,4,heroID); //add to the visitors
					InfoWindow iw;
					iw.components.push_back(Component(id,subid,1,0));
					iw.player = cb->getOwner(heroID);
					iw.text << std::pair<ui8,ui32>(11,148);
					cb->showInfoDialog(&iw);
					cb->changePrimSkill(heroID,4,val);
					break;
				}
				else
				{
					int res, resval;
					if(ttype==1)
					{
						res = 6;
						resval = 2000;
						ot = 149;
					}
					else
					{
						res = 5;
						resval = 10;
						ot = 151;
					}

					if(cb->getResource(h->tempOwner,res) < resval) //not enough resources
					{
						ot++;
						InfoWindow iw;
						iw.player = h->tempOwner;
						iw.text << std::pair<ui8,ui32>(11,ot);
						cb->showInfoDialog(&iw);
						return;
					}

					YesNoDialog sd;
					sd.player = cb->getOwner(heroID);
					sd.text << std::pair<ui8,ui32>(11,ot);
					sd.components.push_back(Component(id,subid,val,0));
					cb->showYesNoDialog(&sd,boost::bind(&CGVisitableOPH::treeSelected,this,heroID,res,resval,val,_1));
				}
				break;
			}
		}
	}
	else
	{
		ot++;
		InfoWindow iw;
		iw.player = cb->getOwner(heroID);
		iw.text << std::pair<ui8,ui32>(11,ot);
		cb->showInfoDialog(&iw);
	}
}

const std::string & CGVisitableOPH::getHoverText() const
{
	int pom = -1;
	switch(ID)
	{
	case 4:
		pom = -1;
		break;
	case 51:
		pom = 8; 
		break;
	case 23:
		pom = 7;
		break;
	case 61:
		pom = 11; 
		break;
	case 32:
		pom = 4; 
		break;
	case 100:
		pom = 5; 
		break;
	case 102:
		pom = 18;
		break;
	default:
		throw std::string("Wrong CGVisitableOPH object ID!\n");
	}
	hoverName = VLC->generaltexth->names[ID];
	if(pom >= 0)
		hoverName += (" " + VLC->generaltexth->xtrainfo[pom]);
	const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
	if(h)
	{
		hoverName += ' ';
		hoverName += (vstd::contains(visitors,h->id)) 
							? (VLC->generaltexth->allTexts[352])  //visited
							: ( VLC->generaltexth->allTexts[353]); //not visited
	}
	return hoverName;
}

void CGVisitableOPH::arenaSelected( int heroID, int primSkill ) const
{
	cb->setObjProperty(id,4,heroID); //add to the visitors
	cb->changePrimSkill(heroID,primSkill,2);
}

bool CArmedInstance::needsLastStack() const
{
	return false;
}

void CGCreature::onHeroVisit( const CGHeroInstance * h ) const
{
	cb->startBattleI(h->id,army,pos,boost::bind(&CGCreature::endBattle,this,_1));
}

void CGCreature::endBattle( BattleResult *result ) const
{
	if(result->winner==0)
	{
		cb->removeObject(id);
	}
	else
	{
		int killedAmount=0;
		for(std::set<std::pair<ui32,si32> >::iterator i=result->casualties[1].begin(); i!=result->casualties[1].end(); i++)
			if(i->first == subID)
				killedAmount += i->second;
		cb->setAmount(id, army.slots.find(0)->second.second - killedAmount);	
		
		MetaString ms;
		int pom = CCreature::getQuantityID(army.slots.find(0)->second.second);
		pom = 174 + 3*pom + 1;
		ms << std::pair<ui8,ui32>(6,pom) << " " << std::pair<ui8,ui32>(7,subID);
		cb->setHoverName(id,&ms);
	}
}

void CGCreature::initObj()
{
	army.slots[0].first = subID;
	si32 &amount = army.slots[0].second;
	CCreature &c = VLC->creh->creatures[subID];
	if(!amount)
		if(c.ammMax == c.ammMin)
			amount = c.ammMax;
		else
			amount = c.ammMin + (ran() % (c.ammMax - c.ammMin));

	MetaString ms;
	int pom = CCreature::getQuantityID(army.slots.find(0)->second.second);
	pom = 174 + 3*pom + 1;
	ms << std::pair<ui8,ui32>(6,pom) << " " << std::pair<ui8,ui32>(7,subID);
	hoverName = toString(ms);
}

void CGMine::onHeroVisit( const CGHeroInstance * h ) const
{
	if(subID == 7) //TODO: support for abandoned mine
		return;

	if(h->tempOwner == tempOwner) //we're visiting our mine
		return; //TODO: leaving garrison

	//TODO: check if mine is guarded
	cb->setOwner(id,h->tempOwner); //not ours? flag it!

	MetaString ms;
	ms << std::pair<ui8,ui32>(9,subID) << " (" << std::pair<ui8,ui32>(6,23+h->tempOwner) << ")";
	cb->setHoverName(id,&ms);

	int vv=1; //amount of resource per turn	
	if (subID==0 || subID==2)
		vv++;
	else if (subID==6)
		vv = 1000;

	InfoWindow iw;
	iw.text << std::pair<ui8,ui32>(10,subID);
	iw.player = h->tempOwner;
	iw.components.push_back(Component(2,subID,vv,-1));
	cb->showInfoDialog(&iw);
}

void CGMine::newTurn() const
{
	if (tempOwner == NEUTRAL_PLAYER)
		return;
	int vv = 1;
	if (subID==0 || subID==2)
		vv++;
	else if (subID==6)
		vv = 1000;
	cb->giveResource(tempOwner,subID,vv);
}

void CGMine::initObj()
{
	MetaString ms;
	ms << std::pair<ui8,ui32>(9,subID);
	if(tempOwner >= PLAYER_LIMIT)
		tempOwner = NEUTRAL_PLAYER;	
	else
		ms << " (" << std::pair<ui8,ui32>(6,23+tempOwner) << ")";
	hoverName = toString(ms);
}

void CGResource::initObj()
{
	blockVisit = true;
	hoverName = VLC->generaltexth->restypes[subID];

	if(!amount)
	{
		switch(subID)
		{
		case 6:
			amount = 500 + (rand()%6)*100;
			break;
		case 0: case 2:
			amount = 6 + (rand()%5);
			break;
		default:
			amount = 3 + (rand()%3);
			break;
		}
	}
}

void CGResource::onHeroVisit( const CGHeroInstance * h ) const
{
	if(army.slots.size())
	{
		if(message.size())
		{
			YesNoDialog ynd;
			ynd.player = h->getOwner();
			ynd.text << message;
			cb->showYesNoDialog(&ynd,boost::bind(&CGResource::fightForRes,this,_1,h));
		}
		else
		{
			fightForRes(0,h);
		}
	}
	else
	{
		if(message.length())
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text << message;
			cb->showInfoDialog(&iw);
		}
		collectRes(h->getOwner());
	}
}

void CGResource::collectRes( int player ) const
{
	cb->giveResource(player,subID,amount);
	ShowInInfobox sii;
	sii.player = player;
	sii.c = Component(2,subID,amount,0);
	sii.text << std::pair<ui8,ui32>(11,113);
	sii.text.replacements.push_back(VLC->generaltexth->restypes[subID]);
	cb->showCompInfo(&sii);
	cb->removeObject(id);
}

void CGResource::fightForRes(ui32 refusedFight, const CGHeroInstance *h) const
{
	if(refusedFight)
		return;

	cb->startBattleI(h->id,army,pos,boost::bind(&CGResource::endBattle,this,_1,h));
}

void CGResource::endBattle( BattleResult *result, const CGHeroInstance *h ) const
{
	if(result->winner == 0) //attacker won
		collectRes(h->getOwner());
}

void CGVisitableOPW::newTurn() const
{
	if (cb->getDate(1)==1) //first day of week
	{
		cb->setObjProperty(id,5,false);
		MetaString ms; //set text to "not visited"
		ms << std::pair<ui8,ui32>(3,ID) << " " << std::pair<ui8,ui32>(1,353);
		cb->setHoverName(id,&ms);
	}
}

void CGVisitableOPW::onHeroVisit( const CGHeroInstance * h ) const
{
	int mid;
	switch (ID)
	{
	case 55:
		mid = 92;
		break;
	case 112:
		mid = 170;
		break;
	case 109:
		mid = 164;
		break;
	}
	if (visited)
	{
		if (ID!=112)
			mid++;
		else 
			mid--;

		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text << std::pair<ui8,ui32>(11,mid);
		cb->showInfoDialog(&iw);
	}
	else
	{
		int type, sub, val;
		type = 2;
		switch (ID)
		{
		case 55:
			if (rand()%2)
			{
				sub = 5;
				val = 5;
			}
			else
			{
				sub = 6;
				val = 500;
			}
			break;
		case 112:
			mid = 170;
			sub = (rand() % 5) + 1;
			val = (rand() % 4) + 3;
			break;
		case 109:
			mid = 164;
			sub = 6;
			if(cb->getDate(2)<2)
				val = 500;
			else
				val = 1000;
		}
		cb->giveResource(h->tempOwner,sub,val);
		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.components.push_back(Component(type,sub,val,0));
		iw.text << std::pair<ui8,ui32>(11,mid);
		cb->showInfoDialog(&iw);
		cb->setObjProperty(id,5,true);
		MetaString ms; //set text to "visited"
		ms << std::pair<ui8,ui32>(3,ID) << " " << std::pair<ui8,ui32>(1,352);
		cb->setHoverName(id,&ms);
	}
}

void CGTeleport::onHeroVisit( const CGHeroInstance * h ) const
{
	int destinationid=-1;
	switch(ID)
	{
	case 43: //one way - find correspong exit monolith
		if(vstd::contains(objs,44) && vstd::contains(objs[44],subID) && objs[44][subID].size())
			destinationid = objs[44][subID][rand()%objs[44][subID].size()];
		else
			tlog2 << "Cannot find corresponding exit monolith for "<< id << std::endl;
		break;
	case 45: //two way monolith - pick any other one
		if(vstd::contains(objs,45) && vstd::contains(objs[45],subID) && objs[45][subID].size()>1)
			while ((destinationid = objs[45][subID][rand()%objs[45][subID].size()])==id);
		else
			tlog2 << "Cannot find corresponding exit monolith for "<< id << std::endl;
		break;
	case 103: //find nearest subterranean gate on the other level
		{
			std::pair<int,double> best(-1,150000); //pair<id,dist>
			for(int i=0; i<objs[103][0].size(); i++)
			{
				if(cb->getObj(objs[103][0][i])->pos.z == pos.z) continue; //gates on our level are not interesting
				double hlp = cb->getObj(objs[103][0][i])->pos.dist2d(pos);
				if(hlp<best.second)
				{
					best.first = objs[103][0][i];
					best.second = hlp;
				}
			}
			if(best.first<0)
				return;
			else 
				destinationid = best.first;
			break;
		}
	}
	if(destinationid < 0)
	{
		tlog2 << "Cannot find exit... :( \n";
		return;
	}
	cb->moveHero(h->id,
				(ID!=103)
					? (CGHeroInstance::convertPosition(cb->getObj(destinationid)->pos,true))
					: (cb->getObj(destinationid)->pos),
				true);
}

void CGTeleport::initObj()
{
	objs[ID][subID].push_back(id);
}

void CGArtifact::initObj()
{
	blockVisit = true;
	if(ID == 5)
		hoverName = VLC->arth->artifacts[subID].Name();
}

void CGArtifact::onHeroVisit( const CGHeroInstance * h ) const
{
	cb->giveHeroArtifact(subID,h->id,-2);
	InfoWindow iw;
	iw.player = h->tempOwner;
	iw.components.push_back(Component(4,subID,0,0));
	iw.text << std::pair<ui8,ui32>(12,subID);
	cb->showInfoDialog(&iw);
}

void CGPickable::initObj()
{
	blockVisit = true;
	switch(ID)
	{
	case 12: //campfire
		val2 = (ran()%3) + 4; //4 - 6
		val1 = val2 * 100;
		type = ran()%6; //given resource
		break;
	case 101: //treasure chest
		{
			int hlp = ran()%100;
			if(hlp >= 95)
			{
				type = 1;
				val1 = VLC->arth->treasures[ran()%VLC->arth->treasures.size()]->id;
				return;
			}
			else if (hlp >= 65)
			{
				val1 = 2000;
			}
			else if(hlp >= 33)
			{
				val1 = 1500;
			}
			else
			{
				val1 = 1000;
			}

			val2 = val1 - 500;
			type = 0;
			break;
		}
	}
}

void CGPickable::onHeroVisit( const CGHeroInstance * h ) const
{

	switch(ID)
	{
	case 12: //campfire
		{
			cb->giveResource(h->tempOwner,type,val2); //non-gold resource
			cb->giveResource(h->tempOwner,6,val1);//gold
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.components.push_back(Component(2,6,val1,0));
			iw.components.push_back(Component(2,type,val2,0));
			iw.text << std::pair<ui8,ui32>(11,23);
			cb->showInfoDialog(&iw);
			break;
		}
	case 101: //treasure chest
		{
			if (subID) //not OH3 treasure chest
			{
				tlog2 << "Not supported WoG treasure chest!\n";
				return; 
			}

			if(type) //there is an artifact
			{
				cb->giveHeroArtifact(val1,h->id,-2);
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.components.push_back(Component(4,val1,1,0));
				iw.text << std::pair<ui8,ui32>(11,145);
				iw.text.replacements.push_back(VLC->arth->artifacts[val1].Name());
				cb->showInfoDialog(&iw);
				break;
			}
			else
			{
				SelectionDialog sd;
				sd.player = h->tempOwner;
				sd.text << std::pair<ui8,ui32>(11,146);
				sd.components.push_back(Component(2,6,val1,0));
				sd.components.push_back(Component(5,0,val2,0));
				boost::function<void(ui32)> fun = boost::bind(&CGPickable::chosen,this,_1,h->id);
				cb->showSelectionDialog(&sd,fun);
				return;
			}
		}
	}
	cb->removeObject(id);
}

void CGPickable::chosen( int which, int heroID ) const
{
	switch(which)
	{
	case 0: //player pick gold
		cb->giveResource(cb->getOwner(heroID),6,val1);
		break;
	case 1: //player pick exp
		cb->changePrimSkill(heroID, 4, val2);
		break;
	default:
		throw std::string("Unhandled treasure choice");
	}
	cb->removeObject(id);
}

void CGWitchHut::initObj()
{
	ability = allowedAbilities[ran()%allowedAbilities.size()];
}

void CGWitchHut::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->getOwner();

	if(h->getSecSkillLevel(ability)) //you alredy know this skill
	{
		iw.text << std::pair<ui8,ui32>(11,172);
		iw.text.replacements.push_back(VLC->generaltexth->skillName[ability]);
	}
	else if(h->secSkills.size() >= SKILL_PER_HERO) //already all skills slots used
	{
		iw.text << std::pair<ui8,ui32>(11,173);
		iw.text.replacements.push_back(VLC->generaltexth->skillName[ability]);
	}
	else //give sec skill
	{
		iw.components.push_back(Component(1, ability, 1, 0));
		iw.text << std::pair<ui8,ui32>(11,171);
		iw.text.replacements.push_back(VLC->generaltexth->skillName[ability]);
		cb->changeSecSkill(h->id,ability,1,true);
	}

	cb->showInfoDialog(&iw);
}
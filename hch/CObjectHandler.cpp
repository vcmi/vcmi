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
#include <boost/lexical_cast.hpp>
#include <boost/random/linear_congruential.hpp>
#include "CTownHandler.h"
#include "CArtHandler.h"
#include "CSoundBase.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/IGameCallback.h"
#include "../CGameState.h"
#include "../lib/NetPacks.h"
#include "../StartInfo.h"
#include "../map.h"

/*
 * CObjectHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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

void IObjectInterface::setProperty( ui8 what, ui32 val )
{}

void CPlayersVisited::setPropertyDer( ui8 what, ui32 val )
{
	if(what == 10)
		players.insert(val);
}

bool CPlayersVisited::hasVisited( ui8 player ) const
{
	return vstd::contains(players,player);
}

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
	return defInfo->height;
}
bool CGObjectInstance::visitableAt(int x, int y) const //returns true if object is visitable at location (x, y) form left top tile of image (x, y in tiles)
{
	if(defInfo==NULL)
	{
		tlog2 << "Warning: VisitableAt for obj "<<id<<": NULL defInfo!\n";
		return false;
	}

	if((defInfo->visitMap[y] >> (7-x) ) & 1)
	{
		return true;
	}

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
	if(cmp.ID==HEROI_TYPE && ID!=HEROI_TYPE)
		return true;
	if(cmp.ID!=HEROI_TYPE && ID==HEROI_TYPE)
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

void CGObjectInstance::setProperty( ui8 what, ui32 val )
{
	switch(what)
	{
	case 1:
		tempOwner = val;
		break;
	case 2:
		blockVisit = val;
		break;
	case 6:
		ID = val;
		break;
	}
	setPropertyDer(what, val);
}

void CGObjectInstance::setPropertyDer( ui8 what, ui32 val )
{}

int3 CGObjectInstance::getSightCenter() const
{
	//return vistiable tile if possible
	for(int i=0; i < 8; i++)
		for(int j=0; j < 6; j++)
			if(visitableAt(i,j))
				return(pos + int3(i-7, j-5, 0));
	return pos;
}

int CGObjectInstance::getSightRadious() const
{
	return 3;
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

unsigned int CGHeroInstance::getTileCost(const TerrainTile &dest, const TerrainTile &from) const
{
	//TODO: check if all creatures are on its native terrain and change cost appropriately

	//base move cost
	unsigned ret = 100;
	
	//if there is road both on dest and src tiles - use road movement cost
	if(dest.malle && from.malle) 
	{
		int road = std::min(dest.malle,from.malle); //used road ID
		switch(road)
		{
		case dirtRoad:
			ret = 75;
			break;
		case grazvelRoad:
			ret = 65;
			break;
		case cobblestoneRoad:
			ret = 50;
			break;
		default:
			tlog1 << "Unknown road type: " << road << "... Something wrong!\n";
			break;
		}
	}
	else 
	{
		ret = std::max(type->heroClass->terrCosts[dest.tertype],type->heroClass->terrCosts[from.tertype]); //take cost of worse of two tiles
		ret = std::max(ret - 25*unsigned(getSecSkillLevel(0)), 100u); //reduce 25% of terrain penalty for each pathfinding level
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

si32 CGHeroInstance::manaLimit() const
{
	double modifier = 1.0;
	switch(getSecSkillLevel(24)) //intelligence level
	{
	case 1:		modifier+=0.25;		break;
	case 2:		modifier+=0.5;		break;
	case 3:		modifier+=1.0;		break;
	}
	return si32(10*getPrimSkillLevel(3)*modifier);
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
int CGHeroInstance::getPrimSkillLevel(int id) const
{
	int ret = primSkills[id];
	for(std::list<HeroBonus>::const_iterator i=bonuses.begin(); i != bonuses.end(); i++)
		if(i->type == HeroBonus::PRIMARY_SKILL && i->subtype==id)
			ret += i->val;
	return ret;
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
	int ret = std::min(2000, 1270+70*lowestSpeed(this)),
		bonus = valOfBonuses(HeroBonus::MOVEMENT) + (onLand ? valOfBonuses(HeroBonus::LAND_MOVEMENT) : valOfBonuses(HeroBonus::SEA_MOVEMENT));

	double modifier = 0;
	if(onLand)
	{
		//logistics:
		switch(getSecSkillLevel(2))
		{
		case 1:
			modifier = 0.1;
			break;
		case 2:
			modifier = 0.2;
			break;
		case 3:
			modifier = 0.3;
			break;
		}
	}
	else
	{
		//navigation:
		switch(getSecSkillLevel(2))
		{
		case 1:
			modifier = 0.5;
			break;
		case 2:
			modifier = 1.0;
			break;
		case 3:
			modifier = 1.5;
			break;
		}
	}
	return int(ret + ret*modifier) + bonus;
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
	ID = HEROI_TYPE;
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
	if(ID == HEROI_TYPE)
		initHeroDefInfo();
	if(!type)
		type = VLC->heroh->heroes[subID];
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

		int x = 0; //how many stacks will hero receives <1 - 3>
		pom = ran()%100;
		if(pom < 5)
			x = 1;
		else if(pom < 67)
			x = 2;
		else
			x = 3;

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
			pom = type->highStack[x] - type->lowStack[x];
			army.slots[x-pom2].second = ran()%(pom+1) + type->lowStack[x];
			army.formation = false;
		}
	}
	hoverName = VLC->generaltexth->allTexts[15];
	boost::algorithm::replace_first(hoverName,"%s",name);
	boost::algorithm::replace_first(hoverName,"%s", type->heroClass->name);

	//clear all bonuses from artifacts (if present) and give them again
	std::remove_if(bonuses.begin(), bonuses.end(), boost::bind(HeroBonus::IsFrom,_1,HeroBonus::ARTIFACT,0xffffff));
	for (std::map<ui16,ui32>::iterator ari = artifWorn.begin(); ari != artifWorn.end(); ari++)
	{
		CArtifact &art = VLC->arth->artifacts[ari->second];
		for(std::list<HeroBonus>::iterator i = art.bonuses.begin(); i != art.bonuses.end(); i++)
			bonuses.push_back(*i);
	}
}

void CGHeroInstance::initHeroDefInfo()
{
	if(!defInfo  ||  defInfo->id != HEROI_TYPE)
	{
		defInfo = new CGDefInfo();
		defInfo->id = HEROI_TYPE;
		defInfo->subid = subID;
		defInfo->printPriority = 0;
		defInfo->visitDir = 0xff;
	}
	for(int i=0;i<6;i++)
	{
		defInfo->blockMap[i]=255;
		defInfo->visitMap[i]=0;
	}
	defInfo->handler=NULL;
	defInfo->blockMap[5] = 253;
	defInfo->visitMap[5] = 2;
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
	if (ID == HEROI_TYPE) //hero
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
	else if(ID == 62) //prison
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.soundID = soundBase::ROGUE;

		if(cb->getHeroCount(h->tempOwner,false) < 8) //free hero slot
		{
			cb->changeObjPos(id,pos+int3(1,0,0),0);
			cb->setObjProperty(id,6,HEROI_TYPE); //set ID to 34
			cb->giveHero(id,h->tempOwner); //recreates def and adds hero to player

			iw.text << std::pair<ui8,ui32>(11,102);
		}
		else //already 8 wandering heroes
		{
			iw.text << std::pair<ui8,ui32>(11,103);
		}

		cb->showInfoDialog(&iw);
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

int CGHeroInstance::getCurrentMorale( int stack, bool town ) const
{
	int ret = 0;
	std::vector<std::pair<int,std::string> > mods = getCurrentMoraleModifiers(stack,town);
	for(int i=0; i < mods.size(); i++)
		ret += mods[i].first;
	if(ret > 3)
		return 3;
	if(ret < -3)
		return -3;
	return ret;
}

std::vector<std::pair<int,std::string> > CGHeroInstance::getCurrentMoraleModifiers( int stack/*=-1*/, bool town/*=false*/ ) const
{
	//TODO: check if stack is undead/mechanic/elemental => always neutrl morale
	std::vector<std::pair<int,std::string> > ret;

	//various morale bonuses (from buildings, artifacts, etc)
	for(std::list<HeroBonus>::const_iterator i=bonuses.begin(); i != bonuses.end(); i++)
	{
		if(i->type == HeroBonus::MORALE   ||   i->type == HeroBonus::MORALE_AND_LUCK)
		{
			ret.push_back(std::make_pair(i->val, i->description));
		}
	}

	//leadership
	if(getSecSkillLevel(6)) 
		ret.push_back(std::make_pair(getSecSkillLevel(6),VLC->generaltexth->arraytxt[104+getSecSkillLevel(6)]));

	//town structures
	if(town && visitedTown)
	{
		if(visitedTown->subID == 0  &&  vstd::contains(visitedTown->builtBuildings,22)) //castle, brotherhood of sword built
			ret.push_back(std::pair<int,std::string>(2,VLC->generaltexth->buildings[0][22].first + " +2"));
		else if(vstd::contains(visitedTown->builtBuildings,5)) //tavern is built
			ret.push_back(std::pair<int,std::string>(2,VLC->generaltexth->buildings[0][5].first + " +1"));
	}

	//number of alignments and presence of undead
	if(stack>=0)
	{
		bool archangelInArmy = false;
		std::set<si8> factions;
		for(std::map<si32,std::pair<ui32,si32> >::const_iterator i=army.slots.begin(); i!=army.slots.end(); i++)
		{
			factions.insert(VLC->creh->creatures[i->second.first].faction);
			if(i->second.first == 13)
				archangelInArmy = true;
		}

		if(factions.size() == 1)
			ret.push_back(std::pair<int,std::string>(1,VLC->generaltexth->arraytxt[115])); //All troops of one alignment +1
		else
		{
			if(VLC->generaltexth->arraytxt[114].length() <= 100)
			{
				char buf[150];
				std::sprintf(buf,VLC->generaltexth->arraytxt[114].c_str(),factions.size(),2-factions.size());
				ret.push_back(std::pair<int,std::string>(2-factions.size(),buf)); //Troops of %d alignments %d
			}
			else
			{
				ret.push_back(std::pair<int,std::string>(2-factions.size(),"")); //Troops of %d alignments %d
			}
		}

		if(vstd::contains(factions,4))
			ret.push_back(std::pair<int,std::string>(-1,VLC->generaltexth->arraytxt[116])); //Undead in group -1

		if(archangelInArmy)
		{
			char buf[100];
			sprintf(buf,VLC->generaltexth->arraytxt[117].c_str(),VLC->creh->creatures[13].namePl.c_str());
			ret.push_back(std::pair<int,std::string>(1,buf)); //%s in group +1
		}
	}

	return ret;
}

int CGHeroInstance::getCurrentLuck( int stack/*=-1*/, bool town/*=false*/ ) const
{
	int ret = 0;
	std::vector<std::pair<int,std::string> > mods = getCurrentLuckModifiers(stack,town);
	for(int i=0; i < mods.size(); i++)
		ret += mods[i].first;
	if(ret > 3)
		return 3;
	if(ret < -3)
		return -3;
	return ret;
}

std::vector<std::pair<int,std::string> > CGHeroInstance::getCurrentLuckModifiers( int stack/*=-1*/, bool town/*=false*/ ) const
{
	std::vector<std::pair<int,std::string> > ret;

	//various morale bonuses (from buildings, artifacts, etc)
	for(std::list<HeroBonus>::const_iterator i=bonuses.begin(); i != bonuses.end(); i++)
		if(i->type == HeroBonus::LUCK   ||   i->type == HeroBonus::MORALE_AND_LUCK)
			ret.push_back(std::make_pair(i->val, i->description));

	//luck skill
	if(getSecSkillLevel(9)) 
		ret.push_back(std::make_pair(getSecSkillLevel(9),VLC->generaltexth->arraytxt[73+getSecSkillLevel(9)]));

	return ret;
}

const HeroBonus * CGHeroInstance::getBonus( int from, int id ) const
{
	for (std::list<HeroBonus>::const_iterator i=bonuses.begin(); i!=bonuses.end(); i++)
		if(i->source == from  &&  i->id == id)
			return &*i;
	return NULL;
}

void CGHeroInstance::setPropertyDer( ui8 what, ui32 val )
{
	if(what == 3)
		army.slots[0].second = val;
}

double CGHeroInstance::getHeroStrength() const
{
	return sqrt((1.0 + 0.05*getPrimSkillLevel(0)) * (1.0 + 0.05*getPrimSkillLevel(1)));
}

int CGHeroInstance::getTotalStrength() const
{
	double ret = getHeroStrength() * getArmyStrength();
	return (int) ret;
}

ui8 CGHeroInstance::getSpellSchoolLevel(const CSpell * spell) const
{
	//TODO: skill level may be different on special terrain
	ui8 skill = 0; //skill level

	if(spell->fire)
		skill = std::max(skill,getSecSkillLevel(14));
	if(spell->air)
		skill = std::max(skill,getSecSkillLevel(15));
	if(spell->water)
		skill = std::max(skill,getSecSkillLevel(16));
	if(spell->earth)
		skill = std::max(skill,getSecSkillLevel(17));

	return skill;
}

int3 CGHeroInstance::getSightCenter() const
{
	return getPosition(false);
}

int CGHeroInstance::getSightRadious() const
{
	return 5 + getSecSkillLevel(3) + valOfBonuses(HeroBonus::SIGHT_RADIOUS); //default + scouting
}

si32 CGHeroInstance::manaRegain() const
{
	return 1 + getSecSkillLevel(8) + valOfBonuses(HeroBonus::MANA_REGENERATION); //1 + Mysticism level 
}

int CGHeroInstance::valOfBonuses( HeroBonus::BonusType type ) const
{
	int ret = 0;
	for(std::list<HeroBonus>::const_iterator i=bonuses.begin(); i != bonuses.end(); i++)
		if(i->type == type)
			ret += i->val;
	return ret;
}

int CGTownInstance::getSightRadious() const //returns sight distance
{
	return 5;
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

int3 CGTownInstance::getSightCenter() const
{
	return pos - int3(2,0,0);
}

void CGVisitableOPH::onHeroVisit( const CGHeroInstance * h ) const
{
	if(visitors.find(h->id)==visitors.end())
	{
		onNAHeroVisit(h->id, false);
		switch(ID)
		{
		case 102: //tree
		case 4: //arena
		case 41://library
		case 47: //School of Magic
		case 107://School of War
			break;
		default:
			cb->setObjProperty(id,4,h->id); //add to the visitors
			break;
		}
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
	if(result) //player agreed to give res for exp
	{
		cb->giveResource(cb->getOwner(heroID),resType,-resVal); //take resource
		cb->changePrimSkill(heroID,4,expVal); //give exp
		cb->setObjProperty(id,4,heroID); //add to the visitors
	}
}
void CGVisitableOPH::onNAHeroVisit(int heroID, bool alreadyVisited) const
{
	int id=0, subid=0, ot=0, val=1, sound = 0;
	switch(ID)
	{
	case 4: //arena
		sound = soundBase::NOMAD;
		ot = 0;
		break;
	case 51: //mercenary camp
		sound = soundBase::NOMAD;
		subid=0;
		ot=80;
		break;
	case 23: //marletto tower
		sound = soundBase::NOMAD;
		subid=1;
		ot=39;
		break;
	case 61:
		sound = soundBase::gazebo;
		subid=2;
		ot=100;
		break;
	case 32:
		sound = soundBase::GETPROTECTION;
		subid=3;
		ot=59;
		break;
	case 100:
		sound = soundBase::gazebo;
		id=5;
		ot=143;
		val=1000;
		break;
	case 102:
		sound = soundBase::gazebo;
		id = 5;
		subid = 1;
		ot = 146;
		val = 1;
		break;
	case 41:
		sound = soundBase::gazebo;
		ot = 66;
		break;
	case 47: //School of Magic
		sound = soundBase::faerie;
		ot = 71;
		break;
	case 107://School of War
		sound = soundBase::MILITARY;
		ot = 158;
		break;
	}
	if (!alreadyVisited)
	{
		switch (ID)
		{
		case 4: //arena
			{
				BlockingDialog sd(false,true);
				sd.soundID = sound;
				sd.text << std::pair<ui8,ui32>(11,ot);
				sd.components.push_back(Component(0,0,2,0));
				sd.components.push_back(Component(0,1,2,0));
				sd.player = cb->getOwner(heroID);
				cb->showBlockingDialog(&sd,boost::bind(&CGVisitableOPH::arenaSelected,this,heroID,_1));
				return;
			}
		case 51:
		case 23:
		case 61:
		case 32:
			{
				cb->changePrimSkill(heroID,subid,val);
				InfoWindow iw;
				iw.soundID = sound;
				iw.components.push_back(Component(0,subid,val,0));
				iw.text << std::pair<ui8,ui32>(11,ot);
				iw.player = cb->getOwner(heroID);
				cb->showInfoDialog(&iw);
				break;
			}
		case 100: //give exp
			{
				InfoWindow iw;
				iw.soundID = sound;
				iw.components.push_back(Component(id,subid,val,0));
				iw.player = cb->getOwner(heroID);
				iw.text << std::pair<ui8,ui32>(11,ot);
				iw.soundID = soundBase::gazebo;
				cb->showInfoDialog(&iw);
				cb->changePrimSkill(heroID,4,val);
				break;
			}
		case 102://tree
			{
				const CGHeroInstance *h = cb->getHero(heroID);
				val = VLC->heroh->reqExp(h->level+val) - VLC->heroh->reqExp(h->level);
				if(!ttype)
				{
					cb->setObjProperty(this->id,4,heroID); //add to the visitors
					InfoWindow iw;
					iw.soundID = sound;
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
						iw.soundID = sound;
						iw.player = h->tempOwner;
						iw.text << std::pair<ui8,ui32>(11,ot);
						cb->showInfoDialog(&iw);
						return;
					}

					BlockingDialog sd(true,false);
					sd.soundID = sound;
					sd.player = cb->getOwner(heroID);
					sd.text << std::pair<ui8,ui32>(11,ot);
					sd.components.push_back(Component(id,subid,val,0));
					cb->showBlockingDialog(&sd,boost::bind(&CGVisitableOPH::treeSelected,this,heroID,res,resval,val,_1));
				}
				break;
			}
		case 41://library of enlightenment
			{
				const CGHeroInstance *h = cb->getHero(heroID);
				if(h->level  <  10 - 2*h->getSecSkillLevel(4)) //not enough level
				{
					InfoWindow iw;
					iw.soundID = sound;
					iw.player = cb->getOwner(heroID);
					iw.text << std::pair<ui8,ui32>(11,68);
					cb->showInfoDialog(&iw);
				}
				else
				{
					cb->setObjProperty(this->id,4,heroID); //add to the visitors
					cb->changePrimSkill(heroID,0,2);
					cb->changePrimSkill(heroID,1,2);
					cb->changePrimSkill(heroID,2,2);
					cb->changePrimSkill(heroID,3,2);
					InfoWindow iw;
					iw.soundID = sound;
					iw.player = cb->getOwner(heroID);
					iw.text << std::pair<ui8,ui32>(11,66);
					cb->showInfoDialog(&iw);
				}
				break;
			}
		case 47: //School of Magic
		case 107://School of War
			{
				int skill = (ID==47 ? 2 : 0);
				if(cb->getResource(cb->getOwner(heroID),6) < 1000) //not enough resources
				{
					InfoWindow iw;
					iw.soundID = sound;
					iw.player = cb->getOwner(heroID);
					iw.text << std::pair<ui8,ui32>(MetaString::ADVOB_TXT,ot+2);
					cb->showInfoDialog(&iw);
				}
				else
				{
					BlockingDialog sd(true,true);
					sd.soundID = sound;
					sd.player = cb->getOwner(heroID);
					sd.text << std::pair<ui8,ui32>(11,ot);
					sd.components.push_back(Component(Component::PRIM_SKILL, skill, +1, 0));
					sd.components.push_back(Component(Component::PRIM_SKILL, skill+1, +1, 0));
					cb->showBlockingDialog(&sd,boost::bind(&CGVisitableOPH::schoolSelected,this,heroID,_1));
				}
			}
			break;
		}
	}
	else
	{
		ot++;
		InfoWindow iw;
		iw.soundID = sound;
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
	case 41:
		break;
	case 47: //School of Magic
		pom = 9;
		break;
	case 107://School of War
		pom = 10;
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
	cb->changePrimSkill(heroID,primSkill-1,2);
}

void CGVisitableOPH::setPropertyDer( ui8 what, ui32 val )
{
	if(what == 4)
		visitors.insert(val);
}

void CGVisitableOPH::schoolSelected(int heroID, ui32 which) const
{
	if(!which) //player refused to pay
		return;

	int base = (ID == 47  ?  2  :  0);
	cb->setObjProperty(id,4,heroID); //add to the visitors
	cb->giveResource(cb->getOwner(heroID),6,-1000); //take 1000 gold
	cb->changePrimSkill(heroID, base + which-1, +1); //give appropriate skill
}

bool CArmedInstance::needsLastStack() const
{
	return false;
}

int CArmedInstance::getArmyStrength() const
{
	int ret = 0;
	for(std::map<si32,std::pair<ui32,si32> >::const_iterator i=army.slots.begin(); i!=army.slots.end(); i++)
		ret += VLC->creh->creatures[i->second.first].AIValue * i->second.second;
	return ret;
}

void CGCreature::onHeroVisit( const CGHeroInstance * h ) const
{
	int action = takenAction(h);
	switch( action ) //decide what we do...
	{
	case -2: //fight
		fight(h);
		break;
	case -1: //flee
		{
			flee(h);
			break;
		}
	case 0: //join for free
		{
			BlockingDialog ynd(true,false);
			ynd.player = h->tempOwner;
			ynd.text << std::pair<ui8,ui32>(11,86); 
			ynd.text.replacements.push_back(VLC->creh->creatures[subID].namePl);
			cb->showBlockingDialog(&ynd,boost::bind(&CGCreature::joinDecision,this,h,0,_1));
			break;
		}
	default: //join for gold
		{
			assert(action > 0);

			//ask if player agrees to pay gold
			BlockingDialog ynd(true,false);
			ynd.player = h->tempOwner;
			std::string tmp = VLC->generaltexth->advobtxt[90];
			boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(army.slots.find(0)->second.second));
			boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(action));
			boost::algorithm::replace_first(tmp,"%s",VLC->creh->creatures[subID].namePl);
			ynd.text << tmp;
			cb->showBlockingDialog(&ynd,boost::bind(&CGCreature::joinDecision,this,h,action,_1));
			break;
		}
	}

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
	blockVisit = true;
	switch(character)
	{
	case 0:
		character = 0;
		break;
	case 1:
		character = 1 + ran()%7;
		break;
	case 2:
		character = 1 + ran()%10;
		break;
	case 3:
		character = 4 + ran()%7;
		break;
	case 4:
		character = 10;
		break;
	}

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

int CGCreature::takenAction(const CGHeroInstance *h, bool allowJoin) const
{
	double hlp = h->getTotalStrength() / getArmyStrength();

	if(!character) //compliant creatures will always join
		return 0;
	else if(allowJoin)//test for joining
	{
		int factor;
		if(hlp >= 7)
			factor = 11;
		else if(hlp >= 1)
			factor = 2*(hlp-1);
		else if(hlp >= 0.5)
			factor = -1;
		else if(hlp >= 0.333)
			factor = -2;
		else
			factor = -3;

		int sympathy = 0;

		std::set<ui32> myKindCres; //what creatures are the same kind as we
		myKindCres.insert(subID); //we
		myKindCres.insert(VLC->creh->creatures[subID].upgrades.begin(),VLC->creh->creatures[subID].upgrades.end()); //our upgrades
		for(std::vector<CCreature>::iterator i=VLC->creh->creatures.begin(); i!=VLC->creh->creatures.end(); i++)
			if(vstd::contains(i->upgrades,id)) //it's our base creatures
				myKindCres.insert(i->idNumber);

		int count = 0, //how many creatures of our kind has hero
			totalCount = 0;
		for (std::map<si32,std::pair<ui32,si32> >::const_iterator i = h->army.slots.begin(); i != h->army.slots.end(); i++)
		{
			if(vstd::contains(myKindCres,i->second.first))
				count += i->second.second;
			totalCount += i->second.second;
		}

		if(count*2 > totalCount)
			sympathy++;
		if(count)
			sympathy++;
		

		int charisma = factor + h->getSecSkillLevel(4) + sympathy;
		if(charisma >= character) //creatures might join...
		{
			if(h->getSecSkillLevel(4) + sympathy + 1 >= character)
				return 0; //join for free
			else if(h->getSecSkillLevel(4) * 2  +  sympathy  +  1 >= character)
				return VLC->creh->creatures[subID].cost[6] * army.slots.find(0)->second.second; //join for gold
		}
	}

	//we are still here - creatures not joined heroes, test for fleeing

	//TODO: it's provisional formula, should be replaced with original one (or something closer to it)
	//TODO: should be deterministic (will be needed for Vision spell)
	int hlp2 = (int) (hlp - 2)*1000;
	if(!neverFlees   
		&& hlp2 >= 0 
		&& rand()%2000 < hlp2
	)
		return -1; //flee
	else
		return -2; //fight

}

void CGCreature::fleeDecision(const CGHeroInstance *h, ui32 pursue) const
{
	if(pursue)
	{
		fight(h);
	}
	else
	{
		cb->removeObject(id);
	}
}

void CGCreature::joinDecision(const CGHeroInstance *h, int cost, ui32 accept) const
{
	if(!accept)
	{
		if(takenAction(h,false) == -1) //they flee
		{
			flee(h);
		}
		else //they fight
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text << std::pair<ui8,ui32>(11,87);  //Insulted by your refusal of their offer, the monsters attack!
			cb->showInfoDialog(&iw);
			fight(h);
		}
	}
	else //accepted
	{
		if (cb->getResource(h->tempOwner,6) < cost) //player don't have enough gold!
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text << std::pair<ui8,ui32>(1,29);  //You don't have enough gold
			cb->showInfoDialog(&iw);

			//act as if player refused
			joinDecision(h,cost,true);
			return;
		}

		//take gold
		if(cost)
			cb->giveResource(h->tempOwner,6,-cost);

		int slot = h->army.getSlotFor(subID);
		if(slot >= 0) //there is place
		{
			//add creatures
			SetGarrisons sg;
			sg.garrs[h->id] = h->army;
			if(vstd::contains(h->army.slots,slot)) //add to already present stack
			{
				sg.garrs[h->id].slots[slot].second += army.slots.find(0)->second.second;
			}
			else //add as a new stack
			{
				sg.garrs[h->id].slots[slot] = army.slots.find(0)->second;
			}
			cb->sendAndApply(&sg);
			cb->removeObject(id);
		}
		else
		{
			cb->showGarrisonDialog(id,h->id,boost::bind(&IGameCallback::removeObject,cb,id)); //show garrison window and remove ourselves from map when player ends
		}
	}
}

void CGCreature::fight( const CGHeroInstance *h ) const
{
	cb->startBattleI(h->id,army,pos,boost::bind(&CGCreature::endBattle,this,_1));
}

void CGCreature::flee( const CGHeroInstance * h ) const
{
	BlockingDialog ynd(true,false);
	ynd.player = h->tempOwner;
	ynd.text << std::pair<ui8,ui32>(11,91); 
	ynd.text.replacements.push_back(VLC->creh->creatures[subID].namePl);
	cb->showBlockingDialog(&ynd,boost::bind(&CGCreature::fleeDecision,this,h,_1));
}

void CGMine::onHeroVisit( const CGHeroInstance * h ) const
{
	if(subID == 7) //TODO: support for abandoned mine
		return;

	if(h->tempOwner == tempOwner) //we're visiting our mine
	{
		cb->showGarrisonDialog(id,h->id,0);
		return; 
	}

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
	iw.soundID = soundBase::FLAGMINE;
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
			BlockingDialog ynd(true,false);
			ynd.player = h->getOwner();
			ynd.text << message;
			cb->showBlockingDialog(&ynd,boost::bind(&CGResource::fightForRes,this,_1,h));
		}
		else
		{
			fightForRes(1,h);
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

void CGResource::fightForRes(ui32 agreed, const CGHeroInstance *h) const
{
	if(agreed)
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
	int mid, sound = 0;
	switch (ID)
	{
	case 55: //mystical garden
		sound = soundBase::experience;
		mid = 92;
		break;
	case 112://windmill
		sound = soundBase::GENIE;
		mid = 170;
		break;
	case 109://waterwheel
		sound = soundBase::GENIE;
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
		iw.soundID = sound;
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
			if(cb->getDate(0)<8)
				val = 500;
			else
				val = 1000;
		}
		cb->giveResource(h->tempOwner,sub,val);
		InfoWindow iw;
		iw.soundID = sound;
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

void CGVisitableOPW::setPropertyDer( ui8 what, ui32 val )
{
	if(what == 5)
		visited = val;
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
	if(!army.slots.size())
	{
		if(ID == 5)
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.components.push_back(Component(4,subID,0,0));
			if(message.length())
				iw.text <<  message;
			else
				iw.text << std::pair<ui8,ui32>(12,subID);
			cb->showInfoDialog(&iw);
		}
		pick(h);
	}
	else
	{
		if(message.size())
		{
			BlockingDialog ynd(true,false);
			ynd.player = h->getOwner();
			ynd.text << message;
			cb->showBlockingDialog(&ynd,boost::bind(&CGArtifact::fightForArt,this,_1,h));
		}
		else
		{
			fightForArt(0,h);
		}
	}
}

void CGArtifact::pick(const CGHeroInstance * h) const
{
	if(ID == 5) //Artifact
	{
		cb->giveHeroArtifact(subID,h->id,-2);
	}
	else if(ID == 93) // Spell scroll 
	{
		//TODO: support for the spell scroll
	}
	cb->removeObject(id);
}

void CGArtifact::fightForArt( ui32 agreed, const CGHeroInstance *h ) const
{
	if(agreed)
		cb->startBattleI(h->id,army,pos,boost::bind(&CGArtifact::endBattle,this,_1,h));
}

void CGArtifact::endBattle( BattleResult *result, const CGHeroInstance *h ) const
{
	if(result->winner == 0) //attacker won
		pick(h);
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
			iw.soundID = soundBase::experience;
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
				iw.soundID = soundBase::treasure;
				iw.player = h->tempOwner;
				iw.components.push_back(Component(4,val1,1,0));
				iw.text << std::pair<ui8,ui32>(11,145);
				iw.text.replacements.push_back(VLC->arth->artifacts[val1].Name());
				cb->showInfoDialog(&iw);
				break;
			}
			else
			{
				BlockingDialog sd(false,true);
				sd.player = h->tempOwner;
				sd.text << std::pair<ui8,ui32>(11,146);
				sd.components.push_back(Component(2,6,val1,0));
				sd.components.push_back(Component(5,0,val2,0));
				sd.soundID = soundBase::chest;
				boost::function<void(ui32)> fun = boost::bind(&CGPickable::chosen,this,_1,h->id);
				cb->showBlockingDialog(&sd,fun);
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
	case 1: //player pick gold
		cb->giveResource(cb->getOwner(heroID),6,val1);
		break;
	case 2: //player pick exp
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
	iw.soundID = soundBase::gazebo;
	iw.player = h->getOwner();
	if(!hasVisited(h->tempOwner))
		cb->setObjProperty(id,10,h->tempOwner);

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

const std::string & CGWitchHut::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID];
	if(hasVisited(cb->getCurrentPlayer())) //TODO: use local player, not current
	{
		hoverName += "\n" + VLC->generaltexth->allTexts[356]; // + (learn %s)
		boost::algorithm::replace_first(hoverName,"%s",VLC->generaltexth->skillName[ability]);
		if(cb->getSelectedHero(cb->getCurrentPlayer())->getSecSkillLevel(ability)) //hero knows that ability
			hoverName += "\n\n" + VLC->generaltexth->allTexts[357]; // (Already learned)
	}
	return hoverName;
}


void CGDwelling::onHeroVisit( const CGHeroInstance * h ) const
{

}

void CGDwelling::initObj()
{

}

void CGBonusingObject::onHeroVisit( const CGHeroInstance * h ) const
{
	bool visited = h->getBonus(HeroBonus::OBJECT,ID);
	int messageID, bonusType, bonusVal;
	int bonusMove = 0, sound = -1;
	InfoWindow iw;
	iw.player = h->tempOwner;
	GiveBonus gbonus;
	gbonus.hid = h->id;
	gbonus.bonus.duration = HeroBonus::ONE_BATTLE;
	gbonus.bonus.source = HeroBonus::OBJECT;
	gbonus.bonus.id = ID;

	switch(ID)
	{
	case 14: //swan pond
		messageID = 29;
		sound = soundBase::LUCK;
		gbonus.bonus.type = HeroBonus::LUCK;
		gbonus.bonus.val = 2;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,67);
		bonusMove = -h->movement;
		break;
	case 28: //Faerie Ring
		messageID = 49;
		sound = soundBase::LUCK;
		gbonus.bonus.type = HeroBonus::LUCK;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,71);
		break;
	case 30: //fountain of fortune
		messageID = 55;
		sound = soundBase::LUCK;
		gbonus.bonus.type = HeroBonus::LUCK;
		gbonus.bonus.val = rand()%5 - 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,69);
		gbonus.bdescr.replacements.push_back((gbonus.bonus.val<0 ? "-" : "+") + boost::lexical_cast<std::string>(gbonus.bonus.val));
		break;
	case 38: //idol of fortune
		messageID = 62;
		iw.soundID = soundBase::experience;
		if(cb->getDate(1) == 7) //7th day of week
			gbonus.bonus.type = HeroBonus::MORALE_AND_LUCK;
		else
			gbonus.bonus.type = (cb->getDate(1)%2) ? HeroBonus::LUCK : HeroBonus::MORALE;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,68);
		break;
	case 64: //Rally Flag
		sound = soundBase::MORALE;
		messageID = 111;
		gbonus.bonus.type = HeroBonus::MORALE_AND_LUCK;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,102);
		bonusMove = 400;
		break;
	case 56: //oasis
		messageID = 95;
		gbonus.bonus.type = HeroBonus::MORALE;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,95);
		bonusMove = 800;
		break;
	case 96: //temple
		messageID = 140;
		iw.soundID = soundBase::temple;
		gbonus.bonus.type = HeroBonus::MORALE;
		if(cb->getDate(1)==7) //sunday
		{
			gbonus.bonus.val = 2;
			gbonus.bdescr <<  std::pair<ui8,ui32>(6,97);
		}
		else
		{
			gbonus.bonus.val = 1;
			gbonus.bdescr <<  std::pair<ui8,ui32>(6,96);
		}
		break;
	case 110://Watering Hole
		sound = soundBase::MORALE;
		messageID = 166;
		gbonus.bonus.type = HeroBonus::MORALE;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,100);
		bonusMove = 400;
		break;
	case 31: //Fountain of Youth
		sound = soundBase::MORALE;
		messageID = 57;
		gbonus.bonus.type = HeroBonus::MORALE;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,103);
		bonusMove = 400;
		break;
	}
	if(visited)
	{
		if(ID==64 || ID==96  ||  ID==56)
			messageID--;
		else
			messageID++;
	}
	else
	{
		if(gbonus.bonus.type == HeroBonus::MORALE   ||   gbonus.bonus.type == HeroBonus::MORALE_AND_LUCK)
			iw.components.push_back(Component(8,0,gbonus.bonus.val,0));
		if(gbonus.bonus.type == HeroBonus::LUCK   ||   gbonus.bonus.type == HeroBonus::MORALE_AND_LUCK)
			iw.components.push_back(Component(9,0,gbonus.bonus.val,0));
		cb->giveHeroBonus(&gbonus);
		if(bonusMove) //swan pond - take all move points
		{
			SetMovePoints smp;
			smp.hid = h->id;
			smp.val = h->movement + bonusMove;
			cb->setMovePoints(&smp);
		}
	}
	iw.soundID = sound;
	iw.text << std::pair<ui8,ui32>(11,messageID);
	cb->showInfoDialog(&iw);
}

const std::string & CGBonusingObject::getHoverText() const
{
	const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
	hoverName = VLC->generaltexth->names[ID];
	if(h) 
	{
		if(!h->getBonus(HeroBonus::OBJECT,ID))
			hoverName += " " + VLC->generaltexth->allTexts[353]; //not visited
		else
			hoverName += " " + VLC->generaltexth->allTexts[352]; //visited
	}
	return hoverName;
}

void CGMagicWell::onHeroVisit( const CGHeroInstance * h ) const
{
	int message;
	InfoWindow iw;
	iw.soundID = soundBase::faerie;
	iw.player = h->tempOwner;
	if(h->getBonus(HeroBonus::OBJECT,ID)) //has already visited Well today
	{
		message = 78;
	}
	else if(h->mana < h->manaLimit())
	{
		GiveBonus gbonus;
		gbonus.bonus.type = HeroBonus::NONE;
		gbonus.hid = h->id;
		gbonus.bonus.duration = HeroBonus::ONE_DAY;
		gbonus.bonus.source = HeroBonus::OBJECT;
		gbonus.bonus.id = ID;
		cb->giveHeroBonus(&gbonus);
		cb->setManaPoints(h->id,h->manaLimit());
		message = 77;
	}
	else
	{
		message = 79;
	}
	iw.text << std::pair<ui8,ui32>(11,message); //"A second drink at the well in one day will not help you."
	cb->showInfoDialog(&iw);
}

const std::string & CGMagicWell::getHoverText() const
{
	const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
	hoverName = VLC->generaltexth->names[ID];
	if(h) 
	{
		if(!h->getBonus(HeroBonus::OBJECT,ID))
			hoverName += " " + VLC->generaltexth->allTexts[353]; //not visited
		else
			hoverName += " " + VLC->generaltexth->allTexts[352]; //visited
	}
	return hoverName;
}

void CGEvent::onHeroVisit( const CGHeroInstance * h ) const
{
	if(!(availableFor & (1 << h->tempOwner))) 
		return;
	if(cb->getPlayerSettings(h->tempOwner)->human)
	{
		if(humanActivate)
			activated(h);
	}
	else if(computerActivate)
		activated(h);
}

void CGEvent::endBattle( const CGHeroInstance *h, BattleResult *result ) const
{
	if(result->winner)
		return;

	giveContents(h,true);
}

void CGEvent::activated( const CGHeroInstance * h ) const
{
	if(army)
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text << message;
		cb->showInfoDialog(&iw);
		cb->startBattleI(h->id,army,pos,boost::bind(&CGEvent::endBattle,this,h,_1));
	}
	else
	{
		giveContents(h,false);
	}
}

void CGEvent::giveContents( const CGHeroInstance *h, bool afterBattle ) const
{
	InfoWindow iw;
	iw.player = h->getOwner();

	bool changesPrimSkill = false;
	for (int i = 0; i < primskills.size(); i++)
	{
		if(primskills[i])
		{
			changesPrimSkill = true;
			break;
		}
	}

	if(gainedExp || changesPrimSkill || abilities.size())
	{
		getText(iw,afterBattle,175,h);

		if(gainedExp)
			iw.components.push_back(Component(Component::EXPERIENCE,0,gainedExp,0));
		for(int i=0; i<primskills.size(); i++)
			if(primskills[i])
				iw.components.push_back(Component(Component::PRIM_SKILL,i,primskills[i],0));

		for(int i=0; i<abilities.size(); i++)
			iw.components.push_back(Component(Component::SEC_SKILL,abilities[i],abilityLevels[i],0));

		cb->showInfoDialog(&iw);

		//give exp
		if(gainedExp)
			cb->changePrimSkill(h->id,5,gainedExp,false);
		//give prim skills
		for(int i=0; i<primskills.size(); i++)
			if(primskills[i])
				cb->changePrimSkill(h->id,i,primskills[i],false);

		//give sec skills
		for(int i=0; i<abilities.size(); i++)
		{
			int curLev = h->getSecSkillLevel(abilities[i]);

			if(curLev  &&  curLev < abilityLevels[i]
				|| h->secSkills.size() < SKILL_PER_HERO )
			{
				cb->changeSecSkill(h->id,abilities[i],abilityLevels[i],true);
			}
		}

	}

	if(spells.size())
	{
		std::set<ui32> spellsToGive;
		iw.components.clear();
		for(int i=0; i<spells.size(); i++)
		{
			iw.components.push_back(Component(Component::SPELL,spells[i],0,0));
			spellsToGive.insert(spells[i]);
		}
		if(spellsToGive.size())
		{
			cb->changeSpells(h->id,true,spellsToGive);
			cb->showInfoDialog(&iw);
		}
	}

	if(manaDiff)
	{
		getText(iw,afterBattle,manaDiff,176,177,h);
		iw.components.push_back(Component(Component::PRIM_SKILL,5,manaDiff,0));
		cb->showInfoDialog(&iw);
		cb->setManaPoints(h->id, h->mana + manaDiff);
	}

	if(moraleDiff)
	{
		getText(iw,afterBattle,moraleDiff,178,179,h);
		iw.components.push_back(Component(Component::MORALE,0,moraleDiff,0));
		cb->showInfoDialog(&iw);
		GiveBonus gb;
		gb.bonus = HeroBonus(HeroBonus::ONE_BATTLE,HeroBonus::MORALE,HeroBonus::OBJECT,moraleDiff,id,"");
		gb.hid = h->id;
		cb->giveHeroBonus(&gb);
	}

	if(luckDiff)
	{
		getText(iw,afterBattle,luckDiff,180,181,h);
		iw.components.push_back(Component(Component::LUCK,0,luckDiff,0));
		cb->showInfoDialog(&iw);
		GiveBonus gb;
		gb.bonus = HeroBonus(HeroBonus::ONE_BATTLE,HeroBonus::LUCK,HeroBonus::OBJECT,luckDiff,id,"");
		gb.hid = h->id;
		cb->giveHeroBonus(&gb);
	}

	iw.components.clear();
	iw.text.clear();
	for(int i=0; i<resources.size(); i++)
	{
		if(resources[i] < 0)
			iw.components.push_back(Component(Component::RESOURCE,i,resources[i],0));
	}
	if(iw.components.size())
	{
		getText(iw,afterBattle,182,h);
		cb->showInfoDialog(&iw);
	}

	iw.components.clear();
	iw.text.clear();
	for(int i=0; i<resources.size(); i++)
	{
		if(resources[i] > 0)
			iw.components.push_back(Component(Component::RESOURCE,i,resources[i],0));
	}
	if(iw.components.size())
	{
		getText(iw,afterBattle,183,h);
		cb->showInfoDialog(&iw);
	}

	iw.components.clear();
	for(int i=0; i<artifacts.size(); i++)
	{
		iw.components.push_back(Component(Component::ARTIFACT,artifacts[i],0,0));
	}
	if(iw.components.size())
	{
		cb->showInfoDialog(&iw);
	}

	for(int i=0; i<resources.size(); i++)
		if(resources[i])
			cb->giveResource(h->getOwner(),i,resources[i]);

	for(int i=0; i<artifacts.size(); i++)
		cb->giveHeroArtifact(artifacts[i],h->id,-2);

	//show dialog with given creatures
	iw.components.clear();
	iw.text.clear();
	for(std::map<si32,std::pair<ui32,si32> >::const_iterator i = creatures.slots.begin(); i != creatures.slots.end(); i++)
	{
		iw.components.push_back(Component(Component::CREATURE,i->second.first,i->second.second,0));
	}
	if(iw.components.size())
	{
		if(afterBattle)
		{
			if(iw.components.front().val == 1)
			{
				iw.text.addTxt(MetaString::ADVOB_TXT,185);//A %s joins %s's army.
				iw.text.replacements.push_back(VLC->creh->creatures[iw.components.front().subtype].nameSing);
			}
			else
			{
				iw.text.addTxt(MetaString::ADVOB_TXT,186);//%s join %s's army.
				iw.text.replacements.push_back(VLC->creh->creatures[iw.components.front().subtype].namePl);
			}
			iw.text.replacements.push_back(h->name);
		}
		else
		{
			iw.text << message;
			afterBattle = true;
		}
		cb->showInfoDialog(&iw);
	}

	//check if creatures can be moved to hero army
	CCreatureSet heroArmy = h->army;
	CCreatureSet ourArmy = creatures;
	while(ourArmy)
	{
		int slot = heroArmy.getSlotFor(ourArmy.slots.begin()->second.first);
		if(slot < 0)
			break;

		heroArmy.slots[slot].first = ourArmy.slots.begin()->second.first;
		heroArmy.slots[slot].second += ourArmy.slots.begin()->second.second;
		ourArmy.slots.erase(ourArmy.slots.begin());
	}

	if(!ourArmy) //all creatures can be moved to hero army - do that
	{
		SetGarrisons sg;
		sg.garrs[h->id] = heroArmy;
		cb->sendAndApply(&sg);
	}
	else //show garrison window and let player pick creatures
	{
		SetGarrisons sg;
		sg.garrs[id] = creatures;
		cb->sendAndApply(&sg);

		if(removeAfterVisit)
			cb->showGarrisonDialog(id,h->id,boost::bind(&IGameCallback::removeObject,cb,id));
		else
			cb->showGarrisonDialog(id,h->id,0);
		return;
	}

	if(!afterBattle)
	{
		iw.text << message;
		cb->showInfoDialog(&iw);
	}

	if(removeAfterVisit)
		cb->removeObject(id);
}

void CGEvent::getText( InfoWindow &iw, bool &afterBattle, int text, const CGHeroInstance * h ) const
{
	if(afterBattle)
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,text);//%s has lost treasure.
		iw.text.replacements.push_back(h->name);
	}
	else
	{
		iw.text << message;
		afterBattle = true;
	}
}

void CGEvent::getText( InfoWindow &iw, bool &afterBattle, int val, int positive, int negative, const CGHeroInstance * h ) const
{
	iw.components.clear();
	iw.text.clear();
	if(afterBattle)
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,val < 0 ? negative : positive); //%s's luck takes a turn for the worse / %s's luck increases
		iw.text.replacements.push_back(h->name);
	}
	else
	{
		iw.text << message;
		afterBattle = true;
	}
}

void CGObservatory::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.soundID = soundBase::LIGHTHOUSE;
	iw.player = h->tempOwner;
	iw.text.addTxt(MetaString::ADVOB_TXT,98 + (ID==60));
	cb->showInfoDialog(&iw);

	FoWChange fw;
	fw.player = h->tempOwner;
	fw.mode = 1;
	cb->getTilesInRange(fw.tiles,pos,20,h->tempOwner,1);
	cb->sendAndApply(&fw);
}

void CGShrine::onHeroVisit( const CGHeroInstance * h ) const
{
	if(spell == 255)
	{
		tlog1 << "Not initialized shrine visited!\n";
		return;
	}

	if(!hasVisited(h->tempOwner))
		cb->setObjProperty(id,10,h->tempOwner);

	InfoWindow iw;
	iw.soundID = soundBase::temple;
	iw.player = h->getOwner();
	iw.text.addTxt(MetaString::ADVOB_TXT,127 + ID - 88);
	iw.text.addTxt(MetaString::SPELL_NAME,spell);
	iw.text << ".";

	if(!h->getArt(17)) //no spellbook
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,131);
	}
	else if(ID == 90  &&  !h->getSecSkillLevel(7)) //it's third level spell and hero doesn't have wisdom
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,130);
	}
	else if(vstd::contains(h->spells,spell))//hero already knows the spell
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,174);
	}
	else //give spell
	{
		std::set<ui32> spells;
		spells.insert(spell);
		cb->changeSpells(h->id,true,spells);

		iw.components.push_back(Component(Component::SPELL,spell,0,0));
	}

	cb->showInfoDialog(&iw);
}

void CGShrine::initObj()
{
	if(spell == 255) //spell not set
	{
		int level = ID-87;
		std::vector<ui32> possibilities;

		//add all allowed spells of wanted level
		for(int i=0; i<SPELLS_QUANTITY; i++)
		{
			if(VLC->spellh->spells[i].level == level
				&& cb->isAllowed(0,VLC->spellh->spells[i].id))
			{
				possibilities.push_back(VLC->spellh->spells[i].id);
			}
		}

		if(!possibilities.size())
		{
			tlog1 << "Error: cannot init shrine, no allowed spells!\n";
			return;
		}

		spell = possibilities[ran() % possibilities.size()];
	}
}

const std::string & CGShrine::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID];
	if(hasVisited(cb->getCurrentPlayer())) //TODO: use local player, not current
	{
		hoverName += "\n" + VLC->generaltexth->allTexts[355]; // + (learn %s)
		boost::algorithm::replace_first(hoverName,"%s",VLC->spellh->spells[spell].name);
		const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
		if(h && vstd::contains(h->spells,spell)) //hero knows that ability
			hoverName += "\n\n" + VLC->generaltexth->allTexts[354]; // (Already learned)
	}
	return hoverName;
}

void CGSignBottle::initObj()
{
	//if no text is set than we pick random from the predefined ones
	if(!message.size())
		message = VLC->generaltexth->randsign[ran()%VLC->generaltexth->randsign.size()];
}

void CGSignBottle::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.soundID = soundBase::STORE;
	iw.player = h->getOwner();
	iw.text << message;
	cb->showInfoDialog(&iw);
}

void CGScholar::giveAnyBonus( const CGHeroInstance * h ) const
{

}

void CGScholar::onHeroVisit( const CGHeroInstance * h ) const
{

	int type = bonusType;
	int bid = bonusID;
	//check if the bonus if applicable, if not - give primary skill (always possible)
	int ssl = h->getSecSkillLevel(bid); //current sec skill level, used if bonusType == 1
	if((type == 1
			&& ((ssl == 3)  ||  (!ssl  &&  h->secSkills.size() == SKILL_PER_HERO))) ////hero already has expert level in the skill or (don't know skill and doesn't have free slot)
		|| (type == 2  &&  (!h->getArt(17) || vstd::contains(h->spells,bid)))) //hero doesn't have a spellbook or already knows the spell
	{
		type = 0;
		bid = ran() % PRIMARY_SKILLS;
	}


	InfoWindow iw;
	iw.soundID = soundBase::gazebo;
	iw.player = h->getOwner();
	iw.text.addTxt(MetaString::ADVOB_TXT,115);

	switch (type)
	{
	case 0:
		cb->changePrimSkill(h->id,bid,+1);
		iw.components.push_back(Component(Component::PRIM_SKILL,bid,1,0));
		break;
	case 1:
		{
			cb->changeSecSkill(h->id,bid,ssl+1);
			iw.components.push_back(Component(Component::SEC_SKILL,bid,ssl+1,0));
		}
		break;
	case 2:
		{
			std::set<ui32> hlp;
			hlp.insert(bid);
			cb->changeSpells(h->id,true,hlp);
			iw.components.push_back(Component(Component::SPELL,bid,0,0));
		}
		break;
	default:
		tlog1 << "Error: wrong bonustype (" << (int)type << ") for Scholar!\n";
		return;
	}

	cb->showInfoDialog(&iw);
	cb->removeObject(id);
}

void CGScholar::initObj()
{
	blockVisit = true;
	if(bonusType == 255)
	{
		bonusType = ran()%3;
		switch(bonusType)
		{
		case 0:
			bonusID = ran() % PRIMARY_SKILLS;
			break;
		case 1:
			bonusID = ran() % SKILL_QUANTITY;
			break;
		case 2:
			bonusID = ran() % SPELLS_QUANTITY;
			break;
		}
	}
}

void CGOnceVisitable::onHeroVisit( const CGHeroInstance * h ) const
{
	int sound = 0;
	int txtid = -1;
	switch(ID)
	{
	case 22: //Corpse
		txtid = 37; 
		sound = soundBase::MYSTERY;
		break;
	case 39: //Lean To
		sound = soundBase::GENIE;
		txtid = 64;
		break;
	case 105://Wagon
		sound = soundBase::GENIE;
		txtid = 154;
		break;
	case 108:
		break;
	default:
		tlog1 << "Error: Unknown object (" << ID <<") treated as CGOnceVisitable!\n";
		return;
	}

	if(ID == 108)//Warrior's Tomb
	{
		//ask if player wants to search the Tomb
		BlockingDialog bd(true, false);
		sound = soundBase::GRAVEYARD;
		bd.player = h->getOwner();
		bd.text.addTxt(MetaString::ADVOB_TXT,161);
		cb->showBlockingDialog(&bd,boost::bind(&CGOnceVisitable::searchTomb,this,h,_1));
		return;
	}

	InfoWindow iw;
	iw.soundID = sound;
	iw.player = h->getOwner();

	if(players.size()) //we have been already visited...
	{
		txtid++;
		if(ID == 105) //wagon has extra text (for finding art) we need to ommit
			txtid++;

		iw.text.addTxt(MetaString::ADVOB_TXT, txtid);
	}
	else //first visit - give bonus!
	{
		if(ID == 105  &&  artOrRes == 1) 
		{
			txtid++;
			iw.text.replacements.push_back(VLC->arth->artifacts[bonusType].Name());
		}


		switch(artOrRes)
		{
		case 0:
			txtid++;
			break;
		case 1: //art
			iw.components.push_back(Component(Component::ARTIFACT,bonusType,0,0));
			cb->giveHeroArtifact(bonusType,h->id,-2);
			break;
		case 2: //res
			iw.components.push_back(Component(Component::RESOURCE,bonusType,bonusVal,0));
			cb->giveResource(h->getOwner(),bonusType,bonusVal);
			break;
		}

		iw.text.addTxt(MetaString::ADVOB_TXT, txtid);
	}

	cb->showInfoDialog(&iw);
	cb->setObjProperty(id,10,h->getOwner());
}

const std::string & CGOnceVisitable::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID] + " ";

	hoverName += (hasVisited(cb->getCurrentPlayer())
		? (VLC->generaltexth->allTexts[352])  //visited
		: ( VLC->generaltexth->allTexts[353])); //not visited

	return hoverName;
}

void CGOnceVisitable::initObj()
{
	switch(ID)
	{
	case 22: //Corpse
		{
			blockVisit = true;
			int hlp = ran()%100;
			if(hlp < 20)
			{
				artOrRes = 1;
				std::vector<CArtifact*> arts; 
				cb->getAllowed(arts, ART_TREASURE | ART_MINOR | ART_MAJOR);
				bonusType = arts[ran() % arts.size()]->id;
			}
			else
			{
				artOrRes = 0;
			}
		}
		break;

	case 39: //Lean To
		{
			artOrRes = 2;
			bonusType = ran()%6; //any basic resource without gold
			bonusVal = ran()%4 + 1;
			break;
		}

	case 108://Warrior's Tomb
		{
			artOrRes = 1;

			std::vector<CArtifact*> arts; 

			int hlp = ran()%100;
			if(hlp < 30)
				cb->getAllowed(arts,ART_TREASURE);
			else if(hlp < 80)
				cb->getAllowed(arts,ART_MINOR);
			else if(hlp < 95)
				cb->getAllowed(arts,ART_MAJOR);
			else
				cb->getAllowed(arts,ART_RELIC);

			bonusType = arts[ran() % arts.size()]->id;
		}
		break;

	case 105://Wagon
		{
			int hlp = ran()%100;

			if(hlp < 10)
			{
				artOrRes = 0; // nothing... :(
			}
			else if(hlp < 50) //minor or treasure art
			{
				artOrRes = 1;
				std::vector<CArtifact*> arts; 
				cb->getAllowed(arts, ART_TREASURE | ART_MINOR);
				bonusType = arts[ran() % arts.size()]->id;
			}
			else //2 - 5 of non-gold resource
			{
				artOrRes = 2;
				bonusType = ran()%6;
				bonusVal = ran()%4 + 2;
			}

			break;
		}
	}
}

void CGOnceVisitable::searchTomb(const CGHeroInstance *h, ui32 accept) const
{
	if(accept)
	{
		InfoWindow iw;
		iw.player = h->getOwner();
		iw.components.push_back(Component(Component::MORALE,0,-3,0));

		if(players.size()) //we've been already visited, player found nothing
		{
			iw.text.addTxt(MetaString::ADVOB_TXT,163);
		}
		else //first visit - give artifact
		{
			iw.text.addTxt(MetaString::ADVOB_TXT,162);
			iw.components.push_back(Component(Component::ARTIFACT,bonusType,0,0));
			iw.text.replacements.push_back(VLC->arth->artifacts[bonusType].Name());

			cb->giveHeroArtifact(bonusType,h->id,-2);
		}

		if(!h->getBonus(HeroBonus::OBJECT,ID)) //we don't have modifier from this object yet
		{
			//ruin morale 
			GiveBonus gb;
			gb.hid = h->id;
			gb.bonus = HeroBonus(HeroBonus::ONE_BATTLE,HeroBonus::MORALE,HeroBonus::OBJECT,-3,id,"");
			gb.bdescr.addTxt(MetaString::ARRAY_TXT,104); //Warrior Tomb Visited -3
			cb->giveHeroBonus(&gb);
		}
	
		cb->showInfoDialog(&iw);

		//add player to the visitors (for visited tooltop)
		cb->setObjProperty(id,10,h->getOwner());
	}
}
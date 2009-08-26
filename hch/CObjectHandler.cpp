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
#include <boost/assign/std/vector.hpp> 
#include <boost/lexical_cast.hpp>
#include <boost/random/linear_congruential.hpp>
#include "CTownHandler.h"
#include "CArtHandler.h"
#include "CSoundBase.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/IGameCallback.h"
#include "../lib/CGameState.h"
#include "../lib/NetPacks.h"
#include "../StartInfo.h"
#include "../lib/map.h"
#include <sstream>
using namespace boost::assign;

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
std::map <ui8, std::set <ui8> > CGKeys::playerKeyMap;
std::map <si32, std::vector<si32> > CGMagi::eyelist;


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

static void readCreatures(std::istream & is, BankConfig & bc, bool guards) //helper function for void CObjectHandler::loadObjects()
{
	const int MAX_BUF = 5000;
	char buffer[MAX_BUF + 1];
	std::pair<si16, si32> guardInfo = std::make_pair(0, 0);
	std::string creName;

	is >> guardInfo.first;
	//one getline just does not work... probably a kind of left whitespace
	is.getline(buffer, MAX_BUF, '\t');
	is.getline(buffer, MAX_BUF, '\t');
	creName = buffer;

	if( std::string(buffer) == "None" ) //no creature to be added
		return;

	//look for the best creature that is described by given name
	if( vstd::contains(VLC->creh->nameToID, creName) )
	{
		guardInfo.second = VLC->creh->nameToID[creName];
	}
	else
	{
		for(int g=0; g<VLC->creh->creatures.size(); ++g)
		{
			if(VLC->creh->creatures[g].namePl == creName
				|| VLC->creh->creatures[g].nameRef == creName
				|| VLC->creh->creatures[g].nameSing == creName)
			{
				guardInfo.second = VLC->creh->creatures[g].idNumber;
			}
		}
	}
	
	if(guards)
		bc.guards.push_back(guardInfo);
	else //given creatures
		bc.creatures.push_back(guardInfo);
}

void CObjectHandler::loadObjects()
{
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
		tlog5 << "\t\tDone loading objects!\n";
	}

	std::string banksConfig = bitmaph->getTextFile("ZCRBANK.TXT");

	std::istringstream istr(banksConfig);
	const int MAX_BUF = 5000;
	char buffer[MAX_BUF + 1];

	//omitting unnecessary lines
	istr.getline(buffer, MAX_BUF);
	istr.getline(buffer, MAX_BUF);
	
	for(int g=0; g<21; ++g) //TODO: remove hardcoded value
	{
		//reading name - TODO: use it if necessary
		istr.getline(buffer, MAX_BUF, '\t');

		for(int i=0; i<4; ++i) //reading levels
		{
			BankConfig bc;
			std::string buf;
			char dump;
			//bc.level is of type char and thus we cannot read directly to it; same for some othre variables
			istr >> buf; 
			bc.level = atoi(buf.c_str());

			istr >> buf;
			bc.chance = atoi(buf.c_str());

			readCreatures(istr, bc, true);
			istr >> buf;
			bc.upgradeChance = atoi(buf.c_str());

			for(int b=0; b<3; ++b)
				readCreatures(istr, bc, true);

			istr >> bc.combatValue;
			bc.resources.resize(RESOURCE_QUANTITY);
			for(int h=0; h<7; ++h)
			{
				istr >> bc.resources[h];
			}
			readCreatures(istr, bc, false);

			bc.artifacts.resize(4);
			for(int b=0; b<4; ++b)
			{
				istr >> bc.artifacts[b];
			}

			istr >> bc.value;
			istr >> bc.rewardDifficulty;
			istr >> buf;
			bc.easiest = atoi(buf.c_str());

			banksInfo[g].push_back(bc);
		}
	}
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
		return false;
	return true;
}

bool CGObjectInstance::coveringAt(int x, int y) const
{
	if((defInfo->coverageMap[y] >> (7-(x) )) & 1)
		return true;
	return false;
}

std::set<int3> CGObjectInstance::getBlockedPos() const
{
	std::set<int3> ret;
	for(int w=0; w<getWidth(); ++w)
	{
		for(int h=0; h<getHeight(); ++h)
		{
			if(blockingAt(w, h))
				ret.insert(int3(pos.x - getWidth() + w + 1, pos.y - getHeight() + h + 1, pos.z));
		}
	}
	return ret;
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

int3 CGObjectInstance::getVisitableOffset() const
{
	for(int y = 0; y < 6; y++)
		for (int x = 0; x < 8; x++)
			if((defInfo->visitMap[5-y] >> x) & 1)
				return int3(x,y,0);

	tlog2 << "Warning: getVisitableOffset called on non-visitable obj!\n";
	return int3(-1,-1,-1);
}

void CGObjectInstance::getNameVis( std::string &hname ) const
{
	const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
	hname = VLC->generaltexth->names[ID];
	if(h) 
	{
		if(!h->getBonus(HeroBonus::OBJECT,ID))
			hname += " " + VLC->generaltexth->allTexts[353]; //not visited
		else
			hname += " " + VLC->generaltexth->allTexts[352]; //visited
	}
}

void CGObjectInstance::giveDummyBonus(int heroID, ui8 duration) const
{
	GiveBonus gbonus;
	gbonus.bonus.type = HeroBonus::NONE;
	gbonus.hid = heroID;
	gbonus.bonus.duration = duration;
	gbonus.bonus.source = HeroBonus::OBJECT;
	gbonus.bonus.id = ID;
	cb->giveHeroBonus(&gbonus);
}

void CGObjectInstance::onHeroVisit( const CGHeroInstance * h ) const
{
}

static int lowestSpeed(const CGHeroInstance * chi)
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
		case TerrainTile::dirtRoad:
			ret = 75;
			break;
		case TerrainTile::grazvelRoad:
			ret = 65;
			break;
		case TerrainTile::cobblestoneRoad:
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
	//TODO: write it - it should check if hero is flying, or something similar
	return false;
}
int CGHeroInstance::getPrimSkillLevel(int id) const
{
	int ret = primSkills[id];
	for(std::list<HeroBonus>::const_iterator i=bonuses.begin(); i != bonuses.end(); i++)
		if(i->type == HeroBonus::PRIMARY_SKILL && i->subtype==id)
			ret += i->val;
	amax(ret, id/2);//minimum value for attack and defense is 0 and for spell power and knowledge - 1
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
		switch(getSecSkillLevel(5))
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
	boat = NULL;
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
	if(type->startingSpell >= 0) //hero starts with a spell
	{
		artifWorn[17] = 0; //give him spellbook
		spells.insert(type->startingSpell);
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
		if(pom < 9)
			x = 1;
		else if(pom < 79)
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

	recreateArtBonuses();
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
		defInfo->blockMap[i] = 255;
		defInfo->visitMap[i] = 0;
		defInfo->coverageMap[i] = 0;
	}
	defInfo->handler=NULL;
	defInfo->blockMap[5] = 253;
	defInfo->visitMap[5] = 2;
	defInfo->coverageMap[4] = defInfo->coverageMap[5] = 224;
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
	if(h == this) return; //exclude potential self-visiting

	if (ID == HEROI_TYPE) //hero
	{
		//TODO: check for allies
		if(tempOwner == h->tempOwner) //our hero
		{
			//exchange
			cb->heroExchange(id, h->id);
		}
		else
		{
			//battle
			cb->startBattleI(h,	this, false);
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
			// Take Angelic Alliance troop-mixing freedom of non-evil, non-Conflux units into account.
			const si8 faction = VLC->creh->creatures[i->second.first].faction;
			if (hasBonusOfType(HeroBonus::NONEVIL_ALIGNMENT_MIX)
				&& ((faction >= 0 && faction <= 2) || faction == 6 || faction == 7))
			{
				factions.insert(0); // Insert a single faction of the affected group, Castle will do.
			}
			else
			{
				factions.insert(faction);
			}

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
	ui8 skill = 0; //skill level

	if(spell->fire)
		skill = std::max(skill,getSecSkillLevel(14));
	if(spell->air)
		skill = std::max(skill,getSecSkillLevel(15));
	if(spell->water)
		skill = std::max(skill,getSecSkillLevel(16));
	if(spell->earth)
		skill = std::max(skill,getSecSkillLevel(17));

	//bonuses (eg. from special terrains)
	skill = std::max<ui8>(skill, valOfBonuses(HeroBonus::MAGIC_SCHOOL_SKILL, 0)); //any school bonus
	if(spell->fire)
		skill = std::max<ui8>(skill, valOfBonuses(HeroBonus::MAGIC_SCHOOL_SKILL, 1));
	if(spell->air)
		skill = std::max<ui8>(skill, valOfBonuses(HeroBonus::MAGIC_SCHOOL_SKILL, 2));
	if(spell->water)
		skill = std::max<ui8>(skill, valOfBonuses(HeroBonus::MAGIC_SCHOOL_SKILL, 4));
	if(spell->earth)
		skill = std::max<ui8>(skill, valOfBonuses(HeroBonus::MAGIC_SCHOOL_SKILL, 8));

	return skill;
}

bool CGHeroInstance::canCastThisSpell(const CSpell * spell) const
{
	if(!getArt(17)) //if hero has no spellbook
		return false;

	if(vstd::contains(spells, spell->id) //hero does not have this spell in spellbook
		|| (spell->air && hasBonusOfType(HeroBonus::AIR_SPELLS)) // this is air spell and hero can cast all air spells
		|| (spell->fire && hasBonusOfType(HeroBonus::FIRE_SPELLS)) // this is fire spell and hero can cast all fire spells
		|| (spell->water && hasBonusOfType(HeroBonus::WATER_SPELLS)) // this is water spell and hero can cast all water spells
		|| (spell->earth && hasBonusOfType(HeroBonus::EARTH_SPELLS)) // this is earth spell and hero can cast all earth spells
		)
		return true;

	for(std::list<HeroBonus>::const_iterator it = bonuses.begin(); it != bonuses.end(); ++it)
	{
		if(it->type == HeroBonus::SPELL && it->subtype == spell->id)
		{
			return true;
		}
		if(it->type == HeroBonus::SPELLS_OF_LEVEL && it->subtype == spell->level)
		{
			return true;
		}
	}

	return false;
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
	if (hasBonusOfType(HeroBonus::FULL_MANA_REGENERATION))
		return manaLimit();

	return 1 + getSecSkillLevel(8) + valOfBonuses(HeroBonus::MANA_REGENERATION); //1 + Mysticism level 
}

int CGHeroInstance::valOfBonuses( HeroBonus::BonusType type, int subtype /*= -1*/ ) const
{
	int ret = 0;
	if(subtype == -1)
	{
		for(std::list<HeroBonus>::const_iterator i=bonuses.begin(); i != bonuses.end(); i++)
			if(i->type == type)
				ret += i->val;
	}
	else
	{
		for(std::list<HeroBonus>::const_iterator i=bonuses.begin(); i != bonuses.end(); i++)
			if(i->type == type && i->subtype == subtype)
				ret += i->val;
	}
	return ret;
}

bool CGHeroInstance::hasBonusOfType(HeroBonus::BonusType type, int subtype /*= -1*/) const
{
	if(subtype == -1) //any subtype
	{
		for(std::list<HeroBonus>::const_iterator i=bonuses.begin(); i != bonuses.end(); i++)
			if(i->type == type)
				return true;
	}
	else //given subtype
	{
		for(std::list<HeroBonus>::const_iterator i=bonuses.begin(); i != bonuses.end(); i++)
			if(i->type == type && i->subtype == subtype)
				return true;
	}
	return false;
}

si32 CGHeroInstance::getArtPos(int aid) const
{
	for(std::map<ui16,ui32>::const_iterator i = artifWorn.begin(); i != artifWorn.end(); i++)
		if(i->second == aid)
			return i->first;
	return -1;
}

void CGHeroInstance::recreateArtBonuses()
{
	//clear all bonuses from artifacts (if present) and give them again
	std::remove_if(bonuses.begin(), bonuses.end(), boost::bind(HeroBonus::IsFrom,_1,HeroBonus::ARTIFACT,0xffffff));
	for (std::map<ui16,ui32>::iterator ari = artifWorn.begin(); ari != artifWorn.end(); ari++)
	{
		CArtifact &art = VLC->arth->artifacts[ari->second];
		for(std::list<HeroBonus>::iterator i = art.bonuses.begin(); i != art.bonuses.end(); i++)
			bonuses.push_back(*i);
	}
}

void CGDwelling::initObj()
{
	switch(ID)
	{
	case 17:
		{
			int crid = VLC->objh->cregens[subID];
			CCreature *crs = &VLC->creh->creatures[crid];

			creatures.resize(1);
			creatures[0].second.push_back(crid);
			hoverName = VLC->generaltexth->creGens[subID];
			if(crs->level > 4)
			{
				army.slots[0].first = crs->idNumber;
				army.slots[0].second = (8 - crs->level) * 3;
			}
		}
		break;

	case 20:
		creatures.resize(4);
		if(subID == 1) //Golem Factory
		{
			creatures[0].second.push_back(32);  //Stone Golem
			creatures[1].second.push_back(33);  //Iron Golem  
			creatures[2].second.push_back(116); //Gold Golem
			creatures[3].second.push_back(117); //Diamond Golem
			//guards
			army.slots[0].first = 116;
			army.slots[0].second = 9;
			army.slots[1].first = 117;
			army.slots[1].second = 6;
		}
		else if(subID == 0) // Elemental Conflux 
		{
			creatures[0].second.push_back(112); //Air Elemental
			creatures[1].second.push_back(114); //Fire Elemental
			creatures[2].second.push_back(113); //Earth Elemental
			creatures[3].second.push_back(115); //Water Elemental
			//guards
			army.slots[0].first = 113;
			army.slots[0].second = 12;
		}
		else
		{
			assert(0);
		}
		hoverName = VLC->generaltexth->creGens4[subID];
		break;

	default:
		assert(0);
		break;
	}
}

void CGDwelling::onHeroVisit( const CGHeroInstance * h ) const
{
	if(h->tempOwner != tempOwner  &&  army) //object is guarded
	{
		BlockingDialog bd;
		bd.player = h->tempOwner;
		bd.flags = BlockingDialog::ALLOW_CANCEL;
		bd.text.addTxt(MetaString::GENERAL_TXT, 421); //Much to your dismay, the %s is guarded by %s %s. Do you wish to fight the guards?
		bd.text.addReplacement(ID == 17 ? MetaString::CREGENS : MetaString::CREGENS4, subID);
		bd.text.addReplacement(MetaString::ARRAY_TXT, 176 + CCreature::getQuantityID(army.slots.begin()->second.second)*3);
		bd.text.addReplacement(MetaString::CRE_PL_NAMES, army.slots.begin()->second.first);
		cb->showBlockingDialog(&bd, boost::bind(&CGDwelling::wantsFight, this, h, _1));
		return;
	}

	if(h->tempOwner != tempOwner)
		cb->setOwner(id, h->tempOwner);

	BlockingDialog bd;
	bd.player = h->tempOwner;
	bd.flags = BlockingDialog::ALLOW_CANCEL;
	bd.text.addTxt(MetaString::ADVOB_TXT, ID == 17 ? 35 : 36); //{%s} Would you like to recruit %s?
	bd.text.addReplacement(ID == 17 ? MetaString::CREGENS : MetaString::CREGENS4, subID);
	for(size_t i = 0; i < creatures.size(); i++)
		bd.text.addReplacement(MetaString::CRE_PL_NAMES, creatures[i].second[0]);

	cb->showBlockingDialog(&bd, boost::bind(&CGDwelling::heroAcceptsCreatures, this, h, _1));
}

void CGDwelling::newTurn() const
{
	if(cb->getDate(1) != 1) //not first day of week
		return;

	//town growths are handled separately
	if(ID == TOWNI_TYPE)
		return;

	bool change = false;

	SetAvailableCreatures sac;
	sac.creatures = creatures;
	sac.tid = id;
	for (size_t i = 0; i < creatures.size(); i++)
	{
		if(creatures[i].second.size())
		{
			if(false /*accumulate creatures*/)
				sac.creatures[i].first += VLC->creh->creatures[creatures[i].second[0]].growth;
			else
				sac.creatures[i].first = VLC->creh->creatures[creatures[i].second[0]].growth;
			change = true;
		}
	}

	if(change)
		cb->sendAndApply(&sac);
}

void CGDwelling::heroAcceptsCreatures( const CGHeroInstance *h, ui32 answer ) const
{
	if(!answer)
		return;

	int crid = creatures[0].second[0];
	CCreature *crs = &VLC->creh->creatures[crid];

	if(crs->level == 1) //first level - creatures are for free
	{
		if(creatures[0].first) //there are available creatures
		{
			int slot = h->army.getSlotFor(crid);
			if(slot < 0) //no available slot
			{
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 425);//The %s would join your hero, but there aren't enough provisions to support them.
				iw.text.addReplacement(MetaString::CRE_PL_NAMES, crid);
				cb->showInfoDialog(&iw);
			}
			else //give creatures
			{
				SetAvailableCreatures sac;
				sac.tid = id;
				sac.creatures = creatures;
				sac.creatures[0].first = 0;

				SetGarrisons sg;
				sg.garrs[h->id] = h->army;
				sg.garrs[h->id].slots[slot].first = crid;
				sg.garrs[h->id].slots[slot].second += creatures[0].first;

				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 423); //%d %s join your army.
				iw.text.addReplacement(creatures[0].first);
				iw.text.addReplacement(MetaString::CRE_PL_NAMES, crid);

				cb->showInfoDialog(&iw);
				cb->sendAndApply(&sac);
				cb->sendAndApply(&sg);
			}
		}
		else //there no creatures
		{
			InfoWindow iw;
			iw.text.addTxt(MetaString::GENERAL_TXT, 422); //There are no %s here to recruit.
			iw.text.addReplacement(MetaString::CRE_PL_NAMES, crid);
			iw.player = h->tempOwner;
			cb->sendAndApply(&iw);
		}
	}
	else
	{
		OpenWindow ow;
		ow.id1 = id;
		ow.id2 = h->id;
		ow.window = ID == 17 
			? OpenWindow::RECRUITMENT_FIRST 
			: OpenWindow::RECRUITMENT_ALL;
		cb->sendAndApply(&ow);
	}
}

void CGDwelling::wantsFight( const CGHeroInstance *h, ui32 answer ) const
{
	if(answer)
		cb->startBattleI(h, this, false, boost::bind(&CGDwelling::fightOver, this, h, _1));
}

void CGDwelling::fightOver(const CGHeroInstance *h, BattleResult *result) const
{
	if (result->winner == 0)
	{
		onHeroVisit(h);
	}
}

int CGTownInstance::getSightRadious() const //returns sight distance
{
	if (subID == 2) //tower
	{
		if ((builtBuildings.find(17)) != builtBuildings.end()) //skyship
			return cb->getMapSize().x + cb->getMapSize().y;
		else if ((builtBuildings.find(21)) != builtBuildings.end()) //lookout tower
			return 20;
	}
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

	//support for legs of legion etc.
	if(garrisonHero)
		ret += garrisonHero->valOfBonuses(HeroBonus::CREATURE_GROWTH, level);
	if(visitingHero)
		ret += visitingHero->valOfBonuses(HeroBonus::CREATURE_GROWTH, level);

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
	:IShipyard(this)
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
		//TODO ally check
		if(army)
		{
			const CGHeroInstance *defendingHero = NULL;
			if(visitingHero)
				defendingHero = visitingHero;
			else if(garrisonHero)
				defendingHero = garrisonHero;

			const CArmedInstance *defendingArmy = this;
			if(defendingHero)
				defendingArmy = defendingHero;

			bool outsideTown = (defendingHero == visitingHero && garrisonHero);
			cb->startBattleI(h, defendingArmy, getSightCenter(), h, defendingHero, false, boost::bind(&CGTownInstance::fightOver, this, h, _1), (outsideTown ? NULL : this));
		}
		else
		{
			cb->setOwner(id, h->tempOwner);
		}
	}
	for (std::vector<CGTownBuilding*>::const_iterator i = bonusingBuildings.begin(); i != bonusingBuildings.end(); i++)
		(*i)->onHeroVisit (h);
	cb->heroVisitCastle(id, h->id);
}

void CGTownInstance::onHeroLeave(const CGHeroInstance * h) const
{
	cb->stopHeroVisitCastle(id,h->id);
}

void CGTownInstance::initObj()
{ 
	hoverName = name + ", " + town->Name();

	creatures.resize(CREATURES_PER_TOWN);
	for (int i = 0; i < CREATURES_PER_TOWN; i++)
	{
		if(creatureDwelling(i,false))
			creatures[i].second.push_back(town->basicCreatures[i]);
		if(creatureDwelling(i,true))
			creatures[i].second.push_back(town->upgradedCreatures[i]);
	}
	switch (alignment)
	{ //add new visitable objects
		case 2: case 3: case 5: case 6:
			bonusingBuildings.push_back(new CTownBonus(23, this));
			break;
		case 7:
			bonusingBuildings.push_back(new CTownBonus(17, this));
			break;
	}
}

int3 CGTownInstance::getSightCenter() const
{
	return pos - int3(2,0,0);
}

void CGTownInstance::getOutOffsets( std::vector<int3> &offsets ) const
{
	offsets += int3(-1,3,0), int3(-3,3,0);
}

void CGTownInstance::fightOver( const CGHeroInstance *h, BattleResult *result ) const
{
	if(result->winner == 0)
	{
		cb->setOwner(id, h->tempOwner);
	}
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

void CGVisitableOPH::treeSelected( int heroID, int resType, int resVal, ui64 expVal, ui32 result ) const
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

CTownBonus::CTownBonus (int index, CGTownInstance *TOWN)
{
	ID = index;
	town = TOWN;
}
void CTownBonus::onHeroVisit (const CGHeroInstance * h) const
{
	int heroID = h->id;
	if ((town->builtBuildings.find(ID) != town->builtBuildings.end()) && (visitors.find(heroID) == visitors.end()))
	{
		InfoWindow iw;
		switch (ID)
		{
			case 23:
				switch(town->alignment)
				{
					case 2: //wall
						cb->changePrimSkill (heroID, 3, 1);
						iw.components.push_back (Component(Component::PRIM_SKILL, 3, 1, 0));
						break;
					case 3: //order of fire
						cb->changePrimSkill (heroID, 2, 1);
						iw.components.push_back (Component(Component::PRIM_SKILL, 2, 1, 0));
						break;
					case 6://hall of valhalla
						cb->changePrimSkill (heroID, 0, 1);
						iw.components.push_back (Component(Component::PRIM_SKILL, 0, 1, 0));
						break;
					case 5://academy of battle scholars
						cb->changePrimSkill (heroID, 4, 1000);
						iw.components.push_back (Component(Component::EXPERIENCE, 0, 1000, 0));
						break;
				}
				break;
			case 17:
				switch(town->alignment)
				{
					case 7: //cage of warlords
						cb->changePrimSkill (heroID, 1, 1);
						iw.components.push_back (Component(Component::PRIM_SKILL, 1, 1, 0));
						break;
				}
				break;
		}
		iw.player = cb->getOwner(heroID);
		iw.text << std::pair<ui8,ui32>(11,66);
		//iw.soundID = sound;
		cb->showInfoDialog(&iw);
		cb->setObjProperty (id, 4, h->id); //add to visitors
	}
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

/*const std::string & CGCreature::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID];
	hoverName += "\n Power rating: ";
	float ratio = ((float)getArmyStrength() / cb->getSelectedHero(cb->getCurrentPlayer())->getTotalStrength());
	if (ratio < 0.1) hoverName += "Effortless";
	else if (ratio < 0.3) hoverName += "Very Weak";
	else if (ratio < 0.6) hoverName += "Weak";
	else if (ratio < 0.9) hoverName += "A bit weaker";
	else if (ratio < 1.1) hoverName += "Equal";
	else if (ratio < 1.3) hoverName += "A bit stronger";
	else if (ratio < 1.8) hoverName += "Strong";
	else if (ratio < 2.5) hoverName += "Very Strong";
	else if (ratio < 4) hoverName += "Challenging";
	else if (ratio < 8) hoverName += "Overpowering";
	else if (ratio < 20) hoverName += "Deadly";
	else hoverName += "Impossible";
	return hoverName;
}*/
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
			ynd.text << std::pair<ui8,ui32>(MetaString::ADVOB_TXT, 86); 
			ynd.text.addReplacement(MetaString::CRE_PL_NAMES, subID);
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
		//int killedAmount=0;
		//for(std::set<std::pair<ui32,si32> >::iterator i=result->casualties[1].begin(); i!=result->casualties[1].end(); i++)
		//	if(i->first == subID)
		//		killedAmount += i->second;
		//cb->setAmount(id, army.slots.find(0)->second.second - killedAmount);	

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
	ms.toString(hoverName);
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
			factor = (int)(2*(hlp-1));
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
			joinDecision(h,cost,false);
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
	cb->startBattleI(h, this, false, boost::bind(&CGCreature::endBattle,this,_1));
}

void CGCreature::flee( const CGHeroInstance * h ) const
{
	BlockingDialog ynd(true,false);
	ynd.player = h->tempOwner;
	ynd.text << std::pair<ui8,ui32>(11,91); 
	ynd.text.addReplacement(MetaString::CRE_PL_NAMES, subID);
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
	if(cb->getDate() == 1)
		return;

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
	ms.toString(hoverName);
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
	sii.text.addReplacement(MetaString::RES_NAMES, subID);
	cb->showCompInfo(&sii);
	cb->removeObject(id);
}

void CGResource::fightForRes(ui32 agreed, const CGHeroInstance *h) const
{
	if(agreed)
		cb->startBattleI(h, this, false, boost::bind(&CGResource::endBattle,this,_1,h));
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
	case 43: //one way - find corresponding exit monolith
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
	cb->moveHero(h->id,CGHeroInstance::convertPosition(cb->getObj(destinationid)->pos,true) - getVisitableOffset(),
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
			iw.soundID = soundBase::treasure;
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
		cb->startBattleI(h, this, false, boost::bind(&CGArtifact::endBattle,this,_1,h));
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
	case 29: //floatsam
		switch(type = ran()%4)
		{
		case 0:
			val1 = val2 = 0;
			break;
		case 1:
			val1 = 5;
			val2 = 0;
			break;
		case 2:
			val1 = 5;
			val2 = 200;
			break;
		case 3:
			val1 = 10;
			val2 = 500;
			break;
		}
		break;
	case 82: //sea chest
		{
			int hlp = ran()%100;
			if(hlp < 20)
			{
				val1 = 0;
				type = 0;
			}
			else if(hlp < 90)
			{
				val1 = 1500;
				type = 2;
			}
			else
			{
				val1 = 1000;
				val2 = VLC->arth->treasures[ran()%VLC->arth->treasures.size()]->id;
				type = 1;
			}
		}
		break;
	case 86: //Shipwreck Survivor
		{
			int hlp = ran()%100;
			if(hlp < 55)
				val1 = VLC->arth->treasures[ran()%VLC->arth->treasures.size()]->id;
			else if(hlp < 75)
				val1 = VLC->arth->minors[ran()%VLC->arth->minors.size()]->id;
			else if(hlp < 95)
				val1 = VLC->arth->majors[ran()%VLC->arth->majors.size()]->id;
			else
				val1 = VLC->arth->relics[ran()%VLC->arth->relics.size()]->id;
		}
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
	case 29: //flotsam
		{
			cb->giveResource(h->tempOwner,0,val1); //wood
			cb->giveResource(h->tempOwner,6,val2);//gold
			InfoWindow iw;
			iw.soundID = soundBase::GENIE;
			iw.player = h->tempOwner;
			if(val1)
				iw.components.push_back(Component(2,0,val1,0));
			if(val2)
				iw.components.push_back(Component(2,6,val2,0));

			iw.text.addTxt(MetaString::ADVOB_TXT, 51+type);
			cb->showInfoDialog(&iw);
			break;
		}
	case 82: //Sea Chest
		{
			InfoWindow iw;
			iw.soundID = soundBase::chest;
			iw.player = h->tempOwner;
			iw.text.addTxt(MetaString::ADVOB_TXT, 116 + type);

			if(val1) //there is gold
			{
				iw.components.push_back(Component(2,6,val1,0));
				cb->giveResource(h->tempOwner,6,val1);
			}
			if(type == 1) //art
			{
				//TODO: what if no space in backpack?
				iw.components.push_back(Component(4, val2, 1, 0));
				iw.text.addReplacement(MetaString::ART_NAMES, val2);
				cb->giveHeroArtifact(val2,h->id,-2);
			}
			cb->showInfoDialog(&iw);
			break;
		}
	case 86: //Shipwreck Survivor
		{
			//TODO: what if no space in backpack?
			InfoWindow iw;
			iw.soundID = soundBase::experience;
			iw.player = h->tempOwner;
			iw.components.push_back(Component(4,val1,1,0));
			iw.text.addTxt(MetaString::ADVOB_TXT, 125);
			iw.text.addReplacement(MetaString::ART_NAMES, val1);
			cb->giveHeroArtifact(val1,h->id,-2);
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
				iw.text.addReplacement(MetaString::ART_NAMES, val1);
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
		iw.text.addReplacement(MetaString::SEC_SKILL_NAME, ability);
	}
	else if(h->secSkills.size() >= SKILL_PER_HERO) //already all skills slots used
	{
		iw.text << std::pair<ui8,ui32>(11,173);
		iw.text.addReplacement(MetaString::SEC_SKILL_NAME, ability);
	}
	else //give sec skill
	{
		iw.components.push_back(Component(1, ability, 1, 0));
		iw.text << std::pair<ui8,ui32>(11,171);
		iw.text.addReplacement(MetaString::SEC_SKILL_NAME, ability);
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
		const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
		if(h && h->getSecSkillLevel(ability)) //hero knows that ability
			hoverName += "\n\n" + VLC->generaltexth->allTexts[357]; // (Already learned)
	}
	return hoverName;
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
	case 11: //buoy
		messageID = 21;
		sound = soundBase::MORALE;
		gbonus.bonus.type = HeroBonus::MORALE;
		gbonus.bonus.val = +1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,94);
		break;
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
		gbonus.bdescr.addReplacement((gbonus.bonus.val<0 ? "-" : "+") + boost::lexical_cast<std::string>(gbonus.bonus.val));
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
	case 52: //Mermaid
		messageID = 83;
		sound = soundBase::LUCK;
		gbonus.bonus.type = HeroBonus::LUCK;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,72);
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
	case 94: //Stables TODO: upgrade Cavaliers
		sound = soundBase::horse20;
		std::set<ui32> slots;
		for (std::map<si32,std::pair<ui32,si32> >::const_iterator i = h->army.slots.begin(); i != h->army.slots.end(); ++i)
		{
			if(i->second.first == 10)
				slots.insert(i->first);
		}
		if (!slots.empty())
		{
			for (std::set<ui32>::const_iterator i = slots.begin(); i != slots.end(); i++)
			{
				UpgradeCreature uc(*i, id, 11);
				 //uc.applyGh (&gh);
			}
		}
		messageID = 137;
		gbonus.bonus.type = HeroBonus::LAND_MOVEMENT;
		gbonus.bonus.val = 600;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6, 100);
		break;
	}
	if(visited)
	{
		if(ID==64 || ID==96  ||  ID==56 || ID==52 || ID==94)
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

void CGBonusingObject::initObj()
{
	if(ID == 11 || ID == 52) //Buoy / Mermaid
	{
		blockVisit = true;
	}
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
		giveDummyBonus(h->id);
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
	getNameVis(hoverName);
	return hoverName;
}

void CGPandoraBox::initObj()
{
	blockVisit = true;
}

void CGPandoraBox::onHeroVisit(const CGHeroInstance * h) const
{
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundBase::QUEST;
		bd.text.addTxt (MetaString::ADVOB_TXT, 14);
		cb->showBlockingDialog (&bd, boost::bind (&CGPandoraBox::open, this, h, _1));	
}

void CGPandoraBox::open( const CGHeroInstance * h, ui32 accept ) const
{
	if (accept)
	{
		if (army) //if pandora's box is protected by army
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text.addTxt(MetaString::ADVOB_TXT, 16);
			cb->showInfoDialog(&iw);
			cb->startBattleI(h, this, false, boost::bind(&CGPandoraBox::endBattle, this, h, _1)); //grants things after battle
		}
		else if (message.size() == 0 && resources.size() == 0
				&& primskills.size() == 0 && abilities.size() == 0
				&& abilityLevels.size() == 0 &&  artifacts.size() == 0
				&& spells.size() == 0 && creatures == 0
				&& gainedExp == 0 && manaDiff == 0 && moraleDiff == 0 && luckDiff == 0) //if it gives nothing without battle
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text.addTxt(MetaString::ADVOB_TXT, 15);
			cb->showInfoDialog(&iw);

		}
		else //if it gives something without battle
		{
			giveContents (h, false);
		}
	}
}

void CGPandoraBox::endBattle( const CGHeroInstance *h, BattleResult *result ) const
{
	if(result->winner)
		return;

	giveContents(h,true);
}

void CGPandoraBox::giveContents( const CGHeroInstance *h, bool afterBattle ) const
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
			cb->changePrimSkill(h->id,4,gainedExp,false);
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
	getText(iw,afterBattle,183,h);
	for(int i=0; i<artifacts.size(); i++)
	{
		iw.components.push_back(Component(Component::ARTIFACT,artifacts[i],0,0));
		if(iw.components.size() >= 14)
		{
			cb->showInfoDialog(&iw);
			iw.components.clear();
		}
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
				iw.text.addReplacement(MetaString::CRE_SING_NAMES, iw.components.front().subtype);
			}
			else
			{
				iw.text.addTxt(MetaString::ADVOB_TXT,186);//%s join %s's army.
				iw.text.addReplacement(MetaString::CRE_PL_NAMES, iw.components.front().subtype);
			}
			iw.text.addReplacement(h->name);
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
		cb->showGarrisonDialog(id,h->id,boost::bind(&IGameCallback::removeObject,cb,id));
	}

	if(!afterBattle && message.size())
	{
		iw.text << message;
		cb->showInfoDialog(&iw);
	}
	cb->removeObject(id);
}

void CGPandoraBox::getText( InfoWindow &iw, bool &afterBattle, int text, const CGHeroInstance * h ) const
{
	if(afterBattle || !message.size())
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,text);//%s has lost treasure.
		iw.text.addReplacement(h->name);
	}
	else
	{
		iw.text << message;
		afterBattle = true;
	}
}

void CGPandoraBox::getText( InfoWindow &iw, bool &afterBattle, int val, int negative, int positive, const CGHeroInstance * h ) const
{
	iw.components.clear();
	iw.text.clear();
	if(afterBattle || !message.size())
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,val < 0 ? negative : positive); //%s's luck takes a turn for the worse / %s's luck increases
		iw.text.addReplacement(h->name);
	}
	else
	{
		iw.text << message;
		afterBattle = true;
	}
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

void CGEvent::activated( const CGHeroInstance * h ) const
{
	if(army)
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		if(message.size())
			iw.text << message;
		else
			iw.text.addTxt(MetaString::ADVOB_TXT, 16);
		cb->showInfoDialog(&iw);
		cb->startBattleI(h, this, false, boost::bind(&CGEvent::endBattle,this,h,_1));
	}
	else
	{
		giveContents(h,false);
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

	if(ID == 59)
	{
		blockVisit = true;
	}
}

void CGSignBottle::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.soundID = soundBase::STORE;
	iw.player = h->getOwner();
	iw.text << message;
	cb->showInfoDialog(&iw);

	if(ID == 59)
		cb->removeObject(id);
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
		iw.components.push_back(Component(Component::PRIM_SKILL,bid,+1,0));
		break;
	case 1:
		cb->changeSecSkill(h->id,bid,+1);
		iw.components.push_back(Component(Component::SEC_SKILL,bid,ssl+1,0));
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

void CGGarrison::onHeroVisit (const CGHeroInstance *h) const
{
	if (h->tempOwner != tempOwner && army) {
		//TODO: Find a way to apply magic garrison effects in battle.
		cb->startBattleI(h, this, false, boost::bind(&CGGarrison::fightOver, this, h, _1));
		return;
	}

	//New owner.
	if (h->tempOwner != tempOwner)
		cb->setOwner(id, h->tempOwner);

	//TODO: Garrison visit screen.
}

void CGGarrison::fightOver (const CGHeroInstance *h, BattleResult *result) const
{
	if (result->winner == 0)
		onHeroVisit(h);
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
		if(ID == 105) //wagon has extra text (for finding art) we need to omit
			txtid++;

		iw.text.addTxt(MetaString::ADVOB_TXT, txtid);
	}
	else //first visit - give bonus!
	{
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
		if(ID == 105  &&  artOrRes == 1) 
		{
			iw.text.localStrings.back().second++;
			iw.text.addReplacement(MetaString::ART_NAMES, bonusType);
		}
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
				cb->getAllowed(arts, CArtifact::ART_TREASURE | CArtifact::ART_MINOR | CArtifact::ART_MAJOR);
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
				cb->getAllowed(arts,CArtifact::ART_TREASURE);
			else if(hlp < 80)
				cb->getAllowed(arts,CArtifact::ART_MINOR);
			else if(hlp < 95)
				cb->getAllowed(arts,CArtifact::ART_MAJOR);
			else
				cb->getAllowed(arts,CArtifact::ART_RELIC);

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
				cb->getAllowed(arts, CArtifact::ART_TREASURE | CArtifact::ART_MINOR);
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
			iw.text.addReplacement(MetaString::ART_NAMES, bonusType);

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
	}
}

void CBank::initObj()
{
	switch (ID) //find apriopriate key
	{
		case 16: //bank
			index = subID; break;
		case 24: //derelict ship
			index = 8; break;
		case 25: //utopia
			index = 10; break;
		case 84: //crypt
			index = 9; break;
		case 85: //shipwreck
			index = 7; break;
	}
	bc = NULL;
	daycounter = 0;
	multiplier = 1;
}
void CBank::reset(ui16 var1, ui16 var2) //prevents desync
{
	int chance = 0;
	for (ui8 i = 0; i < VLC->objh->banksInfo[index].size(); i++)
	{	
		if (var1 < (chance += VLC->objh->banksInfo[index][i].chance))
		{
 			bc = &VLC->objh->banksInfo[index][i];
			break;
		}
	}
	artifacts.clear();
	for (ui8 i = 0; i <= 3; i++)
	{	
		std::vector<CArtifact*> arts; //to avoid addition of different tiers
		for (ui8 n = 0; n < bc->artifacts[i]; n++)
		{
			switch (i)
			{
				case 0:
					cb->getAllowed (arts, CArtifact::ART_TREASURE);
					break;
				case 1:
					cb->getAllowed (arts, CArtifact::ART_MINOR);
					break;
				case 2:
					cb->getAllowed (arts, CArtifact::ART_MAJOR);
					break;
				case 3:
					cb->getAllowed (arts, CArtifact::ART_RELIC);
					break;
			}
			artifacts.push_back (arts[var2 % arts.size()]->id);
			var2 *= (var1 + n * i); //almost like random
		}
	}

}
void CBank::setPropertyDer (ui8 what, ui32 val)
/// random values are passed as arguments and processed identically on all clients
{
	switch (what)
	{
		case 11: //daycounter
			if (val == 0)
				daycounter = 0;
			else
				daycounter++;
			break;
		case 12: //multiplier, in percent
			multiplier = ((float)val)/100;
			break;
		case 13: //bank preset
			bc = &VLC->objh->banksInfo[index][val];
			break;
		case 14:
			reset (val%100, val);
			break;
		case 15:
			bc = NULL;
			break;
		case 16:
			artifacts.clear();
			break;
		case 17: //set ArmedInstance army
			int upgraded = 0;
			if (val%100 < bc->upgradeChance) //once again anti-desync
				upgraded = 1;
			switch (bc->guards.size())
			{
				case 1:
					for	(int i = 0; i <= 4; i++)
						army.setCreature (i, bc->guards[0].first + upgraded, bc->guards[0].second  / 5 );
					break;
				case 4:
				{
					std::vector< std::pair <ui16, ui32> >::const_iterator it;
					for (it = bc->guards.begin(); it != bc->guards.end(); it++)
					{
						int n = army.slots.size(); //debug
						army.setCreature (n, it->first, it->second);
					}
				}
					break;
				default:
					tlog1 << "Error: Unexpected army data: " << bc->guards.size() <<" items found";
					return;
			}
			break;
	}
}

void CBank::newTurn() const 
{
	if (bc == NULL)
	{
		if (daycounter >= 28 || cb->getDate(0) == 1)
		{
			cb->setObjProperty (id, 11, 0); //daycounter 0
			cb->setObjProperty (id, 14, ran()); //reset
			if (ID == 24 && cb->getDate(0) > 1)
			{
				cb->setObjProperty (id, 16, 0);; //derelict ships are usable only once
			}
		}
		else
			cb->setObjProperty (id, 11, 1); //daycounter++
	}
}
void CBank::onHeroVisit (const CGHeroInstance * h) const
{
	if (bc != NULL)
	{
		int banktext = 0;
		switch (ID)
		{
			case 16: //generic bank
				banktext = 32;
				break;
			case 24:
				banktext = 41;
				break;
			case 25: //utopia
				banktext = 47;
				break;
			case 84: //crypt
				banktext = 119;
				break;
			case 85: //shipwreck
				banktext = 122;
				break;
		}
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundBase::DANGER;
		std::string desc =  VLC->generaltexth->advobtxt[banktext];
		boost::algorithm::replace_first (desc, "%s", VLC->generaltexth->names[ID]);
		bd.text << desc;
		cb->showBlockingDialog (&bd, boost::bind (&CBank::fightGuards, this, h, _1));
	}
	else
	{
		InfoWindow iw;
		if (ID == 85)
		{
			iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
			GiveBonus gbonus;
			gbonus.hid = h->id;
			gbonus.bonus.duration = HeroBonus::ONE_BATTLE;
			gbonus.bonus.source = HeroBonus::OBJECT;
			gbonus.bonus.id = ID;
			gbonus.bdescr << "\n" << VLC->generaltexth->arraytxt[ID];
			gbonus.bonus.type = HeroBonus::MORALE;
			gbonus.bonus.val = -1;
			cb->giveHeroBonus(&gbonus);
		}
		iw.soundID = soundBase::GRAVEYARD;
		iw.player = h->getOwner();
		std::string desc = VLC->generaltexth->advobtxt[33];
		boost::algorithm::replace_first (desc, "%s", VLC->generaltexth->names[ID]);
		iw.text << desc;
		cb->showInfoDialog(&iw);
	}
}
void CBank::fightGuards (const CGHeroInstance * h, ui32 accept) const 
{
	if (accept)
	{
		cb->setObjProperty (id, 17, ran()); //get army
		cb->startBattleI (h, this, true, boost::bind (&CBank::endBattle, this, h, _1));
	}
}
void CBank::endBattle (const CGHeroInstance *h, const BattleResult *result) const
{
	if (result->winner == 0)
	{
		int textID = -1;
		InfoWindow iw;
		switch (ID)
		{
			case 16: //generic bank
				textID = 34;
				break;
			case 24: //derelict ship
				if (bc->resources.size() != 0)
					textID = 43;
				else
				{
					textID = 42;
					iw.components.push_back (Component (Component::MORALE, 0 , -2, 0));
				}
				break;
			case 25: //utopia
				textID = 47;
				break;
			case 84: //crypt
				textID = 121;
				break;
			case 85: //shipwreck
				if (bc->resources.size() != 0)
					textID = 124;
				else
				{
					textID = 123;
					
				}
				break;
		}
		iw.text.addTxt (MetaString::ADVOB_TXT, textID);
		iw.player = h->getOwner();
		//grant resources
		for (int it = 0; it < bc->resources.size(); it++)
		{					
			iw.components.push_back (Component (Component::RESOURCE, it, bc->resources[it], 0));
			cb->giveResource (h->getOwner(), it, bc->resources[it]);
		}
		//grant artifacts
		for (std::vector<ui32>::const_iterator it = artifacts.begin(); it != artifacts.end(); it++)
		{
			iw.components.push_back (Component (Component::ARTIFACT, *it, 0, 0));
			iw.text.addReplacement (MetaString::ART_NAMES, *it);
			cb->giveHeroArtifact (*it, h->id ,-2);
		}
		//grant creatures
		CCreatureSet ourArmy;
		for (std::vector< std::pair <ui16, ui32> >::const_iterator it = bc->creatures.begin(); it != bc->creatures.end(); it++)
		{
			int slot = ourArmy.getSlotFor (it->second);
			ourArmy.slots[slot] = *it; //assuming we're not going to add multiple stacks of same creature
		}
		cb->giveCreatures (id, cb->getHero (cb->getSelectedHero()), &ourArmy);
			cb->setObjProperty (id, 15, 0); //bc = NULL
	}
	else
		cb->setObjProperty (id, 14, ran()); //reset
}

void CGKeys::setPropertyDer (ui8 what, ui32 val) //101-108 - enable key for player 1-8
{
	if (what >= 101 && what <= (100 + PLAYER_LIMIT))
		playerKeyMap.find(what-101)->second.insert(val);
}

bool CGKeys::wasMyColorVisited (int player) const
{
	if (vstd::contains(playerKeyMap[player], subID)) //creates set if it's not there
		return true;
	else
		return false;
}

const std::string & CGKeymasterTent::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID];
	if (wasMyColorVisited (cb->getCurrentPlayer()) )//TODO: use local player, not current
		hoverName += "\n" + VLC->generaltexth->allTexts[352];
	else
		hoverName += "\n" + VLC->generaltexth->allTexts[353];
	return hoverName;
}

void CGKeymasterTent::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.soundID = soundBase::CAVEHEAD;
	iw.player = h->getOwner();
	if (!wasMyColorVisited (h->getOwner()) )
	{
		cb->setObjProperty(id, h->tempOwner+101, subID);
		iw.text << std::pair<ui8,ui32>(11,19);
	}
	else
		iw.text << std::pair<ui8,ui32>(11,20);
    cb->showInfoDialog(&iw);
}

void CGBorderGuard::initObj()
{
	blockVisit = true;
}

const std::string & CGBorderGuard::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID];
	if (wasMyColorVisited (cb->getCurrentPlayer()) )//TODO: use local player, not current
		hoverName += "\n" + VLC->generaltexth->allTexts[352];
	else
		hoverName += "\n" + VLC->generaltexth->allTexts[353];
	return hoverName;
}

void CGBorderGuard::onHeroVisit( const CGHeroInstance * h ) const 
{
	if (wasMyColorVisited (h->getOwner()) )
	{
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundBase::QUEST;
		bd.text.addTxt (MetaString::ADVOB_TXT, 17);
		cb->showBlockingDialog (&bd, boost::bind (&CGBorderGuard::openGate, this, h, _1));	
	}	
	else
	{
		InfoWindow iw;
		iw.player = h->getOwner();
		iw.soundID = soundBase::CAVEHEAD;
		iw.text << std::pair<ui8,ui32>(11,18);
		cb->showInfoDialog (&iw);
	}
}

void CGBorderGuard::openGate(const CGHeroInstance *h, ui32 accept) const
{
	if (accept)
		cb->removeObject(id);
}

void CGBorderGate::onHeroVisit( const CGHeroInstance * h ) const //TODO: passability 
{
	InfoWindow iw;
	iw.player = h->getOwner();
	if (!wasMyColorVisited (h->getOwner()) )
	{
		iw.text << std::pair<ui8,ui32>(11,18);
		cb->showInfoDialog(&iw);
	}
}

void CGMagi::initObj()
{
	if (ID == 27)
	{
		blockVisit = true;
		eyelist[subID].push_back(id);
	}
}
void CGMagi::onHeroVisit(const CGHeroInstance * h) const
{
	if (ID == 37)
	{
		InfoWindow iw;
		CenterView cv;
		FoWChange fw;
		cv.player = iw.player = fw.player = h->tempOwner;

		iw.soundID = soundBase::LIGHTHOUSE;
		iw.player = h->tempOwner;
		iw.text.addTxt (MetaString::ADVOB_TXT, 61);
		cb->showInfoDialog(&iw);

		fw.mode = 1;
		std::vector<si32>::iterator it;
		for (it = eyelist[subID].begin(); it < eyelist[subID].end(); it++)
		{
			const CGObjectInstance *eye = cb->getObj(*it);

			cb->getTilesInRange (fw.tiles, eye->pos, 5, h->tempOwner, 1);
			cb->sendAndApply(&fw);
			cv.pos = eye->pos;
			cv.focusTime = 2000;
			cb->sendAndApply(&cv);
		}	
		cv.pos = h->getPosition(false);
		cb->sendAndApply(&cv);	
	}
	else if (ID == 27)
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text.addTxt (MetaString::ADVOB_TXT, 48);
		cb->showInfoDialog(&iw);
	}

}
void CGBoat::initObj()
{
	hero = NULL;
}

void CGSirens::initObj()
{
	blockVisit = true;
}

const std::string & CGSirens::getHoverText() const
{
	getNameVis(hoverName);
	return hoverName;
}

void CGSirens::onHeroVisit( const CGHeroInstance * h ) const
{
	int message;
	InfoWindow iw;
	iw.soundID = soundBase::DANGER;
	iw.player = h->tempOwner;
	if(h->getBonus(HeroBonus::OBJECT,ID)) //has already visited Sirens
	{
		iw.text.addTxt(11,133);
	}
	else
	{
		giveDummyBonus(h->id, HeroBonus::ONE_BATTLE);
		int xp = 0;
		SetGarrisons sg;
		sg.garrs[h->id] = h->army;
		for (std::map<si32,std::pair<ui32,si32> >::const_iterator i = h->army.slots.begin(); i != h->army.slots.end(); i++)
		{
			int drown = (int)(i->second.second * 0.3);
			if(drown)
			{
				sg.garrs[h->id].slots[i->first].second -= drown;
				xp += drown * VLC->creh->creatures[i->second.first].hitPoints;
			}
		}

		if(xp)
		{
			iw.text.addTxt(11,132);
			iw.text.addReplacement(xp);
			cb->sendAndApply(&sg);
		}
		else
		{
			iw.text.addTxt(11,134);
		}
	}
	cb->showInfoDialog(&iw);
}

void IShipyard::getBoatCost( std::vector<si32> &cost ) const
{
	cost.resize(RESOURCE_QUANTITY);
	cost[0] = 10;
	cost[6] = 1000;
}

//bool IShipyard::validLocation() const
//{
//	std::vector<int3> offsets;
//	getOutOffsets(offsets);
//
//	TerrainTile *tile;
//	for(int i = 0; i < offsets.size(); i++)
//		if((tile = IObjectInterface::cb->getTile(o->pos + offsets[i]))  &&  tile->tertype == TerrainTile::water) //tile is in the map and is water
//			return true;
//	return false;
//}

int3 IShipyard::bestLocation() const
{
	std::vector<int3> offsets;
	getOutOffsets(offsets);

	TerrainTile *tile;
	for(int i = 0; i < offsets.size(); i++)
		if((tile = IObjectInterface::cb->getTile(o->pos + offsets[i]))  &&  tile->tertype == TerrainTile::water) //tile is in the map and is water
			return o->pos + offsets[i];
	return int3(-1,-1,-1);
}

IShipyard::IShipyard(const CGObjectInstance *O) 
	: o(O)
{
}

int IShipyard::state() const
{
	int3 tile = bestLocation();
	TerrainTile *t = IObjectInterface::cb->getTile(tile);
	if(!t)
		return 3; //no water
	else if(!t->blockingObjects.size())
		return 0; //OK
	else if(t->blockingObjects.front()->ID == 8)
		return 1; //blocked with boat
	else
		return 2; //blocked
}

IShipyard * IShipyard::castFrom( CGObjectInstance *obj )
{
	if(obj->ID == TOWNI_TYPE)
	{
		return static_cast<CGTownInstance*>(obj);
	}
	else if(obj->ID == 87)
	{
		return static_cast<CGShipyard*>(obj);
	}
	else
	{
		tlog1 << "Cannot cast to IShipyard object with ID " << obj->ID << std::endl;
		return NULL;
	}
}

const IShipyard * IShipyard::castFrom( const CGObjectInstance *obj )
{
	return castFrom(const_cast<CGObjectInstance*>(obj));
}

CGShipyard::CGShipyard()
	:IShipyard(this)
{
}

void CGShipyard::getOutOffsets( std::vector<int3> &offsets ) const
{
	offsets += int3(1,0,0), int3(-3,0,0), int3(1,1,0), int3(-3,1,0), int3(1,-1,0), int3(-3,-1,0), 
		int3(-2,-1,0), int3(0,-1,0), int3(-1,-1,0), int3(-2,1,0), int3(0,1,0), int3(-1,1,0);
}

void CGShipyard::onHeroVisit( const CGHeroInstance * h ) const
{
	if(tempOwner != h->tempOwner)
		cb->setOwner(id, h->tempOwner);

	int s = state();
	if(s)
	{
		InfoWindow iw;
		iw.player = tempOwner;
		switch(s)
		{
		case 1: 
			iw.text.addTxt(MetaString::GENERAL_TXT, 51);
			break;
		case 2:
			iw.text.addTxt(MetaString::ADVOB_TXT, 189);
			break;
		case 3:
			tlog1 << "Shipyard without water!!! " << pos << "\t" << id << std::endl;
			return;
		}

		cb->showInfoDialog(&iw);
	}
	else
	{
		OpenWindow ow;
		ow.id1 = id;
		ow.id2 = h->id;
		ow.window = OpenWindow::SHIPYARD_WINDOW;
		cb->sendAndApply(&ow);
	}
}

void CCartographer::onHeroVisit( const CGHeroInstance * h ) const 
{
	if (!hasVisited (h->getOwner()) ) //if hero has not visited yet this cartographer
	{
		if (cb->getResource(h->tempOwner, 6) >= 1000) //if he can afford a map
		{
			//ask if he wants to buy one
			int text;
			if (cb->getTile(pos)->tertype == 8) //water
					text = 25;
			else
			{
				if (pos.z == 0)
					text = 26;
				else
					text = 27;
			}
			BlockingDialog bd (true, false);
			bd.player = h->getOwner();
			bd.soundID = soundBase::LIGHTHOUSE;
			bd.text.addTxt (MetaString::ADVOB_TXT, text);
			cb->showBlockingDialog (&bd, boost::bind (&CCartographer::buyMap, this, h, _1));
		}
		else //if he cannot afford
		{
			InfoWindow iw;
			iw.player = h->getOwner();
			iw.soundID = soundBase::CAVEHEAD;
			iw.text << std::pair<ui8,ui32>(11,28);
			cb->showInfoDialog (&iw);
		}
	}	
	else //if he already visited carographer
	{
		InfoWindow iw;
		iw.player = h->getOwner();
		iw.soundID = soundBase::CAVEHEAD;
		iw.text << std::pair<ui8,ui32>(11,24);
		cb->showInfoDialog (&iw);
	}
}

void CCartographer::buyMap (const CGHeroInstance *h, ui32 accept) const
{
	if (accept) //if hero wants to buy map
	{
		cb->giveResource (h->tempOwner, 6, -1000);
		FoWChange fw;
		fw.player = h->tempOwner;
		int surface = 0;
		if (cb->getTile(pos)->tertype == 8) //water
			surface = 2;
		else
			surface = 1;

		//reveal apropriate tiles
		cb->getAllTiles (fw.tiles, h->tempOwner, pos.z, surface);
		cb->sendAndApply (&fw);
		cb->setObjProperty (id, 10, h->tempOwner);
	}
}

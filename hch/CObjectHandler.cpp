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
#include "CCreatureHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/IGameCallback.h"
#include "../lib/CGameState.h"
#include "../lib/NetPacks.h"
#include "../StartInfo.h"
#include "../lib/map.h"
#include <sstream>
#include <SDL_stdinc.h>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
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
std::vector<std::pair<int, int> > CGTeleport::gates;
IGameCallback * IObjectInterface::cb = NULL;
DLL_EXPORT void loadToIt(std::string &dest, const std::string &src, int &iter, int mode);
extern CLodHandler * bitmaph;
extern boost::rand48 ran;
std::map <ui8, std::set <ui8> > CGKeys::playerKeyMap;
std::map <si32, std::vector<si32> > CGMagi::eyelist;
ui8 CGObelisk::obeliskCount; //how many obelisks are on map
std::map<ui8, ui8> CGObelisk::visited; //map: team_id => how many obelisks has been visited

std::vector<const CArtifact *> CGTownInstance::merchantArtifacts;
std::vector<int> CGTownInstance::universitySkills;

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

void IObjectInterface::postInit()
{}

void IObjectInterface::preInit()
{}

void CPlayersVisited::setPropertyDer( ui8 what, ui32 val )
{
	if(what == 10)
		players.insert((ui8)val);
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

	is >> guardInfo.second;
	//one getline just does not work... probably a kind of left whitespace
	is.getline(buffer, MAX_BUF, '\t');
	is.getline(buffer, MAX_BUF, '\t');
	creName = buffer;

	if( std::string(buffer) == "None" ) //no creature to be added
		return;

	//look for the best creature that is described by given name
	if( vstd::contains(VLC->creh->nameToID, creName) )
	{
		guardInfo.first = VLC->creh->nameToID[creName];
	}
	else
	{
		for(int g=0; g<VLC->creh->creatures.size(); ++g)
		{
			if(VLC->creh->creatures[g]->namePl == creName
				|| VLC->creh->creatures[g]->nameRef == creName
				|| VLC->creh->creatures[g]->nameSing == creName)
			{
				guardInfo.first = VLC->creh->creatures[g]->idNumber;
			}
		}
	}
	
	if(guards)
		bc.guards.push_back(guardInfo);
	else //given creatures
		bc.creatures.push_back(guardInfo);
}
void CObjectHandler::readConfigLine(std::ifstream &istr, int g)
{
	banksInfo[g].push_back(new BankConfig);

	BankConfig &bc = *banksInfo[g].back();
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
			
	//a dirty trick to make it work if there is no 0 for 0 quantity (like in grotto - last entry)
	char buft[52];
	istr.getline(buft, 50, '\t');
	for(int h=0; h<7; ++h)
	{
		istr.getline(buft, 50, '\t');
		if(buft[0] == '\0')
			bc.resources[h] = 0;
		else
			bc.resources[h] = SDL_atoi(buft);
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
		std::ifstream ifs(DATA_DIR "/config/cregens.txt");
		while(!ifs.eof())
		{
			int dw, cr;
			ifs >> dw >> cr;
			cregens[dw]=cr;
		}
		tlog5 << "\t\tDone loading objects!\n";

		ifs.close();
		ifs.clear();

		int k = -1;
		ifs.open(DATA_DIR "/config/resources.txt");
		ifs >> k;
		int pom;
		for(int i=0;i<k;i++)
		{
			ifs >> pom;
			resVals.push_back(pom);
		}
		tlog5 << "\t\tDone loading resource prices!\n";
	}

	std::ifstream istr;
	istr.open(DATA_DIR "/config/bankconfig.txt", std::ios_base::binary);
	if(!istr.is_open())
	{
		tlog1 << "No config/bankconfig.txt file !!!\n";
	}

	const int MAX_BUF = 5000;
	char buffer[MAX_BUF + 1];

	//omitting unnecessary lines
	istr.getline(buffer, MAX_BUF);
	istr.getline(buffer, MAX_BUF);
	
	for(int g=0; g<21; ++g) //TODO: remove hardcoded value
	{
		//reading name
		istr.getline(buffer, MAX_BUF, '\t');
		creBanksNames[g] = std::string(buffer);
		//remove trailing new line characters
		while(creBanksNames[g][0] == 10 || creBanksNames[g][0] == 13)
			creBanksNames[g].erase(creBanksNames[g].begin());

		for(int i=0; i<4; ++i) //reading levels
		{
			readConfigLine(istr,g);
		}
	}
	//reading name
	istr.getline(buffer, MAX_BUF, '\t');
	creBanksNames[21] = std::string(buffer);
	while(creBanksNames[21][0] == 10 || creBanksNames[21][0] == 13)
			creBanksNames[21].erase(creBanksNames[21].begin());
	readConfigLine(istr,21); //pyramid
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
	if((defInfo->coverageMap[y] >> (7-(x) )) & 1 
		||  (defInfo->shadowCoverage[y] >> (7-(x) )) & 1)
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
	switch(ID)
	{
	case 95:
		blockVisit = true;
		break;
	}
}

void CGObjectInstance::setProperty( ui8 what, ui32 val )
{
	switch(what)
	{
	case ObjProperty::OWNER:
		tempOwner = val;
		break;
	case ObjProperty::BLOCKVIS:
		blockVisit = val;
		break;
	case ObjProperty::ID:
		ID = val;
		break;
	case ObjProperty::SUBID:
		subID = val;
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
void CGObjectInstance::getSightTiles(std::set<int3> &tiles) const //returns reference to the set
{
	cb->getTilesInRange(tiles, getSightCenter(), getSightRadious(), tempOwner, 1);
}
void CGObjectInstance::hideTiles(int ourplayer, int radius) const
{
	for (std::map<ui8, TeamState>::iterator i = cb->gameState()->teams.begin(); i != cb->gameState()->teams.end(); i++)
	{
		if ( !vstd::contains(i->second.players, ourplayer ))//another team
		{
			for (std::set<ui8>::iterator j = i->second.players.begin(); j != i->second.players.end(); j++)
				if ( cb->getPlayerState(*j)->status == PlayerState::INGAME )//seek for living player (if any)
				{
					FoWChange fw;
					fw.mode = 0;
					fw.player = *j;
					cb->getTilesInRange (fw.tiles, pos, radius, (*j), -1);
					cb->sendAndApply (&fw);
					break;
				}
		}
	}
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
		if(!h->hasBonusFrom(Bonus::OBJECT,ID))
			hname += " " + VLC->generaltexth->allTexts[353]; //not visited
		else
			hname += " " + VLC->generaltexth->allTexts[352]; //visited
	}
}

void CGObjectInstance::giveDummyBonus(int heroID, ui8 duration) const
{
	GiveBonus gbonus;
	gbonus.bonus.type = Bonus::NONE;
	gbonus.id = heroID;
	gbonus.bonus.duration = duration;
	gbonus.bonus.source = Bonus::OBJECT;
	gbonus.bonus.id = ID;
	cb->giveHeroBonus(&gbonus);
}

void CGObjectInstance::onHeroVisit( const CGHeroInstance * h ) const
{
	switch(ID)
	{
	case 35: //Hill fort
		{
			OpenWindow ow;
			ow.window = OpenWindow::HILL_FORT_WINDOW;
			ow.id1 = id;
			ow.id2 = h->id;
			cb->sendAndApply(&ow);
		}
		break;
	case 80: //Sanctuary
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.soundID = soundBase::GETPROTECTION;
			iw.text.addTxt(MetaString::ADVOB_TXT, 114);  //You enter the sanctuary and immediately feel as if a great weight has been lifted off your shoulders.  You feel safe here.
			cb->sendAndApply(&iw);
		}
		break;
	case 95: //Tavern
		{
			OpenWindow ow;
			ow.window = OpenWindow::TAVERN_WINDOW;
			ow.id1 = h->id;
			ow.id2 = id;
			cb->sendAndApply(&ow);
		}
		break;
	}
}

ui8 CGObjectInstance::getPassableness() const
{
	return 0;
}

bool CGObjectInstance::hasShadowAt( int x, int y ) const
{
	if( (defInfo->shadowCoverage[y] >> (7-(x) )) & 1 )
		return true;
	return false;
}

int3 CGObjectInstance::visitablePos() const
{
	return pos - getVisitableOffset();
}

static int lowestSpeed(const CGHeroInstance * chi)
{
	if(!chi->Slots().size())
	{
		tlog1 << "Error! Hero " << chi->id << " ("<<chi->name<<") has no army!\n";
		return 20;
	}
	TSlots::const_iterator i = chi->Slots().begin();
	//TODO? should speed modifiers (eg from artifacts) affect hero movement?
	int ret = (i++)->second.valOfBonuses(Bonus::STACKS_SPEED);
	for (;i!=chi->Slots().end();i++)
	{
		ret = std::min(ret, i->second.valOfBonuses(Bonus::STACKS_SPEED));
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
		ret = type->heroClass->terrCosts[from.tertype];
		ret = std::max(ret - 25*unsigned(getSecSkillLevel(0)), 100u); //reduce 25% of terrain penalty for each pathfinding level
	}
	return ret;
}
#if 0
// Unused and buggy method. 
//  - for loop is wrong. will not find all creatures. must use iterator instead.
//  - -> is the slot number. use second->first for creature index
// Is lowestSpeed() the correct equivalent ?
unsigned int CGHeroInstance::getLowestCreatureSpeed() const
{
	unsigned int sl = 100;
	for(size_t h=0; h < stacksCount(); ++h)
	{
		if(VLC->creh->creatures[Slots().find(h)->first]->speed<sl)
			sl = VLC->creh->creatures[Slots().find(h)->first]->speed;
	}
	return sl;
}
#endif
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
	return si32(getPrimSkillLevel(3) * (100.0f + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, 24)) / 10.0f);
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
	return hasBonusOfType(Bonus::FLYING_MOVEMENT) || hasBonusOfType(Bonus::WATER_WALKING);
}

int CGHeroInstance::getPrimSkillLevel(int id) const
{
	int ret = valOfBonuses(Bonus::PRIMARY_SKILL, id);
	amax(ret, id/2); //minimal value is 0 for attack and defense and 1 for spell power and knowledge
	return ret;
}

ui8 CGHeroInstance::getSecSkillLevel(const int & ID) const
{
	for(size_t i=0; i < secSkills.size(); ++i)
		if(secSkills[i].first==ID)
			return secSkills[i].second;
	return 0;
}

void CGHeroInstance::setSecSkillLevel(int which, int val, bool abs)
{
	if(getSecSkillLevel(which) == 0)
	{
		secSkills.push_back(std::pair<int,int>(which, val));
		updateSkill(which, val);
	}
	else
	{
		for (unsigned i=0; i<secSkills.size(); i++)
		{
			if(secSkills[i].first == which)
			{
				if(abs)
					secSkills[i].second = val;
				else
					secSkills[i].second += val;

				if(secSkills[i].second > 3) //workaround to avoid crashes when same sec skill is given more than once
				{
					tlog1 << "Warning: Skill " << which << " increased over limit! Decreasing to Expert.\n";
					secSkills[i].second = 3;
				}
				updateSkill(which, secSkills[i].second); //when we know final value
			}
		}
	}
}

int CGHeroInstance::maxMovePoints(bool onLand) const
{
	int base = -1;
	if(onLand)
	{
		static const int moveForSpeed[] = { 1500, 1560, 1630, 1700, 1760, 1830, 1900, 1960, 2000 }; //first element for 3 and lower; last for 11 and more
		int index = lowestSpeed(this) - 3;
		amin(index, ARRAY_COUNT(moveForSpeed)-1);
		amax(index, 0);
		base = moveForSpeed[index];
	}
	else
	{
		base = 1500; //on water base movement is always 1500 (speed of army doesn't matter)
	}
	
	int bonus = valOfBonuses(Bonus::MOVEMENT) + (onLand ? valOfBonuses(Bonus::LAND_MOVEMENT) : valOfBonuses(Bonus::SEA_MOVEMENT));

	double modifier = 0;
	if(onLand)
	{
		//logistics:
		modifier = valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, 2) / 100.0f;
	}
	else
	{
		//navigation:
		modifier = valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, 5) / 100.0f;
	}
	return int(base + base*modifier) + bonus;
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
		return VLC->arth->artifacts[id];
	else
		return NULL;
}

// int CGHeroInstance::getSpellSecLevel(int spell) const
// {
// 	int bestslvl = 0;
// 	if(VLC->spellh->spells[spell].air)
// 		if(getSecSkillLevel(15) >= bestslvl)
// 		{
// 			bestslvl = getSecSkillLevel(15);
// 		}
// 	if(VLC->spellh->spells[spell].fire)
// 		if(getSecSkillLevel(14) >= bestslvl)
// 		{
// 			bestslvl = getSecSkillLevel(14);
// 		}
// 	if(VLC->spellh->spells[spell].water)
// 		if(getSecSkillLevel(16) >= bestslvl)
// 		{
// 			bestslvl = getSecSkillLevel(16);
// 		}
// 	if(VLC->spellh->spells[spell].earth)
// 		if(getSecSkillLevel(17) >= bestslvl)
// 		{
// 			bestslvl = getSecSkillLevel(17);
// 		}
// 	return bestslvl;
// }

CGHeroInstance::CGHeroInstance()
 : IBoatGenerator(this)
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
	speciality.nodeType = CBonusSystemNode::SPECIALITY;
}

void CGHeroInstance::initHero(int SUBID)
{
	subID = SUBID;
	initHero();
}

void CGHeroInstance::initHero()
{
	assert(validTypes(true));
	if(ID == HEROI_TYPE)
		initHeroDefInfo();
	if(!type)
		type = VLC->heroh->heroes[subID];
	if(!vstd::contains(spells, 0xffffffff) && type->startingSpell >= 0) //hero starts with a spell
		spells.insert(type->startingSpell);
	else //remove placeholder
		spells -= 0xffffffff;

	if(!vstd::contains(artifWorn, 16) && type->startingSpell >= 0) //no catapult means we haven't read pre-existant set
	{
		VLC->arth->equipArtifact(artifWorn, 17, 0); //give spellbook
	}
	VLC->arth->equipArtifact(artifWorn, 16, 3); //everyone has a catapult

	if(portrait < 0 || portrait == 255)
		portrait = subID;
	if(!hasBonus(Selector::sourceType(Bonus::HERO_BASE_SKILL)))
	{
		pushPrimSkill(PrimarySkill::ATTACK, type->heroClass->initialAttack);
		pushPrimSkill(PrimarySkill::DEFENSE, type->heroClass->initialDefence);
		pushPrimSkill(PrimarySkill::SPELL_POWER, type->heroClass->initialPower);
		pushPrimSkill(PrimarySkill::KNOWLEDGE, type->heroClass->initialKnowledge);
	}
	if(secSkills.size() == 1 && secSkills[0] == std::pair<ui8,ui8>(-1, -1)) //set secondary skills to default
		secSkills = type->secSkillsInit;
	if (!name.length())
		name = type->name;
	if (exp == 0xffffffff)
	{
		initExp();
	}
	else
	{
		level = VLC->heroh->level(exp);
	}

	if (sex == 0xFF)//sex is default
		sex = type->sex;

	setFormation(false);
	if (!stacksCount()) //standard army//initial army
	{
		initArmy();
	}
	assert(validTypes());

	hoverName = VLC->generaltexth->allTexts[15];
	boost::algorithm::replace_first(hoverName,"%s",name);
	boost::algorithm::replace_first(hoverName,"%s", type->heroClass->name);

	if (mana < 0)
		mana = manaLimit();
}

void CGHeroInstance::initArmy(CCreatureSet *dst /*= NULL*/)
{
	if(!dst)
		dst = this;

	int howManyStacks = 0; //how many stacks will hero receives <1 - 3>
	int pom = ran()%100;
	int warMachinesGiven = 0;

	if(pom < 9)
		howManyStacks = 1;
	else if(pom < 79)
		howManyStacks = 2;
	else
		howManyStacks = 3;

	for(int stackNo=0; stackNo < howManyStacks; stackNo++)
	{
		int creID = (VLC->creh->nameToID[type->refTypeStack[stackNo]]);
		int range = type->highStack[stackNo] - type->lowStack[stackNo];
		int count = ran()%(range+1) + type->lowStack[stackNo];

		if(creID>=145 && creID<=149) //war machine
		{
			warMachinesGiven++;
			switch (creID)
			{
			case 145: //catapult
				VLC->arth->equipArtifact(artifWorn, 16, 3);
				break;
			default:
				VLC->arth->equipArtifact(
					artifWorn,
					9+CArtHandler::convertMachineID(creID,true),
					  CArtHandler::convertMachineID(creID,true));
				break;
			}
		}
		else
			dst->addStack(stackNo-warMachinesGiven, CStackInstance(creID, count));
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
		defInfo->blockMap[i] = 255;
		defInfo->visitMap[i] = 0;
		defInfo->coverageMap[i] = 0;
		defInfo->shadowCoverage[i] = 0;
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
		if( cb->gameState()->getPlayerRelations(tempOwner, h->tempOwner)) //our or ally hero
		{
			//exchange
			cb->heroExchange(h->id, id);
		}
		else //battle
		{
			if(visitedTown) //we're in town
				visitedTown->onHeroVisit(h); //town will handle attacking
			else
				cb->startBattleI(h,	this);
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
			cb->setObjProperty(id, ObjProperty::ID, HEROI_TYPE); //set ID to 34
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
	speciality.growthsWithLevel = false;
	Bonus bonus;

	if(!type)
		return; //TODO support prison

	for (std::vector<SSpecialtyInfo>::const_iterator it = type->spec.begin(); it != type->spec.end(); it++)
	{
		bonus.val = it->val;
		bonus.id = id; //from the hero, speciality has no unique id
		bonus.duration = Bonus::PERMANENT;
		bonus.source = Bonus::HERO_SPECIAL;
		switch (it->type)
		{
			case 1:// creature speciality
				{
					speciality.growthsWithLevel = true;

					const CCreature &specCreature = *VLC->creh->creatures[it->additionalinfo]; //creature in which we have specialty

					int creLevel = specCreature.level;
					if(!creLevel) //TODO: set fixed level for War Machines
					{
						if(it->additionalinfo == 146)
							creLevel = 5; //treat ballista as 5-level
						else
						{
							tlog2 << "Warning: unknown level of " << specCreature.namePl << std::endl;
							continue;
						}
					}

					bonus.limiter = new CCreatureTypeLimiter (specCreature, true); //with upgrades
					bonus.type = Bonus::PRIMARY_SKILL; 
					bonus.additionalInfo = it->additionalinfo;
					bonus.valType = Bonus::ADDITIVE_VALUE;

					bonus.subtype = PrimarySkill::ATTACK;
					speciality.bonuses.push_back (bonus);

					bonus.subtype = PrimarySkill::DEFENSE;
					speciality.bonuses.push_back (bonus);
					//values will be calculated later

					bonus.type = Bonus::STACKS_SPEED;
					bonus.val = 1; //+1 speed
					speciality.bonuses.push_back (bonus);
				}
				break;
			case 2://secondary skill
				speciality.growthsWithLevel = true;
				bonus.type = Bonus::SPECIAL_SECONDARY_SKILL; //needs to be recalculated with level, based on this value
				bonus.valType = Bonus::BASE_NUMBER; // to receive nonzero value
				bonus.subtype = it->subtype; //skill id
				bonus.val = it->val; //value per level, in percent
				speciality.bonuses.push_back (bonus);
				switch (it->additionalinfo)
				{
					case 0: //normal
						bonus.valType = Bonus::PERCENT_TO_BASE;
						break;
					case 1: //when it's navigation or there's no 'base' at all
						bonus.valType = Bonus::PERCENT_TO_ALL;
						break;
				}
				bonus.type = Bonus::SECONDARY_SKILL_PREMY; //value will be calculated later
				speciality.bonuses.push_back(bonus);
				break;
			case 3://spell damage bonus, level dependant but calculated elsehwere
				bonus.type = Bonus::SPECIAL_SPELL_LEV;
				bonus.subtype = it->subtype;
				speciality.bonuses.push_back (bonus);
				break;
			case 4://creature stat boost
				switch (it->subtype)
				{
					case 1://attack
						bonus.type = Bonus::PRIMARY_SKILL;
						bonus.subtype = PrimarySkill::ATTACK;
						break;
					case 2://defense
						bonus.type = Bonus::PRIMARY_SKILL;
						bonus.subtype = PrimarySkill::DEFENSE;
						break;
					case 3:
						bonus.type = Bonus::CREATURE_DAMAGE;
						bonus.subtype = 0; //both min and max
						break;
					case 4://hp
						bonus.type = Bonus::STACK_HEALTH;
						break;
					case 5:
						bonus.type = Bonus::STACKS_SPEED;
						break;
					default:
						continue;
				}
				bonus.valType = Bonus::ADDITIVE_VALUE;
				bonus.limiter = new CCreatureTypeLimiter (*VLC->creh->creatures[it->additionalinfo], true);
				speciality.bonuses.push_back (bonus);
				break;
			case 5://spell damage bonus in percent
				bonus.type = Bonus::SPECIFIC_SPELL_DAMAGE;
				bonus.valType = Bonus::BASE_NUMBER; // current spell system is screwed
				bonus.subtype = it->subtype; //spell id
				speciality.bonuses.push_back (bonus);
				break;
			case 6://damage bonus for bless (Adela)
				bonus.type = Bonus::SPECIAL_BLESS_DAMAGE;
				bonus.additionalInfo = it->additionalinfo; //damage factor
				speciality.bonuses.push_back (bonus);
				break;
			case 7://maxed mastery for spell
				bonus.type = Bonus::MAXED_SPELL;
				speciality.bonuses.push_back (bonus);
				break;
			case 8://peculiar spells - enchantments
				bonus.type = Bonus::SPECIAL_PECULIAR_ENCHANT;
				bonus.subtype = it->subtype; //0, 1 for Coronius
				speciality.bonuses.push_back (bonus);
				break;
			case 9://upgrade creatures
			{
				std::vector<CCreature*>* creatures = &VLC->creh->creatures;
				bonus.type = Bonus::SPECIAL_UPGRADE;
				bonus.subtype = it->subtype; //base id
				bonus.additionalInfo = it->additionalinfo; //target id
				speciality.bonuses.push_back (bonus);

				for (std::set<ui32>::iterator i = (*creatures)[it->subtype]->upgrades.begin();
					i != (*creatures)[it->subtype]->upgrades.end(); i++)
				{
					bonus.subtype = *i; //propagate for regular upgrades of base creature
					speciality.bonuses.push_back (bonus);
				}
				break;
			}
			case 10://resource generation
				bonus.type = Bonus::GENERATE_RESOURCE;
				bonus.subtype = it->subtype;
				speciality.bonuses.push_back (bonus);
				break;
			case 11://starting skill with mastery (Adrienne)
				cb->changeSecSkill (id, it->val, it->additionalinfo); //simply give it and forget
				break;
			case 12://army speed
				bonus.type = Bonus::STACKS_SPEED;
				speciality.bonuses.push_back (bonus);
				break;
			case 13://Dragon bonuses (Mutare)
				bonus.type = Bonus::PRIMARY_SKILL;
				bonus.valType = Bonus::ADDITIVE_VALUE;
				switch (it->subtype)
				{
					case 1:
						bonus.subtype = PrimarySkill::ATTACK;
						break;
					case 2:
						bonus.subtype = PrimarySkill::DEFENSE;
						break;
				}
				bonus.limiter = new HasAnotherBonusLimiter(Bonus::DRAGON_NATURE);
				speciality.bonuses.push_back (bonus);
				break;
			default:
				tlog2 << "Unexpected hero speciality " << type <<'\n';
		}
	}
	//initialize bonuses
	for (std::vector<std::pair<ui8,ui8> >::iterator it = secSkills.begin(); it != secSkills.end(); it++)
		updateSkill(it->first, it->second);
	UpdateSpeciality();

	mana = manaLimit(); //after all bonuses are taken into account, make sure this line is the last one
}
void CGHeroInstance::UpdateSpeciality()
{
	if (speciality.growthsWithLevel)
	{
		std::vector<CCreature*>* creatures = &VLC->creh->creatures;
		for (std::list<Bonus>::iterator it = speciality.bonuses.begin(); it != speciality.bonuses.end(); it++)
		{
			switch (it->type)
			{
				case Bonus::SECONDARY_SKILL_PREMY:
					it->val = (speciality.valOfBonuses(Bonus::SPECIAL_SECONDARY_SKILL, it->subtype) * level);
					break; //use only hero skills as bonuses to avoid feedback loop
				case Bonus::PRIMARY_SKILL: //for crearures, that is
					int creLevel = (*creatures)[it->additionalInfo]->level;
					if(!creLevel)
					{
						if(it->additionalInfo == 146)
							creLevel = 5; //treat ballista as 5-level
						else
						{
							tlog2 << "Warning: unknown level of " << (*creatures)[it->additionalInfo]->namePl << std::endl;
							continue;
						}
					}

					double primSkillModifier = (int)(level / creLevel) / 20.0;
					int param;
					switch (it->subtype)
					{
						case PrimarySkill::ATTACK:
							param = (*creatures)[it->additionalInfo]->attack;
							break;
						case PrimarySkill::DEFENSE:
							param = (*creatures)[it->additionalInfo]->defence;
							break;
					}
					it->val = ceil(param * (1 + primSkillModifier)) - param; //yep, overcomplicated but matches original
					break;
			}
		}
	}
}
void CGHeroInstance::updateSkill(int which, int val)
{
	int skillVal = 0;
	switch (which)
	{
		case 1: //Archery
			switch (val)
			{
				case 1:
					skillVal = 10; break;
				case 2:
					skillVal = 25; break;
				case 3:
					skillVal = 50; break;
			}
			break;
		case 2: //Logistics
			skillVal = 10 * val; break;
		case 5: //Navigation
			skillVal = 50 * val; break;
		case 8: //Mysticism
			skillVal = val; break;
		case 11: //eagle Eye
			skillVal = 30 + 10 * val; break;
		case 12: //Necromancy
			skillVal = 10 * val; break;
		case 22: //Offense
			skillVal = 10 * val; break;
		case 23: //Armorer
			skillVal = 5 * val; break;
		case 24: //Intelligence
			skillVal = 25 << val-1; break;
		case 25: //Sorcery
			skillVal = 5 * val; break;
		case 26: //Resistance
			skillVal = 5 << val-1; break;
		case 27: //First Aid
			skillVal = 25 + 25*val; break;
	}
	if (skillVal) //we don't need bonuses of other types here
	{
		Bonus * b = bonuses.getFirst(Selector::typeSybtype(Bonus::SECONDARY_SKILL_PREMY, which) && Selector::sourceType(Bonus::SECONDARY_SKILL));
		if (b) //only local hero bonus
		{
			b->val = skillVal;
		}
		else
		{
			Bonus bonus(Bonus::PERMANENT, Bonus::SECONDARY_SKILL_PREMY, id, skillVal, ID, which, Bonus::BASE_NUMBER);
			bonus.source = Bonus::SECONDARY_SKILL;
			bonuses.push_back (bonus);
		}
	}
}
void CGHeroInstance::setPropertyDer( ui8 what, ui32 val )
{
	if(what == ObjProperty::PRIMARY_STACK_COUNT)
		setStackCount(0, val);
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

ui8 CGHeroInstance::getSpellSchoolLevel(const CSpell * spell, int *outSelectedSchool) const
{
	si16 skill = -1; //skill level

#define TRY_SCHOOL(schoolName, schoolMechanicsId, schoolOutId)	\
	if(spell-> schoolName)									\
	{															\
		int thisSchool = std::max<int>(getSecSkillLevel(14 + (schoolMechanicsId)), valOfBonuses(Bonus::MAGIC_SCHOOL_SKILL, 1 << (schoolMechanicsId))); \
		if(thisSchool > skill)									\
		{														\
			skill = thisSchool;									\
			if(outSelectedSchool)								\
				*outSelectedSchool = schoolOutId;				\
		}														\
	}
	TRY_SCHOOL(fire, 0, 1)
	TRY_SCHOOL(air, 1, 0)
	TRY_SCHOOL(water, 2, 2)
	TRY_SCHOOL(earth, 3, 3)
#undef TRY_SCHOOL



	amax(skill, valOfBonuses(Bonus::MAGIC_SCHOOL_SKILL, 0)); //any school bonus
	if (hasBonusOfType(Bonus::SPELL, spell->id))
	{
		amax(skill, valOfBonuses(Bonus::SPELL, spell->id));
	}
	assert(skill >= 0 && skill <= 3);
	return skill;
}

bool CGHeroInstance::canCastThisSpell(const CSpell * spell) const
{
	if(!getArt(17)) //if hero has no spellbook
		return false;

	if(vstd::contains(spells, spell->id) //hero has this spell in spellbook
		|| (spell->air && hasBonusOfType(Bonus::AIR_SPELLS)) // this is air spell and hero can cast all air spells
		|| (spell->fire && hasBonusOfType(Bonus::FIRE_SPELLS)) // this is fire spell and hero can cast all fire spells
		|| (spell->water && hasBonusOfType(Bonus::WATER_SPELLS)) // this is water spell and hero can cast all water spells
		|| (spell->earth && hasBonusOfType(Bonus::EARTH_SPELLS)) // this is earth spell and hero can cast all earth spells
		|| hasBonusOfType(Bonus::SPELL, spell->id)
		|| hasBonusOfType(Bonus::SPELLS_OF_LEVEL, spell->level)
		)
		return true;

	return false;
}

/**
 * Calculates what creatures and how many to be raised from a battle.
 * @param battleResult The results of the battle.
 * @return Returns a pair with the first value indicating the ID of the creature
 * type and second value the amount. Both values are returned as -1 if necromancy
 * could not be applied.
 */
CStackInstance CGHeroInstance::calculateNecromancy (const BattleResult &battleResult) const
{
	const ui8 necromancyLevel = getSecSkillLevel(12);

	// Hero knows necromancy.
	if (necromancyLevel > 0) 
	{
		double necromancySkill = valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, 12)/100.0;
		amin(necromancySkill, 1.0); //it's impossible to raise more creatures than all...
		const std::map<ui32,si32> &casualties = battleResult.casualties[!battleResult.winner];
		ui32 raisedUnits = 0;

		// Figure out what to raise and how many.
		const ui32 creatureTypes[] = {56, 58, 60, 64}; // IDs for Skeletons, Walking Dead, Wights and Liches respectively.
		const bool improvedNecromancy = hasBonusOfType(Bonus::IMPROVED_NECROMANCY);
		const CCreature *raisedUnitType = VLC->creh->creatures[creatureTypes[improvedNecromancy ? necromancyLevel : 0]];
		const ui32 raisedUnitHP = raisedUnitType->valOfBonuses(Bonus::STACK_HEALTH);

		//calculate creatures raised from each defeated stack
		for (std::map<ui32,si32>::const_iterator it = casualties.begin(); it != casualties.end(); it++)
		{
			// Get lost enemy hit points convertible to units.
			const ui32 raisedHP = VLC->creh->creatures[it->first]->valOfBonuses(Bonus::STACK_HEALTH) * it->second * necromancySkill;
			raisedUnits += std::min<ui32>(raisedHP / raisedUnitHP, it->second * necromancySkill); //limit to % of HP and % of original stack count
		}

		// Make room for new units.
		int slot = getSlotFor(raisedUnitType->idNumber);
		if (slot == -1) 
		{
			// If there's no room for unit, try it's upgraded version 2/3rds the size.
			raisedUnitType = VLC->creh->creatures[*raisedUnitType->upgrades.begin()];
			raisedUnits = (raisedUnits*2)/3;

			slot = getSlotFor(raisedUnitType->idNumber);
		}
		if (raisedUnits <= 0)
			raisedUnits = 1;

		return CStackInstance(raisedUnitType->idNumber, raisedUnits);
	}

	return CStackInstance();
}

/**
 * Show the necromancy dialog with information about units raised.
 * @param raisedStack Pair where the first element represents ID of the raised creature
 * and the second element the amount.
 */
void CGHeroInstance::showNecromancyDialog(const CStackInstance &raisedStack) const
{
	InfoWindow iw;
	iw.soundID = soundBase::GENIE;
	iw.player = tempOwner;
	iw.components.push_back(Component(raisedStack));

	if (raisedStack.count > 1) // Practicing the dark arts of necromancy, ... (plural)
	{ 
		iw.text.addTxt(MetaString::GENERAL_TXT, 145);
		iw.text.addReplacement(raisedStack.count);
		iw.text.addReplacement(MetaString::CRE_PL_NAMES, raisedStack.type->idNumber);
	} 
	else // Practicing the dark arts of necromancy, ... (singular)
	{ 
		iw.text.addTxt(MetaString::GENERAL_TXT, 146);
		iw.text.addReplacement(MetaString::CRE_SING_NAMES, raisedStack.type->idNumber);
	}

	cb->showInfoDialog(&iw);
}

int3 CGHeroInstance::getSightCenter() const
{
	return getPosition(false);
}

int CGHeroInstance::getSightRadious() const
{
	return 5 + getSecSkillLevel(3) + valOfBonuses(Bonus::SIGHT_RADIOUS); //default + scouting
}

si32 CGHeroInstance::manaRegain() const
{
	if (hasBonusOfType(Bonus::FULL_MANA_REGENERATION))
		return manaLimit();

	return 1 + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, 8) + valOfBonuses(Bonus::MANA_REGENERATION); //1 + Mysticism level 
}

si32 CGHeroInstance::getArtPos(int aid) const
{
	for(std::map<ui16,ui32>::const_iterator i = artifWorn.begin(); i != artifWorn.end(); i++)
		if(i->second == aid)
			return i->first;
	return -1;
}

/**
 * Places an artifact in hero's backpack. If it's a big artifact equips it
 * or discards it if it cannot be equipped.
 */
void CGHeroInstance::giveArtifact (ui32 aid)
{
	const CArtifact &artifact = *VLC->arth->artifacts[aid];

	if (artifact.isBig()) 
	{
		for (std::vector<ui16>::const_iterator it = artifact.possibleSlots.begin(); it != artifact.possibleSlots.end(); ++it) 
		{
			if (!vstd::contains(artifWorn, *it)) 
			{
				VLC->arth->equipArtifact(artifWorn, *it, aid);
				break;
			}
		}
	} 
	else 
	{
		artifacts.push_back(aid);
	}
}

bool CGHeroInstance::hasArt( ui32 aid ) const
{
	if(vstd::contains(artifacts, aid))
		return true;
	for(std::map<ui16,ui32>::const_iterator i = artifWorn.begin(); i != artifWorn.end(); i++)
		if(i->second == aid)
			return true;

	return false;
}

int CGHeroInstance::getBoatType() const
{
	int alignment = type->heroType / 6; 
	switch(alignment)
	{
	case 0:
		return 1; //good
	case 1:
		return 0; //evil
	case 2:
		return 2;
	default:
		throw std::string("Wrong alignment!");
	}
}

void CGHeroInstance::getOutOffsets(std::vector<int3> &offsets) const
{
	static int3 dirs[] = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0), int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) };
	for (size_t i = 0; i < ARRAY_COUNT(dirs); i++)
		offsets += dirs[i];
}

int CGHeroInstance::getSpellCost(const CSpell *sp) const
{
	return sp->costs[getSpellSchoolLevel(sp)];
}

void CGHeroInstance::getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const
{
	CArmedInstance::getParents(out, root);// 	if(visitedTown && source != this && source != visitedTown)
// 		out.insert(visitedTown);

	if((root == this || contains(static_cast<const CStackInstance *>(root))) &&  visitedTown)
		out.insert(visitedTown);

	for (std::map<ui16,ui32>::const_iterator i = artifWorn.begin(); i != artifWorn.end(); i++)
		out.insert(VLC->arth->artifacts[i->second]);

	out.insert(&speciality);
}

void CGHeroInstance::pushPrimSkill(int which, int val)
{
	bonuses.push_back(Bonus(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::HERO_BASE_SKILL, val, id, which));
}

void CGHeroInstance::getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root /*= NULL*/) const
{
#define FOREACH_OWNER_TOWN(town) if(const PlayerState *p = cb->getPlayerState(tempOwner)) BOOST_FOREACH(const CGTownInstance *town, p->towns)
	CArmedInstance::getBonuses(out, selector, root);

	//TODO eliminate by moving secondary skills effects to bonus system
	if(Selector::matchesType(selector, Bonus::LUCK))
	{
		//luck skill
		if(int luckSkill = getSecSkillLevel(9)) 
			out.push_back(Bonus(Bonus::PERMANENT, Bonus::LUCK, Bonus::SECONDARY_SKILL, luckSkill, 9, VLC->generaltexth->arraytxt[73+luckSkill]));

		//guardian spirit
		FOREACH_OWNER_TOWN(t)
			if(t->subID ==1 && vstd::contains(t->builtBuildings,26)) //rampart with grail
				out.push_back(Bonus(Bonus::PERMANENT, Bonus::LUCK, Bonus::TOWN_STRUCTURE, +2, 26, VLC->generaltexth->buildings[1][26].first + " +2"));
	}

	if(Selector::matchesType(selector, Bonus::SEA_MOVEMENT))
	{
		//lighthouses
		FOREACH_OWNER_TOWN(t)
			if(t->subID == 0 && vstd::contains(t->builtBuildings,17)) //castle
				out.push_back(Bonus(Bonus::PERMANENT, Bonus::SEA_MOVEMENT, Bonus::TOWN_STRUCTURE, +500, 17, VLC->generaltexth->buildings[0][17].first + " +500"));
	}

	if(Selector::matchesType(selector, Bonus::MORALE))
	{
		//leadership
		if(int moraleSkill = getSecSkillLevel(6)) 
			out.push_back(Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::SECONDARY_SKILL, moraleSkill, 6, VLC->generaltexth->arraytxt[104+moraleSkill]));

		//colossus
		FOREACH_OWNER_TOWN(t)
			if(t->subID == 0 && vstd::contains(t->builtBuildings,26)) //castle
				out.push_back(Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::TOWN_STRUCTURE, +2, 26, VLC->generaltexth->buildings[0][26].first + " +2"));
	}

	if(Selector::matchesTypeSubtype(selector, Bonus::SECONDARY_SKILL_PREMY, 12)) //necromancy
	{
		FOREACH_OWNER_TOWN(t)
		{
			if(t->subID == 4) //necropolis
			{
				if(vstd::contains(t->builtBuildings,21)) //necromancy amplifier
					out.push_back(Bonus(Bonus::PERMANENT, Bonus::SECONDARY_SKILL_PREMY, Bonus::TOWN_STRUCTURE, +10, 21, VLC->generaltexth->buildings[4][21].first + " +10%", 12));
				if(vstd::contains(t->builtBuildings,26)) //grail - Soul prison
					out.push_back(Bonus(Bonus::PERMANENT, Bonus::SECONDARY_SKILL_PREMY, Bonus::TOWN_STRUCTURE, +20, 26, VLC->generaltexth->buildings[4][26].first + " +20%", 12));
			}
		}
	}
}

EAlignment CGHeroInstance::getAlignment() const
{
	return type->heroClass->getAlignment();
}

void CGHeroInstance::initExp()
{
	exp=40+  (ran())  % 50;
	level = 1;
}

void CGDwelling::initObj()
{
	switch(ID)
	{
	case 17:
		{
			int crid = VLC->objh->cregens[subID];
			const CCreature *crs = VLC->creh->creatures[crid];

			creatures.resize(1);
			creatures[0].second.push_back(crid);
			hoverName = VLC->generaltexth->creGens[subID];
			if(crs->level > 4)
				addStack(0, CStackInstance(crs, (crs->growth) * 3));
			if (getOwner() != 255)
				cb->gameState()->players[getOwner()].dwellings.push_back (this);
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
			addStack(0, CStackInstance(116, 9));
			addStack(1, CStackInstance(117, 6));
		}
		else if(subID == 0) // Elemental Conflux 
		{
			creatures[0].second.push_back(112); //Air Elemental
			creatures[1].second.push_back(114); //Fire Elemental
			creatures[2].second.push_back(113); //Earth Elemental
			creatures[3].second.push_back(115); //Water Elemental
			//guards
			addStack(0, CStackInstance(113, 12));
		}
		else
		{
			assert(0);
		}
		hoverName = VLC->generaltexth->creGens4[subID];
		break;

	case 78: //Refugee Camp
		//is handled within newturn func
		break; 

	case 106: //War Machine Factory
		creatures.resize(3);
		creatures[0].second.push_back(146); //Ballista 
		creatures[1].second.push_back(147); //First Aid Tent
		creatures[2].second.push_back(148); //Ammo Cart
		break;

	default:
		assert(0);
		break;
	}
}

void CGDwelling::setProperty(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::OWNER: //change owner
			if (ID == 17) //single generators
			{
				if (tempOwner != NEUTRAL_PLAYER)
				{
					std::vector<CGDwelling *>* dwellings = &cb->gameState()->players[tempOwner].dwellings;
					dwellings->erase (std::find(dwellings->begin(), dwellings->end(), this));
				}
				if (val != NEUTRAL_PLAYER) //can new owner be neutral?
					cb->gameState()->players[val].dwellings.push_back (this);
			}
			break;
		case ObjProperty::AVAILABLE_CREATURE:
			creatures.resize(1);
			creatures[0].second.resize(1);
			creatures[0].second[0] = val;
			break;
	}
	CGObjectInstance::setProperty(what,val);
}
void CGDwelling::onHeroVisit( const CGHeroInstance * h ) const
{
	if(ID == 78 && !creatures[0].first) //Refugee Camp, no available cres
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text.addTxt(MetaString::ADVOB_TXT, 44); //{%s} \n\n The camp is deserted.  Perhaps you should try next week.
		iw.text.addReplacement(MetaString::OBJ_NAMES, ID);
		cb->sendAndApply(&iw);
		return;
	}

	int relations = cb->gameState()->getPlayerRelations( h->tempOwner, tempOwner );
	
	if ( relations == 1 )//ally
		return;//do not allow recruiting or capturing
		
	if( !relations  &&  stacksCount() > 0) //object is guarded, owned by enemy
	{
		BlockingDialog bd;
		bd.player = h->tempOwner;
		bd.flags = BlockingDialog::ALLOW_CANCEL;
		bd.text.addTxt(MetaString::GENERAL_TXT, 421); //Much to your dismay, the %s is guarded by %s %s. Do you wish to fight the guards?
		bd.text.addReplacement(ID == 17 ? MetaString::CREGENS : MetaString::CREGENS4, subID);
		bd.text.addReplacement(MetaString::ARRAY_TXT, 176 + Slots().begin()->second.getQuantityID()*3);
		bd.text.addReplacement(Slots().begin()->second);
		cb->showBlockingDialog(&bd, boost::bind(&CGDwelling::wantsFight, this, h, _1));
		return;
	}

	if(!relations  &&  ID != 106)
	{
		cb->setOwner(id, h->tempOwner);
	}

	BlockingDialog bd;
	bd.player = h->tempOwner;
	bd.flags = BlockingDialog::ALLOW_CANCEL;
	if(ID == 17 || ID == 20)
	{
		bd.text.addTxt(MetaString::ADVOB_TXT, ID == 17 ? 35 : 36); //{%s} Would you like to recruit %s? / {%s} Would you like to recruit %s, %s, %s, or %s?
		bd.text.addReplacement(ID == 17 ? MetaString::CREGENS : MetaString::CREGENS4, subID);
		for(size_t i = 0; i < creatures.size(); i++)
			bd.text.addReplacement(MetaString::CRE_PL_NAMES, creatures[i].second[0]);
	}
	else if(ID == 78)
	{
		bd.text.addTxt(MetaString::ADVOB_TXT, 35); //{%s} Would you like to recruit %s?
		bd.text.addReplacement(MetaString::OBJ_NAMES, ID);
		for(size_t i = 0; i < creatures.size(); i++)
			bd.text.addReplacement(MetaString::CRE_PL_NAMES, creatures[i].second[0]);
	}
	else if(ID == 106)
		bd.text.addTxt(MetaString::ADVOB_TXT, 157); //{War Machine Factory} Would you like to purchase War Machines?
	else
		throw std::string("Illegal dwelling!");

	cb->showBlockingDialog(&bd, boost::bind(&CGDwelling::heroAcceptsCreatures, this, h, _1));
}

void CGDwelling::newTurn() const
{
	if(cb->getDate(1) != 1) //not first day of week
		return;

	//town growths and War Machines Factories are handled separately
	if(ID == TOWNI_TYPE  ||  ID == 106)
		return;

	if(ID == 78) //if it's a refugee camp, we need to pick an available creature
	{
		cb->setObjProperty(id, ObjProperty::AVAILABLE_CREATURE, VLC->creh->pickRandomMonster());
	}

	bool change = false;

	SetAvailableCreatures sac;
	sac.creatures = creatures;
	sac.tid = id;
	for (size_t i = 0; i < creatures.size(); i++)
	{
		if(creatures[i].second.size())
		{
			CCreature *cre = VLC->creh->creatures[creatures[i].second[0]];
			TQuantity amount = cre->growth * (1 + cre->valOfBonuses(Bonus::CREATURE_GROWTH_PERCENT)/100) + cre->valOfBonuses(Bonus::CREATURE_GROWTH);
			if(false /*accumulate creatures*/)
				sac.creatures[i].first += amount;
			else
				sac.creatures[i].first = amount;
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
	CCreature *crs = VLC->creh->creatures[crid];

	if(crs->level == 1  &&  ID != 78) //first level - creatures are for free
	{
		if(creatures[0].first) //there are available creatures
		{
			int slot = h->getSlotFor(crid);
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
				sg.garrs[h->id] = *h;
				sg.garrs[h->id].addToSlot(slot, crid, creatures[0].first);

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
		if(ID == 106) //pick available War Machines
		{
			//there is 1 war machine available to recruit if hero doesn't have one
			SetAvailableCreatures sac;
			sac.tid = id;
			sac.creatures = creatures;
			sac.creatures[0].first = !h->getArt(13); //ballista
			sac.creatures[1].first = !h->getArt(15); //first aid tent
			sac.creatures[2].first = !h->getArt(14); //ammo cart
			cb->sendAndApply(&sac);
		}

		OpenWindow ow;
		ow.id1 = id;
		ow.id2 = h->id;
		ow.window = (ID == 17 || ID == 78)
			? OpenWindow::RECRUITMENT_FIRST 
			: OpenWindow::RECRUITMENT_ALL;
		cb->sendAndApply(&ow);
	}
}

void CGDwelling::wantsFight( const CGHeroInstance *h, ui32 answer ) const
{
	if(answer)
		cb->startBattleI(h, this, boost::bind(&CGDwelling::fightOver, this, h, _1));
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
		if ((builtBuildings.find(26)) != builtBuildings.end()) //skyship
			return -1; //entire map
		else if ((builtBuildings.find(21)) != builtBuildings.end()) //lookout tower
			return 20;
	}
	return 5;
}

void CGTownInstance::setPropertyDer(ui8 what, ui32 val)
{
///this is freakin' overcomplicated solution
	switch (what)
	{
		case 11: //add visitor of town building
			bonusingBuildings[val]->setProperty (ObjProperty::VISITORS, visitingHero->id);
			break;
		case 12:
			bonusingBuildings[val]->setProperty (12, 0);
			break;
		case 13: //add garrisoned hero to visitors
			bonusingBuildings[val]->setProperty (ObjProperty::VISITORS, garrisonHero->id);
			break;
		case 14:
			bonusValue.first = val;
			break;
		case 15:
			bonusValue.second = val;
			break;			
	}
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
	if ( level<0 || level >= CREATURES_PER_TOWN )
		return false;
	return vstd::contains(builtBuildings, 30+level+upgraded*CREATURES_PER_TOWN);
}
int CGTownInstance::getHordeLevel(const int & HID)  const//HID - 0 or 1; returns creature level or -1 if that horde structure is not present
{
	return town->hordeLvl[HID];
}
int CGTownInstance::creatureGrowth(const int & level) const
{
	if (level<0 || level >=CREATURES_PER_TOWN)
		return 0;
	TCreature creid = town->basicCreatures[level];
		
	int ret = VLC->creh->creatures[creid]->growth;
	switch(fortLevel())
	{
	case 3:
		ret*=2;break;
	case 2:
		ret*=(1.5); break;
	}
	ret *= (1 + VLC->creh->creatures[creid]->valOfBonuses(Bonus::CREATURE_GROWTH_PERCENT)/100); // double growth or plague
	if(tempOwner != NEUTRAL_PLAYER)
	{
		ret *= (1 + cb->gameState()->players[tempOwner].valOfBonuses(Bonus::CREATURE_GROWTH_PERCENT)/100); //Statue of Legion
		for (std::vector<CGDwelling*>::const_iterator it = cb->gameState()->players[tempOwner].dwellings.begin(); it != cb->gameState()->players[tempOwner].dwellings.end(); ++it)
		{ //+1 for each dwelling
			if (VLC->creh->creatures[creid]->idNumber == (*it)->creatures[0].second[0])
				++ret;
		}
	}
	if(getHordeLevel(0)==level)
		if((builtBuildings.find(18)!=builtBuildings.end()) || (builtBuildings.find(19)!=builtBuildings.end()))
			ret+=VLC->creh->creatures[creid]->hordeGrowth;
	if(getHordeLevel(1)==level)
		if((builtBuildings.find(24)!=builtBuildings.end()) || (builtBuildings.find(25)!=builtBuildings.end()))
			ret+=VLC->creh->creatures[creid]->hordeGrowth;

	//support for legs of legion etc.
	if(garrisonHero)
		ret += garrisonHero->valOfBonuses(Bonus::CREATURE_GROWTH, level);
	if(visitingHero)
		ret += visitingHero->valOfBonuses(Bonus::CREATURE_GROWTH, level);
	if(builtBuildings.find(26)!=builtBuildings.end()) //grail - +50% to ALL growth
		ret*=1.5;
	//spcecial week unaffected by grail (bug in original game?)
	ret += VLC->creh->creatures[creid]->valOfBonuses(Bonus::CREATURE_GROWTH);
	return ret;//check CCastleInterface.cpp->CCastleInterface::CCreaInfo::clickRight if this one will be modified
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
	:IShipyard(this), IMarket(this)
{
	builded=-1;
	destroyed=-1;
	garrisonHero=NULL;
	town=NULL;
	visitingHero = NULL;
}

CGTownInstance::~CGTownInstance()
{
	for (std::vector<CGTownBuilding*>::const_iterator i = bonusingBuildings.begin(); i != bonusingBuildings.end(); i++) 
		delete *i;
}

int CGTownInstance::spellsAtLevel(int level, bool checkGuild) const
{
	if(checkGuild && mageGuildLevel() < level)
		return 0;
	int ret = 6 - level; //how many spells are available at this level
	if(subID == 2   &&   vstd::contains(builtBuildings,22)) //magic library in Tower
		ret++; 
	return ret;
}

int CGTownInstance::defenceBonus(int type) const
{
	int ret=0;
	switch (type)
		{
/*attack*/		case 0: 
						if (subID == 6 && vstd::contains(builtBuildings,26))//Stronghold, grail
							ret += 12;
						if (subID == 7 && vstd::contains(builtBuildings,26))//Fortress, grail
							ret += 10;
						if (subID == 7 && vstd::contains(builtBuildings,22))//Fortress, Blood Obelisk
							ret += 2;
							return ret;
/*defence*/		case 1:
						if (subID == 7 && vstd::contains(builtBuildings,21))//Fortress, Glyphs of Fear
							ret += 2;
						if (subID == 7 && vstd::contains(builtBuildings,26))//Fortress, Grail
							ret += 10;
							return ret;
/*spellpower*/	case 2:
						if (subID == 3 && vstd::contains(builtBuildings,21))//Inferno, Brimstone Clouds
							ret += 2;
						if (subID == 5 && vstd::contains(builtBuildings,26))//Dungeon, Grail
							ret += 12;
							return ret;
/*knowledge*/	case 3:
						if (subID == 2 && vstd::contains(builtBuildings,26))//Tower, Grail
							ret += 15;
							return ret;
		}
		return 0;//Why we are here? wrong type?
}

bool CGTownInstance::needsLastStack() const
{
	if(garrisonHero)
		return true;
	else return false;
}

void CGTownInstance::onHeroVisit(const CGHeroInstance * h) const
{
	if( !cb->gameState()->getPlayerRelations( getOwner(), h->getOwner() ))//if this is enemy
	{
		if(stacksCount() > 0 || visitingHero)
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
			removeCapitols (h->getOwner());
			cb->heroVisitCastle(id, h->id);
		}
	}
	else
		cb->heroVisitCastle(id, h->id);
}

void CGTownInstance::onHeroLeave(const CGHeroInstance * h) const
{
	cb->stopHeroVisitCastle(id,h->id);
}

void CGTownInstance::initObj()
///initialize town structures
{
	blockVisit = true;
	hoverName = name + ", " + town->Name();

	if (subID == 5)
		creatures.resize(CREATURES_PER_TOWN+1);//extra dwelling for Dungeon
	else
		creatures.resize(CREATURES_PER_TOWN);
	for (int i = 0; i < CREATURES_PER_TOWN; i++)
	{
		if(creatureDwelling(i,false))
			creatures[i].second.push_back(town->basicCreatures[i]);
		if(creatureDwelling(i,true))
			creatures[i].second.push_back(town->upgradedCreatures[i]);
	}

	switch (subID)
	{ //add new visitable objects
		case 0:
			bonusingBuildings.push_back (new COPWBonus(21, this)); //Stables
			break;
		case 5:
			bonusingBuildings.push_back (new COPWBonus(21, this)); //Vortex
		case 2: case 3: case 6:
			bonusingBuildings.push_back (new CTownBonus(23, this));
			break;
		case 7:
			bonusingBuildings.push_back (new CTownBonus(17, this));
			break;
	}
	//add special bonuses from buildings
	if(subID == 4 && vstd::contains(builtBuildings, 17))
	{
		bonuses.push_back( Bonus(Bonus::PERMANENT, Bonus::DARKNESS, Bonus::TOWN_STRUCTURE, 20, 17) );
	}
}

void CGTownInstance::newTurn() const
{
	if (cb->getDate(1) == 1) //reset on new week
	{
		if (vstd::contains(builtBuildings,17) && subID == 1 && cb->getDate(0) != 1 && (tempOwner < PLAYER_LIMIT) )//give resources for Rampart, Mystic Pond
		{
			int resID = rand()%4+2;//bonus to random rare resource
			resID = (resID==2)?1:resID;
			int resVal = rand()%4+1;//with size 1..4
			cb->giveResource(tempOwner, resID, resVal);
			cb->setObjProperty (id, 14, resID);
			cb->setObjProperty (id, 15, resVal);
		}

		if ( subID == 5 )
			for (std::vector<CGTownBuilding*>::const_iterator i = bonusingBuildings.begin(); i!=bonusingBuildings.end(); i++)
		{
			if ((*i)->ID == 21)
				cb->setObjProperty (id, 12, (*i)->id); //reset visitors for Mana Vortex
		}

		if (tempOwner == NEUTRAL_PLAYER) //garrison growth for neutral towns
			{
				TSlots slt = Slots(); //meh, waste of time
				std::vector<ui8> nativeCrits; //slots
				for (TSlots::const_iterator it = slt.begin(); it != slt.end(); it++)
				{
					if (it->second.type->faction == subID) //native
					{
						nativeCrits.push_back(it->first); //collect matching slots
					}
				}
				if (nativeCrits.size())
				{
					int pos = nativeCrits[rand() % nativeCrits.size()];
					if (rand()%100 < 90 || slt[pos].type->upgrades.empty()) //increase number if no upgrade avaliable
					{
						SetGarrisons sg;
						sg.garrs[id] = getArmy();
						sg.garrs[id].slots[pos].count += slt[pos].type->growth;
						cb->sendAndApply(&sg);	
					}
					else //upgrade
					{
						SetGarrisons sg; //somewhat better upgrade pack would come in handy
						sg.garrs[id] = getArmy();
						sg.garrs[id].setCreature(pos, *slt[pos].type->upgrades.begin(), slt[pos].count);
						cb->sendAndApply(&sg);	
					}
				}
				if ((stacksCount() < ARMY_SIZE && rand()%100 < 25) || slt.empty()) //add new stack
				{
					int n, i = rand() % std::min (ARMY_SIZE, cb->getDate(3)<<1);	
					{//no lower tiers or above current month

						if (n = getSlotFor(town->basicCreatures[i], ARMY_SIZE));
						{ 
							SetGarrisons sg;
							sg.garrs[id] = getArmy();
							if (slotEmpty(n))
								sg.garrs[id].setCreature (n, town->basicCreatures[i], creatureGrowth(i)); //if the stack is not yet present
							else
								sg.garrs[id].addToSlot(n, town->basicCreatures[i], creatureGrowth(i)); //add to existing
							cb->sendAndApply(&sg);
						}
					}
				}		
			}
	}
}

int3 CGTownInstance::getSightCenter() const
{
	return pos - int3(2,0,0);
}

ui8 CGTownInstance::getPassableness() const
{
	if ( !stacksCount() )//empty castle - anyone can visit
		return ALL_PLAYERS;
	if ( tempOwner == 255 )//neutral guarded - noone can visit
		return 0;
		
	ui8 mask = 0;
	TeamState * ts = cb->gameState()->getPlayerTeam(tempOwner);
	BOOST_FOREACH(ui8 it, ts->players)
		mask |= 1<<it;//allies - add to possible visitors
	
	return mask;
}

void CGTownInstance::getOutOffsets( std::vector<int3> &offsets ) const
{
	offsets += int3(-1,2,0), int3(-3,2,0);
}

void CGTownInstance::fightOver( const CGHeroInstance *h, BattleResult *result ) const
{
	if(result->winner == 0)
	{
		if (hasBonusOfType(Bonus::DARKNESS))
		{
			//TODO: Make some 'change owner' function for bonus, or bonuses independent of player
			/*
			RemoveBonus rb(RemoveBonus::PLAYER);
			rb.whoID = getOwner();
			rb.source = Bonus::TOWN_STRUCTURE;
			rb.id = id;
			cb->sendAndApply(&rb);

			GiveBonus gb(GiveBonus::PLAYER);
			gb.bonus.type = Bonus::DARKNESS;
			gb.bonus.val = 20;
			gb.id = h->tempOwner;
			gb.bonus.duration = Bonus::PERMANENT;
			gb.bonus.source = Bonus::TOWN_STRUCTURE;
			gb.bonus.id = id;
			cb->sendAndApply(&gb);
			*/
		}

		removeCapitols (h->getOwner());
		cb->setOwner (id, h->tempOwner); //give control after checkout is done
		FoWChange fw;
		fw.player = h->tempOwner;
		fw.mode = 1;
		getSightTiles (fw.tiles); //update visibility for castle structures
		cb->sendAndApply (&fw);


	}
}

void CGTownInstance::removeCapitols (ui8 owner) const
{ 
	if (hasCapitol()) // search if there's an older capitol
	{ 
		PlayerState* state = cb->gameState()->getPlayer (owner); //get all towns owned by player
		for (std::vector<CGTownInstance*>::const_iterator i = state->towns.begin(); i < state->towns.end(); ++i) 
		{ 
			if (*i != this && (*i)->hasCapitol()) 
			{ 
				RazeStructures rs; 
				rs.tid = id; 
				rs.bid.insert(13); 
				rs.destroyed = destroyed;  
				cb->sendAndApply(&rs); 
				return; 
			} 
		} 
	} 
} 

int CGTownInstance::getBoatType() const
{
	const CCreature *c = VLC->creh->creatures[town->basicCreatures.front()];
	if (c->isGood())
		return 1;
	else if (c->isEvil())
		return 0;
	else //neutral
		return 2;
}

void CGTownInstance::getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const
{
	CArmedInstance::getParents(out, root);
	if(root == this  &&  visitingHero && visitingHero != root)
		out.insert(visitingHero);
}

void CGTownInstance::getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root /*= NULL*/) const
{
	CArmedInstance::getBonuses(out, selector, root);
	//TODO eliminate by moving structures effects to bonus system

	if(Selector::matchesType(selector, Bonus::LUCK))
	{
		if(subID == 1  &&  vstd::contains(builtBuildings,21)) //rampart, fountain of fortune
			out.push_back(Bonus(Bonus::PERMANENT, Bonus::LUCK, Bonus::TOWN_STRUCTURE, +2, 21, VLC->generaltexth->buildings[1][21].first + " +2"));
	}

	if(Selector::matchesType(selector, Bonus::MORALE))
	{
		if(subID == 0  &&  vstd::contains(builtBuildings,22)) //castle, brotherhood of sword built
			out.push_back(Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::TOWN_STRUCTURE, +2, 22, VLC->generaltexth->buildings[0][22].first + " +2"));
		else if(vstd::contains(builtBuildings,5)) //tavern is built
			out.push_back(Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::TOWN_STRUCTURE, +1, 5, VLC->generaltexth->buildings[0][5].first + " +1"));
	}
}

int CGTownInstance::getMarketEfficiency() const
{
	if(!vstd::contains(builtBuildings, 14)) 
		return 0;

	const PlayerState *p = cb->getPlayerState(tempOwner);
	assert(p);

	int marketCount = 0;
	BOOST_FOREACH(const CGTownInstance *t, p->towns)
		if(vstd::contains(t->builtBuildings, 14))
			marketCount++;

	return marketCount;
}

bool CGTownInstance::allowsTrade(EMarketMode mode) const
{
	switch(mode)
	{
	case RESOURCE_RESOURCE:
	case RESOURCE_PLAYER:
		return vstd::contains(builtBuildings, 14); // 	marketplace
	case ARTIFACT_RESOURCE:
	case RESOURCE_ARTIFACT:
		return (subID == 2 || subID == 5 || subID == 8) && vstd::contains(builtBuildings, 17);//artifact merchants
	case CREATURE_RESOURCE:
		return subID == 6 && vstd::contains(builtBuildings, 21); //Freelancer's guild
	case CREATURE_UNDEAD:
		return subID == 4 && vstd::contains(builtBuildings, 22);//Skeleton transformer
	case RESOURCE_SKILL:
		return subID == 8 && vstd::contains(builtBuildings, 21);//Magic University
	default:
		assert(0);
		return false;
	}
}

std::vector<int> CGTownInstance::availableItemsIds(EMarketMode mode) const
{
	if(mode == RESOURCE_ARTIFACT)
	{
		std::vector<int> ret;
		BOOST_FOREACH(const CArtifact *a, merchantArtifacts)
			if(a)
				ret.push_back(a->id);
			else
				ret.push_back(-1);
		return ret;
	}
	else if ( mode == RESOURCE_SKILL )
	{
		return universitySkills;
	}
	else
		return IMarket::availableItemsIds(mode);
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
			cb->setObjProperty(id, ObjProperty::VISITORS, h->id); //add to the visitors
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

void CGVisitableOPH::treeSelected( int heroID, int resType, int resVal, expType expVal, ui32 result ) const
{
	if(result) //player agreed to give res for exp
	{
		cb->giveResource(cb->getOwner(heroID),resType,-resVal); //take resource
		cb->changePrimSkill(heroID,4,expVal); //give exp
		cb->setObjProperty(id, ObjProperty::VISITORS, heroID); //add to the visitors
	}
}
void CGVisitableOPH::onNAHeroVisit(int heroID, bool alreadyVisited) const
{
	int id=0, subid=0, ot=0, sound = 0;
	expType val=1;
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
				const CGHeroInstance *h = cb->getHero(heroID);
				val = val*(100+h->getSecSkillLevel(21)*5)/100.0f;
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
					cb->setObjProperty(this->id, ObjProperty::VISITORS, heroID); //add to the visitors
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
					ui32 res;
					expType resval;
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

					BlockingDialog sd (true, false);
					sd.soundID = sound;
					sd.player = cb->getOwner(heroID);
					sd.text << std::pair<ui8,ui32>(11,ot);
					sd.components.push_back (Component (Component::RESOURCE, res, resval, 0));
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
					cb->setObjProperty(this->id, ObjProperty::VISITORS, heroID); //add to the visitors
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
		hoverName += ("\n" + VLC->generaltexth->xtrainfo[pom]);
	const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
	if(h)
	{
		hoverName += "\n\n";
		hoverName += (vstd::contains(visitors,h->id)) 
							? (VLC->generaltexth->allTexts[352])  //visited
							: ( VLC->generaltexth->allTexts[353]); //not visited
	}
	return hoverName;
}

void CGVisitableOPH::arenaSelected( int heroID, int primSkill ) const
{
	cb->setObjProperty(id, ObjProperty::VISITORS, heroID); //add to the visitors
	cb->changePrimSkill(heroID,primSkill-1,2);
}

void CGVisitableOPH::setPropertyDer( ui8 what, ui32 val )
{
	if(what == ObjProperty::VISITORS)
		visitors.insert(val);
}

void CGVisitableOPH::schoolSelected(int heroID, ui32 which) const
{
	if(!which) //player refused to pay
		return;

	int base = (ID == 47  ?  2  :  0);
	cb->setObjProperty(id, ObjProperty::VISITORS, heroID); //add to the visitors
	cb->giveResource(cb->getOwner(heroID),6,-1000); //take 1000 gold
	cb->changePrimSkill(heroID, base + which-1, +1); //give appropriate skill
}

COPWBonus::COPWBonus (int index, CGTownInstance *TOWN)
{
	ID = index;
	town = TOWN;
	id = town->bonusingBuildings.size();
}
void COPWBonus::setProperty(ui8 what, ui32 val)
{
	switch (what)
	{
		case 4:
			visitors.insert(val);
			break;
		case 12:
			visitors.clear();
			break;
	}
}
void COPWBonus::onHeroVisit (const CGHeroInstance * h) const
{
	int heroID = h->id;
	if (town->builtBuildings.find(ID) != town->builtBuildings.end())
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		switch (town->subID)
		{
			case 0: //Stables
				if (!h->hasBonusFrom(Bonus::OBJECT, 94)) //does not stack with advMap Stables
				{
					GiveBonus gb;
					gb.bonus = Bonus(Bonus::ONE_WEEK, Bonus::LAND_MOVEMENT, Bonus::OBJECT, 600, 94, VLC->generaltexth->arraytxt[100]);
					gb.id = heroID;
					cb->giveHeroBonus(&gb);
					iw.text << VLC->generaltexth->allTexts[580];
					cb->showInfoDialog(&iw);
				}
				break;
			case 5: //Mana Vortex
				if (visitors.empty() && h->mana <= h->manaLimit())
				{
					cb->setManaPoints (heroID, 2 * h->manaLimit()); 
					cb->setObjProperty (id, ObjProperty::VISITED, true);
					iw.text << VLC->generaltexth->allTexts[579];
					cb->showInfoDialog(&iw);
					cb->setObjProperty (town->id, 11, id); //add to visitors
				}
				break;
		}
	}
}
CTownBonus::CTownBonus (int index, CGTownInstance *TOWN)
{
	ID = index;
	town = TOWN;
	id = town->bonusingBuildings.size();
}
void CTownBonus::setProperty (ui8 what, ui32 val)
{
	if(what == 4)
		visitors.insert(val);
}
void CTownBonus::onHeroVisit (const CGHeroInstance * h) const
{
	int heroID = h->id;
	if ((town->builtBuildings.find(ID) != town->builtBuildings.end()) && (visitors.find(heroID) == visitors.end()))
	{
		InfoWindow iw;
		int what, val, mid;
		switch (ID)
		{
			case 23:
				switch(town->subID)
				{
					case 2: //wall
						what = 3;
						val = 1;
						mid = 581;
						iw.components.push_back (Component(Component::PRIM_SKILL, 3, 1, 0));
						break;
					case 3: //order of fire
						what = 2;
						val = 1;
						mid = 582;
						iw.components.push_back (Component(Component::PRIM_SKILL, 2, 1, 0));
						break;
					case 6://hall of valhalla
						what = 0;
						val = 1;
						mid = 584;
						iw.components.push_back (Component(Component::PRIM_SKILL, 0, 1, 0));
						break;
					case 5://academy of battle scholars
						what = 4;
						val = 1000*(100+h->getSecSkillLevel(21)*5)/100.0f;
						mid = 583;
						iw.components.push_back (Component(Component::EXPERIENCE, 0, val, 0));
						break;
				}
				break;
			case 17:
				switch(town->subID)
				{
					case 7: //cage of warlords
						what = 1;
						val = 1;
						mid = 585;
						iw.components.push_back (Component(Component::PRIM_SKILL, 1, 1, 0));
						break;
				}
				break;
		}
		iw.player = cb->getOwner(heroID);
		iw.text << VLC->generaltexth->allTexts[mid];
		cb->showInfoDialog(&iw);
		cb->changePrimSkill (heroID, what, val);
		if (town->visitingHero == h)
			cb->setObjProperty (town->id, 11, id); //add to visitors
		else
			cb->setObjProperty (town->id, 13, id); //then it must be garrisoned hero
	}
}
const std::string & CGCreature::getHoverText() const
{
	MetaString ms;
	int pom = slots.find(0)->second.getQuantityID();
	pom = 174 + 3*pom + 1;
	ms << std::pair<ui8,ui32>(6,pom) << " " << std::pair<ui8,ui32>(7,subID);
	ms.toString(hoverName);

	if(const CGHeroInstance *selHero = cb->getSelectedHero(cb->getCurrentPlayer()))
	{
		hoverName += "\n\n Threat: ";
		float ratio = ((float)getArmyStrength() / selHero->getTotalStrength());
		if (ratio < 0.1) hoverName += "Effortless";
		else if (ratio < 0.25) hoverName += "Very Weak";
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
	}
	return hoverName;
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
			boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(slots.find(0)->second.count));
			boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(action));
			boost::algorithm::replace_first(tmp,"%s",VLC->creh->creatures[subID]->namePl);
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
		//cb->setAmount(id, slots.find(0)->second.second - killedAmount);	

		/*
		MetaString ms;
		int pom = slots.find(0)->second.getQuantityID();
		pom = 174 + 3*pom + 1;
		ms << std::pair<ui8,ui32>(6,pom) << " " << std::pair<ui8,ui32>(7,subID);
		cb->setHoverName(id,&ms);
		cb->setObjProperty(id, 11, slots.begin()->second.count * 1000);
		*/
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

	slots[0].setType(subID);
	si32 &amount = slots[0].count;
	CCreature &c = *VLC->creh->creatures[subID];
	if(!amount)
		if(c.ammMax == c.ammMin)
			amount = c.ammMax;
		else
			amount = c.ammMin + (ran() % (c.ammMax - c.ammMin));

	temppower = slots[0].count * 1000;
}
void CGCreature::newTurn() const
{//Works only for stacks of single type of size up to 2 millions
	if (slots.begin()->second.count < CREEP_SIZE && cb->getDate(1) == 1 && cb->getDate(0) > 1)
	{
		ui32 power = temppower * (100 + WEEKLY_GROWTH)/100;
		cb->setObjProperty(id, 10, std::min (power/1000 , (ui32)CREEP_SIZE)); //set new amount
		cb->setObjProperty(id, 11, power); //increase temppower
	}
}
void CGCreature::setPropertyDer(ui8 what, ui32 val)
{
	switch (what)
	{
		case 10:
			slots[0].count = val;
			break;
		case 11:
			temppower = val;
			break;
	}
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
		myKindCres.insert(VLC->creh->creatures[subID]->upgrades.begin(),VLC->creh->creatures[subID]->upgrades.end()); //our upgrades
		for(std::vector<CCreature*>::iterator i=VLC->creh->creatures.begin(); i!=VLC->creh->creatures.end(); i++)
			if(vstd::contains((*i)->upgrades, (ui32) id)) //it's our base creatures
				myKindCres.insert((*i)->idNumber);

		int count = 0, //how many creatures of our kind has hero
			totalCount = 0;

		for (TSlots::const_iterator i = h->Slots().begin(); i != h->Slots().end(); i++)
		{
			if(vstd::contains(myKindCres,i->second.type->idNumber))
				count += i->second.count;
			totalCount += i->second.count;
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
				return VLC->creh->creatures[subID]->cost[6] * Slots().find(0)->second.count; //join for gold
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

		int slot = h->getSlotFor(subID);
		if(slot >= 0) //there is place
		{
			//add creatures
			SetGarrisons sg;
			sg.garrs[h->id] = h->getArmy();
			sg.garrs[h->id].addToSlot(slot, subID, getAmount(0));
			cb->sendAndApply(&sg);
			cb->removeObject(id);
		}
		else
		{
			cb->showGarrisonDialog(id,h->id,true,boost::bind(&IGameCallback::removeObject,cb,id)); //show garrison window and remove ourselves from map when player ends
		}
	}
}

void CGCreature::fight( const CGHeroInstance *h ) const
{
	cb->startBattleI(h, this, boost::bind(&CGCreature::endBattle,this,_1));
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
	int relations = cb->gameState()->getPlayerRelations(h->tempOwner, tempOwner);
	
	if(relations == 2) //we're visiting our mine
	{
		cb->showGarrisonDialog(id,h->id,true,0);
		return; 
	}
	else if (relations == 1)//ally
		return;

	if(stacksCount()) //Mine is guarded
	{
		BlockingDialog ynd(true,false);
		ynd.player = h->tempOwner;
		ynd.text << std::pair<ui8,ui32>(MetaString::ADVOB_TXT, subID == 7 ? 84 : 187); 
		cb->showBlockingDialog(&ynd,boost::bind(&CGMine::fight, this, _1, h));
		return;
	}

	flagMine(h->tempOwner);

}

void CGMine::newTurn() const
{
	if(cb->getDate() == 1)
		return;

	if (tempOwner == NEUTRAL_PLAYER)
		return;

	cb->giveResource(tempOwner, producedResource, producedQuantity);
}

void CGMine::initObj()
{
	if(subID >= 7) //Abandoned Mine
	{
		//set guardians
		int howManyTroglodytes = 100 + ran()%100;
		CStackInstance troglodytes(70, howManyTroglodytes);
		addStack(0, troglodytes);

		//after map reading tempOwner placeholds bitmask for allowed resources
		std::vector<int> possibleResources;
		for (int i = 0; i < 8; i++)
			if(tempOwner & 1<<i)
				possibleResources.push_back(i);

		assert(possibleResources.size());
		producedResource = possibleResources[ran()%possibleResources.size()];
		tempOwner = NEUTRAL_PLAYER;
		hoverName = VLC->generaltexth->mines[7].first + "\n" + VLC->generaltexth->allTexts[202] + " " + troglodytes.getQuantityTXT(false) + " " + troglodytes.type->namePl;
	}
	else
	{
		producedResource = subID;

		MetaString ms;
		ms << std::pair<ui8,ui32>(9,producedResource);
		if(tempOwner >= PLAYER_LIMIT)
			tempOwner = NEUTRAL_PLAYER;	
		else
			ms << " (" << std::pair<ui8,ui32>(6,23+tempOwner) << ")";
		ms.toString(hoverName);
	}

	producedQuantity = defaultResProduction();
}

void CGMine::fight(ui32 agreed, const CGHeroInstance *h) const
{
	cb->startBattleI(h, this, boost::bind(&CGMine::endBattle, this, _1, h->tempOwner));
}

void CGMine::endBattle(BattleResult *result, ui8 attackingPlayer) const
{
	if(result->winner == 0) //attacker won
	{
		if(subID == 7)
		{
			InfoWindow iw;
			iw.player = attackingPlayer;
			iw.text.addTxt(MetaString::ADVOB_TXT, 85);
			cb->showInfoDialog(&iw);

		}
		flagMine(attackingPlayer);
	}
}

void CGMine::flagMine(ui8 player) const
{
	assert(tempOwner != player);
	cb->setOwner(id,player); //not ours? flag it!

	MetaString ms;
	ms << std::pair<ui8,ui32>(9,subID) << "\n(" << std::pair<ui8,ui32>(6,23+player) << ")";
	if(subID == 7)
	{
		ms << "(%s)";
		ms.addReplacement(MetaString::RES_NAMES, producedResource);
	}
	cb->setHoverName(id,&ms);

	InfoWindow iw;
	iw.soundID = soundBase::FLAGMINE;
	iw.text.addTxt(MetaString::MINE_EVNTS,producedResource); //not use subID, abandoned mines uses default mine texts
	iw.player = player;
	iw.components.push_back(Component(2,producedResource,producedQuantity,-1));
	cb->showInfoDialog(&iw);
}

ui32 CGMine::defaultResProduction()
{
	switch(producedResource)
	{
	case 0: //wood
	case 2: //ore
		return 2;
	case 6: //gold
		return 1000;
	default:
		return 1;
	}
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
	if(stacksCount())
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
		cb->startBattleI(h, this, boost::bind(&CGResource::endBattle,this,_1,h));
}

void CGResource::endBattle( BattleResult *result, const CGHeroInstance *h ) const
{
	if(result->winner == 0) //attacker won
		collectRes(h->getOwner());
}

void CGVisitableOPW::newTurn() const
{
	if (cb->getDate(1) == 1) //first day of week = 1
	{
		cb->setObjProperty(id, ObjProperty::VISITED, false);
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
		cb->setObjProperty(id, ObjProperty::VISITED, true);
		MetaString ms; //set text to "visited"
		ms << std::pair<ui8,ui32>(3,ID) << " " << std::pair<ui8,ui32>(1,352);
		cb->setHoverName(id,&ms);
	}
}

void CGVisitableOPW::setPropertyDer( ui8 what, ui32 val )
{
	if(what == ObjProperty::VISITED)
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
	case 45://two way monolith - pick any other one
	case 111: //Whirlpool
		if(vstd::contains(objs,ID) && vstd::contains(objs[ID],subID) && objs[ID][subID].size()>1)
		{
			while ((destinationid = objs[ID][subID][rand()%objs[ID][subID].size()]) == id); //choose another exit
			if (ID == 111)
			{
				if (!h->hasBonusOfType(Bonus::WHIRLPOOL_PROTECTION))
				{
					CCreatureSet army = h->getArmy();
					if (army.Slots().size() > 1 || army.Slots().begin()->second.count > 1)
					{ //we can't remove last unit
						int targetstack = army.Slots().begin()->first; //slot numbers may vary
						for(TSlots::const_reverse_iterator i = army.Slots().rbegin(); i != army.Slots().rend(); i++)
						{
							if (army.getPower(targetstack) > army.getPower(i->first))
							{
								targetstack = (i->first);
							}
						}
						CCreatureSet ourArmy;
						ourArmy.addStack (targetstack, army.getStack(targetstack));
						TQuantity tq = (double)(ourArmy.getAmount(targetstack))*0.5;
						amax (tq, 1);

						ourArmy.setStackCount (targetstack, tq);
						InfoWindow iw;
						iw.player = h->tempOwner;
						iw.text.addTxt (MetaString::ADVOB_TXT, 168);
						iw.components.push_back (Component(ourArmy.getStack(targetstack)));
						cb->showInfoDialog(&iw);
					    cb->takeCreatures (h->id, ourArmy.Slots());
					}
				}
			}
		}
		else
			tlog2 << "Cannot find corresponding exit monolith for "<< id << std::endl;
		break;
	case 103: //find nearest subterranean gate on the other level
		{
			int i=0;
			for(; i < gates.size(); i++)
			{
				if(gates[i].first == id)
				{
					destinationid = gates[i].second;
					break;
				}
				else if(gates[i].second == id)
				{
					destinationid = gates[i].first;
					break;
				}
			}

			if(destinationid < 0  ||  i == gates.size()) //no exit
			{
				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::ADVOB_TXT, 153);//Just inside the entrance you find a large pile of rubble blocking the tunnel. You leave discouraged.
				cb->sendAndApply(&iw);
			}
			break;
		}
	}
	if(destinationid < 0)
	{
		tlog2 << "Cannot find exit... (obj at " << pos << ") :( \n";
		return;
	}
	if (ID == 111)
	{
		std::set<int3> tiles = cb->getObj(destinationid)->getBlockedPos();
		std::set<int3>::iterator it = tiles.begin();
		std::advance (it, rand() % tiles.size()); //picking random element of set is tiring
		cb->moveHero (h->id, *it + int3(1,0,0), true);
	}
	else
		cb->moveHero (h->id,CGHeroInstance::convertPosition(cb->getObj(destinationid)->pos,true) - getVisitableOffset(), true);
}

void CGTeleport::initObj()
{
	int si = subID;
	switch (ID)
	{
		case 103://ignore subterranean gates subid
		case 111:
		{
			si = 0;
			break;
		}
		default:
			break;
	}
	objs[ID][si].push_back(id);
}

void CGTeleport::postInit() //matches subterranean gates into pairs
{
	//split on underground and surface gates
	std::vector<const CGObjectInstance *> gatesSplit[2]; //surface and underground gates
	for(size_t i = 0; i < objs[103][0].size(); i++)
	{
		const CGObjectInstance *hlp = cb->getObj(objs[103][0][i]);
		gatesSplit[hlp->pos.z].push_back(hlp);
	}

	//sort by position
	std::sort(gatesSplit[0].begin(), gatesSplit[0].end(), boost::bind(&CGObjectInstance::pos, _1) < boost::bind(&CGObjectInstance::pos, _2));

	for(size_t i = 0; i < gatesSplit[0].size(); i++)
	{
		const CGObjectInstance *cur = gatesSplit[0][i];

		//find nearest underground exit
		std::pair<int,double> best(-1,150000); //pair<pos_in_vector, distance>
		for(int j = 0; j < gatesSplit[1].size(); j++)
		{
			const CGObjectInstance *checked = gatesSplit[1][j];
			if(!checked)
				continue;
			double hlp = checked->pos.dist2d(cur->pos);
			if(hlp < best.second)
			{
				best.first = j;
				best.second = hlp;
			}
		}

		if(best.first >= 0) //found pair
		{
			gates.push_back(std::pair<int, int>(cur->id, gatesSplit[1][best.first]->id));
			gatesSplit[1][best.first] = NULL;
		}
		else
		{
			gates.push_back(std::pair<int, int>(cur->id, -1));
		}
	}
	objs.erase(103);
}

void CGArtifact::initObj()
{
	blockVisit = true;
	if(ID == 5)
		hoverName = VLC->arth->artifacts[subID]->Name();
}

void CGArtifact::onHeroVisit( const CGHeroInstance * h ) const
{
	if(!stacksCount())
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
		cb->startBattleI(h, this, boost::bind(&CGArtifact::endBattle,this,_1,h));
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
				val2 = cb->getRandomArt (CArtifact::ART_TREASURE);
				type = 1;
			}
		}
		break;
	case 86: //Shipwreck Survivor
		{
			int hlp = ran()%100;
			if(hlp < 55)
				val1 = cb->getRandomArt (CArtifact::ART_TREASURE);
			else if(hlp < 75)
				val1 = cb->getRandomArt (CArtifact::ART_MINOR);
			else if(hlp < 95)
				val1 = cb->getRandomArt (CArtifact::ART_MAJOR);
			else
				val1 = cb->getRandomArt (CArtifact::ART_RELIC);
		}
		break;
	case 101: //treasure chest
		{
			int hlp = ran()%100;
			if(hlp >= 95)
			{
				type = 1;
				val1 = cb->getRandomArt (CArtifact::ART_TREASURE);
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
				int expVal = val2*(100+h->getSecSkillLevel(21)*5)/100.0f;
				sd.components.push_back(Component(5,0,expVal, 0));
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
	const CGHeroInstance *h = cb->getHero(heroID);
	switch(which)
	{
	case 1: //player pick gold
		cb->giveResource(cb->getOwner(heroID),6,val1);
		break;
	case 2: //player pick exp
		cb->changePrimSkill(heroID, 4, val2*(100+h->getSecSkillLevel(21)*5)/100.0f);
		break;
	default:
		throw std::string("Unhandled treasure choice");
	}
	cb->removeObject(id);
}

bool CQuest::checkQuest (const CGHeroInstance * h) const
{
	switch (missionType)
	{
		case MISSION_NONE:
			return true;
			break;
		case MISSION_LEVEL:
			if (m13489val <= h->level)
				return true;
			return false;
			break;
		case MISSION_PRIMARY_STAT:
			for (int i = 0; i < 4; ++i)
			{
				if (h->getPrimSkillLevel(i) < m2stats[i])
					return false;
			}
			return true;
			break;
		case MISSION_KILL_HERO:
			if (h->cb->gameState()->map->heroesToBeat[m13489val]->tempOwner < PLAYER_LIMIT)
				return false; //if the pointer is not NULL
			return true;
			break;
		case MISSION_KILL_CREATURE:
			if (h->cb->gameState()->map->monsters[m13489val]->pos == int3(-1,-1,-1))
				return true;
			return false;
			break;
		case MISSION_ART:
			for (int i = 0; i < m5arts.size(); ++i)
			{
				if (h->hasArt(m5arts[i]))
					continue;
				return false; //if the artifact was not found
			}
			return true;
			break;
		case MISSION_ARMY:
			{
				TSlots::const_iterator it, cre;
				ui32 count;
				for (cre = m6creatures.begin(); cre != m6creatures.end(); ++cre)
				{
					for (count = 0, it = h->Slots().begin(); it !=  h->Slots().end(); ++it)
					{
						if (it->second.type == cre->second.type)
							count += it->second.count;
					}
					if (count < cre->second.count) //not enough creatures of this kind
						return false;
				}
			}
			return true;
			break;
		case MISSION_RESOURCES:
			for (int i = 0; i < 7; ++i) //including Mithril ?
			{	//Quest has no direct access to callback
				if (h->cb->getResource (h->tempOwner, i) < m7resources[i]) 
					return false;
			}
			return true;
			break;
		case MISSION_HERO:
			if (m13489val == h->type->ID)
				return true;
			return false;
			break;
		case MISSION_PLAYER:
			if (m13489val == h->getOwner())
				return true;
			return false;
			break;
		default:
			return false;
	}
}
void CGSeerHut::initObj()
{
	seerName = VLC->generaltexth->seerNames[ran()%VLC->generaltexth->seerNames.size()];
	textOption = ran()%3;
	progress = 0;
	if (missionType)
	{
		if (!isCustom)
		{
		
			firstVisitText = VLC->generaltexth->quests[missionType-1][0][textOption];
			nextVisitText = VLC->generaltexth->quests[missionType-1][1][textOption];
			completedText = VLC->generaltexth->quests[missionType-1][2][textOption];
		}
	}
	else
		firstVisitText = VLC->generaltexth->seerEmpty[textOption];

}

const std::string & CGSeerHut::getHoverText() const
{
	switch (ID)
	{
			case 83:
				hoverName = VLC->generaltexth->allTexts[347];
				boost::algorithm::replace_first(hoverName,"%s", seerName);
				break;
			case 215:
				hoverName = VLC->generaltexth->names[ID];
				break;
			default:
				tlog5 << "unrecognized quest object\n";
	}
	if (progress & missionType) //rollover when the quest is active
	{
		MetaString ms;
		ms << "\n\n" << VLC->generaltexth->quests[missionType-1][3][textOption];
		std::string str;
		switch (missionType)
		{
			case MISSION_LEVEL:
				ms.addReplacement (m13489val);
				break;
			case MISSION_PRIMARY_STAT:
			{
				MetaString loot;
				for (int i = 0; i < 4; ++i)
				{
					if (m2stats[i])
					{
						loot << "%d %s";
						loot.addReplacement (m2stats[i]);
						loot.addReplacement (VLC->generaltexth->primarySkillNames[i]);
					}		
				}
				ms.addReplacement(loot.buildList());
			}
				break;
			case MISSION_KILL_HERO:
				ms.addReplacement(cb->gameState()->map->heroesToBeat[m13489val]->name);
				break;
			case MISSION_HERO:
				ms.addReplacement(VLC->heroh->heroes[m13489val]->name);
				break;
			case MISSION_KILL_CREATURE:
				{
					ms.addReplacement(cb->gameState()->map->monsters[m13489val]->getStack(0));
				}
				break;
			case MISSION_ART:
				{
					MetaString loot;
					for (std::vector<ui16>::const_iterator it = m5arts.begin(); it != m5arts.end(); ++it)
					{
						loot << "%s";
						loot.addReplacement (MetaString::ART_NAMES, *it);
					}
					ms.addReplacement(loot.buildList());
				}
				break;
			case MISSION_ARMY:
				{
					MetaString loot;
					for (TSlots::const_iterator it = m6creatures.begin(); it != m6creatures.end(); ++it)
					{
						loot << "%s";
						loot.addReplacement(it->second);
					}
					ms.addReplacement	(loot.buildList());
				}
				break;
			case MISSION_RESOURCES:
				{
					MetaString loot;
					for (int i = 0; i < 7; ++i)
					{
						if (m7resources[i])
						{
							loot << "%d %s";
							loot.addReplacement (m7resources[i]);
							loot.addReplacement (MetaString::RES_NAMES, i);
						}
					}
					ms.addReplacement (loot.buildList());
				}
				break;
			case MISSION_PLAYER:
				ms.addReplacement (VLC->generaltexth->colors[m13489val]);
				break;
			default:
				break;
		}
		ms.toString(str);
		hoverName += str; 
	}
	return hoverName;
}

void CGSeerHut::setPropertyDer (ui8 what, ui32 val)
{
	switch (what)
	{
		case 10:
			progress = val;
			break;
		case 11:
			missionType = CQuest::MISSION_NONE;
			break;
	}
}
void CGSeerHut::newTurn() const
{
	if (lastDay >= 0 && lastDay <= cb->getDate(0)) //time is up
	{
		cb->setObjProperty (id,11,0);
		cb->setObjProperty (id,10,0);
	}

}
void CGSeerHut::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->getOwner();
	if (missionType)
	{
		if (!progress) //propose quest
		{
			iw.text << firstVisitText;
			switch (missionType)
			{
				case MISSION_LEVEL:
					iw.components.push_back (Component (Component::EXPERIENCE, 1, m13489val, 0));
					if (!isCustom)
						iw.text.addReplacement(m13489val);
					break;
				case MISSION_PRIMARY_STAT:
				{
					MetaString loot;
					for (int i = 0; i < 4; ++i)
					{
						if (m2stats[i])
						{
							iw.components.push_back (Component (Component::PRIM_SKILL, i, m2stats[i], 0));
							loot << "%d %s";
							loot.addReplacement (m2stats[i]);
							loot.addReplacement (VLC->generaltexth->primarySkillNames[i]);
						}		
					}
					if (!isCustom)
						iw.text.addReplacement (loot.buildList());
				}
					break;
				case MISSION_KILL_HERO:
					iw.components.push_back (Component (Component::HERO,
						cb->gameState()->map->heroesToBeat[m13489val]->type->ID, 0, 0));
					if (!isCustom)
						iw.text.addReplacement(cb->gameState()->map->heroesToBeat[m13489val]->name);
					break;
				case MISSION_HERO:
					iw.components.push_back (Component (Component::HERO, m13489val, 0, 0));
					if (!isCustom)
						iw.text.addReplacement(VLC->heroh->heroes[m13489val]->name);
					break;
				case MISSION_KILL_CREATURE:
					{
						CStackInstance stack = cb->gameState()->map->monsters[m13489val]->getStack(0);
						iw.components.push_back (Component(stack));
						if (!isCustom)
						{
							iw.text.addReplacement(stack);
							if (std::count(firstVisitText.begin(), firstVisitText.end(), '%') == 2) //say where is placed monster
							{
								iw.text.addReplacement (VLC->generaltexth->arraytxt[147+checkDirection()]);
							}
						}
					}
					break;
				case MISSION_ART:
				{
					MetaString loot;
					for (std::vector<ui16>::const_iterator it = m5arts.begin(); it != m5arts.end(); ++it)
					{
						iw.components.push_back (Component (Component::ARTIFACT, *it, 0, 0));
						loot << "%s";
						loot.addReplacement (MetaString::ART_NAMES, *it);
					}
					if (!isCustom)
						iw.text.addReplacement	(loot.buildList());
				}
					break;
				case MISSION_ARMY:
				{
					MetaString loot;
					for (TSlots::const_iterator it = m6creatures.begin(); it != m6creatures.end(); ++it)
					{
						iw.components.push_back(Component(it->second));
						loot << "%s";
						loot.addReplacement(it->second);
					}
					if (!isCustom)
						iw.text.addReplacement	(loot.buildList());
				}
					break;
				case MISSION_RESOURCES:
				{
					MetaString loot;
					for (int i = 0; i < 7; ++i)
					{
						if (m7resources[i])
						{
							iw.components.push_back (Component (Component::RESOURCE, i, m7resources[i], 0));
							loot << "%d %s";
							loot.addReplacement (m7resources[i]);
							loot.addReplacement (MetaString::RES_NAMES, i);
						}
					}
					if (!isCustom)
						iw.text.addReplacement	(loot.buildList());
				}
					break;
				case MISSION_PLAYER:
					iw.components.push_back (Component (Component::FLAG, m13489val, 0, 0));
					if (!isCustom)
						iw.text.addReplacement	(VLC->generaltexth->colors[m13489val]);
					break;
			}
			cb->setObjProperty (id,10,1);
			cb->showInfoDialog(&iw);
		}
		else if (!checkQuest(h))
		{
			iw.text << nextVisitText;
			cb->showInfoDialog(&iw);
		}
		if (checkQuest(h)) // propose completion, also on first visit
		{
			BlockingDialog bd (true, false);
			bd.player = h->getOwner();
			bd.soundID = soundBase::QUEST;
			bd.text << completedText;
			switch (missionType)
			{
				case CQuest::MISSION_LEVEL:
					if (!isCustom)
						bd.text.addReplacement(m13489val);
					break;
				case CQuest::MISSION_PRIMARY_STAT:
					if (vstd::contains (completedText,'%')) //there's one case when there's nothing to replace
					{
						MetaString loot;
						for (int i = 0; i < 4; ++i)
						{
							if (m2stats[i])
							{
								loot << "%d %s";
								loot.addReplacement (m2stats[i]);
								loot.addReplacement (VLC->generaltexth->primarySkillNames[i]);
							}
						}
						if (!isCustom)
							bd.text.addReplacement (loot.buildList());
					}
					break;
				case CQuest::MISSION_ART:
				{
					MetaString loot;
					for (std::vector<ui16>::const_iterator it = m5arts.begin(); it != m5arts.end(); ++it)
					{
						loot << "%s";
						loot.addReplacement (MetaString::ART_NAMES, *it);
					}
					if (!isCustom)
						bd.text.addReplacement (loot.buildList());
				}
					break;
				case CQuest::MISSION_ARMY:
				{
					MetaString loot;
					for (TSlots::const_iterator it = m6creatures.begin(); it != m6creatures.end(); ++it)
					{
						loot << "%s";
						loot.addReplacement(it->second);
					}
					if (!isCustom)
						bd.text.addReplacement	(loot.buildList());
				}
					break;
				case CQuest::MISSION_RESOURCES:
				{
					MetaString loot;
					for (int i = 0; i < 7; ++i)
					{
						if (m7resources[i])
						{
							loot << "%d %s";
							loot.addReplacement (m7resources[i]);
							loot.addReplacement (MetaString::RES_NAMES, i);
						}
					}
					if (!isCustom)
						bd.text.addReplacement (loot.buildList());
				}
					break;
				case MISSION_KILL_HERO:
					if (!isCustom)
						bd.text.addReplacement(cb->gameState()->map->heroesToBeat[m13489val]->name);
					break;
				case MISSION_HERO:
					if (!isCustom)
						bd.text.addReplacement(VLC->heroh->heroes[m13489val]->name);
					break;
				case MISSION_KILL_CREATURE:
				{
					{
						bd.text.addReplacement(cb->gameState()->map->monsters[m13489val]->getArmy()[0]);
						if (std::count(firstVisitText.begin(), firstVisitText.end(), '%') == 2) //say where is placed monster
						{
							bd.text.addReplacement (VLC->generaltexth->arraytxt[147+checkDirection()]);
						}
					}
				}
				case MISSION_PLAYER:
					if (!isCustom)
						bd.text.addReplacement	(VLC->generaltexth->colors[m13489val]);
					break;
			}
			
			switch (rewardType)
			{
				case 1: bd.components.push_back (Component (Component::EXPERIENCE, 0, rVal*(100+h->getSecSkillLevel(21)*5)/100.0f, 0));
					break;
				case 2: bd.components.push_back (Component (Component::PRIM_SKILL, 5, rVal, 0));
					break;
				case 3: bd.components.push_back (Component (Component::MORALE, 0, rVal, 0));
					break;
				case 4: bd.components.push_back (Component (Component::LUCK, 0, rVal, 0));
					break;
				case 5: bd.components.push_back (Component (Component::RESOURCE, rID, rVal, 0));
					break;
				case 6: bd.components.push_back (Component (Component::PRIM_SKILL, rID, rVal, 0));
					break;
				case 7: bd.components.push_back (Component (Component::SEC_SKILL, rID, rVal, 0));
					break;
				case 8: bd.components.push_back (Component (Component::ARTIFACT, rID, 0, 0));
					break;
				case 9: bd.components.push_back (Component (Component::SPELL, rID, 0, 0));
					break;
				case 10: bd.components.push_back (Component (Component::CREATURE, rID, rVal, 0));
					break;
			}
			
			cb->showBlockingDialog (&bd, boost::bind (&CGSeerHut::finishQuest, this, h, _1));
			return;
		}
	}
	else
	{
		iw.text << VLC->generaltexth->seerEmpty[textOption];
		if (ID == 83)
			iw.text.addReplacement(seerName);
		cb->showInfoDialog(&iw);
	}
}
int CGSeerHut::checkDirection() const
{
	int3 cord = cb->gameState()->map->monsters[m13489val]->pos;
	if ((double)cord.x/(double)cb->getMapSize().x < 0.34) //north
	{
		if ((double)cord.y/(double)cb->getMapSize().y < 0.34) //northwest
			return 8;
		else if ((double)cord.y/(double)cb->getMapSize().y < 0.67) //north
			return 1;
		else //northeast
			return 2;
	}
	else if ((double)cord.x/(double)cb->getMapSize().x < 0.67) //horizontal
	{
		if ((double)cord.y/(double)cb->getMapSize().y < 0.34) //west
			return 7;
		else if ((double)cord.y/(double)cb->getMapSize().y < 0.67) //central
			return 9;
		else //east
			return 3;
	}
	else //south
	{
		if ((double)cord.y/(double)cb->getMapSize().y < 0.34) //southwest
			return 6;
		else if ((double)cord.y/(double)cb->getMapSize().y < 0.67) //south
			return 5;
		else //southeast
			return 4;
	}
}
void CGSeerHut::finishQuest (const CGHeroInstance * h, ui32 accept) const
{
	if (accept)
	{
		switch (missionType)
		{
			case CQuest::MISSION_ART:
				for (std::vector<ui16>::const_iterator it = m5arts.begin(); it != m5arts.end(); ++it)
				{
					cb->removeArtifact(*it, h->id);
				}
				break;
			case CQuest::MISSION_ARMY:
					cb->takeCreatures (h->id, m6creatures);
				break;
			case CQuest::MISSION_RESOURCES:
				for (int i = 0; i < 7; ++i)
				{
					cb->giveResource(h->getOwner(), i, -m7resources[i]);
				}
				break;
			default:
				break;
		}
		cb->setObjProperty (id,11,0); //no more mission avaliable	
		completeQuest(h); //make sure to remove QuestQuard at the very end	
	}
}
void CGSeerHut::completeQuest (const CGHeroInstance * h) const //reward
{
	switch (rewardType)
	{
		case 1: //experience
		{
			int expVal = rVal*(100+h->getSecSkillLevel(21)*5)/100.0f;
			cb->changePrimSkill(h->id, 4, expVal, false);
			break;
		}
		case 2: //mana points
		{
			cb->setManaPoints(h->id, h->mana+rVal);
			break;
		}
		case 3: case 4: //morale /luck
		{
			Bonus hb(Bonus::ONE_WEEK, (rewardType == 3 ? Bonus::MORALE : Bonus::LUCK),
				Bonus::OBJECT, rVal, h->id, "", -1);
			GiveBonus gb;
			gb.id = h->id;
			gb.bonus = hb;
			cb->giveHeroBonus(&gb);
		}
			break;
		case 5: //resources
			cb->giveResource(h->getOwner(), rID, rVal);
			break;
		case 6: //main ability bonus (attak, defence etd.)
			cb->changePrimSkill(h->id, rID, rVal, false);
			break;
		case 7: // secondary ability gain
			cb->changeSecSkill(h->id, rID, rVal, false);
			break;
		case 8: // artifact
			cb->giveHeroArtifact(rID, h->id, -2);
			break;
		case 9:// spell
		{
			std::set<ui32> spell;
			spell.insert (rID);
			cb->changeSpells(h->id, true, spell);
		}
			break;
		case 10:// creature
		{
			CCreatureSet creatures;
			creatures.setCreature (0, rID, rVal);
			cb->giveCreatures (id, h,  creatures);
		}
			break;
		default:
			break;
	}
}

void CGQuestGuard::initObj()
{
	blockVisit = true;
	progress = 0;
	textOption = ran()%3 + 3; //3-5
	if (missionType && !isCustom)
	{
		firstVisitText = VLC->generaltexth->quests[missionType-1][0][textOption];
		nextVisitText = VLC->generaltexth->quests[missionType-1][1][textOption];
		completedText = VLC->generaltexth->quests[missionType-1][2][textOption];
	}
	else
		firstVisitText = VLC->generaltexth->seerEmpty[textOption];
}
void CGQuestGuard::completeQuest(const CGHeroInstance *h) const
{
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
	bool visited = h->hasBonusFrom(Bonus::OBJECT,ID);
	int messageID, bonusType, bonusVal;
	int bonusMove = 0, sound = -1;
	InfoWindow iw;
	iw.player = h->tempOwner;
	GiveBonus gbonus;
	gbonus.id = h->id;
	gbonus.bonus.duration = Bonus::ONE_BATTLE;
	gbonus.bonus.source = Bonus::OBJECT;
	gbonus.bonus.id = ID;

	bool second = false;
	Bonus secondBonus;

	switch(ID)
	{
	case 11: //buoy
		messageID = 21;
		sound = soundBase::MORALE;
		gbonus.bonus.type = Bonus::MORALE;
		gbonus.bonus.val = +1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,94);
		break;
	case 14: //swan pond
		messageID = 29;
		sound = soundBase::LUCK;
		gbonus.bonus.type = Bonus::LUCK;
		gbonus.bonus.val = 2;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,67);
		bonusMove = -h->movement;
		break;
	case 28: //Faerie Ring
		messageID = 49;
		sound = soundBase::LUCK;
		gbonus.bonus.type = Bonus::LUCK;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,71);
		break;
	case 30: //fountain of fortune
		messageID = 55;
		sound = soundBase::LUCK;
		gbonus.bonus.type = Bonus::LUCK;
		gbonus.bonus.val = rand()%5 - 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,69);
		gbonus.bdescr.addReplacement((gbonus.bonus.val<0 ? "-" : "+") + boost::lexical_cast<std::string>(gbonus.bonus.val));
		break;
	case 38: //idol of fortune
		messageID = 62;
		sound = soundBase::experience;

		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,68);
		if(cb->getDate(1) == 7) //7th day of week
		{
			gbonus.bonus.type = Bonus::MORALE;
			second = true;
			secondBonus = gbonus.bonus;
			secondBonus.type = Bonus::LUCK;
		}
		else
		{
			gbonus.bonus.type = (cb->getDate(1)%2) ? Bonus::LUCK : Bonus::MORALE;
		}
		break;
	case 52: //Mermaid
		messageID = 83;
		sound = soundBase::LUCK;
		gbonus.bonus.type = Bonus::LUCK;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,72);
		break;
	case 64: //Rally Flag
		sound = soundBase::MORALE;
		messageID = 111;
		gbonus.bonus.type = Bonus::MORALE;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,102);

		second = true;
		secondBonus = gbonus.bonus;
		secondBonus.type = Bonus::LUCK;

		bonusMove = 400;
		break;
	case 56: //oasis
		messageID = 95;
		gbonus.bonus.type = Bonus::MORALE;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,95);
		bonusMove = 800;
		break;
	case 96: //temple
		messageID = 140;
		iw.soundID = soundBase::temple;
		gbonus.bonus.type = Bonus::MORALE;
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
		gbonus.bonus.type = Bonus::MORALE;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,100);
		bonusMove = 400;
		break;
	case 31: //Fountain of Youth
		sound = soundBase::MORALE;
		messageID = 57;
		gbonus.bonus.type = Bonus::MORALE;
		gbonus.bonus.val = 1;
		gbonus.bdescr <<  std::pair<ui8,ui32>(6,103);
		bonusMove = 400;
		break;
	case 94: //Stables
		sound = soundBase::horse20;
		CCreatureSet creatures;
		for (TSlots::const_iterator i = h->Slots().begin(); i != h->Slots().end(); ++i)
		{
			if(i->second.type->idNumber == 10)
				creatures.slots.insert(*i);
		}
		if (creatures.slots.size())
		{
			messageID = 138;
			iw.components.push_back(Component(Component::CREATURE,11,0,1));
			for (TSlots::const_iterator i = creatures.slots.begin(); i != creatures.slots.end(); ++i)
			{
				cb->changeCreatureType(h->id, i->first, 11);
			}
		}
		else
			messageID = 137;
		gbonus.bonus.type = Bonus::LAND_MOVEMENT;
		gbonus.bonus.val = 600;
		bonusMove = 600;
		gbonus.bonus.duration = Bonus::ONE_WEEK;
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
		//TODO: fix if second bonus val != main bonus val
		if(gbonus.bonus.type == Bonus::MORALE   ||   secondBonus.type == Bonus::MORALE)
			iw.components.push_back(Component(8,0,gbonus.bonus.val,0));
		if(gbonus.bonus.type == Bonus::LUCK   ||   secondBonus.type == Bonus::LUCK)
			iw.components.push_back(Component(9,0,gbonus.bonus.val,0));
		cb->giveHeroBonus(&gbonus);
		if(second)
		{
			gbonus.bonus = secondBonus;
			cb->giveHeroBonus(&gbonus);
		}
		if(bonusMove) //swan pond - take all move points, stables - give move point this day
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
		if(!h->hasBonusFrom(Bonus::OBJECT,ID))
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

void CGMagicSpring::onHeroVisit(const CGHeroInstance * h) const 
{ 
	int messageID; 
	InfoWindow iw; 
	iw.player = h->tempOwner; 
	iw.soundID = soundBase::GENIE; 
	if (!visited) 
	{ 
		if (h->mana > h->manaLimit())  
			messageID = 76; 
		else 
		{ 
			messageID = 74; 
			cb->setManaPoints (h->id, 2 * h->manaLimit()); 
			cb->setObjProperty (id, ObjProperty::VISITED, true); 
		} 
	} 
	else 
		messageID = 75; 
	iw.text << std::pair<ui8,ui32>(11,messageID); 
	cb->showInfoDialog(&iw); 
} 
const std::string & CGMagicSpring::getHoverText() const 
{ 
	hoverName = VLC->generaltexth->names[ID];
	if(!visited)
		hoverName += " " + VLC->generaltexth->allTexts[353]; //not visited
	else
		hoverName += " " + VLC->generaltexth->allTexts[352]; //visited
	return hoverName;
} 

void CGMagicWell::onHeroVisit( const CGHeroInstance * h ) const
{
	int message;
	InfoWindow iw;
	iw.soundID = soundBase::faerie;
	iw.player = h->tempOwner;
	if(h->hasBonusFrom(Bonus::OBJECT,ID)) //has already visited Well today
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
	blockVisit = (ID==6); //block only if it's really pandora's box (events also derive from that class)
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
		if (stacksCount() > 0) //if pandora's box is protected by army
		{
			InfoWindow iw;
			iw.player = h->tempOwner;
			iw.text.addTxt(MetaString::ADVOB_TXT, 16);
			cb->showInfoDialog(&iw);
			cb->startBattleI(h, this, boost::bind(&CGPandoraBox::endBattle, this, h, _1)); //grants things after battle
		}
		else if (message.size() == 0 && resources.size() == 0
				&& primskills.size() == 0 && abilities.size() == 0
				&& abilityLevels.size() == 0 &&  artifacts.size() == 0
				&& spells.size() == 0 && creatures.Slots().size() > 0
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
		expType expVal = gainedExp*(100+h->getSecSkillLevel(21)*5)/100.0f;
		getText(iw,afterBattle,175,h);

		if(expVal)
			iw.components.push_back(Component(Component::EXPERIENCE,0,expVal,0));
		for(int i=0; i<primskills.size(); i++)
			if(primskills[i])
				iw.components.push_back(Component(Component::PRIM_SKILL,i,primskills[i],0));

		for(int i=0; i<abilities.size(); i++)
			iw.components.push_back(Component(Component::SEC_SKILL,abilities[i],abilityLevels[i],0));

		cb->showInfoDialog(&iw);

		//give exp
		if(expVal)
			cb->changePrimSkill(h->id,4,expVal,false);
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
		gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::MORALE,Bonus::OBJECT,moraleDiff,id,"");
		gb.id = h->id;
		cb->giveHeroBonus(&gb);
	}

	if(luckDiff)
	{
		getText(iw,afterBattle,luckDiff,180,181,h);
		iw.components.push_back(Component(Component::LUCK,0,luckDiff,0));
		cb->showInfoDialog(&iw);
		GiveBonus gb;
		gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::LUCK,Bonus::OBJECT,luckDiff,id,"");
		gb.id = h->id;
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
// 	getText(iw,afterBattle,183,h);
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

	iw.components.clear();
	iw.text.clear();

	if (creatures.Slots().size())
	{ //this part is taken straight from creature bank
		MetaString loot; 
		CCreatureSet ourArmy = creatures;
		for(TSlots::const_iterator i = ourArmy.Slots().begin(); i != ourArmy.Slots().end(); i++)
		{ //build list of joined creatures
			iw.components.push_back(Component(i->second));
			loot << "%s";
			loot.addReplacement(i->second);
		}

		if (ourArmy.Slots().size() == 1 && ourArmy.Slots().begin()->second.count == 1)
			iw.text.addTxt (MetaString::ADVOB_TXT, 185);
		else
			iw.text.addTxt (MetaString::ADVOB_TXT, 186);

		iw.text.addReplacement (loot.buildList());
		iw.text.addReplacement (h->name);

		cb->showInfoDialog(&iw);
		cb->giveCreatures (id, h, ourArmy);
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
	if(stacksCount() > 0)
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		if(message.size())
			iw.text << message;
		else
			iw.text.addTxt(MetaString::ADVOB_TXT, 16);
		cb->showInfoDialog(&iw);
		cb->startBattleI(h, this, boost::bind(&CGEvent::endBattle,this,h,_1));
	}
	else
	{
		giveContents(h,false);
	}
}

void CGObservatory::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->tempOwner;
	switch (ID)
	{
		case 58://redwood observatory
		case 60://pillar of fire
		{
			iw.soundID = soundBase::LIGHTHOUSE;
			iw.text.addTxt(MetaString::ADVOB_TXT,98 + (ID==60));

			FoWChange fw;
			fw.player = h->tempOwner;
			fw.mode = 1;
			cb->getTilesInRange (fw.tiles, pos, 20, h->tempOwner, 1);
			cb->sendAndApply (&fw);
			break;
		}
		case 15://cover of darkness
		{
			iw.text.addTxt (MetaString::ADVOB_TXT, 31);
			hideTiles(h->tempOwner, 20);
			break;
		}
	}
	cb->showInfoDialog(&iw);
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
		cb->changeSpells(h->id, true, spells);

		iw.components.push_back(Component(Component::SPELL,spell,0,0));
	}

	cb->showInfoDialog(&iw);
}

void CGShrine::initObj()
{
	if(spell == 255) //spell not set
	{
		int level = ID-87;
		std::vector<ui16> possibilities;
		cb->getAllowedSpells (possibilities, level);

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
		|| (type == 2  &&  (!h->getArt(17) || vstd::contains(h->spells, (ui32) bid)))) //hero doesn't have a spellbook or already knows the spell
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
			std::vector<ui16> possibilities;
			for (int i = 1; i < 6; ++i)
				cb->getAllowedSpells (possibilities, i);
			bonusID = possibilities[ran() % possibilities.size()];
			break;
		}
	}
}

void CGGarrison::onHeroVisit (const CGHeroInstance *h) const
{
	int ally = cb->gameState()->getPlayerRelations(h->tempOwner, tempOwner);
	if (!ally && stacksCount() > 0) {
		//TODO: Find a way to apply magic garrison effects in battle.
		cb->startBattleI(h, this, boost::bind(&CGGarrison::fightOver, this, h, _1));
		return;
	}

	//New owner.
	if (!ally)
		cb->setOwner(id, h->tempOwner);

	cb->showGarrisonDialog(id, h->id, removableUnits, 0);
}

void CGGarrison::fightOver (const CGHeroInstance *h, BattleResult *result) const
{
	if (result->winner == 0)
		onHeroVisit(h);
}

ui8 CGGarrison::getPassableness() const
{
	if ( !stacksCount() )//empty - anyone can visit
		return ALL_PLAYERS;
	if ( tempOwner == 255 )//neutral guarded - noone can visit
		return 0;
		
	ui8 mask = 0;
	TeamState * ts = cb->gameState()->getPlayerTeam(tempOwner);
	BOOST_FOREACH(ui8 it, ts->players)
		mask |= 1<<it;//allies - add to possible visitors
	
	return mask;
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
		case 0: // first visit but empty
			if (ID == 22) //Corpse
				++txtid;
			else
				txtid+=2;
			iw.text.addTxt(MetaString::ADVOB_TXT, txtid);
			break;
		case 1: //art
			iw.components.push_back(Component(Component::ARTIFACT,bonusType,0,0));
			cb->giveHeroArtifact(bonusType,h->id,-2);
			iw.text.addTxt(MetaString::ADVOB_TXT, txtid);
			if (ID == 22) //Corpse
			{
				iw.text << "%s";
				iw.text.addReplacement (MetaString::ART_NAMES, bonusType);
			}
			break;
		case 2: //res
			iw.text.addTxt(MetaString::ADVOB_TXT, txtid);
			iw.components.push_back (Component(Component::RESOURCE, bonusType, bonusVal, 0));
			cb->giveResource(h->getOwner(), bonusType, bonusVal);
			break;
		}
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
				bonusType = cb->getRandomArt (CArtifact::ART_TREASURE | CArtifact::ART_MINOR | CArtifact::ART_MAJOR);
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
		}
		break;

	case 108://Warrior's Tomb
		{
			artOrRes = 1;

			int hlp = ran()%100;
			if(hlp < 30)
				bonusType = cb->getRandomArt (CArtifact::ART_TREASURE);
			else if(hlp < 80)
				bonusType = cb->getRandomArt (CArtifact::ART_MINOR);
			else if(hlp < 95)
				bonusType = cb->getRandomArt (CArtifact::ART_MAJOR);
			else
				bonusType = cb->getRandomArt (CArtifact::ART_RELIC);
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
				bonusType = cb->getRandomArt (CArtifact::ART_TREASURE | CArtifact::ART_MINOR);
			}
			else //2 - 5 of non-gold resource
			{
				artOrRes = 2;
				bonusType = ran()%6;
				bonusVal = ran()%4 + 2;
			}
		}
		break;
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
		
		if(!h->hasBonusFrom(Bonus::OBJECT,ID)) //we don't have modifier from this object yet
		{
			//ruin morale 
			GiveBonus gb;
			gb.id = h->id;
			gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::MORALE,Bonus::OBJECT,-3,id,"");
			gb.bdescr.addTxt(MetaString::ARRAY_TXT,104); //Warrior Tomb Visited -3
			cb->giveHeroBonus(&gb);
		}
		cb->showInfoDialog(&iw);
		cb->setObjProperty(id,10,h->getOwner());
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
const std::string & CBank::getHoverText() const
{
	hoverName = VLC->objh->creBanksNames[index];
	if (bc == NULL)
		hoverName += " " + VLC->generaltexth->allTexts[352];
	else
		hoverName += " " + VLC->generaltexth->allTexts[353];
	return hoverName;
}
void CBank::reset(ui16 var1) //prevents desync
{
	ui8 chance = 0;
	for (ui8 i = 0; i < VLC->objh->banksInfo[index].size(); i++)
	{	
		if (var1 < (chance += VLC->objh->banksInfo[index][i]->chance))
		{
 			bc = VLC->objh->banksInfo[index][i];
			break;
		}
	}
	artifacts.clear();
}

void CBank::initialize() const
{
	cb->setObjProperty (id, 14, ran()); //synchronous reset
	ui32 artid;
	for (ui8 i = 0; i <= 3; i++)
	{
		for (ui8 n = 0; n < bc->artifacts[i]; n++) //new function using proper randomization algorithm
		{	
				cb->setObjProperty (id, 18 + i, ran()); //synchronic artifacts
		}
	}
	cb->setObjProperty (id, 17, ran()); //get army
}
void CBank::setPropertyDer (ui8 what, ui32 val)
/// random values are passed as arguments and processed identically on all clients
{
	switch (what)
	{
		case 11: //daycounter
			if (val == 0)
				daycounter = 1; //yes, 1
			else
				daycounter++;
			break;
		case 12: //multiplier, in percent
			multiplier = ((float)val)/100;
			break;
		case 13: //bank preset
			bc = VLC->objh->banksInfo[index][val];
			break;
		case 14:
			reset (val%100);
			break;
		case 15:
			bc = NULL;
			break;
		case 16: //remove rewards from Derelict Ship
			artifacts.clear();
			break;
		case 17: //set ArmedInstance army
			{
				int upgraded = 0;
				if (val%100 < bc->upgradeChance) //once again anti-desync
					upgraded = 1;
				switch (bc->guards.size())
				{
					case 1:
						for	(int i = 0; i < 4; ++i)
							setCreature (i, bc->guards[0].first, bc->guards[0].second  / 5 );
						setCreature (4, bc->guards[0].first + upgraded, bc->guards[0].second  / 5 );
						break;
					case 4:
					{
						std::vector< std::pair <ui16, ui32> >::const_iterator it;
						if (bc->guards.back().second) //all stacks are present
						{
							for (it = bc->guards.begin(); it != bc->guards.end(); it++)
							{
								setCreature (stacksCount(), it->first, it->second);
							}
						}
						else if (bc->guards[2].second)//Wraiths are present, split two stacks in Crypt
						{
							setCreature (0, bc->guards[0].first, bc->guards[0].second  / 2 );
							setCreature (1, bc->guards[1].first, bc->guards[1].second / 2);
							setCreature (2, bc->guards[2].first + upgraded, bc->guards[2].second);
							setCreature (3, bc->guards[1].first, bc->guards[1].second / 2 );
							setCreature (4, bc->guards[0].first, bc->guards[0].second - (bc->guards[0].second  / 2) );

						}
						else //split both stacks
						{
							for	(int i = 0; i < 3; ++i) //skellies
								setCreature (2*i, bc->guards[0].first, bc->guards[0].second  / 3);
							for	(int i = 0; i < 2; ++i) //zombies
								setCreature (2*i+1, bc->guards[1].first, bc->guards[1].second  / 2);
						}
					}
						break;
					default:
						tlog2 << "Error: Unexpected army data: " << bc->guards.size() <<" items found";
						return;
				}
			}
			break;
		case 18: //add Artifact
		{
			int id = cb->getArtSync(val, CArtifact::ART_TREASURE);
			artifacts.push_back (id);
			cb->erasePickedArt(id);
			break;
		}
		case 19: //add Artifact
		{
			int id = cb->getArtSync(val, CArtifact::ART_MINOR);
			artifacts.push_back (id);
			cb->erasePickedArt(id);
			break;
		}
		case 20: //add Artifact
		{
			int id = cb->getArtSync(val, CArtifact::ART_MAJOR);
			artifacts.push_back (id);
			cb->erasePickedArt(id);
			break;
		}
		case 21: //add Artifact
		{
			int id = cb->getArtSync(val, CArtifact::ART_RELIC);
			artifacts.push_back (id);
			cb->erasePickedArt(id);
			break;
		}
	}
}

void CBank::newTurn() const 
{
	if (bc == NULL)
	{
		if (cb->getDate(0) == 1)
			initialize(); //initialize on first day
		else if (daycounter >= 28 && (subID < 13 || subID > 16)) //no reset for Emissaries
		{
			initialize();
			cb->setObjProperty (id, 11, 0); //daycounter 0
			if (ID == 24 && cb->getDate(0) > 1)
			{
				cb->setObjProperty (id, 12, 0);//ugly hack to make derelict ships usable only once
				cb->setObjProperty (id, 16, 0);
			}
		}
		else
			cb->setObjProperty (id, 11, 1); //daycounter++
	}
}
void CBank::onHeroVisit (const CGHeroInstance * h) const
{
	if (bc)
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
		bd.text << VLC->generaltexth->advobtxt[banktext];
		if (ID == 16)
			bd.text.addReplacement (VLC->objh->creBanksNames[index]);
		cb->showBlockingDialog (&bd, boost::bind (&CBank::fightGuards, this, h, _1));
	}
	else
	{
		InfoWindow iw;
		iw.soundID = soundBase::GRAVEYARD;
		iw.player = h->getOwner();
		if (ID == 84) //morale penalty for empty Crypt
		{
			GiveBonus gbonus;
			gbonus.id = h->id;
			gbonus.bonus.duration = Bonus::ONE_BATTLE;
			gbonus.bonus.source = Bonus::OBJECT;
			gbonus.bonus.id = ID;
			gbonus.bdescr << "\n" << VLC->generaltexth->arraytxt[98];
			gbonus.bonus.type = Bonus::MORALE;
			gbonus.bonus.val = -1;
			cb->giveHeroBonus(&gbonus);
			iw.text << VLC->generaltexth->advobtxt[120];
			iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
		}
		else
		{
			iw.text << VLC->generaltexth->advobtxt[33];
			iw.text.addReplacement (VLC->objh->creBanksNames[index]);
		}
		cb->showInfoDialog(&iw);
	}
}
void CBank::fightGuards (const CGHeroInstance * h, ui32 accept) const 
{
	if (accept)
	{
		cb->startBattleI (h, this, boost::bind (&CBank::endBattle, this, h, _1), true);
	}
}
void CBank::endBattle (const CGHeroInstance *h, const BattleResult *result) const
{
	if (result->winner == 0)
	{
		int textID = -1;
		InfoWindow iw;
		iw.player = h->getOwner();
		MetaString loot;

		switch (ID)
		{
			case 16: case 25:
				textID = 34;
				break;
			case 24: //derelict ship
				if (multiplier)
					textID = 43;
				else
				{
					GiveBonus gbonus;
					gbonus.id = h->id;
					gbonus.bonus.duration = Bonus::ONE_BATTLE;
					gbonus.bonus.source = Bonus::OBJECT;
					gbonus.bonus.id = ID;
					gbonus.bdescr << "\n" << VLC->generaltexth->arraytxt[101];
					gbonus.bonus.type = Bonus::MORALE;
					gbonus.bonus.val = -1;
					cb->giveHeroBonus(&gbonus);
					textID = 42;
					iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
				}
				break;
			case 84: //Crypt
				if (bc->resources.size() != 0)
					textID = 121;
				else
				{
					iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
					GiveBonus gbonus;
					gbonus.id = h->id;
					gbonus.bonus.duration = Bonus::ONE_BATTLE;
					gbonus.bonus.source = Bonus::OBJECT;
					gbonus.bonus.id = ID;
					gbonus.bdescr << "\n" << VLC->generaltexth->arraytxt[ID];
					gbonus.bonus.type = Bonus::MORALE;
					gbonus.bonus.val = -1;
					cb->giveHeroBonus(&gbonus);
					textID = 120;
					iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
				}
				break;
			case 85: //shipwreck
				if (bc->resources.size())
					textID = 124;
				else
					textID = 123;	
				break;
		}

		//grant resources
		if (textID != 42) //empty derelict ship gives no cash
		{
			for (int it = 0; it < bc->resources.size(); it++)
			{	
				if (bc->resources[it] != 0)
				{
					iw.components.push_back (Component (Component::RESOURCE, it, bc->resources[it], 0));
					loot << "%d %s";
					loot.addReplacement (iw.components.back().val);
					loot.addReplacement (MetaString::RES_NAMES, iw.components.back().subtype);
					cb->giveResource (h->getOwner(), it, bc->resources[it]);
				}		
			}
		}
		//grant artifacts
		for (std::vector<ui32>::const_iterator it = artifacts.begin(); it != artifacts.end(); it++)
		{
			iw.components.push_back (Component (Component::ARTIFACT, *it, 0, 0));
			loot << "%s";
			loot.addReplacement (MetaString::ART_NAMES, *it);
			cb->giveHeroArtifact (*it, h->id ,-2);
		}
		//display loot
		if (!iw.components.empty())
		{
			if (textID == 34)
			{
				iw.text.addTxt(MetaString::ADVOB_TXT, 34);//Heaving defeated %s, you discover %s
				iw.text.addReplacement (MetaString::CRE_PL_NAMES, result->casualties[1].begin()->first);
				iw.text.addReplacement (loot.buildList());
			}
			else
				iw.text.addTxt (MetaString::ADVOB_TXT, textID);
			cb->showInfoDialog(&iw);
		}
		loot.clear();
		iw.components.clear();
		iw.text.clear();

		//grant creatures
		CCreatureSet ourArmy;
		for (std::vector< std::pair <ui16, ui32> >::const_iterator it = bc->creatures.begin(); it != bc->creatures.end(); it++)
		{
			int slot = ourArmy.getSlotFor(it->first);
			ourArmy.addToSlot(slot, it->first, it->second);
		}
		for (TSlots::const_iterator i = ourArmy.Slots().begin(); i != ourArmy.Slots().end(); i++)
		{
			iw.components.push_back(Component(i->second));
			loot << "%s";
			loot.addReplacement (i->second);
		}

		if (ourArmy.Slots().size())
		{
			if (ourArmy.Slots().size() == 1 && ourArmy.Slots().begin()->second.count == 1)
				iw.text.addTxt (MetaString::ADVOB_TXT, 185);
			else
				iw.text.addTxt (MetaString::ADVOB_TXT, 186);

			iw.text.addReplacement (loot.buildList());
			iw.text.addReplacement (h->name);
			cb->showInfoDialog(&iw);
			cb->giveCreatures (id, h, ourArmy);
		}
		//cb->setObjProperty (id, 15, 0); //bc = NULL
	}
	//else //in case of defeat
	//	initialize();
}

void CGPyramid::initObj()
{
	std::vector<ui16> available;
	cb->getAllowedSpells (available, 5);
	if (available.size())
	{
		bc = VLC->objh->banksInfo[21].front(); //TODO: remove hardcoded value?
		spell = (available[rand()%available.size()]);
	}
	else
	{
		tlog1 <<"No spells available for Pyramid! Object set to empty.\n";
	}
	setPropertyDer (17,ran()); //set guards at game start
}
const std::string & CGPyramid::getHoverText() const
{
	hoverName = VLC->objh->creBanksNames[21];
	if (bc == NULL)
		hoverName += " " + VLC->generaltexth->allTexts[352];
	else
		hoverName += " " + VLC->generaltexth->allTexts[353];
	return hoverName;
}
void CGPyramid::onHeroVisit (const CGHeroInstance * h) const
{
	if (bc)
	{
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundBase::MYSTERY;
		bd.text << VLC->generaltexth->advobtxt[105];
		cb->showBlockingDialog (&bd, boost::bind (&CBank::fightGuards, this, h, _1));	
	}
	else
	{
		InfoWindow iw;
		iw.player = h->getOwner();
		iw.text << VLC->generaltexth->advobtxt[107];
		iw.components.push_back (Component (Component::LUCK, 0 , -2, 0));
		GiveBonus gb;
		gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::LUCK,Bonus::OBJECT,-2,id,VLC->generaltexth->arraytxt[70]);
		gb.id = h->id;
		cb->giveHeroBonus(&gb);
		cb->showInfoDialog(&iw);
	}
}

void CGPyramid::endBattle (const CGHeroInstance *h, const BattleResult *result) const
{
	if (result->winner == 0)
	{
		InfoWindow iw;
		iw.player = h->getOwner();
		iw.text.addTxt (MetaString::ADVOB_TXT, 106);
		iw.text.addTxt (MetaString::SPELL_NAME, spell);
		if (!h->getArt(17))						
			iw.text.addTxt (MetaString::ADVOB_TXT, 109); //no spellbook
		else if (h->getSecSkillLevel(7) < 3)	
			iw.text.addTxt (MetaString::ADVOB_TXT, 108); //no expert Wisdom
		else
		{
			std::set<ui32> spells;
			spells.insert (spell);
			cb->changeSpells (h->id, true, spells);
				iw.components.push_back(Component (Component::SPELL, spell, 0, 0));
		}
		cb->showInfoDialog(&iw);
		cb->setObjProperty (id, 15, 0);
	}
}
void CGKeys::setPropertyDer (ui8 what, ui32 val) //101-108 - enable key for player 1-8
{
	if (what >= 101 && what <= (100 + PLAYER_LIMIT))
		playerKeyMap.find(what-101)->second.insert((ui8)val);
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
	if (!wasMyColorVisited (h->getOwner()) )
	{
		InfoWindow iw;
		iw.player = h->getOwner();
		iw.text << std::pair<ui8,ui32>(11,18);
		cb->showInfoDialog(&iw);
	}
}

ui8 CGBorderGate::getPassableness() const
{
	ui8 ret = 0;
	for (int i = 0; i < PLAYER_LIMIT; i++)
		ret |= wasMyColorVisited(i)<<i;
	return ret;
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

			cb->getTilesInRange (fw.tiles, eye->pos, 10, h->tempOwner, 1);
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
	if(h->hasBonusFrom(Bonus::OBJECT,ID)) //has already visited Sirens
	{
		iw.text.addTxt(11,133);
	}
	else
	{
		giveDummyBonus(h->id, Bonus::ONE_BATTLE);
		int xp = 0;
		SetGarrisons sg;
		sg.garrs[h->id] = h->getArmy();
		for (TSlots::const_iterator i = h->Slots().begin(); i != h->Slots().end(); i++)
		{
			int drown = (int)(i->second.count * 0.3);
			if(drown)
			{
				sg.garrs[h->id].setStackCount(i->first, i->second.count - drown);
				xp += drown * i->second.type->valOfBonuses(Bonus::STACK_HEALTH);
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

int3 IBoatGenerator::bestLocation() const
{
	std::vector<int3> offsets;
	getOutOffsets(offsets);

	TerrainTile *tile;
	for (int i = 0; i < offsets.size(); ++i)
	{
		if (tile = IObjectInterface::cb->getTile(o->pos + offsets[i])) //tile is in the map 
		{
			if (tile->tertype == TerrainTile::water  &&  (!tile->blocked || tile->blockingObjects.front()->ID == 8)) //and is water and is not blocked or is blocked by boat
				return o->pos + offsets[i];
		}
	}
	return int3 (-1,-1,-1);
}

int IBoatGenerator::state() const
{
	int3 tile = bestLocation();
	TerrainTile *t = IObjectInterface::cb->getTile(tile);
	if(!t)
		return 2; //no available water
	else if(!t->blockingObjects.size())
		return 0; //OK
	else if(t->blockingObjects.front()->ID == 8)
		return 1; //blocked with boat
	else
		return 2; //blocked
}

int IBoatGenerator::getBoatType() const
{
	//We make good ships by default
	return 1;
}


IBoatGenerator::IBoatGenerator(const CGObjectInstance *O) 
: o(O)
{
}

void IBoatGenerator::getProblemText(MetaString &out, const CGHeroInstance *visitor) const
{
	switch(state())
	{
	case 1: 
		out.addTxt(MetaString::GENERAL_TXT, 51);
		break;
	case 2:
		if(visitor)
		{
			out.addTxt(MetaString::GENERAL_TXT, 134);
			out.addReplacement(visitor->name);
		}
		else
			out.addTxt(MetaString::ADVOB_TXT, 189);
		break;
	case 3:
		tlog1 << "Shipyard without water!!! " << o->pos << "\t" << o->id << std::endl;
		return;
	}
}

void IShipyard::getBoatCost( std::vector<si32> &cost ) const
{
	cost.resize(RESOURCE_QUANTITY);
	cost[0] = 10;
	cost[6] = 1000;
}

IShipyard::IShipyard(const CGObjectInstance *O) 
	: IBoatGenerator(O)
{
}

IShipyard * IShipyard::castFrom( CGObjectInstance *obj )
{
	if(!obj)
		return NULL;

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
		//tlog1 << "Cannot cast to IShipyard object with ID " << obj->ID << std::endl;
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
	// H J L K I
	// A x S x B
	// C E G F D
	offsets += int3(-3,0,0), int3(1,0,0), //AB
		int3(-3,1,0), int3(1,1,0), int3(-2,1,0), int3(0,1,0), int3(-1,1,0), //CDEFG
		int3(-3,-1,0), int3(1,-1,0), int3(-2,-1,0), int3(0,-1,0), int3(-1,-1,0); //HIJKL
}

void CGShipyard::onHeroVisit( const CGHeroInstance * h ) const
{
	if(!cb->gameState()->getPlayerRelations(tempOwner, h->tempOwner))
		cb->setOwner(id, h->tempOwner);

	int s = state();
	if(s)
	{
		InfoWindow iw;
		iw.player = tempOwner;
		getProblemText(iw.text, h);
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
			switch (subID)
			{
				case 0:
					text = 25;
					break;
				case 1:
					text = 26;
					break;
				case 2:
					text = 27;
					break;
				default:	
					tlog2 << "Unrecognized subtype of cartographer" << std::endl;
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
		fw.mode = 1;
		fw.player = h->tempOwner;

		//subIDs of different types of cartographers:
		//water = 0; land = 1; underground = 2;
		cb->getAllTiles (fw.tiles, h->tempOwner, subID - 1, !subID + 1); //reveal appropriate tiles
		cb->sendAndApply (&fw);
		cb->setObjProperty (id, 10, h->tempOwner);
	}
}

void CGDenOfthieves::onHeroVisit (const CGHeroInstance * h) const
{
	cb->showThievesGuildWindow(id);
}

void CGObelisk::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->tempOwner;
	TeamState *ts = cb->gameState()->getPlayerTeam(h->tempOwner);
	assert(ts);
	int team = ts->id;
	
	if(!hasVisited(team))
	{
		iw.text.addTxt(MetaString::ADVOB_TXT, 96);
		cb->sendAndApply(&iw);

		cb->setObjProperty(id,20,team); //increment general visited obelisks counter

		OpenWindow ow;
		ow.id1 = h->tempOwner;
		ow.window = OpenWindow::PUZZLE_MAP;
		cb->sendAndApply(&ow);

		cb->setObjProperty(id,10,team); //mark that particular obelisk as visited
	}
	else
	{
		iw.text.addTxt(MetaString::ADVOB_TXT, 97);
		cb->sendAndApply(&iw);
	}

}

void CGObelisk::initObj()
{
	obeliskCount++;
}

const std::string & CGObelisk::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID];
	if(hasVisited(cb->getCurrentPlayer()))
		hoverName += " " + VLC->generaltexth->allTexts[352]; //not visited
	else
		hoverName += " " + VLC->generaltexth->allTexts[353]; //visited
	return hoverName;
}

void CGObelisk::setPropertyDer( ui8 what, ui32 val )
{
	CPlayersVisited::setPropertyDer(what, val);
	switch(what)
	{
	case 20:
		assert(val < PLAYER_LIMIT);
		visited[val]++;

		if(visited[val] > obeliskCount)
		{
			tlog0 << "Error: Visited " << visited[val] << "\t\t" << obeliskCount << std::endl;
			assert(0);
		}

		break;
	}
}

void CGLighthouse::onHeroVisit( const CGHeroInstance * h ) const
{
	if(h->tempOwner != tempOwner)
	{
		ui8 oldOwner = tempOwner;
		cb->setOwner(id,h->tempOwner); //not ours? flag it!


		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text.addTxt(MetaString::ADVOB_TXT, 69);
		iw.soundID = soundBase::LIGHTHOUSE;
		cb->sendAndApply(&iw);

		giveBonusTo(h->tempOwner);

		if(oldOwner < PLAYER_LIMIT) //remove bonus from old owner
		{
			RemoveBonus rb(RemoveBonus::PLAYER);
			rb.whoID = oldOwner;
			rb.source = Bonus::OBJECT;
			rb.id = id;
			cb->sendAndApply(&rb);
		}
	}
}

void CGLighthouse::initObj()
{
	if(tempOwner < PLAYER_LIMIT)
	{
		giveBonusTo(tempOwner);
	}
}

const std::string & CGLighthouse::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID];
	//TODO: owned by %s player
	return hoverName;
}

void CGLighthouse::giveBonusTo( ui8 player ) const
{
	GiveBonus gb(GiveBonus::PLAYER);
	gb.bonus.type = Bonus::SEA_MOVEMENT;
	gb.bonus.val = 500;
	gb.id = player;
	gb.bonus.duration = Bonus::PERMANENT;
	gb.bonus.source = Bonus::OBJECT;
	gb.bonus.id = id;
	cb->sendAndApply(&gb);

}

void CArmedInstance::setArmy(const CCreatureSet &src)
{
	slots.clear();

	for(TSlots::const_iterator i = src.Slots().begin(); i != src.Slots().end(); i++)
	{
		CStackInstance &inserted = slots[i->first];
		inserted = i->second;
		inserted.armyObj = this;
	}
}

CCreatureSet CArmedInstance::getArmy() const
{
	return *this;
}

void CArmedInstance::randomizeArmy(int type)
{
	int max = VLC->creh->creatures.size();
	for (TSlots::iterator j = slots.begin(); j != slots.end();j++)
	{
		if(j->second.idRand > max)
		{
			if(j->second.idRand % 2)
				j->second.setType(VLC->townh->towns[type].basicCreatures[(j->second.idRand-197) / 2 -1]);
			else
				j->second.setType(VLC->townh->towns[type].upgradedCreatures[(j->second.idRand-197) / 2 -1]);

			j->second.idRand = -1;
		}

		assert(j->second.armyObj == this);
	}
	return;
}

void CArmedInstance::getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const
{
	const PlayerState *p = cb->getPlayerState(tempOwner);
	if(p)
		out.insert(p); //hero always inherits bonuses from player

	out.insert(&cb->gameState()->globalEffects); //global effect are always active I believe

	if(battle)
		out.insert(battle);
}

CArmedInstance::CArmedInstance()
{
	battle = NULL;
}

void CArmedInstance::getBonuses(BonusList &out, const CSelector &selector, const CBonusSystemNode *root /*= NULL*/) const
{
	CBonusSystemNode::getBonuses(out, selector, root);

	if(!battle)
	{
		//TODO do it clean, unify with BattleInfo version
		if(Selector::matchesType(selector, Bonus::MORALE) || Selector::matchesType(selector, Bonus::LUCK))
		{
			for(TSlots::const_iterator i=Slots().begin(); i!=Slots().end(); i++)
				i->second.getBonuses(out, selector, Selector::effectRange(Bonus::ONLY_ALLIED_ARMY), this);
		}
	}

	if(Selector::matchesType(selector, Bonus::MORALE))
	{
		//number of alignments and presence of undead
		if(contains(dynamic_cast<const CStackInstance*>(root)))
		{
			bool archangelInArmy = false;
			bool canMix = hasBonusOfType(Bonus::NONEVIL_ALIGNMENT_MIX);
			std::set<si8> factions;
			for(TSlots::const_iterator i=Slots().begin(); i!=Slots().end(); i++)
			{
				// Take Angelic Alliance troop-mixing freedom of non-evil, non-Conflux units into account.
				const si8 faction = i->second.type->faction;
				if (canMix
					&& ((faction >= 0 && faction <= 2) || faction == 6 || faction == 7))
				{
					factions.insert(0); // Insert a single faction of the affected group, Castle will do.
				}
				else
				{
					factions.insert(faction);
				}

				if(i->second.type->idNumber == 13)
					archangelInArmy = true;
			}

			if(factions.size() == 1)
				out.push_back(Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, +1, id, VLC->generaltexth->arraytxt[115]));//All troops of one alignment +1
			else
			{
				int fcountModifier = 2-factions.size();
				out.push_back(Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, fcountModifier, id, boost::str(boost::format(VLC->generaltexth->arraytxt[114]) % factions.size() % fcountModifier)));//Troops of %d alignments %d
			}

			if(vstd::contains(factions,4))
				out.push_back(Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, -1, id, VLC->generaltexth->arraytxt[116]));//Undead in group -1
		}
	}
}

bool IMarket::getOffer(int id1, int id2, int &val1, int &val2, EMarketMode mode) const
{
	switch(mode)
	{
	case RESOURCE_RESOURCE:
		{
			float effectiveness = std::min(((float)getMarketEfficiency()+1.0f) / 20.0f, 0.5f);

			float r = VLC->objh->resVals[id1], //value of given resource
				g = VLC->objh->resVals[id2] / effectiveness; //value of wanted resource

			if(r>g) //if given resource is more expensive than wanted
			{
				val2 = ceil(r / g);
				val1 = 1;
			}
			else //if wanted resource is more expensive
			{
				val1 = (g / r) + 0.5f;
				val2 = 1;
			}
		}
		break;
	case CREATURE_RESOURCE:
		{
			const float effectivenessArray[] = {0, 0.3, 0.45, 0.50, 0.65, 0.7, 0.85, 0.9, 1};
			float effectiveness = effectivenessArray[std::min(getMarketEfficiency(), 8)];

			float r = VLC->creh->creatures[id1]->cost[6], //value of given creature in gold
				g = VLC->objh->resVals[id2] / effectiveness; //value of wanted resource

			if(r>g) //if given resource is more expensive than wanted
			{
				val2 = ceil(r / g);
				val1 = 1;
			}
			else //if wanted resource is more expensive
			{
				val1 = (g / r) + 0.5f;
				val2 = 1;
			}
		}
		break;
	case RESOURCE_PLAYER:
		val1 = 1;
		val2 = 1;
		break;
	case RESOURCE_ARTIFACT:
		{
			float effectiveness = std::min(((float)getMarketEfficiency()+3.0f) / 20.0f, 0.6f);
			float r = VLC->objh->resVals[id1], //value of offered resource
				g = VLC->arth->artifacts[id2]->price / effectiveness; //value of bought artifact in gold
			
			if(id1 != 6) //non-gold prices are doubled
				r /= 2; 

			assert(g >= r); //should we allow artifacts cheaper than unit of resource?
			val1 = (g / r) + 0.5f;
			val2 = 1;
		}
		break;

	case CREATURE_EXP:
		{
			val1 = 1;
			val2 = (VLC->creh->creatures[id1]->AIValue / 40) * 5;
		}
		break;
	case ARTIFACT_EXP:
		{
			val1 = 1;

			int givenClass = VLC->arth->artifacts[id1]->getArtClassSerial();
			if(givenClass < 0 || givenClass > 3)
			{
				val2 = 0;
				return false;
			}

			static const int expPerClass[] = {1000, 1500, 3000, 6000};
			val2 = expPerClass[givenClass];
		}
		break;
	default:
		assert(0);
		return false;
	}

	return true;
}

bool IMarket::allowsTrade(EMarketMode mode) const
{
	return false;
}

int IMarket::availableUnits(EMarketMode mode, int marketItemSerial) const
{
	switch(mode)
	{
	case RESOURCE_RESOURCE:
	case ARTIFACT_RESOURCE:
	case CREATURE_RESOURCE:
			return -1;
	default:
			return 1;
	}
}

std::vector<int> IMarket::availableItemsIds(EMarketMode mode) const
{
	std::vector<int> ret;
	switch(mode)
	{
	case RESOURCE_RESOURCE:
	case ARTIFACT_RESOURCE:
	case CREATURE_RESOURCE:
		for (int i = 0; i < 7; i++)
			ret.push_back(i);
		break;
	}
	return ret;
}

const IMarket * IMarket::castFrom(const CGObjectInstance *obj)
{
	switch(obj->ID)
	{
	case TOWNI_TYPE:
		return static_cast<const CGTownInstance*>(obj);
	case 2: //Altar of Sacrifice
	case 7: //Black Market
	case 99: //Trading Post
	case 221: //Trading Post (snow)
	case 213: //Freelancer's Guild
		return static_cast<const CGMarket*>(obj);
	case 104: //University
		return static_cast<const CGUniversity*>(obj);
	default:
		tlog1 << "Cannot cast to IMarket object with ID " << obj->ID << std::endl;
		return NULL;
	}
}

IMarket::IMarket(const CGObjectInstance *O)
	:o(O)
{

}

std::vector<EMarketMode> IMarket::availableModes() const
{
	std::vector<EMarketMode> ret;
	for (int i = 0; i < MARTKET_AFTER_LAST_PLACEHOLDER; i++)
		if(allowsTrade((EMarketMode)i))
			ret.push_back((EMarketMode)i);

	return ret;
}

void CGMarket::onHeroVisit(const CGHeroInstance * h) const
{
	OpenWindow ow;
	ow.id1 = id;
	ow.id2 = h->id;
	ow.window = OpenWindow::MARKET_WINDOW;
	cb->sendAndApply(&ow);
}

int CGMarket::getMarketEfficiency() const
{
	return 5;
}

bool CGMarket::allowsTrade(EMarketMode mode) const
{
	switch(mode)
	{
	case RESOURCE_RESOURCE:
	case RESOURCE_PLAYER:
		switch(ID)
		{
		case 99: //Trading Post
		case 221: //Trading Post (snow)
			return true;
		default:
			return false;
		}
	case CREATURE_RESOURCE:
		return ID == 213; //Freelancer's Guild
	//case ARTIFACT_RESOURCE:
	case RESOURCE_ARTIFACT:
		return ID == 7; //Black Market
	case ARTIFACT_EXP:
	case CREATURE_EXP:
		return ID == 2; //TODO? check here for alignment of visiting hero? - would not be coherent with other checks here
	case RESOURCE_SKILL:
		return ID == 104;//University
	}
	return false;
}

int CGMarket::availableUnits(EMarketMode mode, int marketItemSerial) const
{
	return -1;
}

std::vector<int> CGMarket::availableItemsIds(EMarketMode mode) const
{
	switch(mode)
	{
	case RESOURCE_RESOURCE:
	case RESOURCE_PLAYER:
		return IMarket::availableItemsIds(mode);
	default:
		return std::vector<int>();
	}
}

CGMarket::CGMarket()
	:IMarket(this)
{
}

std::vector<int> CGBlackMarket::availableItemsIds(EMarketMode mode) const
{
	switch(mode)
	{
	case ARTIFACT_RESOURCE:
		return IMarket::availableItemsIds(mode);
	case RESOURCE_ARTIFACT:
		{
			std::vector<int> ret;
			BOOST_FOREACH(const CArtifact *a, artifacts)
				if(a)
					ret.push_back(a->id);
				else
					ret.push_back(-1);
			return ret;
		}
	default:
		return std::vector<int>();
	}
}

void CGBlackMarket::newTurn() const
{
	if(cb->getDate(2) != 1)
		return;

	SetAvailableArtifacts saa;
	saa.id = id;
	cb->pickAllowedArtsSet(saa.arts);
	cb->sendAndApply(&saa);
}

void CGUniversity::initObj()
{
	std::vector <int> toChoose;
	for (int i=0; i<SKILL_QUANTITY; i++)
		if (cb->isAllowed(2,i))
			toChoose.push_back(i);
	if (toChoose.size() < 4)
	{
		tlog0<<"Warning: less then 4 available skills was found by University initializer!\n";
		return;
	}
	
	for (int i=0; i<4; i++)//get 4 skills
	{
		int skillPos = ran()%toChoose.size();
		skills.push_back(toChoose[skillPos]);//move it to selected
		toChoose.erase(toChoose.begin()+skillPos);//remove from list
	}
}

std::vector<int> CGUniversity::availableItemsIds(EMarketMode mode) const
{
	switch (mode)
	{
		case RESOURCE_SKILL:
			return skills;
			
		default:
			return std::vector <int> ();
	}
}

void CGUniversity::onHeroVisit(const CGHeroInstance * h) const
{
	OpenWindow ow;
	ow.id1 = id;
	ow.id2 = h->id;
	ow.window = OpenWindow::UNIVERSITY_WINDOW;
	cb->sendAndApply(&ow);
}

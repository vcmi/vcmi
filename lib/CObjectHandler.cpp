/*
 * CObjectHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CObjectHandler.h"

#include "CDefObjInfoHandler.h"
#include "CGeneralTextHandler.h"
#include "CDefObjInfoHandler.h"
#include "CHeroHandler.h"
#include "CSpellHandler.h"
#include "CModHandler.h"
#include "../client/CSoundBase.h"
#include "CTownHandler.h"
#include "CCreatureHandler.h"
#include "VCMI_Lib.h"
#include "IGameCallback.h"
#include "CGameState.h"
#include "NetPacks.h"
#include "StartInfo.h"
#include "mapping/CMap.h"
#include <SDL_stdinc.h>
#include "CBuildingHandler.h"
#include "JsonNode.h"
#include "filesystem/Filesystem.h"

using namespace boost::assign;

// It looks that we can't rely on shadowCoverage correctness (Mantis #866). This may result
// in notable performance decrease (SDL blit with custom alpha blit) not notable on my system (Ivan)
#define USE_COVERAGE_MAP 0


std::map<Obj, std::map<int, std::vector<ObjectInstanceID> > > CGTeleport::objs;
std::vector<std::pair<ObjectInstanceID, ObjectInstanceID> > CGTeleport::gates;
IGameCallback * IObjectInterface::cb = nullptr;
extern std::minstd_rand ran;
std::map <PlayerColor, std::set <ui8> > CGKeys::playerKeyMap;
std::map <si32, std::vector<ObjectInstanceID> > CGMagi::eyelist;
ui8 CGObelisk::obeliskCount; //how many obelisks are on map
std::map<TeamID, ui8> CGObelisk::visited; //map: team_id => how many obelisks has been visited

std::vector<const CArtifact *> CGTownInstance::merchantArtifacts;
std::vector<int> CGTownInstance::universitySkills;

///helpers
static void openWindow(const OpenWindow::EWindow type, const int id1, const int id2 = -1)
{
	OpenWindow ow;
	ow.window = type;
	ow.id1 = id1;
	ow.id2 = id2;
	IObjectInterface::cb->sendAndApply(&ow);
}

static void showInfoDialog(const PlayerColor playerID, const ui32 txtID, const ui16 soundID)
{
	InfoWindow iw;
	iw.soundID = soundID;
	iw.player = playerID;
	iw.text.addTxt(MetaString::ADVOB_TXT,txtID);
	IObjectInterface::cb->sendAndApply(&iw);
}

/*static void showInfoDialog(const ObjectInstanceID heroID, const ui32 txtID, const ui16 soundID)
{
	const PlayerColor playerID = IObjectInterface::cb->getOwner(heroID);
	showInfoDialog(playerID,txtID,soundID);
}*/

static void showInfoDialog(const CGHeroInstance* h, const ui32 txtID, const ui16 soundID)
{
	const PlayerColor playerID = h->getOwner();
	showInfoDialog(playerID,txtID,soundID);
}

static std::string & visitedTxt(const bool visited)
{
	int id = visited ? 352 : 353;
	return VLC->generaltexth->allTexts[id];
}

///IObjectInterface
void IObjectInterface::onHeroVisit(const CGHeroInstance * h) const
{}

void IObjectInterface::onHeroLeave(const CGHeroInstance * h) const
{}

void IObjectInterface::newTurn () const
{}

IObjectInterface::~IObjectInterface()
{}

IObjectInterface::IObjectInterface()
{}

void IObjectInterface::initObj()
{}

void IObjectInterface::setProperty( ui8 what, ui32 val )
{}

bool IObjectInterface::wasVisited (PlayerColor player) const
{
	return false;
}
bool IObjectInterface::wasVisited (const CGHeroInstance * h) const
{
	return false;
}

void IObjectInterface::postInit()
{}

void IObjectInterface::preInit()
{}

void IObjectInterface::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{

}

void IObjectInterface::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{

}

void IObjectInterface::garrisonDialogClosed(const CGHeroInstance *hero) const
{

}

void IObjectInterface::heroLevelUpDone(const CGHeroInstance *hero) const
{

}

void CPlayersVisited::setPropertyDer( ui8 what, ui32 val )
{
	if(what == 10)
		players.insert(PlayerColor(val));
}

bool CPlayersVisited::wasVisited( PlayerColor player ) const
{
	return vstd::contains(players,player);
}

bool CPlayersVisited::wasVisited( TeamID team ) const
{
	for(auto i : players)
	{
		if(cb->getPlayer(i)->team == team)
			return true;
	}
	return false;
}

// Bank helper. Find the creature ID and their number, and store the
// result in storage (either guards or reward creatures).
static void readCreatures(const JsonNode &creature, std::vector< std::pair <CreatureID, ui32> > &storage)
{
	std::pair<CreatureID, si32> creInfo = std::make_pair(CreatureID::NONE, 0);

	//TODO: replace numeric id's with mod-friendly string id's
	creInfo.second = creature["number"].Float();
	creInfo.first = CreatureID((si32)creature["id"].Float());
	storage.push_back(creInfo);
}

// Bank helper. Process a bank level.
static void readBankLevel(const JsonNode &level, BankConfig &bc)
{
	int idx;

	bc.chance = level["chance"].Float();

	for(const JsonNode &creature : level["guards"].Vector())
	{
		readCreatures(creature, bc.guards);
	}

	bc.upgradeChance = level["upgrade_chance"].Float();
	bc.combatValue = level["combat_value"].Float();

	bc.resources = Res::ResourceSet(level["reward_resources"]);

	for(const JsonNode &creature : level["reward_creatures"].Vector())
	{
		readCreatures(creature, bc.creatures);
	}

	bc.artifacts.resize(4);
	idx = 0;
	for(const JsonNode &artifact : level["reward_artifacts"].Vector())
	{
		bc.artifacts[idx] = artifact.Float();
		idx ++;
	}

	bc.value = level["value"].Float();
	bc.rewardDifficulty = level["profitability"].Float();
	bc.easiest = level["easiest"].Float();
}

CObjectHandler::CObjectHandler()
{
    logGlobal->traceStream() << "\t\tReading cregens ";

	const JsonNode config(ResourceID("config/dwellings.json"));
	for(const JsonNode &dwelling : config["dwellings"].Vector())
	{
		cregens[dwelling["dwelling"].Float()] = CreatureID((si32)dwelling["creature"].Float());
	}
    logGlobal->traceStream() << "\t\tDone loading cregens!";

    logGlobal->traceStream() << "\t\tReading resources prices ";
	const JsonNode config2(ResourceID("config/resources.json"));
	for(const JsonNode &price : config2["resources_prices"].Vector())
	{
		resVals.push_back(price.Float());
	}
    logGlobal->traceStream() << "\t\tDone loading resource prices!";

    logGlobal->traceStream() << "\t\tReading banks configs";
	const JsonNode config3(ResourceID("config/bankconfig.json"));
	int bank_num = 0;
	for(const JsonNode &bank : config3["banks"].Vector())
	{
		creBanksNames[bank_num] = bank["name"].String();

		int level_num = 0;
		for(const JsonNode &level : bank["levels"].Vector())
		{
			banksInfo[bank_num].push_back(new BankConfig);
			BankConfig &bc = *banksInfo[bank_num].back();
			bc.level = level_num;

			readBankLevel(level, bc);
			level_num ++;
		}

		bank_num ++;
	}
    logGlobal->traceStream() << "\t\tDone loading banks configs";
}

CObjectHandler::~CObjectHandler()
{
	for(auto & mapEntry : banksInfo)
	{
		for(auto & vecEntry : mapEntry.second)
		{
			vecEntry.dellNull();
		}
	}
}

int CObjectHandler::bankObjToIndex (const CGObjectInstance * obj)
{
	switch (obj->ID) //find appriopriate key
	{
	case Obj::CREATURE_BANK:
		return obj->subID;
	case Obj::DERELICT_SHIP:
		return 8;
	case Obj::DRAGON_UTOPIA:
		return 10;
	case Obj::CRYPT:
		return 9;
	case Obj::SHIPWRECK:
		return 7;
	default:
        logGlobal->warnStream() << "Unrecognized Bank indetifier!";
		return 0;
	}
}
PlayerColor CGObjectInstance::getOwner() const
{
	//if (state)
	//	return state->owner;
	//else
		return tempOwner; //won't have owner
}

CGObjectInstance::CGObjectInstance()
{
	pos = int3(-1,-1,-1);
	//state = new CLuaObjectScript();
	ID = Obj::NO_OBJ;
	subID = -1;
	defInfo = nullptr;
	tempOwner = PlayerColor::UNFLAGGABLE;
	blockVisit = false;
}
CGObjectInstance::~CGObjectInstance()
{
	//if (state)
	//	delete state;
	//state=nullptr;
}

const std::string & CGObjectInstance::getHoverText() const
{
	return hoverName;
}
void CGObjectInstance::setOwner(PlayerColor ow)
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
	if(defInfo==nullptr)
	{
        logGlobal->warnStream() << "Warning: VisitableAt for obj "<< id.getNum() <<": nullptr defInfo!";
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
	if(x<0 || y<0 || x>=getWidth() || y>=getHeight() || defInfo==nullptr)
		return false;
	if((defInfo->blockMap[y+6-getHeight()] >> (7-(8-getWidth()+x) )) & 1)
		return false;
	return true;
}

bool CGObjectInstance::coveringAt(int x, int y) const
{
	//input coordinates are always negative
	x = -x;
	y = -y;
#if USE_COVERAGE_MAP
	//NOTE: this code may be broken
	if((defInfo->coverageMap[y] >> (7-(x) )) & 1
		||  (defInfo->shadowCoverage[y] >> (7-(x) )) & 1)
		return true;
	return false;
#else
	return x >= 0 && y >= 0 && x < getWidth() && y < getHeight();
#endif
}

bool CGObjectInstance::hasShadowAt( int x, int y ) const
{
#if USE_COVERAGE_MAP
	//NOTE: this code may be broken
	if( (defInfo->shadowCoverage[y] >> (7-(x) )) & 1 )
		return true;
	return false;
#else
	return coveringAt(x,y);// ignore unreliable shadowCoverage map
#endif
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
	if(cmp.ID==Obj::HERO && ID!=Obj::HERO)
		return true;
	if(cmp.ID!=Obj::HERO && ID==Obj::HERO)
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
	case Obj::TAVERN:
		blockVisit = true;
		break;
	}
}

void CGObjectInstance::setProperty( ui8 what, ui32 val )
{
	switch(what)
	{
	case ObjProperty::OWNER:
		tempOwner = PlayerColor(val);
		break;
	case ObjProperty::BLOCKVIS:
		blockVisit = val;
		break;
	case ObjProperty::ID:
		ID = Obj(val);
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
void CGObjectInstance::getSightTiles(std::unordered_set<int3, ShashInt3> &tiles) const //returns reference to the set
{
	cb->getTilesInRange(tiles, getSightCenter(), getSightRadious(), tempOwner, 1);
}
void CGObjectInstance::hideTiles(PlayerColor ourplayer, int radius) const
{
	for (auto i = cb->gameState()->teams.begin(); i != cb->gameState()->teams.end(); i++)
	{
		if ( !vstd::contains(i->second.players, ourplayer ))//another team
		{
			for (auto & elem : i->second.players)
				if ( cb->getPlayer(elem)->status == EPlayerStatus::INGAME )//seek for living player (if any)
				{
					FoWChange fw;
					fw.mode = 0;
					fw.player = elem;
					cb->getTilesInRange (fw.tiles, pos, radius, (elem), -1);
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

    logGlobal->warnStream() << "Warning: getVisitableOffset called on non-visitable obj!";
	return int3(-1,-1,-1);
}

void CGObjectInstance::getNameVis( std::string &hname ) const
{
	const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
	hname = VLC->generaltexth->names[ID];
	if(h)
	{
		const bool visited = h->hasBonusFrom(Bonus::OBJECT,ID);
		hname + " " + visitedTxt(visited);
	}
}

void CGObjectInstance::giveDummyBonus(ObjectInstanceID heroID, ui8 duration) const
{
	GiveBonus gbonus;
	gbonus.bonus.type = Bonus::NONE;
	gbonus.id = heroID.getNum();
	gbonus.bonus.duration = duration;
	gbonus.bonus.source = Bonus::OBJECT;
	gbonus.bonus.sid = ID;
	cb->giveHeroBonus(&gbonus);
}

void CGObjectInstance::onHeroVisit( const CGHeroInstance * h ) const
{
	switch(ID)
	{
	case Obj::HILL_FORT:
		{
			openWindow(OpenWindow::HILL_FORT_WINDOW,id.getNum(),h->id.getNum());
		}
		break;
	case Obj::SANCTUARY:
		{
			//You enter the sanctuary and immediately feel as if a great weight has been lifted off your shoulders.  You feel safe here.
			showInfoDialog(h,114,soundBase::GETPROTECTION);
		}
		break;
	case Obj::TAVERN:
		{
			openWindow(OpenWindow::TAVERN_WINDOW,h->id.getNum(),id.getNum());
		}
		break;
	}
}

ui8 CGObjectInstance::getPassableness() const
{
	return 0;
}

int3 CGObjectInstance::visitablePos() const
{
	return pos - getVisitableOffset();
}

bool CGObjectInstance::isVisitable() const
{
	for(int g=0; g<ARRAY_COUNT(defInfo->visitMap); ++g)
	{
		if(defInfo->visitMap[g] != 0)
		{
			return true;
		}
	}
	return false;
}

bool CGObjectInstance::passableFor(PlayerColor color) const
{
	return getPassableness() & 1<<color.getNum();
}

static int lowestSpeed(const CGHeroInstance * chi)
{
	if(!chi->Slots().size())
	{
        logGlobal->errorStream() << "Error! Hero " << chi->id.getNum() << " ("<<chi->name<<") has no army!";
		return 20;
	}
	auto i = chi->Slots().begin();
	//TODO? should speed modifiers (eg from artifacts) affect hero movement?
	int ret = (i++)->second->valOfBonuses(Bonus::STACKS_SPEED);
	for (;i!=chi->Slots().end();i++)
	{
		ret = std::min(ret, i->second->valOfBonuses(Bonus::STACKS_SPEED));
	}
	return ret;
}

ui32 CGHeroInstance::getTileCost(const TerrainTile &dest, const TerrainTile &from) const
{
	//base move cost
	unsigned ret = 100;

	//if there is road both on dest and src tiles - use road movement cost
    if(dest.roadType != ERoadType::NO_ROAD && from.roadType != ERoadType::NO_ROAD)
	{
        int road = std::min(dest.roadType,from.roadType); //used road ID
		switch(road)
		{
        case ERoadType::DIRT_ROAD:
			ret = 75;
			break;
        case ERoadType::GRAVEL_ROAD:
			ret = 65;
			break;
        case ERoadType::COBBLESTONE_ROAD:
			ret = 50;
			break;
		default:
            logGlobal->errorStream() << "Unknown road type: " << road << "... Something wrong!";
			break;
		}
	}
	else
	{
		//FIXME: in H3 presence of Nomad in army will remove terrain penalty for sand. Bonus not implemented in VCMI

		// NOTE: in H3 neutral stacks will ignore terrain penalty only if placed as topmost stack(s) in hero army.
		// This is clearly bug in H3 however intended behaviour is not clear.
		// Current VCMI behaviour will ignore neutrals in calculations so army in VCMI
		// will always have best penalty without any influence from player-defined stacks order

		bool nativeArmy = true;
		for(auto stack : stacks)
		{
			int nativeTerrain = VLC->townh->factions[stack.second->type->faction]->nativeTerrain;

            if (nativeTerrain != -1 && nativeTerrain != from.terType)
			{
				nativeArmy = false;
				break;
			}
		}
		if (!nativeArmy)
            ret = VLC->heroh->terrCosts[from.terType];
 	}
	return ret;
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

bool CGHeroInstance::canWalkOnSea() const
{
	return hasBonusOfType(Bonus::FLYING_MOVEMENT) || hasBonusOfType(Bonus::WATER_WALKING);
}

ui8 CGHeroInstance::getSecSkillLevel(SecondarySkill skill) const
{
	for(auto & elem : secSkills)
		if(elem.first == skill)
			return elem.second;
	return 0;
}

void CGHeroInstance::setSecSkillLevel(SecondarySkill which, int val, bool abs)
{
	if(getSecSkillLevel(which) == 0)
	{
		secSkills.push_back(std::pair<SecondarySkill,ui8>(which, val));
		updateSkill(which, val);
	}
	else
	{
		for (auto & elem : secSkills)
		{
			if(elem.first == which)
			{
				if(abs)
					elem.second = val;
				else
					elem.second += val;

				if(elem.second > 3) //workaround to avoid crashes when same sec skill is given more than once
				{
                    logGlobal->warnStream() << "Warning: Skill " << which << " increased over limit! Decreasing to Expert.";
					elem.second = 3;
				}
				updateSkill(which, elem.second); //when we know final value
			}
		}
	}
}

bool CGHeroInstance::canLearnSkill() const
{
	return secSkills.size() < GameConstants::SKILL_PER_HERO;
}

int CGHeroInstance::maxMovePoints(bool onLand) const
{
	int base;

	if(onLand)
	{
		// used function is f(x) = 66.6x + 1300, rounded to second digit, where x is lowest speed in army
		static const int baseSpeed = 1300; // base speed from creature with 0 speed

		int armySpeed = lowestSpeed(this) * 20 / 3;

		base = armySpeed * 10 + baseSpeed; // separate *10 is intentional to receive same rounding as in h3
		vstd::abetween(base, 1500, 2000); // base speed is limited by these values
	}
	else
	{
		base = 1500; //on water base movement is always 1500 (speed of army doesn't matter)
	}

	const Bonus::BonusType bt = onLand ? Bonus::LAND_MOVEMENT : Bonus::SEA_MOVEMENT;
	const int bonus = valOfBonuses(Bonus::MOVEMENT) + valOfBonuses(bt);

	const int subtype = onLand ? SecondarySkill::LOGISTICS : SecondarySkill::NAVIGATION;
	const double modifier = valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, subtype) / 100.0;

	return int(base* (1+modifier)) + bonus;
}

CGHeroInstance::CGHeroInstance()
 : IBoatGenerator(this)
{
	setNodeType(HERO);
	ID = Obj::HERO;
	tacticFormationEnabled = inTownGarrison = false;
	mana = movement = portrait = level = -1;
	isStanding = true;
	moveDir = 4;
	exp = 0xffffffff;
	visitedTown = nullptr;
	type = nullptr;
	boat = nullptr;
	commander = nullptr;
	sex = 0xff;
	secSkills.push_back(std::make_pair(SecondarySkill::DEFAULT, -1));
}

void CGHeroInstance::initHero(HeroTypeID SUBID)
{
	subID = SUBID.getNum();
	initHero();
}

void CGHeroInstance::initHero()
{
	assert(validTypes(true));
	if(ID == Obj::HERO)
		initHeroDefInfo();
	if(!type)
		type = VLC->heroh->heroes[subID];

	if(!vstd::contains(spells, SpellID::PRESET)) //hero starts with a spell
	{
		for(auto spellID : type->spells)
			spells.insert(spellID);
	}
	else //remove placeholder
		spells -= SpellID::PRESET;

	if(!getArt(ArtifactPosition::MACH4) && !getArt(ArtifactPosition::SPELLBOOK) && type->haveSpellBook) //no catapult means we haven't read pre-existent set -> use default rules for spellbook
		putArtifact(ArtifactPosition::SPELLBOOK, CArtifactInstance::createNewArtifactInstance(0));

	if(!getArt(ArtifactPosition::MACH4))
		putArtifact(ArtifactPosition::MACH4, CArtifactInstance::createNewArtifactInstance(3)); //everyone has a catapult

	if(portrait < 0 || portrait == 255)
		portrait = type->imageIndex;
	if(!hasBonus(Selector::sourceType(Bonus::HERO_BASE_SKILL)))
	{
		for(int g=0; g<GameConstants::PRIMARY_SKILLS; ++g)
		{
			pushPrimSkill(static_cast<PrimarySkill::PrimarySkill>(g), type->heroClass->primarySkillInitial[g]);
		}
	}
	if(secSkills.size() == 1 && secSkills[0] == std::pair<SecondarySkill,ui8>(SecondarySkill::DEFAULT, -1)) //set secondary skills to default
		secSkills = type->secSkillsInit;
	if (!name.length())
		name = type->name;

	if (sex == 0xFF)//sex is default
		sex = type->sex;

	setFormation(false);
	if (!stacksCount()) //standard army//initial army
	{
		initArmy();
	}
	assert(validTypes());

	if (exp == 0xffffffff)
	{
		initExp();
	}
	else if (ID != Obj::PRISON)
	{
		level = VLC->heroh->level(exp);
	}
	else //warp hero at the beginning of next turn
	{
		level = 1;
	}

	if (VLC->modh->modules.COMMANDERS && !commander)
	{
		commander = new CCommanderInstance (VLC->townh->factions[type->heroClass->faction]->commander);
		commander->setArmyObj (castToArmyObj()); //TODO: separate function for setting commanders
		commander->giveStackExp (exp); //after our exp is set
	}

	hoverName = VLC->generaltexth->allTexts[15];
	boost::algorithm::replace_first(hoverName,"%s",name);
	boost::algorithm::replace_first(hoverName,"%s", type->heroClass->name);

	if (mana < 0)
		mana = manaLimit();
}

void CGHeroInstance::initArmy(IArmyDescriptor *dst /*= nullptr*/)
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

	vstd::amin(howManyStacks, type->initialArmy.size());

	for(int stackNo=0; stackNo < howManyStacks; stackNo++)
	{
		auto & stack = type->initialArmy[stackNo];

		int range = stack.maxAmount - stack.minAmount;
		int count = ran()%(range+1) + stack.minAmount;

		if(stack.creature >= CreatureID::CATAPULT &&
		   stack.creature <= CreatureID::ARROW_TOWERS) //war machine
		{
			warMachinesGiven++;
			if(dst != this)
				continue;

			int slot = -1;
			ArtifactID aid = ArtifactID::NONE;
			switch (stack.creature)
			{
			case CreatureID::CATAPULT:
				slot = ArtifactPosition::MACH4;
				aid = ArtifactID::CATAPULT;
				break;
			default:
				aid = CArtHandler::creatureToMachineID(stack.creature);
				slot = 9 + aid;
				break;
			}
			auto convSlot = ArtifactPosition(slot);
			if(!getArt(convSlot))
				putArtifact(convSlot, CArtifactInstance::createNewArtifactInstance(aid));
			else
                logGlobal->warnStream() << "Hero " << name << " already has artifact at " << slot << ", omitting giving " << aid;
		}
		else
			dst->setCreature(SlotID(stackNo-warMachinesGiven), stack.creature, count);
	}
}
void CGHeroInstance::initHeroDefInfo()
{
	if(!defInfo  ||  defInfo->id != Obj::HERO)
	{
		defInfo = new CGDefInfo();
		defInfo->id = Obj::HERO;
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
	defInfo->blockMap[5] = 253;
	defInfo->visitMap[5] = 2;
	defInfo->coverageMap[4] = defInfo->coverageMap[5] = 224;
}
CGHeroInstance::~CGHeroInstance()
{
	commander.dellNull();
}

bool CGHeroInstance::needsLastStack() const
{
	return true;
}

void CGHeroInstance::onHeroVisit(const CGHeroInstance * h) const
{
	if(h == this) return; //exclude potential self-visiting

	if (ID == Obj::HERO)
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
	else if(ID == Obj::PRISON)
	{
		int txt_id;

		if(cb->getHeroCount(h->tempOwner,false) < GameConstants::MAX_HEROES_PER_PLAYER) //free hero slot
		{
			cb->changeObjPos(id,pos+int3(1,0,0),0);
			//update hero parameters
			SetMovePoints smp;
			smp.hid = id;
			smp.val = maxMovePoints (true); //TODO: hota prison on water?
			cb->setMovePoints (&smp);
			cb->setManaPoints (id, manaLimit());

			cb->setObjProperty(id, ObjProperty::ID, Obj::HERO); //set ID to 34
			cb->giveHero(id,h->tempOwner); //recreates def and adds hero to player

			txt_id = 102;
		}
		else //already 8 wandering heroes
		{
			txt_id = 103;
		}

		showInfoDialog(h,txt_id,soundBase::ROGUE);
	}
}

const std::string & CGHeroInstance::getBiography() const
{
	if (biography.length())
		return biography;
	return type->biography;
}
void CGHeroInstance::initObj() //TODO: use bonus system
{
	blockVisit = true;
	auto  hs = new HeroSpecial();
	hs->setNodeType(CBonusSystemNode::specialty);
	attachTo(hs); //do we ever need to detach it?

	if(!type)
		initHero(); //TODO: set up everything for prison before specialties are configured

	for(const auto &spec : type->spec) //TODO: unfity with bonus system
	{
		auto bonus = new Bonus();
		bonus->val = spec.val;
		bonus->sid = id.getNum(); //from the hero, specialty has no unique id
		bonus->duration = Bonus::PERMANENT;
		bonus->source = Bonus::HERO_SPECIAL;
		switch (spec.type)
		{
			case 1:// creature specialty
				{
					hs->growsWithLevel = true;

					const CCreature &specCreature = *VLC->creh->creatures[spec.additionalinfo]; //creature in which we have specialty

					int creLevel = specCreature.level;
					if(!creLevel)
					{
						if(spec.additionalinfo == 146)
							creLevel = 5; //treat ballista as 5-level
						else
						{
                            logGlobal->warnStream() << "Warning: unknown level of " << specCreature.namePl;
							continue;
						}
					}

					//bonus->additionalInfo = spec.additionalinfo; //creature id, should not be used again - this works only with limiter
					bonus->limiter.reset(new CCreatureTypeLimiter (specCreature, true)); //with upgrades
					bonus->type = Bonus::PRIMARY_SKILL;
					bonus->valType = Bonus::ADDITIVE_VALUE;

					bonus->subtype = PrimarySkill::ATTACK;
					hs->addNewBonus(bonus);

					bonus = new Bonus(*bonus);
					bonus->subtype = PrimarySkill::DEFENSE;
					hs->addNewBonus(bonus);
					//values will be calculated later

					bonus = new Bonus(*bonus);
					bonus->type = Bonus::STACKS_SPEED;
					bonus->val = 1; //+1 speed
					hs->addNewBonus(bonus);
				}
				break;
			case 2://secondary skill
				hs->growsWithLevel = true;
				bonus->type = Bonus::SPECIAL_SECONDARY_SKILL; //needs to be recalculated with level, based on this value
				bonus->valType = Bonus::BASE_NUMBER; // to receive nonzero value
				bonus->subtype = spec.subtype; //skill id
				bonus->val = spec.val; //value per level, in percent
				hs->addNewBonus(bonus);
				bonus = new Bonus(*bonus);

				switch (spec.additionalinfo)
				{
					case 0: //normal
						bonus->valType = Bonus::PERCENT_TO_BASE;
						break;
					case 1: //when it's navigation or there's no 'base' at all
						bonus->valType = Bonus::PERCENT_TO_ALL;
						break;
				}
				bonus->type = Bonus::SECONDARY_SKILL_PREMY; //value will be calculated later
				hs->addNewBonus(bonus);
				break;
			case 3://spell damage bonus, level dependent but calculated elsewhere
				bonus->type = Bonus::SPECIAL_SPELL_LEV;
				bonus->subtype = spec.subtype;
				hs->addNewBonus(bonus);
				break;
			case 4://creature stat boost
				switch (spec.subtype)
				{
					case 1://attack
						bonus->type = Bonus::PRIMARY_SKILL;
						bonus->subtype = PrimarySkill::ATTACK;
						break;
					case 2://defense
						bonus->type = Bonus::PRIMARY_SKILL;
						bonus->subtype = PrimarySkill::DEFENSE;
						break;
					case 3:
						bonus->type = Bonus::CREATURE_DAMAGE;
						bonus->subtype = 0; //both min and max
						break;
					case 4://hp
						bonus->type = Bonus::STACK_HEALTH;
						break;
					case 5:
						bonus->type = Bonus::STACKS_SPEED;
						break;
					default:
						continue;
				}
				bonus->additionalInfo = spec.additionalinfo; //creature id
				bonus->valType = Bonus::ADDITIVE_VALUE;
				bonus->limiter.reset(new CCreatureTypeLimiter (*VLC->creh->creatures[spec.additionalinfo], true));
				hs->addNewBonus(bonus);
				break;
			case 5://spell damage bonus in percent
				bonus->type = Bonus::SPECIFIC_SPELL_DAMAGE;
				bonus->valType = Bonus::BASE_NUMBER; // current spell system is screwed
				bonus->subtype = spec.subtype; //spell id
				hs->addNewBonus(bonus);
				break;
			case 6://damage bonus for bless (Adela)
				bonus->type = Bonus::SPECIAL_BLESS_DAMAGE;
				bonus->subtype = spec.subtype; //spell id if you ever wanted to use it otherwise
				bonus->additionalInfo = spec.additionalinfo; //damage factor
				hs->addNewBonus(bonus);
				break;
			case 7://maxed mastery for spell
				bonus->type = Bonus::MAXED_SPELL;
				bonus->subtype = spec.subtype; //spell i
				hs->addNewBonus(bonus);
				break;
			case 8://peculiar spells - enchantments
				bonus->type = Bonus::SPECIAL_PECULIAR_ENCHANT;
				bonus->subtype = spec.subtype; //spell id
				bonus->additionalInfo = spec.additionalinfo;//0, 1 for Coronius
				hs->addNewBonus(bonus);
				break;
			case 9://upgrade creatures
			{
				const auto &creatures = VLC->creh->creatures;
				bonus->type = Bonus::SPECIAL_UPGRADE;
				bonus->subtype = spec.subtype; //base id
				bonus->additionalInfo = spec.additionalinfo; //target id
				hs->addNewBonus(bonus);
				bonus = new Bonus(*bonus);

				for(auto cre_id : creatures[spec.subtype]->upgrades)
				{
					bonus->subtype = cre_id; //propagate for regular upgrades of base creature
					hs->addNewBonus(bonus);
					bonus = new Bonus(*bonus);
				}
				vstd::clear_pointer(bonus);
				break;
			}
			case 10://resource generation
				bonus->type = Bonus::GENERATE_RESOURCE;
				bonus->subtype = spec.subtype;
				hs->addNewBonus(bonus);
				break;
			case 11://starting skill with mastery (Adrienne)
				cb->changeSecSkill(this, SecondarySkill(spec.val), spec.additionalinfo); //simply give it and forget
				break;
			case 12://army speed
				bonus->type = Bonus::STACKS_SPEED;
				hs->addNewBonus(bonus);
				break;
			case 13://Dragon bonuses (Mutare)
				bonus->type = Bonus::PRIMARY_SKILL;
				bonus->valType = Bonus::ADDITIVE_VALUE;
				switch (spec.subtype)
				{
					case 1:
						bonus->subtype = PrimarySkill::ATTACK;
						break;
					case 2:
						bonus->subtype = PrimarySkill::DEFENSE;
						break;
				}
				bonus->limiter.reset(new HasAnotherBonusLimiter(Bonus::DRAGON_NATURE));
				hs->addNewBonus(bonus);
				break;
			default:
                logGlobal->warnStream() << "Unexpected hero specialty " << type;
		}
	}
	specialty.push_back(hs); //will it work?

	for (auto hs2 : type->specialty) //copy active (probably growing) bonuses from hero prootype to hero object
	{
		auto  hs = new HeroSpecial();
		attachTo(hs); //do we ever need to detach it?

		hs->setNodeType(CBonusSystemNode::specialty);
		for (auto bonus : hs2.bonuses)
		{
			hs->addNewBonus (bonus);
		}
		hs->growsWithLevel = hs2.growsWithLevel;

		specialty.push_back(hs); //will it work?
	}

	//initialize bonuses
	for(auto skill_info : secSkills)
		updateSkill(SecondarySkill(skill_info.first), skill_info.second);
	Updatespecialty();

	mana = manaLimit(); //after all bonuses are taken into account, make sure this line is the last one
	type->name = name;
}
void CGHeroInstance::Updatespecialty() //TODO: calculate special value of bonuses on-the-fly?
{
	for (auto hs : specialty)
	{
		if (hs->growsWithLevel)
		{
			//const auto &creatures = VLC->creh->creatures;

			for(Bonus * b : hs->getBonusList())
			{
				switch (b->type)
				{
					case Bonus::SECONDARY_SKILL_PREMY:
						b->val = (hs->valOfBonuses(Bonus::SPECIAL_SECONDARY_SKILL, b->subtype) * level);
						break; //use only hero skills as bonuses to avoid feedback loop
					case Bonus::PRIMARY_SKILL: //for creatures, that is
					{
						const CCreature * cre = nullptr;
						int creLevel = 0;
						if (auto creatureLimiter = std::dynamic_pointer_cast<CCreatureTypeLimiter>(b->limiter)) //TODO: more general eveluation of bonuses?
						{
							cre = creatureLimiter->creature;
							creLevel = cre->level;
							if (!creLevel)
							{
								creLevel = 5; //treat ballista as tier 5
							}
						}
						else //no creature found, can't calculate value
						{
                            logGlobal->warnStream() << "Primary skill specialty growth supported only with creature type limiters";
							break;
						}

						double primSkillModifier = (int)(level / creLevel) / 20.0;
						int param;
						switch (b->subtype)
						{
							case PrimarySkill::ATTACK:
								param = cre->Attack();
								break;
							case PrimarySkill::DEFENSE:
								param = cre->Defense();
								break;
							default:
								continue;
						}
						b->val = ceil(param * (1 + primSkillModifier)) - param; //yep, overcomplicated but matches original
						break;
					}
				}
			}
		}
	}
}
void CGHeroInstance::updateSkill(SecondarySkill which, int val)
{
	if(which == SecondarySkill::LEADERSHIP || which == SecondarySkill::LUCK)
	{ //luck-> VLC->generaltexth->arraytxt[73+luckSkill]; VLC->generaltexth->arraytxt[104+moraleSkill]
		bool luck = which == SecondarySkill::LUCK;
		Bonus::BonusType type[] = {Bonus::MORALE, Bonus::LUCK};

		Bonus *b = getBonusLocalFirst(Selector::type(type[luck]).And(Selector::sourceType(Bonus::SECONDARY_SKILL)));
		if(!b)
		{
			b = new Bonus(Bonus::PERMANENT, type[luck], Bonus::SECONDARY_SKILL, +val, which, which, Bonus::BASE_NUMBER);
			addNewBonus(b);
		}
		else
			b->val = +val;
	}
	else if(which == SecondarySkill::DIPLOMACY) //surrender discount: 20% per level
	{

		if(Bonus *b = getBonusLocalFirst(Selector::type(Bonus::SURRENDER_DISCOUNT).And(Selector::sourceType(Bonus::SECONDARY_SKILL))))
			b->val = +val;
		else
			addNewBonus(new Bonus(Bonus::PERMANENT, Bonus::SURRENDER_DISCOUNT, Bonus::SECONDARY_SKILL, val * 20, which));
	}

	int skillVal = 0;
	switch (which)
	{
	case SecondarySkill::ARCHERY:
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
	case SecondarySkill::LOGISTICS:
		skillVal = 10 * val; break;
	case SecondarySkill::NAVIGATION:
		skillVal = 50 * val; break;
	case SecondarySkill::MYSTICISM:
		skillVal = val; break;
	case SecondarySkill::EAGLE_EYE:
		skillVal = 30 + 10 * val; break;
	case SecondarySkill::NECROMANCY:
		skillVal = 10 * val; break;
	case SecondarySkill::LEARNING:
		skillVal = 5 * val; break;
	case SecondarySkill::OFFENCE:
		skillVal = 10 * val; break;
	case SecondarySkill::ARMORER:
		skillVal = 5 * val; break;
	case SecondarySkill::INTELLIGENCE:
		skillVal = 25 << (val-1); break;
	case SecondarySkill::SORCERY:
		skillVal = 5 * val; break;
	case SecondarySkill::RESISTANCE:
		skillVal = 5 << (val-1); break;
	case SecondarySkill::FIRST_AID:
		skillVal = 25 + 25*val; break;
	case SecondarySkill::ESTATES:
		skillVal = 125 << (val-1); break;
	}


	Bonus::ValueType skillValType = skillVal ? Bonus::BASE_NUMBER : Bonus::INDEPENDENT_MIN;
	if(Bonus * b = getBonusList().getFirst(Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, which)
										.And(Selector::sourceType(Bonus::SECONDARY_SKILL)))) //only local hero bonus
	{
		b->val = skillVal;
		b->valType = skillValType;
	}
	else
	{
		auto bonus = new Bonus(Bonus::PERMANENT, Bonus::SECONDARY_SKILL_PREMY, Bonus::SECONDARY_SKILL, skillVal, id.getNum(), which, skillValType);
		bonus->source = Bonus::SECONDARY_SKILL;
		addNewBonus(bonus);
	}

}
void CGHeroInstance::setPropertyDer( ui8 what, ui32 val )
{
	if(what == ObjProperty::PRIMARY_STACK_COUNT)
		setStackCount(SlotID(0), val);
}

double CGHeroInstance::getHeroStrength() const
{
	return sqrt((1.0 + 0.05*getPrimSkillLevel(PrimarySkill::ATTACK)) * (1.0 + 0.05*getPrimSkillLevel(PrimarySkill::DEFENSE)));
}

ui64 CGHeroInstance::getTotalStrength() const
{
	double ret = getHeroStrength() * getArmyStrength();
	return (ui64) ret;
}

TExpType CGHeroInstance::calculateXp(TExpType exp) const
{
	return exp * (100 + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::LEARNING))/100.0;
}

ui8 CGHeroInstance::getSpellSchoolLevel(const CSpell * spell, int *outSelectedSchool) const
{
	si16 skill = -1; //skill level

#define TRY_SCHOOL(schoolName, schoolMechanicsId, schoolOutId)	\
	if(spell-> schoolName)									\
	{															\
		int thisSchool = std::max<int>(getSecSkillLevel( \
			SecondarySkill(14 + (schoolMechanicsId))), \
			valOfBonuses(Bonus::MAGIC_SCHOOL_SKILL, 1 << (schoolMechanicsId))); \
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



	vstd::amax(skill, valOfBonuses(Bonus::MAGIC_SCHOOL_SKILL, 0)); //any school bonus
	vstd::amax(skill, valOfBonuses(Bonus::SPELL, spell->id.toEnum())); //given by artifact or other effect
	if (hasBonusOfType(Bonus::MAXED_SPELL, spell->id))//hero specialty (Daremyth, Melodia)
		skill = 3;
	assert(skill >= 0 && skill <= 3);
	return skill;
}

bool CGHeroInstance::canCastThisSpell(const CSpell * spell) const
{
	if(!getArt(ArtifactPosition::SPELLBOOK)) //if hero has no spellbook
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
CStackBasicDescriptor CGHeroInstance::calculateNecromancy (const BattleResult &battleResult) const
{
	const ui8 necromancyLevel = getSecSkillLevel(SecondarySkill::NECROMANCY);

	// Hero knows necromancy or has Necromancer Cloak
	if (necromancyLevel > 0 || hasBonusOfType(Bonus::IMPROVED_NECROMANCY))
	{
		double necromancySkill = valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::NECROMANCY)/100.0;
		vstd::amin(necromancySkill, 1.0); //it's impossible to raise more creatures than all...
		const std::map<ui32,si32> &casualties = battleResult.casualties[!battleResult.winner];
		ui32 raisedUnits = 0;

		// Figure out what to raise and how many.
		const CreatureID creatureTypes[] = {CreatureID::SKELETON, CreatureID::WALKING_DEAD, CreatureID::WIGHTS, CreatureID::LICHES};
		const bool improvedNecromancy = hasBonusOfType(Bonus::IMPROVED_NECROMANCY);
		const CCreature *raisedUnitType = VLC->creh->creatures[creatureTypes[improvedNecromancy ? necromancyLevel : 0]];
		const ui32 raisedUnitHP = raisedUnitType->valOfBonuses(Bonus::STACK_HEALTH);

		//calculate creatures raised from each defeated stack
		for (auto & casualtie : casualties)
		{
			// Get lost enemy hit points convertible to units.
			CCreature * c = VLC->creh->creatures[casualtie.first];

			const ui32 raisedHP = c->valOfBonuses(Bonus::STACK_HEALTH) * casualtie.second * necromancySkill;
			raisedUnits += std::min<ui32>(raisedHP / raisedUnitHP, casualtie.second * necromancySkill); //limit to % of HP and % of original stack count
		}

		// Make room for new units.
		SlotID slot = getSlotFor(raisedUnitType->idNumber);
		if (slot == SlotID())
		{
			// If there's no room for unit, try it's upgraded version 2/3rds the size.
			raisedUnitType = VLC->creh->creatures[*raisedUnitType->upgrades.begin()];
			raisedUnits = (raisedUnits*2)/3;

			slot = getSlotFor(raisedUnitType->idNumber);
		}
		if (raisedUnits <= 0)
			raisedUnits = 1;

		return CStackBasicDescriptor(raisedUnitType->idNumber, raisedUnits);
	}

	return CStackBasicDescriptor();
}

/**
 * Show the necromancy dialog with information about units raised.
 * @param raisedStack Pair where the first element represents ID of the raised creature
 * and the second element the amount.
 */
void CGHeroInstance::showNecromancyDialog(const CStackBasicDescriptor &raisedStack) const
{
	InfoWindow iw;
	iw.soundID = soundBase::pickup01 + ran() % 7;
	iw.player = tempOwner;
	iw.components.push_back(Component(raisedStack));

	if (raisedStack.count > 1) // Practicing the dark arts of necromancy, ... (plural)
	{
		iw.text.addTxt(MetaString::GENERAL_TXT, 145);
		iw.text.addReplacement(raisedStack.count);
	}
	else // Practicing the dark arts of necromancy, ... (singular)
	{
		iw.text.addTxt(MetaString::GENERAL_TXT, 146);
	}
	iw.text.addReplacement(raisedStack);

	cb->showInfoDialog(&iw);
}

int3 CGHeroInstance::getSightCenter() const
{
	return getPosition(false);
}

int CGHeroInstance::getSightRadious() const
{
	return 5 + getSecSkillLevel(SecondarySkill::SCOUTING) + valOfBonuses(Bonus::SIGHT_RADIOUS); //default + scouting
}

si32 CGHeroInstance::manaRegain() const
{
	if (hasBonusOfType(Bonus::FULL_MANA_REGENERATION))
		return manaLimit();

	return 1 + valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, 8) + valOfBonuses(Bonus::MANA_REGENERATION); //1 + Mysticism level
}

// /**
//  * Places an artifact in hero's backpack. If it's a big artifact equips it
//  * or discards it if it cannot be equipped.
//  */
// void CGHeroInstance::giveArtifact (ui32 aid) //use only for fixed artifacts
// {
// 	CArtifact * const artifact = VLC->arth->artifacts[aid]; //pointer to constant object
// 	CArtifactInstance *ai = CArtifactInstance::createNewArtifactInstance(artifact);
// 	ai->putAt(this, ai->firstAvailableSlot(this));
// }

int CGHeroInstance::getBoatType() const
{
	switch(type->heroClass->getAlignment())
	{
	case EAlignment::GOOD:
		return 1;
	case EAlignment::EVIL:
		return 0;
	case EAlignment::NEUTRAL:
		return 2;
	default:
		throw std::runtime_error("Wrong alignment!");
	}
}

void CGHeroInstance::getOutOffsets(std::vector<int3> &offsets) const
{
	static int3 dirs[] = { int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0), int3(1,1,0),int3(-1,1,0),int3(1,-1,0),int3(-1,-1,0) };
	for (auto & dir : dirs)
		offsets += dir;
}

int CGHeroInstance::getSpellCost(const CSpell *sp) const
{
	return sp->costs[getSpellSchoolLevel(sp)];
}

void CGHeroInstance::pushPrimSkill( PrimarySkill::PrimarySkill which, int val )
{
	addNewBonus(new Bonus(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::HERO_BASE_SKILL, val, id.getNum(), which));
}

EAlignment::EAlignment CGHeroInstance::getAlignment() const
{
	return type->heroClass->getAlignment();
}

void CGHeroInstance::initExp()
{
	exp=40+  (ran())  % 50;
	level = 1;
}

std::string CGHeroInstance::nodeName() const
{
	return "Hero " + name;
}

void CGHeroInstance::putArtifact(ArtifactPosition pos, CArtifactInstance *art)
{
	assert(!getArt(pos));
	art->putAt(ArtifactLocation(this, pos));
}

void CGHeroInstance::putInBackpack(CArtifactInstance *art)
{
	putArtifact(art->firstBackpackSlot(this), art);
}

bool CGHeroInstance::hasSpellbook() const
{
	return getArt(ArtifactPosition::SPELLBOOK);
}

void CGHeroInstance::deserializationFix()
{
	artDeserializationFix(this);

	//for (auto hs : specialty)
	//{
	//	attachTo (hs);
	//}
}

CBonusSystemNode * CGHeroInstance::whereShouldBeAttached(CGameState *gs)
{
	if(visitedTown)
	{
		if(inTownGarrison)
			return visitedTown;
		else
			return &visitedTown->townAndVis;
	}
	else
		return CArmedInstance::whereShouldBeAttached(gs);
}

int CGHeroInstance::movementPointsAfterEmbark(int MPsBefore, int basicCost, bool disembark /*= false*/) const
{
	if(hasBonusOfType(Bonus::FREE_SHIP_BOARDING))
		return (MPsBefore - basicCost) * static_cast<double>(maxMovePoints(disembark)) / maxMovePoints(!disembark);

	return 0; //take all MPs otherwise
}

CGHeroInstance::ECanDig CGHeroInstance::diggingStatus() const
{
	if(movement < maxMovePoints(true))
		return LACK_OF_MOVEMENT;
    else if(cb->getTile(getPosition(false))->terType == ETerrainType::WATER)
		return WRONG_TERRAIN;
	else
	{
		const TerrainTile *t = cb->getTile(getPosition());
		//TODO look for hole
		//CGI->mh->getTerrainDescr(h->getPosition(false), hlp, false);
		if(/*hlp.length() || */t->blockingObjects.size() > 1)
			return TILE_OCCUPIED;
		else
			return CAN_DIG;
	}
}

ArtBearer::ArtBearer CGHeroInstance::bearerType() const
{
	return ArtBearer::HERO;
}

std::vector<SecondarySkill> CGHeroInstance::levelUpProposedSkills() const
{
	std::vector<SecondarySkill> skills;
	//picking sec. skills for choice
	std::set<SecondarySkill> basicAndAdv, expert, none;
	for(int i=0;i<GameConstants::SKILL_QUANTITY;i++)
		if (cb->isAllowed(2,i))
			none.insert(SecondarySkill(i));

	for(auto & elem : secSkills)
	{
		if(elem.second < 3)
			basicAndAdv.insert(elem.first);
		else
			expert.insert(elem.first);
		none.erase(elem.first);
	}

	//first offered skill
	if(basicAndAdv.size())
	{
		SecondarySkill s = type->heroClass->chooseSecSkill(basicAndAdv);//upgrade existing
		skills.push_back(s);
		basicAndAdv.erase(s);
	}
	else if(none.size() && canLearnSkill())
	{
		skills.push_back(type->heroClass->chooseSecSkill(none)); //give new skill
		none.erase(skills.back());
	}

	//second offered skill
	if(none.size() && canLearnSkill()) //hero have free skill slot
	{
		skills.push_back(type->heroClass->chooseSecSkill(none)); //new skill
	}
	else if(basicAndAdv.size())
	{
		skills.push_back(type->heroClass->chooseSecSkill(basicAndAdv)); //upgrade existing
	}

	return skills;
}

bool CGHeroInstance::gainsLevel() const
{
	return exp >= VLC->heroh->reqExp(level+1);
}

void CGDwelling::initObj()
{
	switch(ID)
	{
	case Obj::CREATURE_GENERATOR1:
		{
			CreatureID crid = VLC->objh->cregens[subID];
			const CCreature *crs = VLC->creh->creatures[crid];

			creatures.resize(1);
			creatures[0].second.push_back(crid);
			if (subID >= VLC->generaltexth->creGens.size()) //very messy workaround
			{
				auto & dwellingNames = VLC->townh->factions[crs->faction]->town->dwellingNames;
				assert (!dwellingNames.empty());
				hoverName = dwellingNames[VLC->creh->creatures[subID]->level - 1];
			}
			else
				hoverName = VLC->generaltexth->creGens[subID];
			if(crs->level > 4)
				putStack(SlotID(0), new CStackInstance(crs, (crs->growth) * 3));
			if (getOwner() != PlayerColor::NEUTRAL)
				cb->gameState()->players[getOwner()].dwellings.push_back (this);
		}
		break;

	case Obj::CREATURE_GENERATOR4:
		creatures.resize(4);
		if(subID == 1) //Golem Factory
		{
			creatures[0].second.push_back(CreatureID::STONE_GOLEM);
			creatures[1].second.push_back(CreatureID::IRON_GOLEM);
			creatures[2].second.push_back(CreatureID::GOLD_GOLEM);
			creatures[3].second.push_back(CreatureID::DIAMOND_GOLEM);
			//guards
			putStack(SlotID(0), new CStackInstance(CreatureID::GOLD_GOLEM, 9));
			putStack(SlotID(1), new CStackInstance(CreatureID::DIAMOND_GOLEM, 6));
		}
		else if(subID == 0) // Elemental Conflux
		{
			creatures[0].second.push_back(CreatureID::AIR_ELEMENTAL);
			creatures[1].second.push_back(CreatureID::FIRE_ELEMENTAL);
			creatures[2].second.push_back(CreatureID::EARTH_ELEMENTAL);
			creatures[3].second.push_back(CreatureID::WATER_ELEMENTAL);
			//guards
			putStack(SlotID(0), new CStackInstance(CreatureID::EARTH_ELEMENTAL, 12));
		}
		else
		{
			assert(0);
		}
		hoverName = VLC->generaltexth->creGens4[subID];
		break;

	case Obj::REFUGEE_CAMP:
		//is handled within newturn func
		break;

	case Obj::WAR_MACHINE_FACTORY:
		creatures.resize(3);
		creatures[0].second.push_back(CreatureID::BALLISTA);
		creatures[1].second.push_back(CreatureID::FIRST_AID_TENT);
		creatures[2].second.push_back(CreatureID::AMMO_CART);
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
			if (ID == Obj::CREATURE_GENERATOR1) //single generators
			{
				if (tempOwner != PlayerColor::NEUTRAL)
				{
					std::vector<ConstTransitivePtr<CGDwelling> >* dwellings = &cb->gameState()->players[tempOwner].dwellings;
					dwellings->erase (std::find(dwellings->begin(), dwellings->end(), this));
				}
				if (PlayerColor(val) != PlayerColor::NEUTRAL) //can new owner be neutral?
					cb->gameState()->players[PlayerColor(val)].dwellings.push_back (this);
			}
			break;
		case ObjProperty::AVAILABLE_CREATURE:
			creatures.resize(1);
			creatures[0].second.resize(1);
			creatures[0].second[0] = CreatureID(val);
			break;
	}
	CGObjectInstance::setProperty(what,val);
}
void CGDwelling::onHeroVisit( const CGHeroInstance * h ) const
{
	if(ID == Obj::REFUGEE_CAMP && !creatures[0].first) //Refugee Camp, no available cres
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		iw.text.addTxt(MetaString::ADVOB_TXT, 44); //{%s} \n\n The camp is deserted.  Perhaps you should try next week.
		iw.text.addReplacement(MetaString::OBJ_NAMES, ID);
		cb->sendAndApply(&iw);
		return;
	}

	PlayerRelations::PlayerRelations relations = cb->gameState()->getPlayerRelations( h->tempOwner, tempOwner );

	if ( relations == PlayerRelations::ALLIES )
		return;//do not allow recruiting or capturing

	if( !relations  &&  stacksCount() > 0) //object is guarded, owned by enemy
	{
		BlockingDialog bd(true,false);
		bd.player = h->tempOwner;
		bd.text.addTxt(MetaString::GENERAL_TXT, 421); //Much to your dismay, the %s is guarded by %s %s. Do you wish to fight the guards?
		bd.text.addReplacement(ID == Obj::CREATURE_GENERATOR1 ? MetaString::CREGENS : MetaString::CREGENS4, subID);
		bd.text.addReplacement(MetaString::ARRAY_TXT, 176 + Slots().begin()->second->getQuantityID()*3);
		bd.text.addReplacement(*Slots().begin()->second);
		cb->showBlockingDialog(&bd);
		return;
	}

	if(!relations  &&  ID != Obj::WAR_MACHINE_FACTORY)
	{
		cb->setOwner(this, h->tempOwner);
	}

	BlockingDialog bd (true,false);
	bd.player = h->tempOwner;
	if(ID == Obj::CREATURE_GENERATOR1 || ID == Obj::CREATURE_GENERATOR4)
	{
		bd.text.addTxt(MetaString::ADVOB_TXT, ID == Obj::CREATURE_GENERATOR1 ? 35 : 36); //{%s} Would you like to recruit %s? / {%s} Would you like to recruit %s, %s, %s, or %s?
		bd.text.addReplacement(ID == Obj::CREATURE_GENERATOR1 ? MetaString::CREGENS : MetaString::CREGENS4, subID);
		for(auto & elem : creatures)
			bd.text.addReplacement(MetaString::CRE_PL_NAMES, elem.second[0]);
	}
	else if(ID == Obj::REFUGEE_CAMP)
	{
		bd.text.addTxt(MetaString::ADVOB_TXT, 35); //{%s} Would you like to recruit %s?
		bd.text.addReplacement(MetaString::OBJ_NAMES, ID);
		for(auto & elem : creatures)
			bd.text.addReplacement(MetaString::CRE_PL_NAMES, elem.second[0]);
	}
	else if(ID == Obj::WAR_MACHINE_FACTORY)
		bd.text.addTxt(MetaString::ADVOB_TXT, 157); //{War Machine Factory} Would you like to purchase War Machines?
	else
		throw std::runtime_error("Illegal dwelling!");

	cb->showBlockingDialog(&bd);
}

void CGDwelling::newTurn() const
{
	if(cb->getDate(Date::DAY_OF_WEEK) != 1) //not first day of week
		return;

	//town growths and War Machines Factories are handled separately
	if(ID == Obj::TOWN  ||  ID == Obj::WAR_MACHINE_FACTORY)
		return;

	if(ID == Obj::REFUGEE_CAMP) //if it's a refugee camp, we need to pick an available creature
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
			if (VLC->modh->settings.DWELLINGS_ACCUMULATE_CREATURES && ID != Obj::REFUGEE_CAMP) //camp should not try to accumulate different kinds of creatures
				sac.creatures[i].first += amount;
			else
				sac.creatures[i].first = amount;
			change = true;
		}
	}

	if(change)
		cb->sendAndApply(&sac);
}

void CGDwelling::heroAcceptsCreatures( const CGHeroInstance *h) const
{
	CreatureID crid = creatures[0].second[0];
	CCreature *crs = VLC->creh->creatures[crid];
	TQuantity count = creatures[0].first;

	if(crs->level == 1  &&  ID != Obj::REFUGEE_CAMP) //first level - creatures are for free
	{
		if(count) //there are available creatures
		{
			SlotID slot = h->getSlotFor(crid);
			if(!slot.validSlot()) //no available slot
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


				InfoWindow iw;
				iw.player = h->tempOwner;
				iw.text.addTxt(MetaString::GENERAL_TXT, 423); //%d %s join your army.
				iw.text.addReplacement(count);
				iw.text.addReplacement(MetaString::CRE_PL_NAMES, crid);

				cb->showInfoDialog(&iw);
				cb->sendAndApply(&sac);
				cb->addToSlot(StackLocation(h, slot), crs, count);
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
		if(ID == Obj::WAR_MACHINE_FACTORY) //pick available War Machines
		{
			//there is 1 war machine available to recruit if hero doesn't have one
			SetAvailableCreatures sac;
			sac.tid = id;
			sac.creatures = creatures;
			sac.creatures[0].first = !h->getArt(ArtifactPosition::MACH1); //ballista
			sac.creatures[1].first = !h->getArt(ArtifactPosition::MACH3); //first aid tent
			sac.creatures[2].first = !h->getArt(ArtifactPosition::MACH2); //ammo cart
			cb->sendAndApply(&sac);
		}

		OpenWindow ow;
		ow.id1 = id.getNum();
		ow.id2 = h->id.getNum();
		ow.window = (ID == Obj::CREATURE_GENERATOR1 || ID == Obj::REFUGEE_CAMP)
			? OpenWindow::RECRUITMENT_FIRST
			: OpenWindow::RECRUITMENT_ALL;
		cb->sendAndApply(&ow);
	}
}

void CGDwelling::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const
{
	if (result.winner == 0)
	{
		onHeroVisit(hero);
	}
}

void CGDwelling::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const
{
	auto relations = cb->getPlayerRelations(getOwner(), hero->getOwner());
	if(stacksCount() > 0  && relations == PlayerRelations::ENEMIES) //guards present
	{
		if(answer)
			cb->startBattleI(hero, this);
	}
	else if(answer)
	{
		heroAcceptsCreatures(hero);
	}
}

int CGTownInstance::getSightRadious() const //returns sight distance
{
	if (subID == ETownType::TOWER)
	{
		if (hasBuilt(BuildingID::GRAIL)) //skyship
			return -1; //entire map
		if (hasBuilt(BuildingID::LOOKOUT_TOWER)) //lookout tower
			return 20;
	}
	return 5;
}

void CGTownInstance::setPropertyDer(ui8 what, ui32 val)
{
///this is freakin' overcomplicated solution
	switch (what)
	{
	case ObjProperty::STRUCTURE_ADD_VISITING_HERO:
			bonusingBuildings[val]->setProperty (ObjProperty::VISITORS, visitingHero->id.getNum());
			break;
		case ObjProperty::STRUCTURE_CLEAR_VISITORS:
			bonusingBuildings[val]->setProperty (ObjProperty::STRUCTURE_CLEAR_VISITORS, 0);
			break;
		case ObjProperty::STRUCTURE_ADD_GARRISONED_HERO: //add garrisoned hero to visitors
			bonusingBuildings[val]->setProperty (ObjProperty::VISITORS, garrisonHero->id.getNum());
			break;
		case ObjProperty::BONUS_VALUE_FIRST:
			bonusValue.first = val;
			break;
		case ObjProperty::BONUS_VALUE_SECOND:
			bonusValue.second = val;
			break;
	}
}
CGTownInstance::EFortLevel CGTownInstance::fortLevel() const //0 - none, 1 - fort, 2 - citadel, 3 - castle
{
	if (hasBuilt(BuildingID::CASTLE))
		return CASTLE;
	if (hasBuilt(BuildingID::CITADEL))
		return CITADEL;
	if (hasBuilt(BuildingID::FORT))
		return FORT;
	return NONE;
}

int CGTownInstance::hallLevel() const // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
{
	if (hasBuilt(BuildingID::CAPITOL))
		return 3;
	if (hasBuilt(BuildingID::CITY_HALL))
		return 2;
	if (hasBuilt(BuildingID::TOWN_HALL))
		return 1;
	if (hasBuilt(BuildingID::VILLAGE_HALL))
		return 0;
	return -1;
}
int CGTownInstance::mageGuildLevel() const
{
	if (hasBuilt(BuildingID::MAGES_GUILD_5))
		return 5;
	if (hasBuilt(BuildingID::MAGES_GUILD_4))
		return 4;
	if (hasBuilt(BuildingID::MAGES_GUILD_3))
		return 3;
	if (hasBuilt(BuildingID::MAGES_GUILD_2))
		return 2;
	if (hasBuilt(BuildingID::MAGES_GUILD_1))
		return 1;
	return 0;
}

int CGTownInstance::creatureDwellingLevel(int dwelling) const
{
	if ( dwelling<0 || dwelling >= GameConstants::CREATURES_PER_TOWN )
		return -1;
	for (int i=0; ; i++)
	{
		if (!hasBuilt(BuildingID(BuildingID::DWELL_FIRST+dwelling+i*GameConstants::CREATURES_PER_TOWN)))
			return i-1;
	}
}
int CGTownInstance::getHordeLevel(const int & HID)  const//HID - 0 or 1; returns creature level or -1 if that horde structure is not present
{
	return town->hordeLvl[HID];
}
int CGTownInstance::creatureGrowth(const int & level) const
{
	return getGrowthInfo(level).totalGrowth();
}

GrowthInfo CGTownInstance::getGrowthInfo(int level) const
{
	GrowthInfo ret;

	if (level<0 || level >=GameConstants::CREATURES_PER_TOWN)
		return ret;
	if (!hasBuilt(BuildingID(BuildingID::DWELL_FIRST+level)))
		return ret; //no dwelling

	const CCreature *creature = VLC->creh->creatures[creatures[level].second.back()];
	const int base = creature->growth;
	int castleBonus = 0;

	ret.entries.push_back(GrowthInfo::Entry(VLC->generaltexth->allTexts[590], base));// \n\nBasic growth %d"

	if (hasBuilt(BuildingID::CASTLE))
		ret.entries.push_back(GrowthInfo::Entry(subID, BuildingID::CASTLE, castleBonus = base));
	else if (hasBuilt(BuildingID::CITADEL))
		ret.entries.push_back(GrowthInfo::Entry(subID, BuildingID::CITADEL, castleBonus = base / 2));

	if(town->hordeLvl[0] == level)//horde 1
		if(hasBuilt(BuildingID::HORDE_1))
			ret.entries.push_back(GrowthInfo::Entry(subID, BuildingID::HORDE_1, creature->hordeGrowth));

	if(town->hordeLvl[1] == level)//horde 2
		if(hasBuilt(BuildingID::HORDE_2))
			ret.entries.push_back(GrowthInfo::Entry(subID, BuildingID::HORDE_2, creature->hordeGrowth));

	int dwellingBonus = 0;
	if(const PlayerState *p = cb->getPlayer(tempOwner, false))
	{
		for(const CGDwelling *dwelling : p->dwellings)
			if(vstd::contains(creatures[level].second, dwelling->creatures[0].second[0]))
				dwellingBonus++;
	}

	if(dwellingBonus)
		ret.entries.push_back(GrowthInfo::Entry(VLC->generaltexth->allTexts[591], dwellingBonus));// \nExternal dwellings %+d

	//other *-of-legion-like bonuses (%d to growth cumulative with grail)
	TBonusListPtr bonuses = getBonuses(Selector::type(Bonus::CREATURE_GROWTH).And(Selector::subtype(level)));
	for(const Bonus *b : *bonuses)
		ret.entries.push_back(GrowthInfo::Entry(b->Description() + " %+d", b->val));

	//statue-of-legion-like bonus: % to base+castle
	TBonusListPtr bonuses2 = getBonuses(Selector::type(Bonus::CREATURE_GROWTH_PERCENT));
	for(const Bonus *b : *bonuses2)
		ret.entries.push_back(GrowthInfo::Entry(b->Description() + " %+d", b->val * (base + castleBonus) / 100));

	if(hasBuilt(BuildingID::GRAIL)) //grail - +50% to ALL (so far added) growth
		ret.entries.push_back(GrowthInfo::Entry(subID, BuildingID::GRAIL, ret.totalGrowth() / 2));

	return ret;
}

int CGTownInstance::dailyIncome() const
{
	int ret = 0;
	if (hasBuilt(BuildingID::GRAIL))
		ret+=5000;

	if (hasBuilt(BuildingID::CAPITOL))
		ret+=4000;
	else if (hasBuilt(BuildingID::CITY_HALL))
		ret+=2000;
	else if (hasBuilt(BuildingID::TOWN_HALL))
		ret+=1000;
	else if (hasBuilt(BuildingID::VILLAGE_HALL))
		ret+=500;
	return ret;
}
bool CGTownInstance::hasFort() const
{
	return hasBuilt(BuildingID::FORT);
}
bool CGTownInstance::hasCapitol() const
{
	return hasBuilt(BuildingID::CAPITOL);
}
CGTownInstance::CGTownInstance()
	:IShipyard(this), IMarket(this), town(nullptr), builded(0), destroyed(0), identifier(0), alignment(0xff)
{

}

CGTownInstance::~CGTownInstance()
{
	for (auto & elem : bonusingBuildings)
		delete elem;
}

int CGTownInstance::spellsAtLevel(int level, bool checkGuild) const
{
	if(checkGuild && mageGuildLevel() < level)
		return 0;
	int ret = 6 - level; //how many spells are available at this level

	if (hasBuilt(BuildingID::LIBRARY, ETownType::TOWER))
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
	if( !cb->gameState()->getPlayerRelations( getOwner(), h->getOwner() ))//if this is enemy
	{
		if(armedGarrison() || visitingHero)
		{
			const CGHeroInstance *defendingHero = nullptr;
			const CArmedInstance *defendingArmy = this;

			if(visitingHero)
				defendingHero = visitingHero;
			else if(garrisonHero)
				defendingHero = garrisonHero;

			if(defendingHero)
				defendingArmy = defendingHero;

			bool outsideTown = (defendingHero == visitingHero && garrisonHero);

			//TODO
			//"borrowing" army from garrison to visiting hero

			cb->startBattlePrimary(h, defendingArmy, getSightCenter(), h, defendingHero, false, (outsideTown ? nullptr : this));
		}
		else
		{
			cb->setOwner(this, h->tempOwner);
			removeCapitols(h->getOwner());
			cb->heroVisitCastle(this, h);
		}
	}
	else if(h->visitablePos() == visitablePos())
	{
		if (h->commander && !h->commander->alive) //rise commander. TODO: interactive script
		{
			SetCommanderProperty scp;
			scp.heroid = h->id;
			scp.which = SetCommanderProperty::ALIVE;
			scp.amount = 1;
			cb->sendAndApply (&scp);
		}
		cb->heroVisitCastle(this, h);
	}
	else
	{
		logGlobal->errorStream() << h->name << " visits allied town of " << name << " from different pos?";
	}
}

void CGTownInstance::onHeroLeave(const CGHeroInstance * h) const
{
	cb->stopHeroVisitCastle(this, h);
}

void CGTownInstance::initObj()
///initialize town structures
{
	blockVisit = true;
	hoverName = name + ", " + town->faction->name;

	if (subID == ETownType::DUNGEON)
		creatures.resize(GameConstants::CREATURES_PER_TOWN+1);//extra dwelling for Dungeon
	else
		creatures.resize(GameConstants::CREATURES_PER_TOWN);
	for (int i = 0; i < GameConstants::CREATURES_PER_TOWN; i++)
	{
		int dwellingLevel = creatureDwellingLevel(i);
		int creaturesTotal = town->creatures[i].size();
		for (int j=0; j< std::min(dwellingLevel + 1, creaturesTotal); j++)
			creatures[i].second.push_back(town->creatures[i][j]);
	}

	switch (subID)
	{ //add new visitable objects
		case 0:
			bonusingBuildings.push_back (new COPWBonus(BuildingID::STABLES, this));
			break;
		case 5:
			bonusingBuildings.push_back (new COPWBonus(BuildingID::MANA_VORTEX, this));
		case 2: case 3: case 6:
			bonusingBuildings.push_back (new CTownBonus(BuildingID::SPECIAL_4, this));
			break;
		case 7:
			bonusingBuildings.push_back (new CTownBonus(BuildingID::SPECIAL_1, this));
			break;
	}
	//add special bonuses from buildings

	recreateBuildingsBonuses();
}

void CGTownInstance::newTurn() const
{
	if (cb->getDate(Date::DAY_OF_WEEK) == 1) //reset on new week
	{
		//give resources for Rampart, Mystic Pond
		if (hasBuilt(BuildingID::MYSTIC_POND, ETownType::RAMPART)
			&& cb->getDate(Date::DAY) != 1 && (tempOwner < PlayerColor::PLAYER_LIMIT))
		{
			int resID = rand()%4+2;//bonus to random rare resource
			resID = (resID==2)?1:resID;
			int resVal = rand()%4+1;//with size 1..4
			cb->giveResource(tempOwner, static_cast<Res::ERes>(resID), resVal);
			cb->setObjProperty (id, ObjProperty::BONUS_VALUE_FIRST, resID);
			cb->setObjProperty (id, ObjProperty::BONUS_VALUE_SECOND, resVal);
		}

		if ( subID == ETownType::DUNGEON )
			for (auto & elem : bonusingBuildings)
		{
			if ((elem)->ID == BuildingID::MANA_VORTEX)
				cb->setObjProperty (id, ObjProperty::STRUCTURE_CLEAR_VISITORS, (elem)->id); //reset visitors for Mana Vortex
		}

		if (tempOwner == PlayerColor::NEUTRAL) //garrison growth for neutral towns
			{
				std::vector<SlotID> nativeCrits; //slots
				for (auto & elem : Slots())
				{
					if (elem.second->type->faction == subID) //native
					{
						nativeCrits.push_back(elem.first); //collect matching slots
					}
				}
				if (nativeCrits.size())
				{
					SlotID pos = nativeCrits[rand() % nativeCrits.size()];
					StackLocation sl(this, pos);

					const CCreature *c = getCreature(pos);
					if (rand()%100 < 90 || c->upgrades.empty()) //increase number if no upgrade available
					{
						cb->changeStackCount(sl, c->growth);
					}
					else //upgrade
					{
						cb->changeStackType(sl, VLC->creh->creatures[*c->upgrades.begin()]);
					}
				}
				if ((stacksCount() < GameConstants::ARMY_SIZE && rand()%100 < 25) || Slots().empty()) //add new stack
				{
					int i = rand() % std::min (GameConstants::ARMY_SIZE, cb->getDate(Date::MONTH)<<1);
					CreatureID c = town->creatures[i][0];
					SlotID n;

					TQuantity count = creatureGrowth(i);
					if (!count) // no dwelling
						count = VLC->creh->creatures[c]->growth;

					{//no lower tiers or above current month

						if ((n = getSlotFor(c)).validSlot())
						{
							StackLocation sl(this, n);
							if (slotEmpty(n))
								cb->insertNewStack(sl, VLC->creh->creatures[c], count);
							else //add to existing
								cb->changeStackCount(sl, count);
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
	if (!armedGarrison())//empty castle - anyone can visit
		return GameConstants::ALL_PLAYERS;
	if ( tempOwner == PlayerColor::NEUTRAL )//neutral guarded - no one can visit
		return 0;

	ui8 mask = 0;
	TeamState * ts = cb->gameState()->getPlayerTeam(tempOwner);
	for(PlayerColor it : ts->players)
		mask |= 1<<it.getNum();//allies - add to possible visitors

	return mask;
}

void CGTownInstance::getOutOffsets( std::vector<int3> &offsets ) const
{
	offsets += int3(-1,2,0), int3(-3,2,0);
}

void CGTownInstance::removeCapitols (PlayerColor owner) const
{
	if (hasCapitol()) // search if there's an older capitol
	{
		PlayerState* state = cb->gameState()->getPlayer (owner); //get all towns owned by player
		for (auto i = state->towns.cbegin(); i < state->towns.cend(); ++i)
		{
			if (*i != this && (*i)->hasCapitol())
			{
				RazeStructures rs;
				rs.tid = id;
				rs.bid.insert(BuildingID::CAPITOL);
				rs.destroyed = destroyed;
				cb->sendAndApply(&rs);
				return;
			}
		}
	}
}

int CGTownInstance::getBoatType() const
{
	switch (town->faction->alignment)
	{
	case EAlignment::EVIL : return 0;
	case EAlignment::GOOD : return 1;
	case EAlignment::NEUTRAL : return 2;
	}
	assert(0);
	return -1;
}

int CGTownInstance::getMarketEfficiency() const
{
	if (!hasBuilt(BuildingID::MARKETPLACE))
		return 0;

	const PlayerState *p = cb->getPlayer(tempOwner);
	assert(p);

	int marketCount = 0;
	for(const CGTownInstance *t : p->towns)
		if(t->hasBuilt(BuildingID::MARKETPLACE))
			marketCount++;

	return marketCount;
}

bool CGTownInstance::allowsTrade(EMarketMode::EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::RESOURCE_PLAYER:
		return hasBuilt(BuildingID::MARKETPLACE);

	case EMarketMode::ARTIFACT_RESOURCE:
	case EMarketMode::RESOURCE_ARTIFACT:
		return hasBuilt(BuildingID::ARTIFACT_MERCHANT, ETownType::TOWER)
		    || hasBuilt(BuildingID::ARTIFACT_MERCHANT, ETownType::DUNGEON)
		    || hasBuilt(BuildingID::ARTIFACT_MERCHANT, ETownType::CONFLUX);

	case EMarketMode::CREATURE_RESOURCE:
		return hasBuilt(BuildingID::FREELANCERS_GUILD, ETownType::STRONGHOLD);

	case EMarketMode::CREATURE_UNDEAD:
		return hasBuilt(BuildingID::SKELETON_TRANSFORMER, ETownType::NECROPOLIS);

	case EMarketMode::RESOURCE_SKILL:
		return hasBuilt(BuildingID::MAGIC_UNIVERSITY, ETownType::CONFLUX);
	default:
		assert(0);
		return false;
	}
}

std::vector<int> CGTownInstance::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	if(mode == EMarketMode::RESOURCE_ARTIFACT)
	{
		std::vector<int> ret;
		for(const CArtifact *a : merchantArtifacts)
			if(a)
				ret.push_back(a->id);
			else
				ret.push_back(-1);
		return ret;
	}
	else if ( mode == EMarketMode::RESOURCE_SKILL )
	{
		return universitySkills;
	}
	else
		return IMarket::availableItemsIds(mode);
}

std::string CGTownInstance::nodeName() const
{
	return "Town (" + (town ? town->faction->name : "unknown") + ") of " +  name;
}

void CGTownInstance::deserializationFix()
{
	attachTo(&townAndVis);

	//Hero is already handled by CGameState::attachArmedObjects

// 	if(visitingHero)
// 		visitingHero->attachTo(&townAndVis);
// 	if(garrisonHero)
// 		garrisonHero->attachTo(this);
}

void CGTownInstance::updateMoraleBonusFromArmy()
{
	Bonus *b = getBonusList().getFirst(Selector::sourceType(Bonus::ARMY).And(Selector::type(Bonus::MORALE)));
	if(!b)
	{
		b = new Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, 0, -1);
		addNewBonus(b);
	}

	if (garrisonHero)
		b->val = 0;
	else
		CArmedInstance::updateMoraleBonusFromArmy();
}

void CGTownInstance::recreateBuildingsBonuses()
{
	static TPropagatorPtr playerProp(new CPropagatorNodeType(PLAYER));

	BonusList bl;
	getExportedBonusList().getBonuses(bl, Selector::sourceType(Bonus::TOWN_STRUCTURE));
	for(Bonus *b : bl)
		removeBonus(b);

	//tricky! -> checks tavern only if no bratherhood of sword or not a castle
	if(subID != ETownType::CASTLE || !addBonusIfBuilt(BuildingID::BROTHERHOOD, Bonus::MORALE, +2))
		addBonusIfBuilt(BuildingID::TAVERN, Bonus::MORALE, +1);

	if(subID == ETownType::CASTLE) //castle
	{
		addBonusIfBuilt(BuildingID::LIGHTHOUSE, Bonus::SEA_MOVEMENT, +500, playerProp);
		addBonusIfBuilt(BuildingID::GRAIL,      Bonus::MORALE, +2, playerProp); //colossus
	}
	else if(subID == ETownType::RAMPART) //rampart
	{
		addBonusIfBuilt(BuildingID::FOUNTAIN_OF_FORTUNE, Bonus::LUCK, +2); //fountain of fortune
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::LUCK, +2, playerProp); //guardian spirit
	}
	else if(subID == ETownType::TOWER) //tower
	{
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::PRIMARY_SKILL, +15, PrimarySkill::KNOWLEDGE); //grail
	}
	else if(subID == ETownType::INFERNO) //Inferno
	{
		addBonusIfBuilt(BuildingID::STORMCLOUDS, Bonus::PRIMARY_SKILL, +2, PrimarySkill::SPELL_POWER); //Brimstone Clouds
	}
	else if(subID == ETownType::NECROPOLIS) //necropolis
	{
		addBonusIfBuilt(BuildingID::COVER_OF_DARKNESS,    Bonus::DARKNESS, +20);
		addBonusIfBuilt(BuildingID::NECROMANCY_AMPLIFIER, Bonus::SECONDARY_SKILL_PREMY, +10, playerProp, SecondarySkill::NECROMANCY); //necromancy amplifier
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::SECONDARY_SKILL_PREMY, +20, playerProp, SecondarySkill::NECROMANCY); //Soul prison
	}
	else if(subID == ETownType::DUNGEON) //Dungeon
	{
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::PRIMARY_SKILL, +12, PrimarySkill::SPELL_POWER); //grail
	}
	else if(subID == ETownType::STRONGHOLD) //Stronghold
	{
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::PRIMARY_SKILL, +20, PrimarySkill::ATTACK); //grail
	}
	else if(subID == ETownType::FORTRESS) //Fortress
	{
		addBonusIfBuilt(BuildingID::GLYPHS_OF_FEAR, Bonus::PRIMARY_SKILL, +2, PrimarySkill::DEFENSE); //Glyphs of Fear
		addBonusIfBuilt(BuildingID::BLOOD_OBELISK,  Bonus::PRIMARY_SKILL, +2, PrimarySkill::ATTACK); //Blood Obelisk
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::PRIMARY_SKILL, +10, PrimarySkill::ATTACK); //grail
		addBonusIfBuilt(BuildingID::GRAIL, Bonus::PRIMARY_SKILL, +10, PrimarySkill::DEFENSE); //grail
	}
	else if(subID == ETownType::CONFLUX)
	{

	}
}

bool CGTownInstance::addBonusIfBuilt(BuildingID building, Bonus::BonusType type, int val, int subtype /*= -1*/)
{
	static auto emptyPropagator = TPropagatorPtr();
	return addBonusIfBuilt(building, type, val, emptyPropagator, subtype);
}

bool CGTownInstance::addBonusIfBuilt(BuildingID building, Bonus::BonusType type, int val, TPropagatorPtr & prop, int subtype /*= -1*/)
{
	if(hasBuilt(building))
	{
		std::ostringstream descr;
		descr << town->buildings[building]->Name() << " ";
		if(val > 0)
			descr << "+";
		else if(val < 0)
			descr << "-";
		descr << val;

		Bonus *b = new Bonus(Bonus::PERMANENT, type, Bonus::TOWN_STRUCTURE, val, building, descr.str(), subtype);
		if(prop)
			b->addPropagator(prop);
		addNewBonus(b);
		return true;
	}

	return false;
}

void CGTownInstance::setVisitingHero(CGHeroInstance *h)
{
	assert(!!visitingHero == !h);
	if(h)
	{
		PlayerState *p = cb->gameState()->getPlayer(h->tempOwner);
		assert(p);
		h->detachFrom(p);
		h->attachTo(&townAndVis);
		visitingHero = h;
		h->visitedTown = this;
		h->inTownGarrison = false;
	}
	else
	{
		PlayerState *p = cb->gameState()->getPlayer(visitingHero->tempOwner);
		visitingHero->visitedTown = nullptr;
		visitingHero->detachFrom(&townAndVis);
		visitingHero->attachTo(p);
		visitingHero = nullptr;
	}
}

void CGTownInstance::setGarrisonedHero(CGHeroInstance *h)
{
	assert(!!garrisonHero == !h);
	if(h)
	{
		PlayerState *p = cb->gameState()->getPlayer(h->tempOwner);
		assert(p);
		h->detachFrom(p);
		h->attachTo(this);
		garrisonHero = h;
		h->visitedTown = this;
		h->inTownGarrison = true;
	}
	else
	{
		PlayerState *p = cb->gameState()->getPlayer(garrisonHero->tempOwner);
		garrisonHero->visitedTown = nullptr;
		garrisonHero->inTownGarrison = false;
		garrisonHero->detachFrom(this);
		garrisonHero->attachTo(p);
		garrisonHero = nullptr;
	}
	updateMoraleBonusFromArmy(); //avoid giving morale bonus for same army twice
}

bool CGTownInstance::armedGarrison() const
{
	return stacksCount() || garrisonHero;
}

int CGTownInstance::getTownLevel() const
{
	// count all buildings that are not upgrades
	return boost::range::count_if(builtBuildings, [&](const BuildingID & build)
	{
		return town->buildings[build] && town->buildings[build]->upgrade == -1;
	});
}

CBonusSystemNode * CGTownInstance::whatShouldBeAttached()
{
	return &townAndVis;
}

const CArmedInstance * CGTownInstance::getUpperArmy() const
{
	if(garrisonHero)
		return garrisonHero;
	return this;
}

bool CGTownInstance::hasBuilt(BuildingID buildingID, int townID) const
{
	if (townID == town->faction->index || townID == ETownType::ANY)
		return hasBuilt(buildingID);
	return false;
}

bool CGTownInstance::hasBuilt(BuildingID buildingID) const
{
	return vstd::contains(builtBuildings, buildingID);
}

void CGTownInstance::addHeroToStructureVisitors( const CGHeroInstance *h, si32 structureInstanceID ) const
{
	if(visitingHero == h)
		cb->setObjProperty(id, ObjProperty::STRUCTURE_ADD_VISITING_HERO, structureInstanceID); //add to visitors
	else if(garrisonHero == h)
		cb->setObjProperty(id, ObjProperty::STRUCTURE_ADD_GARRISONED_HERO, structureInstanceID); //then it must be garrisoned hero
	else
	{
		//should never ever happen
        logGlobal->errorStream() << "Cannot add hero " << h->name << " to visitors of structure #" << structureInstanceID;
		assert(0);
	}
}

void CGTownInstance::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const 
{
	if(result.winner == 0)
	{
		removeCapitols(hero->getOwner());
		cb->setOwner (this, hero->tempOwner); //give control after checkout is done
		FoWChange fw;
		fw.player = hero->tempOwner;
		fw.mode = 1;
		getSightTiles (fw.tiles); //update visibility for castle structures
		cb->sendAndApply (&fw);
	}
}


bool CGVisitableOPH::wasVisited (const CGHeroInstance * h) const
{
	return vstd::contains(visitors, h->id);
}

void CGVisitableOPH::onHeroVisit( const CGHeroInstance * h ) const
{
	if(!vstd::contains(visitors, h->id))
	{
		onNAHeroVisit (h, false);
		switch(ID)
		{
		case Obj::TREE_OF_KNOWLEDGE:
		case Obj::ARENA:
		case Obj::LIBRARY_OF_ENLIGHTENMENT:
		case Obj::SCHOOL_OF_MAGIC:
		case Obj::SCHOOL_OF_WAR:
			break;
		default:
			cb->setObjProperty(id, ObjProperty::VISITORS, h->id.getNum()); //add to the visitors
			break;
		}
	}
	else
	{
		onNAHeroVisit(h, true);
	}
}

void CGVisitableOPH::initObj()
{
	if(ID==Obj::TREE_OF_KNOWLEDGE)
	{
		switch (ran() % 3)
		{
		case 1:
			treePrice[Res::GOLD] = 2000;
			break;
		case 2:
			treePrice[Res::GEMS] = 10;
			break;
		default:
			break;
		}
	}
}

void CGVisitableOPH::treeSelected (const CGHeroInstance * h, ui32 result) const
{
	if(result) //player agreed to give res for exp
	{
		si64 expToGive = VLC->heroh->reqExp(h->level+1) - VLC->heroh->reqExp(h->level);;
		cb->giveResources (h->getOwner(), -treePrice);
		cb->changePrimSkill (h, PrimarySkill::EXPERIENCE, expToGive);
		cb->setObjProperty (id, ObjProperty::VISITORS, h->id.getNum()); //add to the visitors
	}
}
void CGVisitableOPH::onNAHeroVisit (const CGHeroInstance * h, bool alreadyVisited) const
{
	Component::EComponentType c_id = Component::PRIM_SKILL; //most used here
	int subid=0, ot=0, sound = 0;
	TExpType val=1;
	switch(ID)
	{
	case Obj::ARENA:
		sound = soundBase::NOMAD;
		ot = 0;
		break;
	case Obj::MERCENARY_CAMP:
		sound = soundBase::NOMAD;
		subid=PrimarySkill::ATTACK;
		ot=80;
		break;
	case Obj::MARLETTO_TOWER:
		sound = soundBase::NOMAD;
		subid=PrimarySkill::DEFENSE;
		ot=39;
		break;
	case Obj::STAR_AXIS:
		sound = soundBase::gazebo;
		subid=PrimarySkill::SPELL_POWER;
		ot=100;
		break;
	case Obj::GARDEN_OF_REVELATION:
		sound = soundBase::GETPROTECTION;
		subid=PrimarySkill::KNOWLEDGE;
		ot=59;
		break;
	case Obj::LEARNING_STONE:
		sound = soundBase::gazebo;
		c_id=Component::EXPERIENCE;
		ot=143;
		val=1000;
		break;
	case Obj::TREE_OF_KNOWLEDGE:
		sound = soundBase::gazebo;
		c_id = Component::EXPERIENCE;
		subid = 1;
		ot = 147;
		val = 1;
		break;
	case Obj::LIBRARY_OF_ENLIGHTENMENT:
		sound = soundBase::gazebo;
		ot = 66;
		break;
	case Obj::SCHOOL_OF_MAGIC:
		sound = soundBase::faerie;
		ot = 71;
		break;
	case Obj::SCHOOL_OF_WAR:
		c_id=Component::PRIM_SKILL;
		sound = soundBase::MILITARY;
		ot = 158;
		break;
	}
	if (!alreadyVisited)
	{
		switch (ID)
		{
		case Obj::ARENA:
			{
				BlockingDialog sd(false,true);
				sd.soundID = sound;
				sd.text.addTxt(MetaString::ADVOB_TXT,ot);
				sd.components.push_back(Component(c_id, PrimarySkill::ATTACK, 2, 0));
				sd.components.push_back(Component(c_id, PrimarySkill::DEFENSE, 2, 0));
				sd.player = h->getOwner();
				cb->showBlockingDialog(&sd);
				return;
			}
		case Obj::MERCENARY_CAMP:
		case Obj::MARLETTO_TOWER:
		case Obj::STAR_AXIS:
		case Obj::GARDEN_OF_REVELATION:
			{
				cb->changePrimSkill (h, static_cast<PrimarySkill::PrimarySkill>(subid), val);
				InfoWindow iw;
				iw.soundID = sound;
				iw.components.push_back(Component(c_id, subid, val, 0));
				iw.text.addTxt(MetaString::ADVOB_TXT,ot);
				iw.player = h->getOwner();
				cb->showInfoDialog(&iw);
				break;
			}
		case Obj::LEARNING_STONE: //give exp
			{
				val = h->calculateXp(val);
				InfoWindow iw;
				iw.soundID = sound;
				iw.components.push_back (Component(c_id,subid,val,0));
				iw.player = h->getOwner();
				iw.text.addTxt(MetaString::ADVOB_TXT,ot);
				cb->showInfoDialog(&iw);
				cb->changePrimSkill(h, PrimarySkill::EXPERIENCE, val);
				break;
			}
		case Obj::TREE_OF_KNOWLEDGE:
			{
				val = VLC->heroh->reqExp (h->level + val) - VLC->heroh->reqExp(h->level);
				if(!treePrice.nonZero())
				{
					cb->setObjProperty (id, ObjProperty::VISITORS, h->id.getNum()); //add to the visitors
					InfoWindow iw;
					iw.soundID = sound;
					iw.components.push_back (Component(c_id,subid,1,0));
					iw.player = h->getOwner();
					iw.text.addTxt (MetaString::ADVOB_TXT,148);
					cb->showInfoDialog (&iw);
					cb->changePrimSkill (h, PrimarySkill::EXPERIENCE, val);
					break;
				}
				else
				{
					if(treePrice[Res::GOLD] > 0)
						ot = 149;
					else
						ot = 151;

					if(!cb->getPlayer(h->tempOwner)->resources.canAfford(treePrice)) //not enough resources
					{
						ot++;
						showInfoDialog(h,ot,sound);
						return;
					}

					BlockingDialog sd (true, false);
					sd.soundID = sound;
					sd.player = h->getOwner();
					sd.text.addTxt (MetaString::ADVOB_TXT,ot);
					sd.addResourceComponents (treePrice);
					cb->showBlockingDialog (&sd);
				}
				break;
			}
		case Obj::LIBRARY_OF_ENLIGHTENMENT:
			{
				int txt_id = 66;
				if(h->level  <  10 - 2*h->getSecSkillLevel(SecondarySkill::DIPLOMACY)) //not enough level
				{
					txt_id += 2;
				}
				else
				{
					cb->setObjProperty(id, ObjProperty::VISITORS, h->id.getNum()); //add to the visitors
					cb->changePrimSkill (h, PrimarySkill::ATTACK, 2);
					cb->changePrimSkill (h, PrimarySkill::DEFENSE, 2);
					cb->changePrimSkill (h, PrimarySkill::KNOWLEDGE, 2);
					cb->changePrimSkill (h, PrimarySkill::SPELL_POWER, 2);
				}
				showInfoDialog(h,txt_id,sound);
				break;
			}
		case Obj::SCHOOL_OF_MAGIC:
		case Obj::SCHOOL_OF_WAR:
			{
				int skill = (ID==Obj::SCHOOL_OF_MAGIC ? 2 : 0);
				if (cb->getResource (h->getOwner(), Res::GOLD) < 1000) //not enough resources
				{
					showInfoDialog (h->getOwner(), ot+2, sound);
				}
				else
				{
					BlockingDialog sd(true,true);
					sd.soundID = sound;
					sd.player = h->getOwner();
					sd.text.addTxt(MetaString::ADVOB_TXT,ot);
					sd.components.push_back(Component(c_id, skill, +1, 0));
					sd.components.push_back(Component(c_id, skill+1, +1, 0));
					cb->showBlockingDialog(&sd);
				}
			}
			break;
		}
	}
	else
	{
		ot++;
		showInfoDialog (h->getOwner(),ot,sound);
	}
}

const std::string & CGVisitableOPH::getHoverText() const
{
	int pom = -1;
	switch(ID)
	{
	case Obj::ARENA:
		pom = -1;
		break;
	case Obj::MERCENARY_CAMP:
		pom = 8;
		break;
	case Obj::MARLETTO_TOWER:
		pom = 7;
		break;
	case Obj::STAR_AXIS:
		pom = 11;
		break;
	case Obj::GARDEN_OF_REVELATION:
		pom = 4;
		break;
	case Obj::LEARNING_STONE:
		pom = 5;
		break;
	case Obj::TREE_OF_KNOWLEDGE:
		pom = 18;
		break;
	case Obj::LIBRARY_OF_ENLIGHTENMENT:
		break;
	case Obj::SCHOOL_OF_MAGIC:
		pom = 9;
		break;
	case Obj::SCHOOL_OF_WAR:
		pom = 10;
		break;
	default:
		throw std::runtime_error("Wrong CGVisitableOPH object ID!\n");
	}
	hoverName = VLC->generaltexth->names[ID];
	if(pom >= 0)
		hoverName += ("\n" + VLC->generaltexth->xtrainfo[pom]);
	const CGHeroInstance *h = cb->getSelectedHero (cb->getCurrentPlayer());
	if(h)
	{
		hoverName += "\n\n";
		bool visited = vstd::contains (visitors, h->id);
		hoverName += visitedTxt (visited);
	}
	return hoverName;
}

void CGVisitableOPH::arenaSelected(const CGHeroInstance * h, int primSkill ) const
{
	cb->setObjProperty(id, ObjProperty::VISITORS, h->id.getNum()); //add to the visitors
	cb->changePrimSkill(h, static_cast<PrimarySkill::PrimarySkill>(primSkill-1), 2);
}

void CGVisitableOPH::setPropertyDer( ui8 what, ui32 val )
{
	if(what == ObjProperty::VISITORS)
		visitors.insert(ObjectInstanceID(val));
}

void CGVisitableOPH::schoolSelected(const CGHeroInstance * h, ui32 which) const
{
	if(!which) //player refused to pay
		return;

	int base = (ID == Obj::SCHOOL_OF_MAGIC  ?  2  :  0);
	cb->setObjProperty (id, ObjProperty::VISITORS, h->id.getNum()); //add to the visitors
	cb->giveResource (h->getOwner(),Res::GOLD,-1000); //take 1000 gold
	cb->changePrimSkill (h, static_cast<PrimarySkill::PrimarySkill>(base + which-1), +1); //give appropriate skill
}

void CGVisitableOPH::blockingDialogAnswered(const CGHeroInstance *h, ui32 answer) const 
{
	switch (ID)
	{
	case Obj::ARENA:
		arenaSelected(h, answer);
		break;

	case Obj::TREE_OF_KNOWLEDGE:
		treeSelected(h, answer);
		break;

	case Obj::SCHOOL_OF_MAGIC:
	case Obj::SCHOOL_OF_WAR:
		schoolSelected(h, answer);
		break;

	default:
		assert(0);
		break;
	}
}

COPWBonus::COPWBonus (BuildingID index, CGTownInstance *TOWN)
{
	ID = index;
	town = TOWN;
	id = town->bonusingBuildings.size();
}
void COPWBonus::setProperty(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::VISITORS:
			visitors.insert(val);
			break;
		case ObjProperty::STRUCTURE_CLEAR_VISITORS:
			visitors.clear();
			break;
	}
}
void COPWBonus::onHeroVisit (const CGHeroInstance * h) const
{
	ObjectInstanceID heroID = h->id;
	if (town->hasBuilt(ID))
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		switch (town->subID)
		{
			case ETownType::CASTLE: //Stables
				if (!h->hasBonusFrom(Bonus::OBJECT, Obj::STABLES)) //does not stack with advMap Stables
				{
					GiveBonus gb;
					gb.bonus = Bonus(Bonus::ONE_WEEK, Bonus::LAND_MOVEMENT, Bonus::OBJECT, 600, 94, VLC->generaltexth->arraytxt[100]);
					gb.id = heroID.getNum();
					cb->giveHeroBonus(&gb);
					iw.text << VLC->generaltexth->allTexts[580];
					cb->showInfoDialog(&iw);
				}
				break;
			case ETownType::DUNGEON: //Mana Vortex
				if (visitors.empty() && h->mana <= h->manaLimit() * 2)
				{
					cb->setManaPoints (heroID, 2 * h->manaLimit());
					//TODO: investigate line below
					//cb->setObjProperty (town->id, ObjProperty::VISITED, true);
					iw.text << VLC->generaltexth->allTexts[579];
					cb->showInfoDialog(&iw);
					town->addHeroToStructureVisitors(h, id);
				}
				break;
		}
	}
}
CTownBonus::CTownBonus (BuildingID index, CGTownInstance *TOWN)
{
	ID = index;
	town = TOWN;
	id = town->bonusingBuildings.size();
}
void CTownBonus::setProperty (ui8 what, ui32 val)
{
	if(what == ObjProperty::VISITORS)
		visitors.insert(ObjectInstanceID(val));
}
void CTownBonus::onHeroVisit (const CGHeroInstance * h) const
{
	ObjectInstanceID heroID = h->id;
	if (town->hasBuilt(ID) && visitors.find(heroID) == visitors.end())
	{
		InfoWindow iw;
		PrimarySkill::PrimarySkill what = PrimarySkill::ATTACK;
		int val=0, mid=0;
		switch (ID)
		{
			case BuildingID::SPECIAL_4:
				switch(town->subID)
				{
					case ETownType::TOWER: //wall
						what = PrimarySkill::KNOWLEDGE;
						val = 1;
						mid = 581;
						iw.components.push_back (Component(Component::PRIM_SKILL, 3, 1, 0));
						break;
					case ETownType::INFERNO: //order of fire
						what = PrimarySkill::SPELL_POWER;
						val = 1;
						mid = 582;
						iw.components.push_back (Component(Component::PRIM_SKILL, 2, 1, 0));
						break;
					case ETownType::STRONGHOLD://hall of Valhalla
						what = PrimarySkill::ATTACK;
						val = 1;
						mid = 584;
						iw.components.push_back (Component(Component::PRIM_SKILL, 0, 1, 0));
						break;
					case ETownType::DUNGEON://academy of battle scholars
						what = PrimarySkill::EXPERIENCE;
						val = h->calculateXp(1000);
						mid = 583;
						iw.components.push_back (Component(Component::EXPERIENCE, 0, val, 0));
						break;
				}
				break;
			case BuildingID::SPECIAL_1:
				switch(town->subID)
				{
					case ETownType::FORTRESS: //cage of warlords
						what = PrimarySkill::DEFENSE;
						val = 1;
						mid = 585;
						iw.components.push_back (Component(Component::PRIM_SKILL, 1, 1, 0));
						break;
				}
				break;
		}
		assert(mid);
		iw.player = cb->getOwner(heroID);
		iw.text << VLC->generaltexth->allTexts[mid];
		cb->showInfoDialog(&iw);
		cb->changePrimSkill (cb->getHero(heroID), what, val);
		town->addHeroToStructureVisitors(h, id);
	}
}
const std::string & CGCreature::getHoverText() const
{
	if(stacks.empty())
	{
		static const std::string errorValue("!!!INVALID_STACK!!!");

		//should not happen... 
		logGlobal->errorStream() << "Invalid stack at tile " << pos << ": subID=" << subID << "; id=" << id;
		return errorValue; // references to temporary are illegal - use pre-constructed string
	}

	MetaString ms;
	int pom = stacks.begin()->second->getQuantityID();
	pom = 172 + 3*pom;
	ms.addTxt(MetaString::ARRAY_TXT,pom);
	ms << " " ;
	ms.addTxt(MetaString::CRE_PL_NAMES,subID);
	ms.toString(hoverName);

	if(const CGHeroInstance *selHero = cb->getSelectedHero(cb->getCurrentPlayer()))
	{
		std::vector<std::string> * texts = &VLC->generaltexth->threat;
		hoverName += "\n\n ";
		hoverName += (*texts)[0];
		int choice;
		double ratio = ((double)getArmyStrength() / selHero->getTotalStrength());
		if (ratio < 0.1) choice = 1;
		else if (ratio < 0.25) choice = 2;
		else if (ratio < 0.6) choice = 3;
		else if (ratio < 0.9) choice = 4;
		else if (ratio < 1.1) choice = 5;
		else if (ratio < 1.3) choice = 6;
		else if (ratio < 1.8) choice = 7;
		else if (ratio < 2.5) choice = 8;
		else if (ratio < 4) choice = 9;
		else if (ratio < 8) choice = 10;
		else if (ratio < 20) choice = 11;
		else choice = 12;
		hoverName += (*texts)[choice];
	}
	return hoverName;
}
void CGCreature::onHeroVisit( const CGHeroInstance * h ) const
{
	int action = takenAction(h);
	switch( action ) //decide what we do...
	{
	case FIGHT:
		fight(h);
		break;
	case FLEE: //flee
		{
			flee(h);
			break;
		}
	case JOIN_FOR_FREE: //join for free
		{
			BlockingDialog ynd(true,false);
			ynd.player = h->tempOwner;
			ynd.text.addTxt(MetaString::ADVOB_TXT, 86);
			ynd.text.addReplacement(MetaString::CRE_PL_NAMES, subID);
			cb->showBlockingDialog(&ynd);
			break;
		}
	default: //join for gold
		{
			assert(action > 0);

			//ask if player agrees to pay gold
			BlockingDialog ynd(true,false);
			ynd.player = h->tempOwner;
			std::string tmp = VLC->generaltexth->advobtxt[90];
			boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(getStackCount(SlotID(0))));
			boost::algorithm::replace_first(tmp,"%d",boost::lexical_cast<std::string>(action));
			boost::algorithm::replace_first(tmp,"%s",VLC->creh->creatures[subID]->namePl);
			ynd.text << tmp;
			cb->showBlockingDialog(&ynd);
			break;
		}
	}

}

void CGCreature::initObj()
{
	blockVisit = true;
	switch(character)
	{
	case 0:
		character = -4;
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

	stacks[SlotID(0)]->setType(CreatureID(subID));
	TQuantity &amount = stacks[SlotID(0)]->count;
	CCreature &c = *VLC->creh->creatures[subID];
	if(!amount)
	{
		if(c.ammMax == c.ammMin)
			amount = c.ammMax;
		else
			amount = c.ammMin + (ran() % (c.ammMax - c.ammMin));

		if(!amount) //armies with 0 creatures are illegal
		{
            logGlobal->warnStream() << "Problem: stack " << nodeName() << " cannot have 0 creatures. Check properties of " << c.nodeName();
			amount = 1;
		}
	}
	formation.randomFormation = rand();

	temppower = stacks[SlotID(0)]->count * 1000;
	refusedJoining = false;
}

void CGCreature::newTurn() const
{//Works only for stacks of single type of size up to 2 millions
	if (stacks.begin()->second->count < VLC->modh->settings.CREEP_SIZE && cb->getDate(Date::DAY_OF_WEEK) == 1 && cb->getDate(Date::DAY) > 1)
	{
		ui32 power = temppower * (100 +  VLC->modh->settings.WEEKLY_GROWTH)/100;
		cb->setObjProperty(id, ObjProperty::MONSTER_COUNT, std::min (power/1000 , (ui32)VLC->modh->settings.CREEP_SIZE)); //set new amount
		cb->setObjProperty(id, ObjProperty::MONSTER_POWER, power); //increase temppower
	}
	if (VLC->modh->modules.STACK_EXP)
		cb->setObjProperty(id, ObjProperty::MONSTER_EXP, VLC->modh->settings.NEUTRAL_STACK_EXP); //for testing purpose
}
void CGCreature::setPropertyDer(ui8 what, ui32 val)
{
	switch (what)
	{
		case ObjProperty::MONSTER_COUNT:
			stacks[SlotID(0)]->count = val;
			break;
		case ObjProperty::MONSTER_POWER:
			temppower = val;
			break;
		case ObjProperty::MONSTER_EXP:
			giveStackExp(val);
			break;
		case ObjProperty::MONSTER_RESTORE_TYPE:
			formation.basicType = val;
			break;
		case ObjProperty::MONSTER_REFUSED_JOIN:
			refusedJoining = val;
			break;
	}
}

int CGCreature::takenAction(const CGHeroInstance *h, bool allowJoin) const
{
	//calculate relative strength of hero and creatures armies
	double relStrength = double(h->getTotalStrength()) / getArmyStrength();

	int powerFactor;
	if(relStrength >= 7)
		powerFactor = 11;

	else if(relStrength >= 1)
		powerFactor = (int)(2*(relStrength-1));

	else if(relStrength >= 0.5)
		powerFactor = -1;

	else if(relStrength >= 0.333)
		powerFactor = -2;
	else
		powerFactor = -3;

	std::set<CreatureID> myKindCres; //what creatures are the same kind as we
	const CCreature * myCreature = VLC->creh->creatures[subID];
	myKindCres.insert(myCreature->idNumber); //we
	myKindCres.insert(myCreature->upgrades.begin(), myCreature->upgrades.end()); //our upgrades

	for(ConstTransitivePtr<CCreature> &crea : VLC->creh->creatures)
	{
		if(vstd::contains(crea->upgrades, myCreature->idNumber)) //it's our base creatures
			myKindCres.insert(crea->idNumber);
	}

	int count = 0, //how many creatures of similar kind has hero
		totalCount = 0;

	for (auto & elem : h->Slots())
	{
		if(vstd::contains(myKindCres,elem.second->type->idNumber))
			count += elem.second->count;
		totalCount += elem.second->count;
	}

	int sympathy = 0; // 0 if hero have no similar creatures
	if(count)
		sympathy++; // 1 - if hero have at least 1 similar creature
	if(count*2 > totalCount)
		sympathy++; // 2 - hero have similar creatures more that 50%

	int charisma = powerFactor + h->getSecSkillLevel(SecondarySkill::DIPLOMACY) + sympathy;

	if(charisma < character) //creatures will fight
		return -2;

	if (allowJoin)
	{
		if(h->getSecSkillLevel(SecondarySkill::DIPLOMACY) + sympathy + 1 >= character)
			return 0; //join for free

		else if(h->getSecSkillLevel(SecondarySkill::DIPLOMACY) * 2  +  sympathy  +  1 >= character)
			return VLC->creh->creatures[subID]->cost[6] * getStackCount(SlotID(0)); //join for gold
	}

	//we are still here - creatures have not joined hero, flee or fight

	if (charisma > character)
		return -1; //flee
	else
		return -2; //fight
}

void CGCreature::fleeDecision(const CGHeroInstance *h, ui32 pursue) const
{
	if(refusedJoining)
		cb->setObjProperty(id, ObjProperty::MONSTER_REFUSED_JOIN, false);

	if(pursue)
	{
		fight(h);
	}
	else
	{
		cb->removeObject(this);
	}
}

void CGCreature::joinDecision(const CGHeroInstance *h, int cost, ui32 accept) const
{
	if(!accept)
	{
		if(takenAction(h,false) == -1) //they flee
		{
			cb->setObjProperty(id, ObjProperty::MONSTER_REFUSED_JOIN, true);
			flee(h);
		}
		else //they fight
		{
			showInfoDialog(h,87,0);//Insulted by your refusal of their offer, the monsters attack!
			fight(h);
		}
	}
	else //accepted
	{
		if (cb->getResource(h->tempOwner, Res::GOLD) < cost) //player don't have enough gold!
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
			cb->giveResource(h->tempOwner,Res::GOLD,-cost);

		cb->tryJoiningArmy(this, h, true, true);
	}
}

void CGCreature::fight( const CGHeroInstance *h ) const
{
	//split stacks
	//TODO: multiple creature types in a stack?
	int basicType = stacks.begin()->second->type->idNumber;
	cb->setObjProperty(id, ObjProperty::MONSTER_RESTORE_TYPE, basicType); //store info about creature stack

	double relativePower = static_cast<double>(h->getTotalStrength()) / getArmyStrength();
	int stacksCount;
	//TODO: number depends on tile type
	if (relativePower < 0.5)
	{
		stacksCount = 7;
	}
	else if (relativePower < 0.67)
	{
		stacksCount = 7;
	}
	else if (relativePower < 1)
	{
		stacksCount = 6;
	}
	else if (relativePower < 1.5)
	{
		stacksCount = 5;
	}
	else if (relativePower < 2)
	{
		stacksCount = 4;
	}
	else
	{
		stacksCount = 3;
	}
	int stackSize;
	SlotID sourceSlot = stacks.begin()->first;
	SlotID destSlot;
	for (int stacksLeft = stacksCount; stacksLeft > 1; --stacksLeft)
	{
		stackSize = stacks.begin()->second->count / stacksLeft;
		if (stackSize)
		{
			if ((destSlot = getFreeSlot()).validSlot())
				cb->moveStack(StackLocation(this, sourceSlot), StackLocation(this, destSlot), stackSize);
			else
			{
                logGlobal->warnStream() <<"Warning! Not enough empty slots to split stack!";
				break;
			}
		}
		else break;
	}
	if (stacksCount > 1)
	{
		if (formation.randomFormation % 100 < 50) //upgrade
		{
			SlotID slotId = SlotID(stacks.size() / 2);
			if(ui32 upgradesSize = getStack(slotId).type->upgrades.size())
			{
				auto it = getStack(slotId).type->upgrades.cbegin(); //pick random in case there are more
				std::advance (it, rand() % upgradesSize);
				cb->changeStackType(StackLocation(this, slotId), VLC->creh->creatures[*it]);
			}
		}
	}

	cb->startBattleI(h, this);

}

void CGCreature::flee( const CGHeroInstance * h ) const
{
	BlockingDialog ynd(true,false);
	ynd.player = h->tempOwner;
	ynd.text.addTxt(MetaString::ADVOB_TXT,91);
	ynd.text.addReplacement(MetaString::CRE_PL_NAMES, subID);
	cb->showBlockingDialog(&ynd);
}

void CGCreature::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const 
{
	
	if(result.winner==0)
	{
		cb->removeObject(this);
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

		//merge stacks into one
		TSlots::const_iterator i;
		CCreature * cre = VLC->creh->creatures[formation.basicType];
		for (i = stacks.begin(); i != stacks.end(); i++)
		{
			if (cre->isMyUpgrade(i->second->type))
			{
				cb->changeStackType (StackLocation(this, i->first), cre); //un-upgrade creatures
			}
		}

		//first stack has to be at slot 0 -> if original one got killed, move there first remaining stack
		if(!hasStackAtSlot(SlotID(0)))
			cb->moveStack(StackLocation(this, stacks.begin()->first), StackLocation(this, SlotID(0)), stacks.begin()->second->count);

		while (stacks.size() > 1) //hopefully that's enough
		{
			// TODO it's either overcomplicated (if we assume there'll be only one stack) or buggy (if we allow multiple stacks... but that'll also cause troubles elsewhere)
			i = stacks.end();
			i--;
			SlotID slot = getSlotFor(i->second->type);
			if (slot == i->first) //no reason to move stack to its own slot
				break;
			else
				cb->moveStack (StackLocation(this, i->first), StackLocation(this, slot), i->second->count);
		}

		cb->setObjProperty(id, ObjProperty::MONSTER_POWER, stacks.begin()->second->count * 1000); //remember casualties
	}
}

void CGCreature::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	auto action = takenAction(hero);
	if(!refusedJoining && action >= JOIN_FOR_FREE) //higher means price
		joinDecision(hero, action, answer);
	else if(action != FIGHT)
		fleeDecision(hero, answer);
	else
		assert(0);
}

void CGMine::onHeroVisit( const CGHeroInstance * h ) const
{
	int relations = cb->gameState()->getPlayerRelations(h->tempOwner, tempOwner);

	if(relations == 2) //we're visiting our mine
	{
		cb->showGarrisonDialog(id,h->id,true);
		return;
	}
	else if (relations == 1)//ally
		return;

	if(stacksCount()) //Mine is guarded
	{
		BlockingDialog ynd(true,false);
		ynd.player = h->tempOwner;
		ynd.text.addTxt(MetaString::ADVOB_TXT, subID == 7 ? 84 : 187);
		cb->showBlockingDialog(&ynd);
		return;
	}

	flagMine(h->tempOwner);

}

void CGMine::newTurn() const
{
	if(cb->getDate() == 1)
		return;

	if (tempOwner == PlayerColor::NEUTRAL)
		return;

	cb->giveResource(tempOwner, producedResource, producedQuantity);
}

void CGMine::initObj()
{
	if(subID >= 7) //Abandoned Mine
	{
		//set guardians
		int howManyTroglodytes = 100 + ran()%100;
		auto troglodytes = new CStackInstance(CreatureID::TROGLODYTES, howManyTroglodytes);
		putStack(SlotID(0), troglodytes);

		//after map reading tempOwner placeholds bitmask for allowed resources
		std::vector<Res::ERes> possibleResources;
		for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
			if(tempOwner.getNum() & 1<<i) //NOTE: reuse of tempOwner
				possibleResources.push_back(static_cast<Res::ERes>(i));

		assert(possibleResources.size());
		producedResource = possibleResources[ran()%possibleResources.size()];
		tempOwner = PlayerColor::NEUTRAL;
		hoverName = VLC->generaltexth->mines[7].first + "\n" + VLC->generaltexth->allTexts[202] + " " + troglodytes->getQuantityTXT(false) + " " + troglodytes->type->namePl;
	}
	else
	{
		producedResource = static_cast<Res::ERes>(subID);

		MetaString ms;
		ms << std::pair<ui8,ui32>(9,producedResource);
		if(tempOwner >= PlayerColor::PLAYER_LIMIT)
			tempOwner = PlayerColor::NEUTRAL;
		else
			ms << " (" << std::pair<ui8,ui32>(6,23+tempOwner.getNum()) << ")";
		ms.toString(hoverName);
	}

	producedQuantity = defaultResProduction();
}

void CGMine::flagMine(PlayerColor player) const
{
	assert(tempOwner != player);
	cb->setOwner(this, player); //not ours? flag it!

	MetaString ms;
	ms << std::pair<ui8,ui32>(9,subID) << "\n(" << std::pair<ui8,ui32>(6,23+player.getNum()) << ")";
	if(subID == 7)
	{
		ms << "(%s)";
		ms.addReplacement(MetaString::RES_NAMES, producedResource);
	}
	cb->setHoverName(this,&ms);

	InfoWindow iw;
	iw.soundID = soundBase::FLAGMINE;
	iw.text.addTxt(MetaString::MINE_EVNTS,producedResource); //not use subID, abandoned mines uses default mine texts
	iw.player = player;
	iw.components.push_back(Component(Component::RESOURCE,producedResource,producedQuantity,-1));
	cb->showInfoDialog(&iw);
}

ui32 CGMine::defaultResProduction()
{
	switch(producedResource)
	{
	case Res::WOOD:
	case Res::ORE:
		return 2;
	case Res::GOLD:
		return 1000;
	default:
		return 1;
	}
}

void CGMine::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const 
{
	if(result.winner == 0) //attacker won
	{
		if(subID == 7)
		{
			showInfoDialog(hero->tempOwner, 85, 0);
		}
		flagMine(hero->tempOwner);
	}
}

void CGMine::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	if(answer)
		cb->startBattleI(hero, this);
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
			amount = 500 + (ran()%6)*100;
			break;
		case 0: case 2:
			amount = 6 + (ran()%5);
			break;
		default:
			amount = 3 + (ran()%3);
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
			cb->showBlockingDialog(&ynd);
		}
		else
		{
			blockingDialogAnswered(h, true); //behave as if player accepted battle
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

void CGResource::collectRes( PlayerColor player ) const
{
	cb->giveResource(player, static_cast<Res::ERes>(subID), amount);
	ShowInInfobox sii;
	sii.player = player;
	sii.c = Component(Component::RESOURCE,subID,amount,0);
	sii.text.addTxt(MetaString::ADVOB_TXT,113);
	sii.text.addReplacement(MetaString::RES_NAMES, subID);
	cb->showCompInfo(&sii);
	cb->removeObject(this);
}

void CGResource::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const 
{
	if(result.winner == 0) //attacker won
		collectRes(hero->getOwner());
}

void CGResource::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	if(answer)
		cb->startBattleI(hero, this);
}

void CGVisitableOPW::newTurn() const
{
	if (cb->getDate(Date::DAY_OF_WEEK) == 1) //first day of week = 1
	{
		cb->setObjProperty(id, ObjProperty::VISITED, false);
		MetaString ms; //set text to "not visited"
		ms << std::pair<ui8,ui32>(3,ID) << " " << std::pair<ui8,ui32>(1,353);
		cb->setHoverName(this,&ms);
	}
}
bool CGVisitableOPW::wasVisited(PlayerColor player) const
{
	return visited; //TODO: other players should see object as unvisited
}

void CGVisitableOPW::onHeroVisit( const CGHeroInstance * h ) const
{
	int mid=0, sound = 0;
	switch (ID)
	{
	case Obj::MYSTICAL_GARDEN:
		sound = soundBase::experience;
		mid = 92;
		break;
	case Obj::WINDMILL:
		sound = soundBase::GENIE;
		mid = 170;
		break;
	case Obj::WATER_WHEEL:
		sound = soundBase::GENIE;
		mid = 164;
		break;
	default:
		assert(0);
	}
	if (visited)
	{
		if (ID!=Obj::WINDMILL)
			mid++;
		else
			mid--;
		showInfoDialog(h,mid,sound);
	}
	else
	{
		Component::EComponentType type = Component::RESOURCE;
		Res::ERes sub=Res::WOOD;
		int val=0;

		switch (ID)
		{
		case Obj::MYSTICAL_GARDEN:
			if (rand()%2)
			{
				sub = Res::GEMS;
				val = 5;
			}
			else
			{
				sub = Res::GOLD;
				val = 500;
			}
			break;
		case Obj::WINDMILL:
			mid = 170;
			sub = static_cast<Res::ERes>((rand() % 5) + 1);
			val = (rand() % 4) + 3;
			break;
		case Obj::WATER_WHEEL:
			mid = 164;
			sub = Res::GOLD;
			if(cb->getDate(Date::DAY)<8)
				val = 500;
			else
				val = 1000;
		}
		cb->giveResource(h->tempOwner, sub, val);
		InfoWindow iw;
		iw.soundID = sound;
		iw.player = h->tempOwner;
		iw.components.push_back(Component(type,sub,val,0));
		iw.text.addTxt(MetaString::ADVOB_TXT,mid);
		cb->showInfoDialog(&iw);
		cb->setObjProperty(id, ObjProperty::VISITED, true);
		MetaString ms; //set text to "visited"
		ms.addTxt(MetaString::OBJ_NAMES,ID); ms << " "; ms.addTxt(MetaString::GENERAL_TXT,352);
		cb->setHoverName(this,&ms);
	}
}

void CGVisitableOPW::setPropertyDer( ui8 what, ui32 val )
{
	if(what == ObjProperty::VISITED)
		visited = val;
}

void CGTeleport::onHeroVisit( const CGHeroInstance * h ) const
{
	ObjectInstanceID destinationid;
	switch(ID)
	{
	case Obj::MONOLITH1: //one way - find corresponding exit monolith
		if(vstd::contains(objs,Obj::MONOLITH2) && vstd::contains(objs[Obj::MONOLITH2],subID) && objs[Obj::MONOLITH2][subID].size())
			destinationid = objs[Obj::MONOLITH2][subID][rand()%objs[Obj::MONOLITH2][subID].size()];
		else
            logGlobal->warnStream() << "Cannot find corresponding exit monolith for "<< id;
		break;
	case Obj::MONOLITH3://two way monolith - pick any other one
	case Obj::WHIRLPOOL: //Whirlpool
		if(vstd::contains(objs,ID) && vstd::contains(objs[ID],subID) && objs[ID][subID].size()>1)
		{
			while ((destinationid = objs[ID][subID][rand()%objs[ID][subID].size()]) == id); //choose another exit
			if (ID == Obj::WHIRLPOOL)
			{
				if (!h->hasBonusOfType(Bonus::WHIRLPOOL_PROTECTION))
				{
					if (h->Slots().size() > 1 || h->Slots().begin()->second->count > 1)
					{ //we can't remove last unit
						SlotID targetstack = h->Slots().begin()->first; //slot numbers may vary
						for(auto i = h->Slots().rbegin(); i != h->Slots().rend(); i++)
						{
							if (h->getPower(targetstack) > h->getPower(i->first))
							{
								targetstack = (i->first);
							}
						}

						TQuantity countToTake = h->getStackCount(targetstack) * 0.5;
						vstd::amax(countToTake, 1);


						InfoWindow iw;
						iw.player = h->tempOwner;
						iw.text.addTxt (MetaString::ADVOB_TXT, 168);
						iw.components.push_back (Component(CStackBasicDescriptor(h->getCreature(targetstack), countToTake)));
						cb->showInfoDialog(&iw);
						cb->changeStackCount(StackLocation(h, targetstack), -countToTake);
					}
				}
			}
		}
		else
            logGlobal->warnStream() << "Cannot find corresponding exit monolith for "<< id;
		break;
	case Obj::SUBTERRANEAN_GATE: //find nearest subterranean gate on the other level
		{
			destinationid = getMatchingGate(id);
			if(destinationid == ObjectInstanceID()) //no exit
			{
				showInfoDialog(h,153,0);//Just inside the entrance you find a large pile of rubble blocking the tunnel. You leave discouraged.
			}
			break;
		}
	}
	if(destinationid == ObjectInstanceID())
	{
        logGlobal->warnStream() << "Cannot find exit... (obj at " << pos << ") :( ";
		return;
	}
	if (ID == Obj::WHIRLPOOL)
	{
		std::set<int3> tiles = cb->getObj(destinationid)->getBlockedPos();
		auto it = tiles.begin();
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
	case Obj::SUBTERRANEAN_GATE://ignore subterranean gates subid
	case Obj::WHIRLPOOL:
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
	for(auto & elem : objs[Obj::SUBTERRANEAN_GATE][0])
	{
		const CGObjectInstance *hlp = cb->getObj(elem);
		gatesSplit[hlp->pos.z].push_back(hlp);
	}

	//sort by position
	std::sort(gatesSplit[0].begin(), gatesSplit[0].end(), [](const CGObjectInstance * a, const CGObjectInstance * b)
	{
		return a->pos < b->pos;
	});

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
			gates.push_back(std::make_pair(cur->id, gatesSplit[1][best.first]->id));
			gatesSplit[1][best.first] = nullptr;
		}
		else
		{
			gates.push_back(std::make_pair(cur->id, ObjectInstanceID()));
		}
	}
	objs.erase(Obj::SUBTERRANEAN_GATE);
}

ObjectInstanceID CGTeleport::getMatchingGate(ObjectInstanceID id)
{
	for(auto & gate : gates)
	{
		if(gate.first == id)
			return gate.second;
		if(gate.second == id)
			return gate.first;
	}

	return ObjectInstanceID();
}

void CGArtifact::initObj()
{
	blockVisit = true;
	if(ID == Obj::ARTIFACT)
	{
		hoverName = VLC->arth->artifacts[subID]->Name();
		if(!storedArtifact->artType)
			storedArtifact->setType(VLC->arth->artifacts[subID]);
	}
	if(ID == Obj::SPELL_SCROLL)
		subID = 1;
	
	assert(storedArtifact->artType);
	assert(storedArtifact->getParentNodes().size());

	//assert(storedArtifact->artType->id == subID); //this does not stop desync
}

void CGArtifact::onHeroVisit( const CGHeroInstance * h ) const
{
	if(!stacksCount())
	{
		InfoWindow iw;
		iw.player = h->tempOwner;
		switch(ID)
		{
		case Obj::ARTIFACT:
			{
				iw.soundID = soundBase::treasure; //play sound only for non-scroll arts
				iw.components.push_back(Component(Component::ARTIFACT,subID,0,0));
				if(message.length())
					iw.text <<  message;
				else
				{
					if (VLC->arth->artifacts[subID]->EventText().size())
						iw.text << std::pair<ui8, ui32> (MetaString::ART_EVNTS, subID);
					else //fix for mod artifacts with no event text
					{
						iw.text.addTxt (MetaString::ADVOB_TXT, 183); //% has found treasure
						iw.text.addReplacement (h->name);
					}

				}
			}
			break;
		case Obj::SPELL_SCROLL:
			{
				int spellID = storedArtifact->getGivenSpellID();
				iw.components.push_back (Component(Component::SPELL, spellID,0,0));
				iw.text.addTxt (MetaString::ADVOB_TXT,135);
				iw.text.addReplacement(MetaString::SPELL_NAME, spellID);
			}
			break;
		}
		cb->showInfoDialog(&iw);
		pick(h);
	}
	else
	{
		if(message.size())
		{
			BlockingDialog ynd(true,false);
			ynd.player = h->getOwner();
			ynd.text << message;
			cb->showBlockingDialog(&ynd);
		}
		else
		{
			blockingDialogAnswered(h, true);
		}
	}
}

void CGArtifact::pick(const CGHeroInstance * h) const
{
	cb->giveHeroArtifact(h, storedArtifact, ArtifactPosition::FIRST_AVAILABLE);
	cb->removeObject(this);
}

void CGArtifact::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const 
{
	if(result.winner == 0) //attacker won
		pick(hero);
}

void CGArtifact::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	if(answer)
		cb->startBattleI(hero, this);
}

void CGPickable::initObj()
{
	blockVisit = true;
	switch(ID)
	{
	case Obj::CAMPFIRE:
		val2 = (ran()%3) + 4; //4 - 6
		val1 = val2 * 100;
		type = ran()%6; //given resource
		break;
	case Obj::FLOTSAM:
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
	case Obj::SEA_CHEST:
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
	case Obj::SHIPWRECK_SURVIVOR:
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
	case Obj::TREASURE_CHEST:
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
	case Obj::CAMPFIRE:
		{
			cb->giveResource(h->tempOwner,static_cast<Res::ERes>(type),val2); //non-gold resource
			cb->giveResource(h->tempOwner,Res::GOLD,val1);//gold
			InfoWindow iw;
			iw.soundID = soundBase::experience;
			iw.player = h->tempOwner;
			iw.components.push_back(Component(Component::RESOURCE,Res::GOLD,val1,0));
			iw.components.push_back(Component(Component::RESOURCE,type,val2,0));
			iw.text.addTxt(MetaString::ADVOB_TXT,23);
			cb->showInfoDialog(&iw);
			break;
		}
	case Obj::FLOTSAM:
		{
			cb->giveResource(h->tempOwner,Res::WOOD,val1); //wood
			cb->giveResource(h->tempOwner,Res::GOLD,val2);//gold
			InfoWindow iw;
			iw.soundID = soundBase::GENIE;
			iw.player = h->tempOwner;
			if(val1)
				iw.components.push_back(Component(Component::RESOURCE,Res::WOOD,val1,0));
			if(val2)
				iw.components.push_back(Component(Component::RESOURCE,Res::GOLD,val2,0));

			iw.text.addTxt(MetaString::ADVOB_TXT, 51+type);
			cb->showInfoDialog(&iw);
			break;
		}
	case Obj::SEA_CHEST:
		{
			InfoWindow iw;
			iw.soundID = soundBase::chest;
			iw.player = h->tempOwner;
			iw.text.addTxt(MetaString::ADVOB_TXT, 116 + type);

			if(val1) //there is gold
			{
				iw.components.push_back(Component(Component::RESOURCE,Res::GOLD,val1,0));
				cb->giveResource(h->tempOwner,Res::GOLD,val1);
			}
			if(type == 1) //art
			{
				//TODO: what if no space in backpack?
				iw.components.push_back(Component(Component::ARTIFACT, val2, 1, 0));
				iw.text.addReplacement(MetaString::ART_NAMES, val2);
				cb->giveHeroNewArtifact(h, VLC->arth->artifacts[val2],ArtifactPosition::FIRST_AVAILABLE);
			}
			cb->showInfoDialog(&iw);
			break;
		}
	case Obj::SHIPWRECK_SURVIVOR:
		{
			InfoWindow iw;
			iw.soundID = soundBase::experience;
			iw.player = h->tempOwner;
			iw.components.push_back(Component(Component::ARTIFACT,val1,1,0));
			iw.text.addTxt(MetaString::ADVOB_TXT, 125);
			iw.text.addReplacement(MetaString::ART_NAMES, val1);
			cb->giveHeroNewArtifact(h, VLC->arth->artifacts[val1],ArtifactPosition::FIRST_AVAILABLE);
			cb->showInfoDialog(&iw);
			break;
		}
	case Obj::TREASURE_CHEST:
		{
			if (subID) //not OH3 treasure chest
			{
                logGlobal->warnStream() << "Not supported WoG treasure chest!";
				return;
			}

			if(type) //there is an artifact
			{
				cb->giveHeroNewArtifact(h, VLC->arth->artifacts[val1],ArtifactPosition::FIRST_AVAILABLE);
				InfoWindow iw;
				iw.soundID = soundBase::treasure;
				iw.player = h->tempOwner;
				iw.components.push_back(Component(Component::ARTIFACT,val1,1,0));
				iw.text.addTxt(MetaString::ADVOB_TXT,145);
				iw.text.addReplacement(MetaString::ART_NAMES, val1);
				cb->showInfoDialog(&iw);
				break;
			}
			else
			{
				BlockingDialog sd(false,true);
				sd.player = h->tempOwner;
				sd.text.addTxt(MetaString::ADVOB_TXT,146);
				sd.components.push_back(Component(Component::RESOURCE,Res::GOLD,val1,0));
				TExpType expVal = h->calculateXp(val2);
				sd.components.push_back(Component(Component::EXPERIENCE,0,expVal, 0));
				sd.soundID = soundBase::chest;
				cb->showBlockingDialog(&sd);
				return;
			}
		}
	}
	cb->removeObject(this);
}

void CGPickable::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	switch(answer)
	{
	case 1: //player pick gold
		cb->giveResource(hero->tempOwner, Res::GOLD, val1);
		break;
	case 2: //player pick exp
		cb->changePrimSkill(hero, PrimarySkill::EXPERIENCE, hero->calculateXp(val2));
		break;
	default:
		throw std::runtime_error("Unhandled treasure choice");
	}
	cb->removeObject(this);
}

bool CQuest::checkQuest (const CGHeroInstance * h) const
{
	switch (missionType)
	{
		case MISSION_NONE:
			return true;
		case MISSION_LEVEL:
			if (m13489val <= h->level)
				return true;
			return false;
		case MISSION_PRIMARY_STAT:
			for (int i = 0; i < GameConstants::PRIMARY_SKILLS; ++i)
			{
				if (h->getPrimSkillLevel(static_cast<PrimarySkill::PrimarySkill>(i)) < m2stats[i])
					return false;
			}
			return true;
		case MISSION_KILL_HERO:
		case MISSION_KILL_CREATURE:
			if (!h->cb->getObjByQuestIdentifier(m13489val))
				return true;
			return false;
		case MISSION_ART:
			for (auto & elem : m5arts)
			{
				if (h->hasArt(elem))
					continue;
				return false; //if the artifact was not found
			}
			return true;
		case MISSION_ARMY:
			{
				std::vector<CStackBasicDescriptor>::const_iterator cre;
				TSlots::const_iterator it;
				ui32 count;
				for (cre = m6creatures.begin(); cre != m6creatures.end(); ++cre)
				{
					for (count = 0, it = h->Slots().begin(); it !=  h->Slots().end(); ++it)
					{
						if (it->second->type == cre->type)
							count += it->second->count;
					}
					if (count < cre->count) //not enough creatures of this kind
						return false;
				}
			}
			return true;
		case MISSION_RESOURCES:
			for (Res::ERes i = Res::WOOD; i <= Res::GOLD; vstd::advance(i, +1)) //including Mithril ?
			{	//Quest has no direct access to callback
				if (h->cb->getResource (h->tempOwner, i) < m7resources[i])
					return false;
			}
			return true;
		case MISSION_HERO:
			if (m13489val == h->type->ID.getNum())
				return true;
			return false;
		case MISSION_PLAYER:
			if (m13489val == h->getOwner().getNum())
				return true;
			return false;
		default:
			return false;
	}
}
void CQuest::getVisitText (MetaString &iwText, std::vector<Component> &components, bool isCustom, bool firstVisit, const CGHeroInstance * h) const
{
	std::string text;
	bool failRequirements = (h ? !checkQuest(h) : true);

	if (firstVisit)
	{
		isCustom = isCustomFirst;
		iwText << (text = firstVisitText);
	}
	else if (failRequirements)
	{
		isCustom = isCustomNext;
		iwText << (text = nextVisitText);
	}
	switch (missionType)
	{
		case MISSION_LEVEL:
			components.push_back(Component (Component::EXPERIENCE, 0, m13489val, 0));
			if (!isCustom)
				iwText.addReplacement(m13489val);
			break;
		case MISSION_PRIMARY_STAT:
		{
			MetaString loot;
			for (int i = 0; i < 4; ++i)
			{
				if (m2stats[i])
				{
					components.push_back(Component (Component::PRIM_SKILL, i, m2stats[i], 0));
					loot << "%d %s";
					loot.addReplacement(m2stats[i]);
					loot.addReplacement(VLC->generaltexth->primarySkillNames[i]);
				}
			}
			if (!isCustom)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case MISSION_KILL_HERO:
			components.push_back(Component(Component::HERO, heroPortrait, 0, 0));
			if (!isCustom)
				addReplacements(iwText, text);
			break;
		case MISSION_HERO:
			components.push_back(Component (Component::HERO, m13489val, 0, 0));
			if (!isCustom)
				iwText.addReplacement(VLC->heroh->heroes[m13489val]->name);
			break;
		case MISSION_KILL_CREATURE:
			{
				components.push_back(Component(stackToKill));
				if (!isCustom)
				{
					addReplacements(iwText, text);
				}
			}
			break;
		case MISSION_ART:
		{
			MetaString loot;
			for (auto & elem : m5arts)
			{
				components.push_back(Component (Component::ARTIFACT, elem, 0, 0));
				loot << "%s";
				loot.addReplacement(MetaString::ART_NAMES, elem);
			}
			if (!isCustom)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case MISSION_ARMY:
		{
			MetaString loot;
			for (auto & elem : m6creatures)
			{
				components.push_back(Component(elem));
				loot << "%s";
				loot.addReplacement(elem);
			}
			if (!isCustom)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case MISSION_RESOURCES:
		{
			MetaString loot;
			for (int i = 0; i < 7; ++i)
			{
				if (m7resources[i])
				{
					components.push_back(Component (Component::RESOURCE, i, m7resources[i], 0));
					loot << "%d %s";
					loot.addReplacement(m7resources[i]);
					loot.addReplacement(MetaString::RES_NAMES, i);
				}
			}
			if (!isCustom)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case MISSION_PLAYER:
			components.push_back(Component (Component::FLAG, m13489val, 0, 0));
			if (!isCustom)
				iwText.addReplacement(VLC->generaltexth->colors[m13489val]);
			break;
	}
}
void CQuest::getRolloverText (MetaString &ms, bool onHover) const
{
	if (onHover)
		ms << "\n\n";

	ms << VLC->generaltexth->quests[missionType-1][onHover ? 3 : 4][textOption];

	switch (missionType)
	{
		case MISSION_LEVEL:
			ms.addReplacement(m13489val);
			break;
		case MISSION_PRIMARY_STAT:
			{
				MetaString loot;
				for (int i = 0; i < 4; ++i)
				{
					if (m2stats[i])
					{
						loot << "%d %s";
						loot.addReplacement(m2stats[i]);
						loot.addReplacement(VLC->generaltexth->primarySkillNames[i]);
					}
				}
				ms.addReplacement(loot.buildList());
			}
			break;
		case MISSION_KILL_HERO:
			ms.addReplacement(heroName);
			break;
		case MISSION_KILL_CREATURE:
			ms.addReplacement(stackToKill);
			break;
		case MISSION_ART:
			{
				MetaString loot;
				for (auto & elem : m5arts)
				{
					loot << "%s";
					loot.addReplacement(MetaString::ART_NAMES, elem);
				}
				ms.addReplacement(loot.buildList());
			}
			break;
		case MISSION_ARMY:
			{
				MetaString loot;
				for (auto & elem : m6creatures)
				{
					loot << "%s";
					loot.addReplacement(elem);
				}
				ms.addReplacement(loot.buildList());
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
						loot.addReplacement(m7resources[i]);
						loot.addReplacement(MetaString::RES_NAMES, i);
					}
				}
				ms.addReplacement(loot.buildList());
			}
			break;
		case MISSION_HERO:
			ms.addReplacement(VLC->heroh->heroes[m13489val]->name);
			break;
		case MISSION_PLAYER:
			ms.addReplacement(VLC->generaltexth->colors[m13489val]);
			break;
		default:
			break;
	}
}

void CQuest::getCompletionText (MetaString &iwText, std::vector<Component> &components, bool isCustom, const CGHeroInstance * h) const
{
	iwText << completedText;
	switch (missionType)
	{
		case CQuest::MISSION_LEVEL:
			if (!isCustomComplete)
				iwText.addReplacement(m13489val);
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
						loot.addReplacement(m2stats[i]);
						loot.addReplacement(VLC->generaltexth->primarySkillNames[i]);
					}
				}
				if (!isCustomComplete)
					iwText.addReplacement(loot.buildList());
			}
			break;
		case CQuest::MISSION_ART:
		{
			MetaString loot;
			for (auto & elem : m5arts)
			{
				loot << "%s";
				loot.addReplacement(MetaString::ART_NAMES, elem);
			}
			if (!isCustomComplete)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case CQuest::MISSION_ARMY:
		{
			MetaString loot;
			for (auto & elem : m6creatures)
			{
				loot << "%s";
				loot.addReplacement(elem);
			}
			if (!isCustomComplete)
				iwText.addReplacement(loot.buildList());
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
					loot.addReplacement(m7resources[i]);
					loot.addReplacement(MetaString::RES_NAMES, i);
				}
			}
			if (!isCustomComplete)
				iwText.addReplacement(loot.buildList());
		}
			break;
		case MISSION_KILL_HERO:
		case MISSION_KILL_CREATURE:
			if (!isCustomComplete)
				addReplacements(iwText, completedText);
			break;
		case MISSION_HERO:
			if (!isCustomComplete)
				iwText.addReplacement(VLC->heroh->heroes[m13489val]->name);
			break;
		case MISSION_PLAYER:
			if (!isCustomComplete)
				iwText.addReplacement(VLC->generaltexth->colors[m13489val]);
			break;
	}
}
void CGSeerHut::setObjToKill()
{
	if (quest->missionType == CQuest::MISSION_KILL_CREATURE)
	{
		quest->stackToKill = getCreatureToKill(false)->getStack(SlotID(0)); //FIXME: stacks tend to dissapear (desync?) on server :?
		assert(quest->stackToKill.type);
		quest->stackToKill.count = 0; //no count in info window
		quest->stackDirection = checkDirection();
	}
	else if (quest->missionType == CQuest::MISSION_KILL_HERO)
	{
		quest->heroName = getHeroToKill(false)->name;
		quest->heroPortrait = getHeroToKill(false)->portrait;
	}
}

void CGSeerHut::init()
{
	seerName = VLC->generaltexth->seerNames[ran()%VLC->generaltexth->seerNames.size()];
	quest->textOption = ran() % 3;
}

void CGSeerHut::initObj()
{
	init();

	quest->progress = CQuest::NOT_ACTIVE;
	if (quest->missionType)
	{
		if (!quest->isCustomFirst)
			quest->firstVisitText = VLC->generaltexth->quests[quest->missionType-1][0][quest->textOption];
		if (!quest->isCustomNext)
			quest->nextVisitText = VLC->generaltexth->quests[quest->missionType-1][1][quest->textOption];
		if (!quest->isCustomComplete)
			quest->completedText = VLC->generaltexth->quests[quest->missionType-1][2][quest->textOption];
	}
	else
	{
		quest->progress = CQuest::COMPLETE;
		quest->firstVisitText = VLC->generaltexth->seerEmpty[quest->textOption];
	}

}

void CGSeerHut::getRolloverText (MetaString &text, bool onHover) const
{
	quest->getRolloverText (text, onHover);//TODO: simplify?
	if (!onHover)
		text.addReplacement(seerName);
}

const std::string & CGSeerHut::getHoverText() const
{
	switch (ID)
	{
	case Obj::SEER_HUT:
		if (quest->progress != CQuest::NOT_ACTIVE)
		{
			hoverName = VLC->generaltexth->allTexts[347];
			boost::algorithm::replace_first(hoverName,"%s", seerName);
		}
		else //just seer hut
			hoverName = VLC->generaltexth->names[ID];
		break;
	case Obj::QUEST_GUARD:
		hoverName = VLC->generaltexth->names[ID];
		break;
	default:
        logGlobal->debugStream() << "unrecognized quest object";
	}
	if (quest->progress & quest->missionType) //rollover when the quest is active
	{
		MetaString ms;
		getRolloverText (ms, true);
		hoverName += ms.toString();
	}
	return hoverName;
}

void CQuest::addReplacements(MetaString &out, const std::string &base) const
{
	switch(missionType)
	{
	case MISSION_KILL_CREATURE:
		out.addReplacement(stackToKill);
		if (std::count(base.begin(), base.end(), '%') == 2) //say where is placed monster
		{
			out.addReplacement(VLC->generaltexth->arraytxt[147+stackDirection]);
		}
		break;
	case MISSION_KILL_HERO:
		out.addReplacement(heroName);
		break;
	}
}

bool IQuestObject::checkQuest(const CGHeroInstance* h) const
{
	return quest->checkQuest(h);
}

void IQuestObject::getVisitText (MetaString &text, std::vector<Component> &components, bool isCustom, bool FirstVisit, const CGHeroInstance * h) const
{
	quest->getVisitText (text,components, isCustom, FirstVisit, h);
}

void CGSeerHut::getCompletionText(MetaString &text, std::vector<Component> &components, bool isCustom, const CGHeroInstance * h) const
{
	quest->getCompletionText (text, components, isCustom, h);
	switch (rewardType)
	{
		case EXPERIENCE: components.push_back(Component (Component::EXPERIENCE, 0, h->calculateXp(rVal), 0));
			break;
		case MANA_POINTS: components.push_back(Component (Component::PRIM_SKILL, 5, rVal, 0));
			break;
		case MORALE_BONUS: components.push_back(Component (Component::MORALE, 0, rVal, 0));
			break;
		case LUCK_BONUS: components.push_back(Component (Component::LUCK, 0, rVal, 0));
			break;
		case RESOURCES: components.push_back(Component (Component::RESOURCE, rID, rVal, 0));
			break;
		case PRIMARY_SKILL: components.push_back(Component (Component::PRIM_SKILL, rID, rVal, 0));
			break;
		case SECONDARY_SKILL: components.push_back(Component (Component::SEC_SKILL, rID, rVal, 0));
			break;
		case ARTIFACT: components.push_back(Component (Component::ARTIFACT, rID, 0, 0));
			break;
		case SPELL: components.push_back(Component (Component::SPELL, rID, 0, 0));
			break;
		case CREATURE: components.push_back(Component (Component::CREATURE, rID, rVal, 0));
			break;
	}
}

void CGSeerHut::setPropertyDer (ui8 what, ui32 val)
{
	switch (what)
	{
		case 10:
			quest->progress = static_cast<CQuest::Eprogress>(val);
			break;
	}
}
void CGSeerHut::newTurn() const
{
	if (quest->lastDay >= 0 && quest->lastDay < cb->getDate()-1) //time is up
	{
		cb->setObjProperty (id, 10, CQuest::COMPLETE);
	}

}
void CGSeerHut::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->getOwner();
	if (quest->progress < CQuest::COMPLETE)
	{
		bool firstVisit = !quest->progress;
		bool failRequirements = !checkQuest(h);
		bool isCustom=false;

		if (firstVisit)
		{
			isCustom = quest->isCustomFirst;
			cb->setObjProperty (id, 10, CQuest::IN_PROGRESS);

			AddQuest aq;
			aq.quest = QuestInfo (quest, this, visitablePos());
			aq.player = h->tempOwner;
			cb->sendAndApply (&aq); //TODO: merge with setObjProperty?
		}
		else if (failRequirements)
		{
			isCustom = quest->isCustomNext;
		}

		if (firstVisit || failRequirements)
		{
			getVisitText (iw.text, iw.components, isCustom, firstVisit, h);

			cb->showInfoDialog(&iw);
		}
		if (!failRequirements) // propose completion, also on first visit
		{
			BlockingDialog bd (true, false);
			bd.player = h->getOwner();
			bd.soundID = soundBase::QUEST;

			getCompletionText (bd.text, bd.components, isCustom, h);

			cb->showBlockingDialog (&bd);
			return;
		}
	}
	else
	{
		iw.text << VLC->generaltexth->seerEmpty[quest->textOption];
		if (ID == Obj::SEER_HUT)
			iw.text.addReplacement(seerName);
		cb->showInfoDialog(&iw);
	}
}
int CGSeerHut::checkDirection() const
{
	int3 cord = getCreatureToKill()->pos;
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
void CGSeerHut::finishQuest(const CGHeroInstance * h, ui32 accept) const
{
	if (accept)
	{
		switch (quest->missionType)
		{
			case CQuest::MISSION_ART:
				for (auto & elem : quest->m5arts)
				{
					cb->removeArtifact(ArtifactLocation(h, h->getArtPos(elem, false)));
				}
				break;
			case CQuest::MISSION_ARMY:
					cb->takeCreatures(h->id, quest->m6creatures);
				break;
			case CQuest::MISSION_RESOURCES:
				for (int i = 0; i < 7; ++i)
				{
					cb->giveResource(h->getOwner(), static_cast<Res::ERes>(i), -quest->m7resources[i]);
				}
				break;
			default:
				break;
		}
		cb->setObjProperty (id, 10, CQuest::COMPLETE); //mission complete
		completeQuest(h); //make sure to remove QuestGuard at the very end
	}
}
void CGSeerHut::completeQuest (const CGHeroInstance * h) const //reward
{
	switch (rewardType)
	{
		case EXPERIENCE:
		{
			TExpType expVal = h->calculateXp(rVal);
			cb->changePrimSkill(h, PrimarySkill::EXPERIENCE, expVal, false);
			break;
		}
		case MANA_POINTS:
		{
			cb->setManaPoints(h->id, h->mana+rVal);
			break;
		}
		case MORALE_BONUS: case LUCK_BONUS:
		{
			Bonus hb(Bonus::ONE_WEEK, (rewardType == 3 ? Bonus::MORALE : Bonus::LUCK),
				Bonus::OBJECT, rVal, h->id.getNum(), "", -1);
			GiveBonus gb;
			gb.id = h->id.getNum();
			gb.bonus = hb;
			cb->giveHeroBonus(&gb);
		}
			break;
		case RESOURCES:
			cb->giveResource(h->getOwner(), static_cast<Res::ERes>(rID), rVal);
			break;
		case PRIMARY_SKILL:
			cb->changePrimSkill(h, static_cast<PrimarySkill::PrimarySkill>(rID), rVal, false);
			break;
		case SECONDARY_SKILL:
			cb->changeSecSkill(h, SecondarySkill(rID), rVal, false);
			break;
		case ARTIFACT:
			cb->giveHeroNewArtifact(h, VLC->arth->artifacts[rID],ArtifactPosition::FIRST_AVAILABLE);
			break;
		case SPELL:
		{
			std::set<SpellID> spell;
			spell.insert (SpellID(rID));
			cb->changeSpells(h, true, spell);
		}
			break;
		case CREATURE:
			{
				CCreatureSet creatures;
				creatures.setCreature(SlotID(0), CreatureID(rID), rVal);
				cb->giveCreatures(this, h, creatures, false);
			}
			break;
		default:
			break;
	}
}

const CGHeroInstance * CGSeerHut::getHeroToKill(bool allowNull) const
{
	const CGObjectInstance *o = cb->getObjByQuestIdentifier(quest->m13489val);
	if(allowNull && !o)
		return nullptr;
	assert(o && o->ID == Obj::HERO);
	return static_cast<const CGHeroInstance*>(o);
}

const CGCreature * CGSeerHut::getCreatureToKill(bool allowNull) const
{
	const CGObjectInstance *o = cb->getObjByQuestIdentifier(quest->m13489val);
	if(allowNull && !o)
		return nullptr;
	assert(o && o->ID == Obj::MONSTER);
	return static_cast<const CGCreature*>(o);
}

void CGSeerHut::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	finishQuest(hero, answer);
}

void CGQuestGuard::init()
{
	blockVisit = true;
	quest->textOption = (ran() % 3) + 3; //3-5
}
void CGQuestGuard::completeQuest(const CGHeroInstance *h) const
{
	cb->removeObject(this);
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
	if(!wasVisited(h->tempOwner))
		cb->setObjProperty(id, 10, h->tempOwner.getNum());
	ui32 txt_id;
	if(h->getSecSkillLevel(SecondarySkill(ability))) //you already know this skill
	{
		txt_id =172;
	}
	else if(!h->canLearnSkill()) //already all skills slots used
	{
		txt_id = 173;
	}
	else //give sec skill
	{
		iw.components.push_back(Component(Component::SEC_SKILL, ability, 1, 0));
		txt_id = 171;
		cb->changeSecSkill(h, SecondarySkill(ability), 1, true);
	}

	iw.text.addTxt(MetaString::ADVOB_TXT,txt_id);
	iw.text.addReplacement(MetaString::SEC_SKILL_NAME, ability);
	cb->showInfoDialog(&iw);
}

const std::string & CGWitchHut::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID];
	if(wasVisited(cb->getLocalPlayer()))
	{
		hoverName += "\n" + VLC->generaltexth->allTexts[356]; // + (learn %s)
		boost::algorithm::replace_first(hoverName,"%s",VLC->generaltexth->skillName[ability]);
		const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
		if(h && h->getSecSkillLevel(SecondarySkill(ability))) //hero knows that ability
			hoverName += "\n\n" + VLC->generaltexth->allTexts[357]; // (Already learned)
	}
	return hoverName;
}

bool CGBonusingObject::wasVisited (const CGHeroInstance * h) const
{
	return h->hasBonusFrom(Bonus::OBJECT, ID);
}

void CGBonusingObject::onHeroVisit( const CGHeroInstance * h ) const
{
	bool visited = h->hasBonusFrom(Bonus::OBJECT,ID);
	int messageID=0;
	int bonusMove = 0;
	ui32 descr_id = 0;
	InfoWindow iw;
	iw.player = h->tempOwner;
	GiveBonus gbonus;
	gbonus.id = h->id.getNum();
	gbonus.bonus.duration = Bonus::ONE_BATTLE;
	gbonus.bonus.source = Bonus::OBJECT;
	gbonus.bonus.sid = ID;

	bool second = false;
	Bonus secondBonus;

	switch(ID)
	{
	case Obj::BUOY:
		messageID = 21;
		iw.soundID = soundBase::MORALE;
		gbonus.bonus.type = Bonus::MORALE;
		gbonus.bonus.val = +1;
		descr_id = 94;
		break;
	case Obj::SWAN_POND:
		messageID = 29;
		iw.soundID = soundBase::LUCK;
		gbonus.bonus.type = Bonus::LUCK;
		gbonus.bonus.val = 2;
		descr_id = 67;
		bonusMove = -h->movement;
		break;
	case Obj::FAERIE_RING:
		messageID = 49;
		iw.soundID = soundBase::LUCK;
		gbonus.bonus.type = Bonus::LUCK;
		gbonus.bonus.val = 1;
		descr_id = 71;
		break;
	case Obj::FOUNTAIN_OF_FORTUNE:
		messageID = 55;
		iw.soundID = soundBase::LUCK;
		gbonus.bonus.type = Bonus::LUCK;
		gbonus.bonus.val = rand()%5 - 1;
		descr_id = 69;
		gbonus.bdescr.addReplacement((gbonus.bonus.val<0 ? "-" : "+") + boost::lexical_cast<std::string>(gbonus.bonus.val));
		break;
	case Obj::IDOL_OF_FORTUNE:
		messageID = 62;
		iw.soundID = soundBase::experience;

		gbonus.bonus.val = 1;
		descr_id = 68;
		if(cb->getDate(Date::DAY_OF_WEEK) == 7) //7th day of week
		{
			gbonus.bonus.type = Bonus::MORALE;
			second = true;
			secondBonus = gbonus.bonus;
			secondBonus.type = Bonus::LUCK;
		}
		else
		{
			gbonus.bonus.type = (cb->getDate(Date::DAY_OF_WEEK)%2) ? Bonus::LUCK : Bonus::MORALE;
		}
		break;
	case Obj::MERMAID:
		messageID = 83;
		iw.soundID = soundBase::LUCK;
		gbonus.bonus.type = Bonus::LUCK;
		gbonus.bonus.val = 1;
		descr_id = 72;
		break;
	case Obj::RALLY_FLAG:
		iw.soundID = soundBase::MORALE;
		messageID = 111;
		gbonus.bonus.type = Bonus::MORALE;
		gbonus.bonus.val = 1;
		descr_id = 102;

		second = true;
		secondBonus = gbonus.bonus;
		secondBonus.type = Bonus::LUCK;

		bonusMove = 400;
		break;
	case Obj::OASIS:
		iw.soundID = soundBase::MORALE;
		messageID = 95;
		gbonus.bonus.type = Bonus::MORALE;
		gbonus.bonus.val = 1;
		descr_id = 95;
		bonusMove = 800;
		break;
	case Obj::TEMPLE:
		messageID = 140;
		iw.soundID = soundBase::temple;
		gbonus.bonus.type = Bonus::MORALE;
		if(cb->getDate(Date::DAY_OF_WEEK)==7) //sunday
		{
			gbonus.bonus.val = 2;
			descr_id = 97;
		}
		else
		{
			gbonus.bonus.val = 1;
			descr_id = 96;
		}
		break;
	case Obj::WATERING_HOLE:
		iw.soundID = soundBase::MORALE;
		messageID = 166;
		gbonus.bonus.type = Bonus::MORALE;
		gbonus.bonus.val = 1;
		descr_id = 100;
		bonusMove = 400;
		break;
	case Obj::FOUNTAIN_OF_YOUTH:
		iw.soundID = soundBase::MORALE;
		messageID = 57;
		gbonus.bonus.type = Bonus::MORALE;
		gbonus.bonus.val = 1;
		descr_id = 103;
		bonusMove = 400;
		break;
	case Obj::STABLES:
		iw.soundID = soundBase::STORE;
		bool someUpgradeDone = false;

		for (auto i = h->Slots().begin(); i != h->Slots().end(); ++i)
		{
			if(i->second->type->idNumber == CreatureID::CAVALIER)
			{
				cb->changeStackType(StackLocation(h, i->first), VLC->creh->creatures[CreatureID::CHAMPION]);
				someUpgradeDone = true;
			}
		}
		if (someUpgradeDone)
		{
			messageID = 138;
			iw.components.push_back(Component(Component::CREATURE,11,0,1));
		}
		else
			messageID = 137;

		gbonus.bonus.type = Bonus::LAND_MOVEMENT;
		gbonus.bonus.val = 600;
		bonusMove = 600;
		gbonus.bonus.duration = Bonus::ONE_WEEK;
		//gbonus.bdescr <<  std::pair<ui8,ui32>(6, 100);
		break;
	}
	if (descr_id != 0)
		gbonus.bdescr.addTxt(MetaString::ARRAY_TXT,descr_id);
	assert(messageID);
	if(visited)
	{
		if(ID==Obj::RALLY_FLAG || ID==Obj::OASIS || ID==Obj::MERMAID || ID==Obj::STABLES)
			messageID--;
		else
			messageID++;
	}
	else
	{
		//TODO: fix if second bonus val != main bonus val
		if(gbonus.bonus.type == Bonus::MORALE   ||   secondBonus.type == Bonus::MORALE)
			iw.components.push_back(Component(Component::MORALE,0,gbonus.bonus.val,0));
		if(gbonus.bonus.type == Bonus::LUCK   ||   secondBonus.type == Bonus::LUCK)
			iw.components.push_back(Component(Component::LUCK,0,gbonus.bonus.val,0));
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
	iw.text.addTxt(MetaString::ADVOB_TXT,messageID);
	cb->showInfoDialog(&iw);
}

const std::string & CGBonusingObject::getHoverText() const
{
	const CGHeroInstance *h = cb->getSelectedHero(cb->getCurrentPlayer());
	hoverName = VLC->generaltexth->names[ID];
	if(h)
	{
		bool visited = h->hasBonusFrom(Bonus::OBJECT,ID);
		hoverName += " " + visitedTxt(visited);
	}
	return hoverName;
}

void CGBonusingObject::initObj()
{
	if(ID == Obj::BUOY || ID == Obj::MERMAID)
	{
		blockVisit = true;
	}
}

void CGMagicSpring::onHeroVisit(const CGHeroInstance * h) const
{
	int messageID;

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
	showInfoDialog(h,messageID,soundBase::GENIE);
}

const std::string & CGMagicSpring::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID] + " " + visitedTxt(visited);
	return hoverName;
}

void CGMagicWell::onHeroVisit( const CGHeroInstance * h ) const
{
	int message;

	if(h->hasBonusFrom(Bonus::OBJECT,ID)) //has already visited Well today
	{
		message = 78;//"A second drink at the well in one day will not help you."
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
	showInfoDialog(h,message,soundBase::faerie);
}

const std::string & CGMagicWell::getHoverText() const
{
	getNameVis(hoverName);
	return hoverName;
}

void CGPandoraBox::initObj()
{
	blockVisit = (ID==Obj::PANDORAS_BOX); //block only if it's really pandora's box (events also derive from that class)
	hasGuardians = stacks.size();
}

void CGPandoraBox::onHeroVisit(const CGHeroInstance * h) const
{
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundBase::QUEST;
		bd.text.addTxt (MetaString::ADVOB_TXT, 14);
		cb->showBlockingDialog (&bd);
}

void CGPandoraBox::giveContentsUpToExp(const CGHeroInstance *h) const
{
	cb->removeAfterVisit(this);

	InfoWindow iw;
	iw.player = h->getOwner();

	bool changesPrimSkill = false;
	for (auto & elem : primskills)
	{
		if(elem)
		{
			changesPrimSkill = true;
			break;
		}
	}

	if(gainedExp || changesPrimSkill || abilities.size())
	{
		TExpType expVal = h->calculateXp(gainedExp);
		//getText(iw,afterBattle,175,h); //wtf?
		iw.text.addTxt(MetaString::ADVOB_TXT, 175); //%s learns something
		iw.text.addReplacement(h->name);

		if(expVal)
			iw.components.push_back(Component(Component::EXPERIENCE,0,expVal,0));
		for(int i=0; i<primskills.size(); i++)
			if(primskills[i])
				iw.components.push_back(Component(Component::PRIM_SKILL,i,primskills[i],0));

		for(int i=0; i<abilities.size(); i++)
			iw.components.push_back(Component(Component::SEC_SKILL,abilities[i],abilityLevels[i],0));

		cb->showInfoDialog(&iw);

		//give sec skills
		for(int i=0; i<abilities.size(); i++)
		{
			int curLev = h->getSecSkillLevel(abilities[i]);

			if( (curLev  &&  curLev < abilityLevels[i])	|| (h->canLearnSkill() ))
			{
				cb->changeSecSkill(h,abilities[i],abilityLevels[i],true);
			}
		}

		//give prim skills
		for(int i=0; i<primskills.size(); i++)
			if(primskills[i])
				cb->changePrimSkill(h,static_cast<PrimarySkill::PrimarySkill>(i),primskills[i],false);

		assert(!cb->isVisitCoveredByAnotherQuery(this, h));

		//give exp
		if(expVal)
			cb->changePrimSkill(h, PrimarySkill::EXPERIENCE, expVal, false);
	}

	if(!cb->isVisitCoveredByAnotherQuery(this, h))
		giveContentsAfterExp(h);
	//Otherwise continuation occurs via post-level-up callback.
}

void CGPandoraBox::giveContentsAfterExp(const CGHeroInstance *h) const
{
	bool hadGuardians = hasGuardians; //copy, because flag will be emptied after issuing first post-battle message

	std::string msg = message; //in case box is removed in the meantime
	InfoWindow iw;
	iw.player = h->getOwner();

	if(spells.size())
	{
		std::set<SpellID> spellsToGive;
		iw.components.clear();
		if (spells.size() > 1)
		{
			iw.text.addTxt(MetaString::ADVOB_TXT, 188); //%s learns spells
		}
		else
		{
			iw.text.addTxt(MetaString::ADVOB_TXT, 184); //%s learns a spell
		}
		iw.text.addReplacement(h->name);
		std::vector<ConstTransitivePtr<CSpell> > * sp = &VLC->spellh->spells;
		for(auto i=spells.cbegin(); i != spells.cend(); i++)
		{
			if ((*sp)[*i]->level <= h->getSecSkillLevel(SecondarySkill::WISDOM) + 2) //enough wisdom
			{
				iw.components.push_back(Component(Component::SPELL,*i,0,0));
				spellsToGive.insert(*i);
			}
		}
		if(spellsToGive.size())
		{
			cb->changeSpells(h,true,spellsToGive);
			cb->showInfoDialog(&iw);
		}
	}

	if(manaDiff)
	{
		getText(iw,hadGuardians,manaDiff,176,177,h);
		iw.components.push_back(Component(Component::PRIM_SKILL,5,manaDiff,0));
		cb->showInfoDialog(&iw);
		cb->setManaPoints(h->id, h->mana + manaDiff);
	}

	if(moraleDiff)
	{
		getText(iw,hadGuardians,moraleDiff,178,179,h);
		iw.components.push_back(Component(Component::MORALE,0,moraleDiff,0));
		cb->showInfoDialog(&iw);
		GiveBonus gb;
		gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::MORALE,Bonus::OBJECT,moraleDiff,id.getNum(),"");
		gb.id = h->id.getNum();
		cb->giveHeroBonus(&gb);
	}

	if(luckDiff)
	{
		getText(iw,hadGuardians,luckDiff,180,181,h);
		iw.components.push_back(Component(Component::LUCK,0,luckDiff,0));
		cb->showInfoDialog(&iw);
		GiveBonus gb;
		gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::LUCK,Bonus::OBJECT,luckDiff,id.getNum(),"");
		gb.id = h->id.getNum();
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
		getText(iw,hadGuardians,182,h);
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
		getText(iw,hadGuardians,183,h);
		cb->showInfoDialog(&iw);
	}

	iw.components.clear();
	// 	getText(iw,afterBattle,183,h);
	iw.text.addTxt(MetaString::ADVOB_TXT, 183); //% has found treasure
	iw.text.addReplacement(h->name);
	for(auto & elem : artifacts)
	{
		iw.components.push_back(Component(Component::ARTIFACT,elem,0,0));
		if(iw.components.size() >= 14)
		{
			cb->showInfoDialog(&iw);
			iw.components.clear();
			iw.text.addTxt(MetaString::ADVOB_TXT, 183); //% has found treasure - once more?
			iw.text.addReplacement(h->name);
		}
	}
	if(iw.components.size())
	{
		cb->showInfoDialog(&iw);
	}

	for(int i=0; i<resources.size(); i++)
		if(resources[i])
			cb->giveResource(h->getOwner(),static_cast<Res::ERes>(i),resources[i]);

	for(auto & elem : artifacts)
		cb->giveHeroNewArtifact(h, VLC->arth->artifacts[elem],ArtifactPosition::FIRST_AVAILABLE);

	iw.components.clear();
	iw.text.clear();

	if (creatures.Slots().size())
	{ //this part is taken straight from creature bank
		MetaString loot;
		for(auto & elem : creatures.Slots())
		{ //build list of joined creatures
			iw.components.push_back(Component(*elem.second));
			loot << "%s";
			loot.addReplacement(*elem.second);
		}

		if (creatures.Slots().size() == 1 && creatures.Slots().begin()->second->count == 1)
			iw.text.addTxt(MetaString::ADVOB_TXT, 185);
		else
			iw.text.addTxt(MetaString::ADVOB_TXT, 186);

		iw.text.addReplacement(loot.buildList());
		iw.text.addReplacement(h->name);

		cb->showInfoDialog(&iw);
		cb->giveCreatures(this, h, creatures, true);
	}
	if(!hasGuardians && msg.size())
	{
		iw.text << msg;
		cb->showInfoDialog(&iw);
	}
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

void CGPandoraBox::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const 
{
	if(result.winner)
		return;

	giveContentsUpToExp(hero);
}

void CGPandoraBox::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	if (answer)
	{
		if (stacksCount() > 0) //if pandora's box is protected by army
		{
			showInfoDialog(hero,16,0);
			cb->startBattleI(hero, this); //grants things after battle
		}
		else if (message.size() == 0 && resources.size() == 0
			&& primskills.size() == 0 && abilities.size() == 0
			&& abilityLevels.size() == 0 &&  artifacts.size() == 0
			&& spells.size() == 0 && creatures.Slots().size() > 0
			&& gainedExp == 0 && manaDiff == 0 && moraleDiff == 0 && luckDiff == 0) //if it gives nothing without battle
		{
			showInfoDialog(hero,15,0);
			cb->removeObject(this);
		}
		else //if it gives something without battle
		{
			giveContentsUpToExp(hero);
		}
	}
}

void CGPandoraBox::heroLevelUpDone(const CGHeroInstance *hero) const 
{
	giveContentsAfterExp(hero);
}

void CGEvent::onHeroVisit( const CGHeroInstance * h ) const
{
	if(!(availableFor & (1 << h->tempOwner.getNum())))
		return;
	if(cb->getPlayerSettings(h->tempOwner)->playerID)
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
		cb->startBattleI(h, this);
	}
	else
	{
		giveContentsUpToExp(h);
	}
}

void CGObservatory::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->tempOwner;
	switch (ID)
	{
	case Obj::REDWOOD_OBSERVATORY:
	case Obj::PILLAR_OF_FIRE:
	{
		iw.soundID = soundBase::LIGHTHOUSE;
		iw.text.addTxt(MetaString::ADVOB_TXT,98 + (ID==Obj::PILLAR_OF_FIRE));

		FoWChange fw;
		fw.player = h->tempOwner;
		fw.mode = 1;
		cb->getTilesInRange (fw.tiles, pos, 20, h->tempOwner, 1);
		cb->sendAndApply (&fw);
		break;
	}
	case Obj::COVER_OF_DARKNESS:
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
	if(spell == SpellID::NONE)
	{
        logGlobal->errorStream() << "Not initialized shrine visited!";
		return;
	}

	if(!wasVisited(h->tempOwner))
		cb->setObjProperty(id, 10, h->tempOwner.getNum());

	InfoWindow iw;
	iw.soundID = soundBase::temple;
	iw.player = h->getOwner();
	iw.text.addTxt(MetaString::ADVOB_TXT,127 + ID - 88);
	iw.text.addTxt(MetaString::SPELL_NAME,spell);
	iw.text << ".";

	if(!h->getArt(ArtifactPosition::SPELLBOOK))
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,131);
	}
	else if(ID == Obj::SHRINE_OF_MAGIC_THOUGHT  && !h->getSecSkillLevel(SecondarySkill::WISDOM)) //it's third level spell and hero doesn't have wisdom
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,130);
	}
	else if(vstd::contains(h->spells,spell))//hero already knows the spell
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,174);
	}
	else //give spell
	{
		std::set<SpellID> spells;
		spells.insert(spell);
		cb->changeSpells(h, true, spells);

		iw.components.push_back(Component(Component::SPELL,spell,0,0));
	}

	cb->showInfoDialog(&iw);
}

void CGShrine::initObj()
{
	if(spell == SpellID::NONE) //spell not set
	{
		int level = ID-87;
		std::vector<SpellID> possibilities;
		cb->getAllowedSpells (possibilities, level);

		if(!possibilities.size())
		{
            logGlobal->errorStream() << "Error: cannot init shrine, no allowed spells!";
			return;
		}

		spell = possibilities[ran() % possibilities.size()];
	}
}

const std::string & CGShrine::getHoverText() const
{
	hoverName = VLC->generaltexth->names[ID];
	if(wasVisited(cb->getCurrentPlayer())) //TODO: use local player, not current
	{
		hoverName += "\n" + VLC->generaltexth->allTexts[355]; // + (learn %s)
		boost::algorithm::replace_first(hoverName,"%s", spell.toSpell()->name);
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

	if(ID == Obj::OCEAN_BOTTLE)
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

	if(ID == Obj::OCEAN_BOTTLE)
		cb->removeObject(this);
}

//TODO: remove
//void CGScholar::giveAnyBonus( const CGHeroInstance * h ) const
//{
//
//}

void CGScholar::onHeroVisit( const CGHeroInstance * h ) const
{

	EBonusType type = bonusType;
	int bid = bonusID;
	//check if the bonus if applicable, if not - give primary skill (always possible)
	int ssl = h->getSecSkillLevel(SecondarySkill(bid)); //current sec skill level, used if bonusType == 1
	if((type == SECONDARY_SKILL
			&& ((ssl == 3)  ||  (!ssl  &&  !h->canLearnSkill()))) ////hero already has expert level in the skill or (don't know skill and doesn't have free slot)
		|| (type == SPELL  &&  (!h->getArt(ArtifactPosition::SPELLBOOK) || vstd::contains(h->spells, (ui32) bid)
		|| ( SpellID(bid).toSpell()->level > h->getSecSkillLevel(SecondarySkill::WISDOM) + 2)
		))) //hero doesn't have a spellbook or already knows the spell or doesn't have Wisdom
	{
		type = PRIM_SKILL;
		bid = ran() % GameConstants::PRIMARY_SKILLS;
	}


	InfoWindow iw;
	iw.soundID = soundBase::gazebo;
	iw.player = h->getOwner();
	iw.text.addTxt(MetaString::ADVOB_TXT,115);

	switch (type)
	{
	case PRIM_SKILL:
		cb->changePrimSkill(h,static_cast<PrimarySkill::PrimarySkill>(bid),+1);
		iw.components.push_back(Component(Component::PRIM_SKILL,bid,+1,0));
		break;
	case SECONDARY_SKILL:
		cb->changeSecSkill(h,SecondarySkill(bid),+1);
		iw.components.push_back(Component(Component::SEC_SKILL,bid,ssl+1,0));
		break;
	case SPELL:
		{
			std::set<SpellID> hlp;
			hlp.insert(SpellID(bid));
			cb->changeSpells(h,true,hlp);
			iw.components.push_back(Component(Component::SPELL,bid,0,0));
		}
		break;
	default:
		logGlobal->errorStream() << "Error: wrong bonus type (" << (int)type << ") for Scholar!\n";
		return;
	}

	cb->showInfoDialog(&iw);
	cb->removeObject(this);
}

void CGScholar::initObj()
{
	blockVisit = true;
	if(bonusType == RANDOM)
	{
		bonusType = static_cast<EBonusType>(ran()%3);
		switch(bonusType)
		{
		case PRIM_SKILL:
			bonusID = ran() % GameConstants::PRIMARY_SKILLS;
			break;
		case SECONDARY_SKILL:
			bonusID = ran() % GameConstants::SKILL_QUANTITY;
			break;
		case SPELL:
			std::vector<SpellID> possibilities;
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
		cb->startBattleI(h, this);
		return;
	}

	//New owner.
	if (!ally)
		cb->setOwner(this, h->tempOwner);

	cb->showGarrisonDialog(id, h->id, removableUnits);
}

ui8 CGGarrison::getPassableness() const
{
	if ( !stacksCount() )//empty - anyone can visit
		return GameConstants::ALL_PLAYERS;
	if ( tempOwner == PlayerColor::NEUTRAL )//neutral guarded - no one can visit
		return 0;

	ui8 mask = 0;
	TeamState * ts = cb->gameState()->getPlayerTeam(tempOwner);
	for(PlayerColor it : ts->players)
		mask |= 1<<it.getNum(); //allies - add to possible visitors

	return mask;
}

void CGGarrison::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const 
{
	if (result.winner == 0)
		onHeroVisit(hero);
}

void CGOnceVisitable::onHeroVisit( const CGHeroInstance * h ) const
{
	int sound = soundBase::sound_todo;
	int txtid;

	switch(ID)
	{
	case Obj::CORPSE:
		txtid = 37;
		sound = soundBase::MYSTERY;
		break;
	case Obj::LEAN_TO:
		sound = soundBase::GENIE;
		txtid = 64;
		break;
	case Obj::WAGON:
		sound = soundBase::GENIE;
		txtid = 154;
		break;
	case Obj::WARRIORS_TOMB:
		{
			//ask if player wants to search the Tomb
			BlockingDialog bd(true, false);
			bd.soundID = soundBase::GRAVEYARD;
			bd.player = h->getOwner();
			bd.text.addTxt(MetaString::ADVOB_TXT,161);
			cb->showBlockingDialog(&bd);
			return;
		}
		break;
	default:
        logGlobal->errorStream() << "Error: Unknown object (" << ID <<") treated as CGOnceVisitable!";
		return;
	}

	InfoWindow iw;
	iw.soundID = sound;
	iw.player = h->getOwner();

	if(players.size()) //we have been already visited...
	{
		txtid++;
		if(ID == Obj::WAGON) //wagon has extra text (for finding art) we need to omit
			txtid++;
		iw.text.addTxt(MetaString::ADVOB_TXT, txtid);
	}
	else //first visit - give bonus!
	{
		switch(artOrRes)
		{
		case 0: // first visit but empty
			if (ID == Obj::CORPSE)
				++txtid;
			else
				txtid+=2;
			iw.text.addTxt(MetaString::ADVOB_TXT, txtid);
			break;
		case 1: //art
			iw.components.push_back(Component(Component::ARTIFACT,bonusType,0,0));
			cb->giveHeroNewArtifact(h, VLC->arth->artifacts[bonusType],ArtifactPosition::FIRST_AVAILABLE);
			iw.text.addTxt(MetaString::ADVOB_TXT, txtid);
			if (ID == Obj::CORPSE)
			{
				iw.text << "%s";
				iw.text.addReplacement(MetaString::ART_NAMES, bonusType);
			}
			break;
		case 2: //res
			iw.text.addTxt(MetaString::ADVOB_TXT, txtid);
			iw.components.push_back (Component(Component::RESOURCE, bonusType, bonusVal, 0));
			cb->giveResource(h->getOwner(), static_cast<Res::ERes>(bonusType), bonusVal);
			break;
		}
		if(ID == Obj::WAGON  &&  artOrRes == 1)
		{
			iw.text.localStrings.back().second++;
			iw.text.addReplacement(MetaString::ART_NAMES, bonusType);
		}
	}

	cb->showInfoDialog(&iw);
	cb->setObjProperty(id, 10, h->getOwner().getNum());
}

const std::string & CGOnceVisitable::getHoverText() const
{
	const bool visited = wasVisited(cb->getCurrentPlayer());
	hoverName = VLC->generaltexth->names[ID] + " " + visitedTxt(visited);
	return hoverName;
}

void CGOnceVisitable::initObj()
{
	switch(ID)
	{
	case Obj::CORPSE:
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

	case Obj::LEAN_TO:
		{
			artOrRes = 2;
			bonusType = ran()%6; //any basic resource without gold
			bonusVal = ran()%4 + 1;
		}
		break;

	case Obj::WARRIORS_TOMB:
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

	case Obj::WAGON:
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

void CGOnceVisitable::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	//must have been Tomb
	if(answer)
	{
		InfoWindow iw;
		iw.player = hero->getOwner();
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

			cb->giveHeroNewArtifact(hero, VLC->arth->artifacts[bonusType],ArtifactPosition::FIRST_AVAILABLE);
		}

		if(!hero->hasBonusFrom(Bonus::OBJECT,ID)) //we don't have modifier from this object yet
		{
			//ruin morale
			GiveBonus gb;
			gb.id = hero->id.getNum();
			gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::MORALE,Bonus::OBJECT,-3,id.getNum(),"");
			gb.bdescr.addTxt(MetaString::ARRAY_TXT,104); //Warrior Tomb Visited -3
			cb->giveHeroBonus(&gb);
		}
		cb->showInfoDialog(&iw);
		cb->setObjProperty(id, 10, hero->getOwner().getNum());
	}
}

void CBank::initObj()
{
	index = VLC->objh->bankObjToIndex(this);
	bc = nullptr;
	daycounter = 0;
	multiplier = 1;
}
const std::string & CBank::getHoverText() const
{
	bool visited = (bc == nullptr);
	hoverName = VLC->objh->creBanksNames[index] + " " + visitedTxt(visited);
	return hoverName;
}
void CBank::reset(ui16 var1) //prevents desync
{
	ui8 chance = 0;
	for (auto & elem : VLC->objh->banksInfo[index])
	{
		if (var1 < (chance += elem->chance))
		{
 			bc = elem;
			break;
		}
	}
	artifacts.clear();
}

void CBank::initialize() const
{
	cb->setObjProperty (id, ObjProperty::BANK_RESET, ran()); //synchronous reset
	
	for (ui8 i = 0; i <= 3; i++)
	{
		for (ui8 n = 0; n < bc->artifacts[i]; n++) 
		{
			CArtifact::EartClass artClass;
			switch(i)
			{
			case 0: artClass = CArtifact::ART_TREASURE; break;
			case 1: artClass = CArtifact::ART_MINOR; break;
			case 2: artClass = CArtifact::ART_MAJOR; break;
			case 3: artClass = CArtifact::ART_RELIC; break;
			default: assert(0); continue;
			}

			int artID = cb->getArtSync(ran(), artClass, true);
			cb->setObjProperty(id, ObjProperty::BANK_ADD_ARTIFACT, artID); 
		}
	}

	cb->setObjProperty (id, ObjProperty::BANK_INIT_ARMY, ran()); //get army
}
void CBank::setPropertyDer (ui8 what, ui32 val)
/// random values are passed as arguments and processed identically on all clients
{
	switch (what)
	{
		case ObjProperty::BANK_DAYCOUNTER: //daycounter
			if (val == 0)
				daycounter = 1; //yes, 1
			else
				daycounter++;
			break;
		case ObjProperty::BANK_MULTIPLIER: //multiplier, in percent
			multiplier = val / 100.0;
			break;
		case 13: //bank preset
			bc = VLC->objh->banksInfo[index][val];
			break;
		case ObjProperty::BANK_RESET:
			reset (val%100);
			break;
		case ObjProperty::BANK_CLEAR_CONFIG:
			bc = nullptr;
			break;
		case ObjProperty::BANK_CLEAR_ARTIFACTS: //remove rewards from Derelict Ship
			artifacts.clear();
			break;
		case ObjProperty::BANK_INIT_ARMY: //set ArmedInstance army
			{
				int upgraded = 0;
				if (val%100 < bc->upgradeChance) //once again anti-desync
					upgraded = 1;
				switch (bc->guards.size())
				{
					case 1:
						for	(int i = 0; i < 4; ++i)
							setCreature (SlotID(i), bc->guards[0].first, bc->guards[0].second  / 5 );
						setCreature (SlotID(4), CreatureID(bc->guards[0].first + upgraded), bc->guards[0].second  / 5 );
						break;
					case 4:
					{
						if (bc->guards.back().second) //all stacks are present
						{
							for (auto & elem : bc->guards)
							{
								setCreature (SlotID(stacksCount()), elem.first, elem.second);
							}
						}
						else if (bc->guards[2].second)//Wraiths are present, split two stacks in Crypt
						{
							setCreature (SlotID(0), bc->guards[0].first, bc->guards[0].second  / 2 );
							setCreature (SlotID(1), bc->guards[1].first, bc->guards[1].second / 2);
							setCreature (SlotID(2), CreatureID(bc->guards[2].first + upgraded), bc->guards[2].second);
							setCreature (SlotID(3), bc->guards[1].first, bc->guards[1].second / 2 );
							setCreature (SlotID(4), bc->guards[0].first, bc->guards[0].second - (bc->guards[0].second  / 2) );

						}
						else //split both stacks
						{
							for	(int i = 0; i < 3; ++i) //skellies
								setCreature (SlotID(2*i), bc->guards[0].first, bc->guards[0].second  / 3);
							for	(int i = 0; i < 2; ++i) //zombies
								setCreature (SlotID(2*i+1), bc->guards[1].first, bc->guards[1].second  / 2);
						}
					}
						break;
					default:
                        logGlobal->warnStream() << "Error: Unexpected army data: " << bc->guards.size() <<" items found";
						return;
				}
			}
			break;
		case ObjProperty::BANK_ADD_ARTIFACT: //add Artifact
		{
			artifacts.push_back (val);
			break;
		}
	}
}

void CBank::newTurn() const
{
	if (bc == nullptr)
	{
		if (cb->getDate() == 1)
			initialize(); //initialize on first day
		else if (daycounter >= 28 && (subID < 13 || subID > 16)) //no reset for Emissaries
		{
			initialize();
			cb->setObjProperty (id, ObjProperty::BANK_DAYCOUNTER, 0); //daycounter 0
			if (ID == Obj::DERELICT_SHIP && cb->getDate() > 1)
			{
				cb->setObjProperty (id, ObjProperty::BANK_MULTIPLIER, 0);//ugly hack to make derelict ships usable only once
				cb->setObjProperty (id, ObjProperty::BANK_CLEAR_ARTIFACTS, 0);
			}
		}
		else
			cb->setObjProperty (id, ObjProperty::BANK_DAYCOUNTER, 1); //daycounter++
	}
}
bool CBank::wasVisited (PlayerColor player) const
{
	return !bc;
}

void CBank::onHeroVisit (const CGHeroInstance * h) const
{
	if (bc)
	{
		int banktext = 0;
		switch (ID)
		{
		case Obj::CREATURE_BANK:
			banktext = 32;
			break;
		case Obj::DERELICT_SHIP:
			banktext = 41;
			break;
		case Obj::DRAGON_UTOPIA:
			banktext = 47;
			break;
		case Obj::CRYPT:
			banktext = 119;
			break;
		case Obj::SHIPWRECK:
			banktext = 122;
			break;
		}
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundBase::ROGUE;
		bd.text.addTxt(MetaString::ADVOB_TXT,banktext);
		if (ID == Obj::CREATURE_BANK)
			bd.text.addReplacement(VLC->objh->creBanksNames[index]);
		cb->showBlockingDialog (&bd);
	}
	else
	{
		InfoWindow iw;
		iw.soundID = soundBase::GRAVEYARD;
		iw.player = h->getOwner();
		if (ID == Obj::CRYPT) //morale penalty for empty Crypt
		{
			GiveBonus gbonus;
			gbonus.id = h->id.getNum();
			gbonus.bonus.duration = Bonus::ONE_BATTLE;
			gbonus.bonus.source = Bonus::OBJECT;
			gbonus.bonus.sid = ID;
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
			iw.text.addReplacement(VLC->objh->creBanksNames[index]);
		}
		cb->showInfoDialog(&iw);
	}
}

void CBank::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const 
{
	if (result.winner == 0)
	{
		int textID = -1;
		InfoWindow iw;
		iw.player = hero->getOwner();
		MetaString loot;

		switch (ID)
		{
		case Obj::CREATURE_BANK: case Obj::DRAGON_UTOPIA:
			textID = 34;
			break;
		case Obj::DERELICT_SHIP:
			if (multiplier)
				textID = 43;
			else
			{
				GiveBonus gbonus;
				gbonus.id = hero->id.getNum();
				gbonus.bonus.duration = Bonus::ONE_BATTLE;
				gbonus.bonus.source = Bonus::OBJECT;
				gbonus.bonus.sid = ID;
				gbonus.bdescr << "\n" << VLC->generaltexth->arraytxt[101];
				gbonus.bonus.type = Bonus::MORALE;
				gbonus.bonus.val = -1;
				cb->giveHeroBonus(&gbonus);
				textID = 42;
				iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
			}
			break;
		case Obj::CRYPT:
			if (bc->resources.size() != 0)
				textID = 121;
			else
			{
				iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
				GiveBonus gbonus;
				gbonus.id = hero->id.getNum();
				gbonus.bonus.duration = Bonus::ONE_BATTLE;
				gbonus.bonus.source = Bonus::OBJECT;
				gbonus.bonus.sid = ID;
				gbonus.bdescr << "\n" << VLC->generaltexth->arraytxt[ID];
				gbonus.bonus.type = Bonus::MORALE;
				gbonus.bonus.val = -1;
				cb->giveHeroBonus(&gbonus);
				textID = 120;
				iw.components.push_back (Component (Component::MORALE, 0 , -1, 0));
			}
			break;
		case Obj::SHIPWRECK:
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
					loot.addReplacement(iw.components.back().val);
					loot.addReplacement(MetaString::RES_NAMES, iw.components.back().subtype);
					cb->giveResource (hero->getOwner(), static_cast<Res::ERes>(it), bc->resources[it]);
				}
			}
		}
		//grant artifacts
		for (auto & elem : artifacts)
		{
			iw.components.push_back (Component (Component::ARTIFACT, elem, 0, 0));
			loot << "%s";
			loot.addReplacement(MetaString::ART_NAMES, elem);
			cb->giveHeroNewArtifact (hero, VLC->arth->artifacts[elem], ArtifactPosition::FIRST_AVAILABLE);
		}
		//display loot
		if (!iw.components.empty())
		{
			iw.text.addTxt (MetaString::ADVOB_TXT, textID);
			if (textID == 34)
			{
				iw.text.addReplacement(MetaString::CRE_PL_NAMES, result.casualties[1].begin()->first);
				iw.text.addReplacement(loot.buildList());
			}
			cb->showInfoDialog(&iw);
		}
		loot.clear();
		iw.components.clear();
		iw.text.clear();

		//grant creatures
		CCreatureSet ourArmy;
		for (auto it = bc->creatures.cbegin(); it != bc->creatures.cend(); it++)
		{
			SlotID slot = ourArmy.getSlotFor(it->first);
			ourArmy.addToSlot(slot, it->first, it->second);
		}
		for (auto & elem : ourArmy.Slots())
		{
			iw.components.push_back(Component(*elem.second));
			loot << "%s";
			loot.addReplacement(*elem.second);
		}

		if (ourArmy.Slots().size())
		{
			if (ourArmy.Slots().size() == 1 && ourArmy.Slots().begin()->second->count == 1)
				iw.text.addTxt (MetaString::ADVOB_TXT, 185);
			else
				iw.text.addTxt (MetaString::ADVOB_TXT, 186);

			iw.text.addReplacement(loot.buildList());
			iw.text.addReplacement(hero->name);
			cb->showInfoDialog(&iw);
			cb->giveCreatures(this, hero, ourArmy, false);
		}
		cb->setObjProperty (id, ObjProperty::BANK_CLEAR_CONFIG, 0); //bc = nullptr
	}
}

void CBank::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	if (answer)
	{
		cb->startBattleI(hero, this, true);
	}
}

void CGPyramid::initObj()
{
	std::vector<SpellID> available;
	cb->getAllowedSpells (available, 5);
	if (available.size())
	{
		bc = VLC->objh->banksInfo[21].front(); //TODO: remove hardcoded value?
		spell = (available[ran()%available.size()]);
	}
	else
	{
        logGlobal->errorStream() <<"No spells available for Pyramid! Object set to empty.";
	}
	setPropertyDer (ObjProperty::BANK_INIT_ARMY,ran()); //set guards at game start
}
const std::string & CGPyramid::getHoverText() const
{
	hoverName = VLC->objh->creBanksNames[21]+ " " + visitedTxt((bc==nullptr));
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
		cb->showBlockingDialog(&bd);
	}
	else
	{
		InfoWindow iw;
		iw.player = h->getOwner();
		iw.text << VLC->generaltexth->advobtxt[107];
		iw.components.push_back (Component (Component::LUCK, 0 , -2, 0));
		GiveBonus gb;
		gb.bonus = Bonus(Bonus::ONE_BATTLE,Bonus::LUCK,Bonus::OBJECT,-2,id.getNum(),VLC->generaltexth->arraytxt[70]);
		gb.id = h->id.getNum();
		cb->giveHeroBonus(&gb);
		cb->showInfoDialog(&iw);
	}
}

void CGPyramid::battleFinished(const CGHeroInstance *hero, const BattleResult &result) const 
{
	if (result.winner == 0)
	{
		InfoWindow iw;
		iw.player = hero->getOwner();
		iw.text.addTxt (MetaString::ADVOB_TXT, 106);
		iw.text.addTxt (MetaString::SPELL_NAME, spell);
		if (!hero->getArt(ArtifactPosition::SPELLBOOK))
			iw.text.addTxt (MetaString::ADVOB_TXT, 109); //no spellbook
		else if (hero->getSecSkillLevel(SecondarySkill::WISDOM) < 3)
			iw.text.addTxt (MetaString::ADVOB_TXT, 108); //no expert Wisdom
		else
		{
			std::set<SpellID> spells;
			spells.insert (SpellID(spell));
			cb->changeSpells (hero, true, spells);
			iw.components.push_back(Component (Component::SPELL, spell, 0, 0));
		}
		cb->showInfoDialog(&iw);
		cb->setObjProperty (id, ObjProperty::BANK_CLEAR_CONFIG, 0);
	}
}

void CGKeys::setPropertyDer (ui8 what, ui32 val) //101-108 - enable key for player 1-8
{
	if (what >= 101 && what <= (100 + PlayerColor::PLAYER_LIMIT_I))
		playerKeyMap.find(PlayerColor(what-101))->second.insert((ui8)val);
}

bool CGKeys::wasMyColorVisited (PlayerColor player) const
{
	if (vstd::contains(playerKeyMap[player], subID)) //creates set if it's not there
		return true;
	else
		return false;
}

const std::string& CGKeys::getHoverText() const
{
	bool visited = wasMyColorVisited (cb->getLocalPlayer());
	hoverName = getName() + "\n" + visitedTxt(visited);
	return hoverName;
}


const std::string CGKeys::getName() const
{
	std::string name;
	name = VLC->generaltexth->tentColors[subID] + " " + VLC->generaltexth->names[ID];
	return name;
}

bool CGKeymasterTent::wasVisited (PlayerColor player) const
{
	return wasMyColorVisited (player);
}

void CGKeymasterTent::onHeroVisit( const CGHeroInstance * h ) const
{
	int txt_id;
	if (!wasMyColorVisited (h->getOwner()) )
	{
		cb->setObjProperty(id, h->tempOwner.getNum()+101, subID);
		txt_id=19;
	}
	else
		txt_id=20;
    showInfoDialog(h,txt_id,soundBase::CAVEHEAD);
}

void CGBorderGuard::initObj()
{
	//ui32 m13489val = subID; //store color as quest info
	blockVisit = true;
}

void CGBorderGuard::getVisitText (MetaString &text, std::vector<Component> &components, bool isCustom, bool FirstVisit, const CGHeroInstance * h) const
{
	text << std::pair<ui8,ui32>(11,18);
}

void CGBorderGuard::getRolloverText (MetaString &text, bool onHover) const
{
	if (!onHover)
		text << VLC->generaltexth->tentColors[subID] << " " << VLC->generaltexth->names[Obj::KEYMASTER];
}

bool CGBorderGuard::checkQuest (const CGHeroInstance * h) const
{
	return wasMyColorVisited (h->tempOwner);
}

void CGBorderGuard::onHeroVisit( const CGHeroInstance * h ) const
{
	if (wasMyColorVisited (h->getOwner()) )
	{
		BlockingDialog bd (true, false);
		bd.player = h->getOwner();
		bd.soundID = soundBase::QUEST;
		bd.text.addTxt (MetaString::ADVOB_TXT, 17);
		cb->showBlockingDialog (&bd);
	}
	else
	{
		showInfoDialog(h,18,soundBase::CAVEHEAD);

		AddQuest aq;
		aq.quest = QuestInfo (quest, this, visitablePos());
		aq.player = h->tempOwner;
		cb->sendAndApply (&aq);
		//TODO: add this quest only once OR check for multiple instances later
	}
}

void CGBorderGuard::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	if (answer)
		cb->removeObject(this);
}

void CGBorderGate::onHeroVisit( const CGHeroInstance * h ) const //TODO: passability
{
	if (!wasMyColorVisited (h->getOwner()) )
	{
		showInfoDialog(h,18,0);

		AddQuest aq;
		aq.quest = QuestInfo (quest, this, visitablePos());
		aq.player = h->tempOwner;
		cb->sendAndApply (&aq);
	}
}

ui8 CGBorderGate::getPassableness() const
{
	ui8 ret = 0;
	for (int i = 0; i < PlayerColor::PLAYER_LIMIT_I; i++)
		ret |= wasMyColorVisited(PlayerColor(i))<<i;
	return ret;
}

void CGMagi::initObj()
{
	if (ID == Obj::EYE_OF_MAGI)
	{
		blockVisit = true;
		eyelist[subID].push_back(id);
	}
}
void CGMagi::onHeroVisit(const CGHeroInstance * h) const
{
	if (ID == Obj::HUT_OF_MAGI)
	{
		showInfoDialog(h, 61, soundBase::LIGHTHOUSE);

		if (!eyelist[subID].empty())
		{
			CenterView cv;
			cv.player = h->tempOwner;
			cv.focusTime = 2000;

			FoWChange fw;
			fw.player = h->tempOwner;
			fw.mode = 1;

			for(auto it : eyelist[subID])
			{
				const CGObjectInstance *eye = cb->getObj(it);

				cb->getTilesInRange (fw.tiles, eye->pos, 10, h->tempOwner, 1);
				cb->sendAndApply(&fw);
				cv.pos = eye->pos;

				cb->sendAndApply(&cv);
			}
			cv.pos = h->getPosition(false);
			cb->sendAndApply(&cv);
		}
	}
	else if (ID == Obj::EYE_OF_MAGI)
	{
		showInfoDialog(h,48,soundBase::invalid);
	}

}
void CGBoat::initObj()
{
	hero = nullptr;
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
	InfoWindow iw;
	iw.soundID = soundBase::DANGER;
	iw.player = h->tempOwner;
	if(h->hasBonusFrom(Bonus::OBJECT,ID)) //has already visited Sirens
	{
		iw.text.addTxt(MetaString::ADVOB_TXT,133);
	}
	else
	{
		giveDummyBonus(h->id, Bonus::ONE_BATTLE);
		TExpType xp = 0;

		for (auto i = h->Slots().begin(); i != h->Slots().end(); i++)
		{
			TQuantity drown = i->second->count * 0.3;
			if(drown)
			{
				cb->changeStackCount(StackLocation(h, i->first), -drown);
				xp += drown * i->second->type->valOfBonuses(Bonus::STACK_HEALTH);
			}
		}

		if(xp)
		{
			xp = h->calculateXp(xp);
			iw.text.addTxt(MetaString::ADVOB_TXT,132);
			iw.text.addReplacement(xp);
			cb->changePrimSkill(h, PrimarySkill::EXPERIENCE, xp, false);
		}
		else
		{
			iw.text.addTxt(MetaString::ADVOB_TXT,134);
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
//		if((tile = IObjectInterface::cb->getTile(o->pos + offsets[i]))  &&  tile->terType == TerrainTile::water) //tile is in the map and is water
//			return true;
//	return false;
//}

int3 IBoatGenerator::bestLocation() const
{
	std::vector<int3> offsets;
	getOutOffsets(offsets);

	for (auto & offset : offsets)
	{
		if (const TerrainTile *tile = IObjectInterface::cb->getTile(o->pos + offset, false)) //tile is in the map
		{
            if (tile->terType == ETerrainType::WATER  &&  (!tile->blocked || tile->blockingObjects.front()->ID == 8)) //and is water and is not blocked or is blocked by boat
				return o->pos + offset;
		}
	}
	return int3 (-1,-1,-1);
}

IBoatGenerator::EGeneratorState IBoatGenerator::shipyardStatus() const
{
	int3 tile = bestLocation();
	const TerrainTile *t = IObjectInterface::cb->getTile(tile);
	if(!t)
		return TILE_BLOCKED; //no available water
	else if(!t->blockingObjects.size())
		return GOOD; //OK
	else if(t->blockingObjects.front()->ID == Obj::BOAT)
		return BOAT_ALREADY_BUILT; //blocked with boat
	else
		return TILE_BLOCKED; //blocked
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
	switch(shipyardStatus())
	{
	case BOAT_ALREADY_BUILT:
		out.addTxt(MetaString::GENERAL_TXT, 51);
		break;
	case TILE_BLOCKED:
		if(visitor)
		{
			out.addTxt(MetaString::GENERAL_TXT, 134);
			out.addReplacement(visitor->name);
		}
		else
			out.addTxt(MetaString::ADVOB_TXT, 189);
		break;
	case NO_WATER:
        logGlobal->errorStream() << "Shipyard without water!!! " << o->pos << "\t" << o->id;
		return;
	}
}

void IShipyard::getBoatCost( std::vector<si32> &cost ) const
{
	cost.resize(GameConstants::RESOURCE_QUANTITY);
	cost[Res::WOOD] = 10;
	cost[Res::GOLD] = 1000;
}

IShipyard::IShipyard(const CGObjectInstance *O)
	: IBoatGenerator(O)
{
}

IShipyard * IShipyard::castFrom( CGObjectInstance *obj )
{
	if(!obj)
		return nullptr;

	if(obj->ID == Obj::TOWN)
	{
		return static_cast<CGTownInstance*>(obj);
	}
	else if(obj->ID == Obj::SHIPYARD)
	{
		return static_cast<CGShipyard*>(obj);
	}
	else
	{
		return nullptr;
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
		cb->setOwner(this, h->tempOwner);

	auto s = shipyardStatus();
	if(s != IBoatGenerator::GOOD)
	{
		InfoWindow iw;
		iw.player = tempOwner;
		getProblemText(iw.text, h);
		cb->showInfoDialog(&iw);
	}
	else
	{
		openWindow(OpenWindow::SHIPYARD_WINDOW,id.getNum(),h->id.getNum());
	}
}

void CCartographer::onHeroVisit( const CGHeroInstance * h ) const
{
	if (!wasVisited (h->getOwner()) ) //if hero has not visited yet this cartographer
	{
		if (cb->getResource(h->tempOwner, Res::GOLD) >= 1000) //if he can afford a map
		{
			//ask if he wants to buy one
			int text=0;
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
                    logGlobal->warnStream() << "Unrecognized subtype of cartographer";
			}
			assert(text);
			BlockingDialog bd (true, false);
			bd.player = h->getOwner();
			bd.soundID = soundBase::LIGHTHOUSE;
			bd.text.addTxt (MetaString::ADVOB_TXT, text);
			cb->showBlockingDialog (&bd);
		}
		else //if he cannot afford
		{
			showInfoDialog(h,28,soundBase::CAVEHEAD);
		}
	}
	else //if he already visited carographer
	{
		showInfoDialog(h,24,soundBase::CAVEHEAD);
	}
}

void CCartographer::blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const 
{
	if (answer) //if hero wants to buy map
	{
		cb->giveResource (hero->tempOwner, Res::GOLD, -1000);
		FoWChange fw;
		fw.mode = 1;
		fw.player = hero->tempOwner;

		//subIDs of different types of cartographers:
		//water = 0; land = 1; underground = 2;
		cb->getAllTiles (fw.tiles, hero->tempOwner, subID - 1, !subID + 1); //reveal appropriate tiles
		cb->sendAndApply (&fw);
		cb->setObjProperty (id, 10, hero->tempOwner.getNum());
	}
}

void CGDenOfthieves::onHeroVisit (const CGHeroInstance * h) const
{
	cb->showThievesGuildWindow(h->tempOwner, id);
}

void CGObelisk::onHeroVisit( const CGHeroInstance * h ) const
{
	InfoWindow iw;
	iw.player = h->tempOwner;
	TeamState *ts = cb->gameState()->getPlayerTeam(h->tempOwner);
	assert(ts);
	TeamID team = ts->id;

	if(!wasVisited(team))
	{
		iw.text.addTxt(MetaString::ADVOB_TXT, 96);
		cb->sendAndApply(&iw);

		cb->setObjProperty(id, 20, h->tempOwner.getNum()); //increment general visited obelisks counter

		openWindow(OpenWindow::PUZZLE_MAP, h->tempOwner.getNum());

		cb->setObjProperty(id, 10, h->tempOwner.getNum()); //mark that particular obelisk as visited
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
	bool visited = wasVisited(cb->getLocalPlayer());
	hoverName = VLC->generaltexth->names[ID] + " " + visitedTxt(visited);
	return hoverName;
}

void CGObelisk::setPropertyDer( ui8 what, ui32 val )
{
	CPlayersVisited::setPropertyDer(what, val);
	switch(what)
	{
	case 20:
		assert(val < PlayerColor::PLAYER_LIMIT_I);
		visited[TeamID(val)]++;

		if(visited[TeamID(val)] > obeliskCount)
		{
            logGlobal->errorStream() << "Error: Visited " << visited[TeamID(val)] << "\t\t" << obeliskCount;
			assert(0);
		}

		break;
	}
}

void CGLighthouse::onHeroVisit( const CGHeroInstance * h ) const
{
	if(h->tempOwner != tempOwner)
	{
		PlayerColor oldOwner = tempOwner;
		cb->setOwner(this,h->tempOwner); //not ours? flag it!
		showInfoDialog(h,69,soundBase::LIGHTHOUSE);
		giveBonusTo(h->tempOwner);

		if(oldOwner < PlayerColor::PLAYER_LIMIT) //remove bonus from old owner
		{
			RemoveBonus rb(RemoveBonus::PLAYER);
			rb.whoID = oldOwner.getNum();
			rb.source = Bonus::OBJECT;
			rb.id = id.getNum();
			cb->sendAndApply(&rb);
		}
	}
}

void CGLighthouse::initObj()
{
	if(tempOwner < PlayerColor::PLAYER_LIMIT)
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

void CGLighthouse::giveBonusTo( PlayerColor player ) const
{
	GiveBonus gb(GiveBonus::PLAYER);
	gb.bonus.type = Bonus::SEA_MOVEMENT;
	gb.bonus.val = 500;
	gb.id = player.getNum();
	gb.bonus.duration = Bonus::PERMANENT;
	gb.bonus.source = Bonus::OBJECT;
	gb.bonus.sid = id.getNum();
	cb->sendAndApply(&gb);
}

void CArmedInstance::randomizeArmy(int type)
{
	int max = VLC->creh->creatures.size();
	for (auto & elem : stacks)
	{
		int randID = elem.second->idRand;
		if(randID > max)
		{
			int level = (randID-VLC->creh->creatures.size()) / 2 -1;
			bool upgrade = !(randID % 2);
			elem.second->setType(VLC->townh->factions[type]->town->creatures[level][upgrade]);
			randID = -1;
		}

		assert(elem.second->armyObj == this);
	}
	return;
}

CArmedInstance::CArmedInstance()
{
	battle = nullptr;
}

//int CArmedInstance::valOfGlobalBonuses(CSelector selector) const
//{
////	if (tempOwner != NEUTRAL_PLAYER)
//	return cb->gameState()->players[tempOwner].valOfBonuses(selector);
//}

void CArmedInstance::updateMoraleBonusFromArmy()
{
	if(!validTypes(false)) //object not randomized, don't bother
		return;

	Bonus *b = getBonusList().getFirst(Selector::sourceType(Bonus::ARMY).And(Selector::type(Bonus::MORALE)));
	if(!b)
	{
		b = new Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, 0, -1);
		addNewBonus(b);
	}

	//number of alignments and presence of undead
	std::set<TFaction> factions;
	bool hasUndead = false;

	for(auto slot : Slots())
	{
		const CStackInstance * inst = slot.second;
		const CCreature * creature  = VLC->creh->creatures[inst->getCreatureID()];

		factions.insert(creature->faction);
		// Check for undead flag instead of faction (undead mummies are neutral)
		hasUndead |= inst->hasBonusOfType(Bonus::UNDEAD);
	}

	size_t factionsInArmy = factions.size(); //town garrison seems to take both sets into account

	// Take Angelic Alliance troop-mixing freedom of non-evil units into account.
	if (hasBonusOfType(Bonus::NONEVIL_ALIGNMENT_MIX))
	{
		size_t mixableFactions = 0;

		for(TFaction f : factions)
		{
			if (VLC->townh->factions[f]->alignment != EAlignment::EVIL)
				mixableFactions++;
		}
		if (mixableFactions > 0)
			factionsInArmy -= mixableFactions - 1;
	}

	if(factionsInArmy == 1)
	{
		b->val = +1;
		b->description = VLC->generaltexth->arraytxt[115]; //All troops of one alignment +1
	}
	else if (factions.size()) // no bonus from empty garrison
	{
	 	b->val = 2 - factionsInArmy;
		b->description = boost::str(boost::format(VLC->generaltexth->arraytxt[114]) % factionsInArmy % b->val); //Troops of %d alignments %d
	}
	boost::algorithm::trim(b->description);

	//-1 modifier for any Undead unit in army
	const ui8 UNDEAD_MODIFIER_ID = -2;
	Bonus *undeadModifier = getBonusList().getFirst(Selector::source(Bonus::ARMY, UNDEAD_MODIFIER_ID));
 	if(hasUndead)
	{
		if(!undeadModifier)
			addNewBonus(new Bonus(Bonus::PERMANENT, Bonus::MORALE, Bonus::ARMY, -1, UNDEAD_MODIFIER_ID, VLC->generaltexth->arraytxt[116]));
	}
	else if(undeadModifier)
		removeBonus(undeadModifier);

}

void CArmedInstance::armyChanged()
{
	updateMoraleBonusFromArmy();
}

CBonusSystemNode * CArmedInstance::whereShouldBeAttached(CGameState *gs)
{
	if(tempOwner < PlayerColor::PLAYER_LIMIT)
		return gs->getPlayer(tempOwner);
	else
		return &gs->globalEffects;
}

CBonusSystemNode * CArmedInstance::whatShouldBeAttached()
{
	return this;
}

bool IMarket::getOffer(int id1, int id2, int &val1, int &val2, EMarketMode::EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		{
			double effectiveness = std::min((getMarketEfficiency() + 1.0) / 20.0, 0.5);

			double r = VLC->objh->resVals[id1], //value of given resource
				g = VLC->objh->resVals[id2] / effectiveness; //value of wanted resource

			if(r>g) //if given resource is more expensive than wanted
			{
				val2 = ceil(r / g);
				val1 = 1;
			}
			else //if wanted resource is more expensive
			{
				val1 = (g / r) + 0.5;
				val2 = 1;
			}
		}
		break;
	case EMarketMode::CREATURE_RESOURCE:
		{
			const double effectivenessArray[] = {0.0, 0.3, 0.45, 0.50, 0.65, 0.7, 0.85, 0.9, 1.0};
			double effectiveness = effectivenessArray[std::min(getMarketEfficiency(), 8)];

			double r = VLC->creh->creatures[id1]->cost[6], //value of given creature in gold
				g = VLC->objh->resVals[id2] / effectiveness; //value of wanted resource

			if(r>g) //if given resource is more expensive than wanted
			{
				val2 = ceil(r / g);
				val1 = 1;
			}
			else //if wanted resource is more expensive
			{
				val1 = (g / r) + 0.5;
				val2 = 1;
			}
		}
		break;
	case EMarketMode::RESOURCE_PLAYER:
		val1 = 1;
		val2 = 1;
		break;
	case EMarketMode::RESOURCE_ARTIFACT:
		{
			double effectiveness = std::min((getMarketEfficiency() + 3.0) / 20.0, 0.6);
			double r = VLC->objh->resVals[id1], //value of offered resource
				g = VLC->arth->artifacts[id2]->price / effectiveness; //value of bought artifact in gold

			if(id1 != 6) //non-gold prices are doubled
				r /= 2;

			val1 = std::max(1, (int)((g / r) + 0.5)); //don't sell arts for less than 1 resource
			val2 = 1;
		}
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		{
			double effectiveness = std::min((getMarketEfficiency() + 3.0) / 20.0, 0.6);
			double r = VLC->arth->artifacts[id1]->price * effectiveness,
				g = VLC->objh->resVals[id2];

// 			if(id2 != 6) //non-gold prices are doubled
// 				r /= 2;

			val1 = 1;
			val2 = std::max(1, (int)((r / g) + 0.5)); //at least one resource is given in return
		}
		break;
	case EMarketMode::CREATURE_EXP:
		{
			val1 = 1;
			val2 = (VLC->creh->creatures[id1]->AIValue / 40) * 5;
		}
		break;
	case EMarketMode::ARTIFACT_EXP:
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

bool IMarket::allowsTrade(EMarketMode::EMarketMode mode) const
{
	return false;
}

int IMarket::availableUnits(EMarketMode::EMarketMode mode, int marketItemSerial) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::ARTIFACT_RESOURCE:
	case EMarketMode::CREATURE_RESOURCE:
			return -1;
	default:
			return 1;
	}
}

std::vector<int> IMarket::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	std::vector<int> ret;
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::ARTIFACT_RESOURCE:
	case EMarketMode::CREATURE_RESOURCE:
		for (int i = 0; i < 7; i++)
			ret.push_back(i);
	default:
		break;
	}
	return ret;
}

const IMarket * IMarket::castFrom(const CGObjectInstance *obj, bool verbose /*= true*/)
{
	switch(obj->ID)
	{
	case Obj::TOWN:
		return static_cast<const CGTownInstance*>(obj);
	case Obj::ALTAR_OF_SACRIFICE:
	case Obj::BLACK_MARKET:
	case Obj::TRADING_POST:
	case Obj::TRADING_POST_SNOW:
	case Obj::FREELANCERS_GUILD:
		return static_cast<const CGMarket*>(obj);
	case Obj::UNIVERSITY:
		return static_cast<const CGUniversity*>(obj);
	default:
		if(verbose)
            logGlobal->errorStream() << "Cannot cast to IMarket object with ID " << obj->ID;
		return nullptr;
	}
}

IMarket::IMarket(const CGObjectInstance *O)
	:o(O)
{

}

std::vector<EMarketMode::EMarketMode> IMarket::availableModes() const
{
	std::vector<EMarketMode::EMarketMode> ret;
	for (int i = 0; i < EMarketMode::MARTKET_AFTER_LAST_PLACEHOLDER; i++)
		if(allowsTrade((EMarketMode::EMarketMode)i))
			ret.push_back((EMarketMode::EMarketMode)i);

	return ret;
}

void CGMarket::onHeroVisit(const CGHeroInstance * h) const
{
	openWindow(OpenWindow::MARKET_WINDOW,id.getNum(),h->id.getNum());
}

int CGMarket::getMarketEfficiency() const
{
	return 5;
}

bool CGMarket::allowsTrade(EMarketMode::EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::RESOURCE_PLAYER:
		switch(ID)
		{
		case Obj::TRADING_POST:
		case Obj::TRADING_POST_SNOW:
			return true;
		default:
			return false;
		}
	case EMarketMode::CREATURE_RESOURCE:
		return ID == Obj::FREELANCERS_GUILD;
	//case ARTIFACT_RESOURCE:
	case EMarketMode::RESOURCE_ARTIFACT:
		return ID == Obj::BLACK_MARKET;
	case EMarketMode::ARTIFACT_EXP:
	case EMarketMode::CREATURE_EXP:
		return ID == Obj::ALTAR_OF_SACRIFICE; //TODO? check here for alignment of visiting hero? - would not be coherent with other checks here
	case EMarketMode::RESOURCE_SKILL:
		return ID == Obj::UNIVERSITY;
	default:
		return false;
	}
}

int CGMarket::availableUnits(EMarketMode::EMarketMode mode, int marketItemSerial) const
{
	return -1;
}

std::vector<int> CGMarket::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
	case EMarketMode::RESOURCE_PLAYER:
		return IMarket::availableItemsIds(mode);
	default:
		return std::vector<int>();
	}
}

CGMarket::CGMarket()
	:IMarket(this)
{
}

std::vector<int> CGBlackMarket::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	switch(mode)
	{
	case EMarketMode::ARTIFACT_RESOURCE:
		return IMarket::availableItemsIds(mode);
	case EMarketMode::RESOURCE_ARTIFACT:
		{
			std::vector<int> ret;
			for(const CArtifact *a : artifacts)
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
	if(cb->getDate(Date::DAY_OF_MONTH) != 1) //new month
		return;

	SetAvailableArtifacts saa;
	saa.id = id.getNum();
	cb->pickAllowedArtsSet(saa.arts);
	cb->sendAndApply(&saa);
}

void CGUniversity::initObj()
{
	std::vector <int> toChoose;
	for (int i=0; i<GameConstants::SKILL_QUANTITY; i++)
		if (cb->isAllowed(2,i))
			toChoose.push_back(i);
	if (toChoose.size() < 4)
	{
        logGlobal->warnStream()<<"Warning: less then 4 available skills was found by University initializer!";
		return;
	}

	for (int i=0; i<4; i++)//get 4 skills
	{
		int skillPos = ran()%toChoose.size();
		skills.push_back(toChoose[skillPos]);//move it to selected
		toChoose.erase(toChoose.begin()+skillPos);//remove from list
	}
}

std::vector<int> CGUniversity::availableItemsIds(EMarketMode::EMarketMode mode) const
{
	switch (mode)
	{
		case EMarketMode::RESOURCE_SKILL:
			return skills;

		default:
			return std::vector <int> ();
	}
}

void CGUniversity::onHeroVisit(const CGHeroInstance * h) const
{
	openWindow(OpenWindow::UNIVERSITY_WINDOW,id.getNum(),h->id.getNum());
}

GrowthInfo::Entry::Entry(const std::string &format, int _count)
	: count(_count)
{
	description = boost::str(boost::format(format) % count);
}

GrowthInfo::Entry::Entry(int subID, BuildingID building, int _count)
	: count(_count)
{
	description = boost::str(boost::format("%s %+d") % VLC->townh->factions[subID]->town->buildings[building]->Name() % count);
}

CTownAndVisitingHero::CTownAndVisitingHero()
{
	setNodeType(TOWN_AND_VISITOR);
}

int GrowthInfo::totalGrowth() const
{
	int ret = 0;
	for(const Entry &entry : entries)
		ret += entry.count;

	return ret;
}

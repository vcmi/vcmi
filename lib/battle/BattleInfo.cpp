/*
 * BattleInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleInfo.h"
#include "../CStack.h"
#include "../CHeroHandler.h"
#include "../NetPacks.h"
#include "../filesystem/Filesystem.h"
#include "../mapObjects/CGTownInstance.h"
#include "../CGeneralTextHandler.h"
#include "../Terrain.h"
#include "../BattleFieldHandler.h"
#include "../ObstacleHandler.h"

//TODO: remove
#include "../IGameCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

///BattleInfo
std::pair< std::vector<BattleHex>, int > BattleInfo::getPath(BattleHex start, BattleHex dest, const CStack * stack)
{
	auto reachability = getReachability(stack);

	if(reachability.predecessors[dest] == -1) //cannot reach destination
	{
		return std::make_pair(std::vector<BattleHex>(), 0);
	}

	//making the Path
	std::vector<BattleHex> path;
	BattleHex curElem = dest;
	while(curElem != start)
	{
		path.push_back(curElem);
		curElem = reachability.predecessors[curElem];
	}

	return std::make_pair(path, reachability.distances[dest]);
}

void BattleInfo::calculateCasualties(std::map<ui32,si32> * casualties) const
{
	for(auto & elem : stacks)//setting casualties
	{
		const CStack * const st = elem;
		si32 killed = st->getKilled();
		if(killed > 0)
			casualties[st->side][st->getCreature()->idNumber] += killed;
	}
}

CStack * BattleInfo::generateNewStack(uint32_t id, const CStackInstance & base, ui8 side, SlotID slot, BattleHex position)
{
	PlayerColor owner = sides[side].color;
	assert((owner >= PlayerColor::PLAYER_LIMIT) ||
		(base.armyObj && base.armyObj->tempOwner == owner));

	auto ret = new CStack(&base, owner, id, side, slot);
	ret->initialPosition = getAvaliableHex(base.getCreatureID(), side, position); //TODO: what if no free tile on battlefield was found?
	stacks.push_back(ret);
	return ret;
}

CStack * BattleInfo::generateNewStack(uint32_t id, const CStackBasicDescriptor & base, ui8 side, SlotID slot, BattleHex position)
{
	PlayerColor owner = sides[side].color;
	auto ret = new CStack(&base, owner, id, side, slot);
	ret->initialPosition = position;
	stacks.push_back(ret);
	return ret;
}

void BattleInfo::localInit()
{
	for(int i = 0; i < 2; i++)
	{
		auto armyObj = battleGetArmyObject(i);
		armyObj->battle = this;
		armyObj->attachTo(*this);
	}

	for(CStack * s : stacks)
		s->localInit(this);

	exportBonuses();
}

namespace CGH
{
	static void readBattlePositions(const JsonNode &node, std::vector< std::vector<int> > & dest)
	{
		for(const JsonNode &level : node.Vector())
		{
			std::vector<int> pom;
			for(const JsonNode &value : level.Vector())
			{
				pom.push_back((int)value.Float());
			}

			dest.push_back(pom);
		}
	}
}

//RNG that works like H3 one
struct RandGen
{
	ui32 seed;

	void srand(ui32 s)
	{
		seed = s;
	}
	void srand(int3 pos)
	{
		srand(110291 * ui32(pos.x) + 167801 * ui32(pos.y) + 81569);
	}
	int rand()
	{
		seed = 214013 * seed + 2531011;
		return (seed >> 16) & 0x7FFF;
	}
	int rand(int min, int max)
	{
		if(min == max)
			return min;
		if(min > max)
			return min;
		return min + rand() % (max - min + 1);
	}
};

struct RangeGenerator
{
	class ExhaustedPossibilities : public std::exception
	{
	};

	RangeGenerator(int _min, int _max, std::function<int()> _myRand):
		min(_min),
		remainingCount(_max - _min + 1),
		remaining(remainingCount, true),
		myRand(_myRand)
	{
	}

	int generateNumber()
	{
		if(!remainingCount)
			throw ExhaustedPossibilities();
		if(remainingCount == 1)
			return 0;
		return myRand() % remainingCount;
	}

	//get number fulfilling predicate. Never gives the same number twice.
	int getSuchNumber(std::function<bool(int)> goodNumberPred = nullptr)
	{
		int ret = -1;
		do
		{
			int n = generateNumber();
			int i = 0;
			for(;;i++)
			{
				assert(i < (int)remaining.size());
				if(!remaining[i])
					continue;
				if(!n)
					break;
				n--;
			}

			remainingCount--;
			remaining[i] = false;
			ret = i + min;
		} while(goodNumberPred && !goodNumberPred(ret));
		return ret;
	}

	int min, remainingCount;
	std::vector<bool> remaining;
	std::function<int()> myRand;
};

BattleInfo * BattleInfo::setupBattle(const int3 & tile, TerrainId terrain, const BattleField & battlefieldType, const CArmedInstance * armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance * town)
{
	CMP_stack cmpst;
	auto curB = new BattleInfo();

	for(auto i = 0u; i < curB->sides.size(); i++)
		curB->sides[i].init(heroes[i], armies[i]);


	std::vector<CStack*> & stacks = (curB->stacks);

	curB->tile = tile;
	curB->battlefieldType = battlefieldType;
	curB->round = -2;
	curB->activeStack = -1;

	if(town)
	{
		curB->town = town;
		curB->terrainType = (*VLC->townh)[town->subID]->nativeTerrain;
	}
	else
	{
		curB->town = nullptr;
		curB->terrainType = terrain;
	}

	//setting up siege obstacles
	if (town && town->hasFort())
	{
		curB->si.gateState = EGateState::CLOSED;

		curB->si.wallState[EWallPart::GATE] = EWallState::INTACT;

		for (auto const wall : {EWallPart::BOTTOM_WALL, EWallPart::BELOW_GATE, EWallPart::OVER_GATE, EWallPart::UPPER_WALL} )
		{
			if (town->hasBuilt(BuildingID::CASTLE))
				curB->si.wallState[wall] = EWallState::REINFORCED;
			else
				curB->si.wallState[wall] = EWallState::INTACT;
		}

		if (town->hasBuilt(BuildingID::CITADEL))
			curB->si.wallState[EWallPart::KEEP] = EWallState::INTACT;

		if (town->hasBuilt(BuildingID::CASTLE))
		{
			curB->si.wallState[EWallPart::UPPER_TOWER] = EWallState::INTACT;
			curB->si.wallState[EWallPart::BOTTOM_TOWER] = EWallState::INTACT;
		}
	}

	//randomize obstacles
 	if (town == nullptr && !creatureBank) //do it only when it's not siege and not creature bank
 	{
		RandGen r;
		auto ourRand = [&](){ return r.rand(); };
		r.srand(tile);
		r.rand(1,8); //battle sound ID to play... can't do anything with it here
		int tilesToBlock = r.rand(5,12);

		std::vector<BattleHex> blockedTiles;

		auto appropriateAbsoluteObstacle = [&](int id)
		{
			auto * info = Obstacle(id).getInfo();
			return info && info->isAbsoluteObstacle && info->isAppropriate(curB->terrainType, battlefieldType);
		};
		auto appropriateUsualObstacle = [&](int id)
		{
			auto * info = Obstacle(id).getInfo();
			return info && !info->isAbsoluteObstacle && info->isAppropriate(curB->terrainType, battlefieldType);
		};
		
		if(r.rand(1,100) <= 40) //put cliff-like obstacle
		{
			try
			{
				RangeGenerator obidgen(0, VLC->obstacleHandler->objects.size() - 1, ourRand);
				auto obstPtr = std::make_shared<CObstacleInstance>();
				obstPtr->obstacleType = CObstacleInstance::ABSOLUTE_OBSTACLE;
				obstPtr->ID = obidgen.getSuchNumber(appropriateAbsoluteObstacle);
				obstPtr->uniqueID = static_cast<si32>(curB->obstacles.size());
				curB->obstacles.push_back(obstPtr);

				for(BattleHex blocked : obstPtr->getBlockedTiles())
					blockedTiles.push_back(blocked);
				tilesToBlock -= Obstacle(obstPtr->ID).getInfo()->blockedTiles.size() / 2;
			}
			catch(RangeGenerator::ExhaustedPossibilities &)
			{
				//silently ignore, if we can't place absolute obstacle, we'll go with the usual ones
				logGlobal->debug("RangeGenerator::ExhaustedPossibilities exception occured - cannot place absolute obstacle");
			}
		}

		try
		{
			while(tilesToBlock > 0)
			{
				RangeGenerator obidgen(0, VLC->obstacleHandler->objects.size() - 1, ourRand);
				auto tileAccessibility = curB->getAccesibility();
				const int obid = obidgen.getSuchNumber(appropriateUsualObstacle);
				const ObstacleInfo &obi = *Obstacle(obid).getInfo();

				auto validPosition = [&](BattleHex pos) -> bool
				{
					if(obi.height >= pos.getY())
						return false;
					if(pos.getX() == 0)
						return false;
					if(pos.getX() + obi.width > 15)
						return false;
					if(vstd::contains(blockedTiles, pos))
						return false;

					for(BattleHex blocked : obi.getBlocked(pos))
					{
						if(tileAccessibility[blocked] == EAccessibility::UNAVAILABLE) //for ship-to-ship battlefield - exclude hardcoded unavailable tiles
							return false;
						if(vstd::contains(blockedTiles, blocked))
							return false;
						int x = blocked.getX();
						if(x <= 2 || x >= 14)
							return false;
					}

					return true;
				};

				RangeGenerator posgenerator(18, 168, ourRand);

				auto obstPtr = std::make_shared<CObstacleInstance>();
				obstPtr->ID = obid;
				obstPtr->pos = posgenerator.getSuchNumber(validPosition);
				obstPtr->uniqueID = static_cast<si32>(curB->obstacles.size());
				curB->obstacles.push_back(obstPtr);

				for(BattleHex blocked : obstPtr->getBlockedTiles())
					blockedTiles.push_back(blocked);
				tilesToBlock -= static_cast<int>(obi.blockedTiles.size());
			}
		}
		catch(RangeGenerator::ExhaustedPossibilities &)
		{
			logGlobal->debug("RangeGenerator::ExhaustedPossibilities exception occured - cannot place usual obstacle");
		}
	}

	//reading battleStartpos - add creatures AFTER random obstacles are generated
	//TODO: parse once to some structure
	std::vector< std::vector<int> > looseFormations[2], tightFormations[2], creBankFormations[2];
	std::vector <int> commanderField, commanderBank;
	const JsonNode config(ResourceID("config/battleStartpos.json"));
	const JsonVector &positions = config["battle_positions"].Vector();

	CGH::readBattlePositions(positions[0]["levels"], looseFormations[0]);
	CGH::readBattlePositions(positions[1]["levels"], looseFormations[1]);
	CGH::readBattlePositions(positions[2]["levels"], tightFormations[0]);
	CGH::readBattlePositions(positions[3]["levels"], tightFormations[1]);
	CGH::readBattlePositions(positions[4]["levels"], creBankFormations[0]);
	CGH::readBattlePositions(positions[5]["levels"], creBankFormations[1]);

	for (auto position : config["commanderPositions"]["field"].Vector())
	{
		commanderField.push_back ((int)position.Float());
	}
	for (auto position : config["commanderPositions"]["creBank"].Vector())
	{
		commanderBank.push_back ((int)position.Float());
	}


	//adding war machines
	if(!creatureBank)
	{
		//Checks if hero has artifact and create appropriate stack
		auto handleWarMachine= [&](int side, ArtifactPosition artslot, BattleHex hex)
		{
			const CArtifactInstance * warMachineArt = heroes[side]->getArt(artslot);

			if(nullptr != warMachineArt)
			{
				CreatureID cre = warMachineArt->artType->warMachine;

				if(cre != CreatureID::NONE)
					curB->generateNewStack(curB->nextUnitId(), CStackBasicDescriptor(cre, 1), side, SlotID::WAR_MACHINES_SLOT, hex);
			}
		};

		if(heroes[0])
		{

			handleWarMachine(0, ArtifactPosition::MACH1, 52);
			handleWarMachine(0, ArtifactPosition::MACH2, 18);
			handleWarMachine(0, ArtifactPosition::MACH3, 154);
			if(town && town->hasFort())
				handleWarMachine(0, ArtifactPosition::MACH4, 120);
		}

		if(heroes[1])
		{
			if(!town) //defending hero shouldn't receive ballista (bug #551)
				handleWarMachine(1, ArtifactPosition::MACH1, 66);
			handleWarMachine(1, ArtifactPosition::MACH2, 32);
			handleWarMachine(1, ArtifactPosition::MACH3, 168);
		}
	}
	//war machines added

	//battleStartpos read
	for(int side = 0; side < 2; side++)
	{
		int formationNo = armies[side]->stacksCount() - 1;
		vstd::abetween(formationNo, 0, GameConstants::ARMY_SIZE - 1);

		int k = 0; //stack serial
		for(auto i = armies[side]->Slots().begin(); i != armies[side]->Slots().end(); i++, k++)
		{
			std::vector<int> *formationVector = nullptr;
			if(creatureBank)
				formationVector = &creBankFormations[side][formationNo];
			else if(armies[side]->formation)
				formationVector = &tightFormations[side][formationNo];
			else
				formationVector = &looseFormations[side][formationNo];

			BattleHex pos = (k < formationVector->size() ? formationVector->at(k) : 0);
			if(creatureBank && i->second->type->isDoubleWide())
				pos += side ? BattleHex::LEFT : BattleHex::RIGHT;

			curB->generateNewStack(curB->nextUnitId(), *i->second, side, i->first, pos);
		}
	}

	//adding commanders
	for (int i = 0; i < 2; ++i)
	{
		if (heroes[i] && heroes[i]->commander && heroes[i]->commander->alive)
		{
			curB->generateNewStack(curB->nextUnitId(), *heroes[i]->commander, i, SlotID::COMMANDER_SLOT_PLACEHOLDER, creatureBank ? commanderBank[i] : commanderField[i]);
		}

	}

	if (curB->town && curB->town->fortLevel() >= CGTownInstance::CITADEL)
	{
		// keep tower
		curB->generateNewStack(curB->nextUnitId(), CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), 1, SlotID::ARROW_TOWERS_SLOT, BattleHex::CASTLE_CENTRAL_TOWER);

		if (curB->town->fortLevel() >= CGTownInstance::CASTLE)
		{
			// lower tower + upper tower
			curB->generateNewStack(curB->nextUnitId(), CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), 1, SlotID::ARROW_TOWERS_SLOT, BattleHex::CASTLE_UPPER_TOWER);

			curB->generateNewStack(curB->nextUnitId(), CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), 1, SlotID::ARROW_TOWERS_SLOT, BattleHex::CASTLE_BOTTOM_TOWER);
		}

		//moat
		auto moat = std::make_shared<MoatObstacle>();
		moat->ID = curB->town->subID;
		moat->obstacleType = CObstacleInstance::MOAT;
		moat->uniqueID = static_cast<si32>(curB->obstacles.size());
		curB->obstacles.push_back(moat);
	}

	std::stable_sort(stacks.begin(),stacks.end(),cmpst);

	auto neutral = std::make_shared<CreatureAlignmentLimiter>(EAlignment::NEUTRAL);
	auto good = std::make_shared<CreatureAlignmentLimiter>(EAlignment::GOOD);
	auto evil = std::make_shared<CreatureAlignmentLimiter>(EAlignment::EVIL);

	auto bgInfo = VLC->battlefields()->getById(battlefieldType);

	for(const std::shared_ptr<Bonus> & bonus : bgInfo->bonuses)
	{
		curB->addNewBonus(bonus);
	}

	//native terrain bonuses
	static auto nativeTerrain = std::make_shared<CreatureTerrainLimiter>();
	
	curB->addNewBonus(std::make_shared<Bonus>(Bonus::ONE_BATTLE, Bonus::STACKS_SPEED, Bonus::TERRAIN_NATIVE, 1, 0, 0)->addLimiter(nativeTerrain));
	curB->addNewBonus(std::make_shared<Bonus>(Bonus::ONE_BATTLE, Bonus::PRIMARY_SKILL, Bonus::TERRAIN_NATIVE, 1, 0, PrimarySkill::ATTACK)->addLimiter(nativeTerrain));
	curB->addNewBonus(std::make_shared<Bonus>(Bonus::ONE_BATTLE, Bonus::PRIMARY_SKILL, Bonus::TERRAIN_NATIVE, 1, 0, PrimarySkill::DEFENSE)->addLimiter(nativeTerrain));
	//////////////////////////////////////////////////////////////////////////

	//tactics
	bool isTacticsAllowed = !creatureBank; //no tactics in creature banks

	int tacticLvls[2] = {0};
	for(int i = 0; i < ARRAY_COUNT(tacticLvls); i++)
	{
		if(heroes[i])
			tacticLvls[i] += heroes[i]->valOfBonuses(Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::TACTICS));
	}
	int tacticsSkillDiff = tacticLvls[0] - tacticLvls[1];

	if(tacticsSkillDiff && isTacticsAllowed)
	{
		curB->tacticsSide = tacticsSkillDiff < 0;
		//bonus specifies distance you can move beyond base row; this allows 100% compatibility with HMM3 mechanics
		curB->tacticDistance = 1 + std::abs(tacticsSkillDiff);
	}
	else
		curB->tacticDistance = 0;

	return curB;
}


const CGHeroInstance * BattleInfo::getHero(PlayerColor player) const
{
	for(int i = 0; i < sides.size(); i++)
		if(sides[i].color == player)
			return sides[i].hero;

	logGlobal->error("Player %s is not in battle!", player.getStr());
	return nullptr;
}

ui8 BattleInfo::whatSide(PlayerColor player) const
{
	for(int i = 0; i < sides.size(); i++)
		if(sides[i].color == player)
			return i;

	logGlobal->warn("BattleInfo::whatSide: Player %s is not in battle!", player.getStr());
	return -1;
}

CStack * BattleInfo::getStack(int stackID, bool onlyAlive)
{
	return const_cast<CStack *>(battleGetStackByID(stackID, onlyAlive));
}

BattleInfo::BattleInfo():
	round(-1),
	activeStack(-1),
	town(nullptr),
	tile(-1,-1,-1),
	battlefieldType(BattleField::NONE),
	terrainType(),
	tacticsSide(0),
	tacticDistance(0)
{
	setBattle(this);
	setNodeType(BATTLE);
}

BattleInfo::~BattleInfo() = default;

int32_t BattleInfo::getActiveStackID() const
{
	return activeStack;
}

TStacks BattleInfo::getStacksIf(TStackFilter predicate) const
{
	TStacks ret;
	vstd::copy_if(stacks, std::back_inserter(ret), predicate);
	return ret;
}

battle::Units BattleInfo::getUnitsIf(battle::UnitFilter predicate) const
{
	battle::Units ret;
	vstd::copy_if(stacks, std::back_inserter(ret), predicate);
	return ret;
}


BattleField BattleInfo::getBattlefieldType() const
{
	return battlefieldType;
}

TerrainId BattleInfo::getTerrainType() const
{
	return terrainType;
}

IBattleInfo::ObstacleCList BattleInfo::getAllObstacles() const
{
	ObstacleCList ret;

	for(auto iter = obstacles.cbegin(); iter != obstacles.cend(); iter++)
		ret.push_back(*iter);

	return ret;
}

PlayerColor BattleInfo::getSidePlayer(ui8 side) const
{
	return sides.at(side).color;
}

const CArmedInstance * BattleInfo::getSideArmy(ui8 side) const
{
	return sides.at(side).armyObject;
}

const CGHeroInstance * BattleInfo::getSideHero(ui8 side) const
{
	return sides.at(side).hero;
}

ui8 BattleInfo::getTacticDist() const
{
	return tacticDistance;
}

ui8 BattleInfo::getTacticsSide() const
{
	return tacticsSide;
}

const CGTownInstance * BattleInfo::getDefendedTown() const
{
	return town;
}

EWallState BattleInfo::getWallState(EWallPart partOfWall) const
{
	return si.wallState.at(partOfWall);
}

EGateState BattleInfo::getGateState() const
{
	return si.gateState;
}

uint32_t BattleInfo::getCastSpells(ui8 side) const
{
	return sides.at(side).castSpellsCount;
}

int32_t BattleInfo::getEnchanterCounter(ui8 side) const
{
	return sides.at(side).enchanterCounter;
}

const IBonusBearer * BattleInfo::asBearer() const
{
	return this;
}

int64_t BattleInfo::getActualDamage(const TDmgRange & damage, int32_t attackerCount, vstd::RNG & rng) const
{
	if(damage.first != damage.second)
	{
		int64_t sum = 0;

		auto howManyToAv = std::min<int32_t>(10, attackerCount);
		auto rangeGen = rng.getInt64Range(damage.first, damage.second);

		for(int32_t g = 0; g < howManyToAv; ++g)
			sum += rangeGen();

		return sum / howManyToAv;
	}
	else
	{
		return damage.first;
	}
}

void BattleInfo::nextRound(int32_t roundNr)
{
	for(int i = 0; i < 2; ++i)
	{
		sides.at(i).castSpellsCount = 0;
		vstd::amax(--sides.at(i).enchanterCounter, 0);
	}
	round = roundNr;

	for(CStack * s : stacks)
	{
		// new turn effects
		s->reduceBonusDurations(Bonus::NTurns);

		s->afterNewRound();
	}

	for(auto & obst : obstacles)
		obst->battleTurnPassed();
}

void BattleInfo::nextTurn(uint32_t unitId)
{
	activeStack = unitId;

	CStack * st = getStack(activeStack);

	//remove bonuses that last until when stack gets new turn
	st->removeBonusesRecursive(Bonus::UntilGetsTurn);

	st->afterGetsTurn();
}

void BattleInfo::addUnit(uint32_t id, const JsonNode & data)
{
	battle::UnitInfo info;
	info.load(id, data);
	CStackBasicDescriptor base(info.type, info.count);

	PlayerColor owner = getSidePlayer(info.side);

	auto ret = new CStack(&base, owner, info.id, info.side, SlotID::SUMMONED_SLOT_PLACEHOLDER);
	ret->initialPosition = info.position;
	stacks.push_back(ret);
	ret->localInit(this);
	ret->summoned = info.summoned;
}

void BattleInfo::moveUnit(uint32_t id, BattleHex destination)
{
	auto sta = getStack(id);
	if(!sta)
	{
		logGlobal->error("Cannot find stack %d", id);
		return;
	}
	sta->position = destination;
}

void BattleInfo::setUnitState(uint32_t id, const JsonNode & data, int64_t healthDelta)
{
	CStack * changedStack = getStack(id, false);
	if(!changedStack)
		throw std::runtime_error("Invalid unit id in BattleInfo update");

	if(!changedStack->alive() && healthDelta > 0)
	{
		//checking if we resurrect a stack that is under a living stack
		auto accessibility = getAccesibility();

		if(!accessibility.accessible(changedStack->getPosition(), changedStack))
		{
			logNetwork->error("Cannot resurrect %s because hex %d is occupied!", changedStack->nodeName(), changedStack->getPosition().hex);
			return; //position is already occupied
		}
	}

	bool killed = (-healthDelta) >= changedStack->getAvailableHealth();//todo: check using alive state once rebirth will be handled separately

	bool resurrected = !changedStack->alive() && healthDelta > 0;

	//applying changes
	changedStack->load(data);


	if(healthDelta < 0)
	{
		changedStack->removeBonusesRecursive(Bonus::UntilBeingAttacked);
	}

	resurrected = resurrected || (killed && changedStack->alive());

	if(killed)
	{
		if(changedStack->cloneID >= 0)
		{
			//remove clone as well
			CStack * clone = getStack(changedStack->cloneID);
			if(clone)
				clone->makeGhost();

			changedStack->cloneID = -1;
		}
	}

	if(resurrected || killed)
	{
		//removing all spells effects
		auto selector = [](const Bonus * b)
		{
			//Special case: DISRUPTING_RAY is absolutely permanent
			return b->source == Bonus::SPELL_EFFECT && b->sid != SpellID::DISRUPTING_RAY;
		};
		changedStack->removeBonusesRecursive(selector);
	}

	if(!changedStack->alive() && changedStack->isClone())
	{
		for(CStack * s : stacks)
		{
			if(s->cloneID == changedStack->unitId())
				s->cloneID = -1;
		}
	}
}

void BattleInfo::removeUnit(uint32_t id)
{
	std::set<uint32_t> ids;
	ids.insert(id);

	while(!ids.empty())
	{
		auto toRemoveId = *ids.begin();
		auto toRemove = getStack(toRemoveId, false);

		if(!toRemove)
		{
			logGlobal->error("Cannot find stack %d", toRemoveId);
			return;
		}

		if(!toRemove->ghost)
		{
			toRemove->onRemoved();
			toRemove->detachFromAll();

			//stack may be removed instantly (not being killed first)
			//handle clone remove also here
			if(toRemove->cloneID >= 0)
			{
				ids.insert(toRemove->cloneID);
				toRemove->cloneID = -1;
			}

			//cleanup remaining clone links if any
			for(auto s : stacks)
			{
				if(s->cloneID == toRemoveId)
					s->cloneID = -1;
			}
		}

		ids.erase(toRemoveId);
	}
}

void BattleInfo::updateUnit(uint32_t id, const JsonNode & data)
{
	//TODO
}

void BattleInfo::addUnitBonus(uint32_t id, const std::vector<Bonus> & bonus)
{
	CStack * sta = getStack(id, false);

	if(!sta)
	{
		logGlobal->error("Cannot find stack %d", id);
		return;
	}

	for(const Bonus & b : bonus)
		addOrUpdateUnitBonus(sta, b, true);
}

void BattleInfo::updateUnitBonus(uint32_t id, const std::vector<Bonus> & bonus)
{
	CStack * sta = getStack(id, false);

	if(!sta)
	{
		logGlobal->error("Cannot find stack %d", id);
		return;
	}

	for(const Bonus & b : bonus)
		addOrUpdateUnitBonus(sta, b, false);
}

void BattleInfo::removeUnitBonus(uint32_t id, const std::vector<Bonus> & bonus)
{
	CStack * sta = getStack(id, false);

	if(!sta)
	{
		logGlobal->error("Cannot find stack %d", id);
		return;
	}

	for(const Bonus & one : bonus)
	{
		auto selector = [one](const Bonus * b)
		{
			//compare everything but turnsRemain, limiter and propagator
			return one.duration == b->duration
			&& one.type == b->type
			&& one.subtype == b->subtype
			&& one.source == b->source
			&& one.val == b->val
			&& one.sid == b->sid
			&& one.valType == b->valType
			&& one.additionalInfo == b->additionalInfo
			&& one.effectRange == b->effectRange
			&& one.description == b->description;
		};
		sta->removeBonusesRecursive(selector);
	}
}

uint32_t BattleInfo::nextUnitId() const
{
	return static_cast<uint32_t>(stacks.size());
}

void BattleInfo::addOrUpdateUnitBonus(CStack * sta, const Bonus & value, bool forceAdd)
{
	if(forceAdd || !sta->hasBonus(Selector::source(Bonus::SPELL_EFFECT, value.sid).And(Selector::typeSubtype(value.type, value.subtype))))
	{
		//no such effect or cumulative - add new
		logBonus->trace("%s receives a new bonus: %s", sta->nodeName(), value.Description());
		sta->addNewBonus(std::make_shared<Bonus>(value));
	}
	else
	{
		logBonus->trace("%s updated bonus: %s", sta->nodeName(), value.Description());

		for(auto stackBonus : sta->getExportedBonusList()) //TODO: optimize
		{
			if(stackBonus->source == value.source && stackBonus->sid == value.sid && stackBonus->type == value.type && stackBonus->subtype == value.subtype)
			{
				stackBonus->turnsRemain = std::max(stackBonus->turnsRemain, value.turnsRemain);
			}
		}
		CBonusSystemNode::treeHasChanged();
	}
}

void BattleInfo::setWallState(EWallPart partOfWall, EWallState state)
{
	si.wallState[partOfWall] = state;
}

void BattleInfo::addObstacle(const ObstacleChanges & changes)
{
	std::shared_ptr<SpellCreatedObstacle> obstacle = std::make_shared<SpellCreatedObstacle>();
	obstacle->fromInfo(changes);
	obstacles.push_back(obstacle);
}

void BattleInfo::updateObstacle(const ObstacleChanges& changes)
{
	std::shared_ptr<SpellCreatedObstacle> changedObstacle = std::make_shared<SpellCreatedObstacle>();
	changedObstacle->fromInfo(changes);

	for(int i = 0; i < obstacles.size(); ++i)
	{
		if(obstacles[i]->uniqueID == changes.id) // update this obstacle
		{
			SpellCreatedObstacle * spellObstacle = dynamic_cast<SpellCreatedObstacle *>(obstacles[i].get());
			assert(spellObstacle);

			// Currently we only support to update the "revealed" property
			spellObstacle->revealed = changedObstacle->revealed;

			break;
		}
	}
}

void BattleInfo::removeObstacle(uint32_t id)
{
	for(int i=0; i < obstacles.size(); ++i)
	{
		if(obstacles[i]->uniqueID == id) //remove this obstacle
		{
			obstacles.erase(obstacles.begin() + i);
			break;
		}
	}
}

CArmedInstance * BattleInfo::battleGetArmyObject(ui8 side) const
{
	return const_cast<CArmedInstance*>(CBattleInfoEssentials::battleGetArmyObject(side));
}

CGHeroInstance * BattleInfo::battleGetFightingHero(ui8 side) const
{
	return const_cast<CGHeroInstance*>(CBattleInfoEssentials::battleGetFightingHero(side));
}

#if SCRIPTING_ENABLED
scripting::Pool * BattleInfo::getContextPool() const
{
	//this is real battle, use global scripting context pool
	//TODO: make this line not ugly
	return IObjectInterface::cb->getGlobalContextPool();
}
#endif

bool CMP_stack::operator()(const battle::Unit * a, const battle::Unit * b)
{
	switch(phase)
	{
	case 0: //catapult moves after turrets
		return a->creatureIndex() > b->creatureIndex(); //catapult is 145 and turrets are 149
	case 1:
	case 2:
	case 3:
		{
			int as = a->getInitiative(turn), bs = b->getInitiative(turn);

			if(as != bs)
				return as > bs;

			if(a->unitSide() == b->unitSide())
				return a->unitSlot() < b->unitSlot();

			return (a->unitSide() == side || b->unitSide() == side)
				? a->unitSide() != side
				: a->unitSide() < b->unitSide();
			}
	default:
		assert(false);
		return false;
	}

	assert(false);
	return false;
}

CMP_stack::CMP_stack(int Phase, int Turn, uint8_t Side)
{
	phase = Phase;
	turn = Turn;
	side = Side;
}

VCMI_LIB_NAMESPACE_END

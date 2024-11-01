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

#include "BattleLayout.h"
#include "CObstacleInstance.h"
#include "bonuses/Limiters.h"
#include "bonuses/Updaters.h"
#include "../CStack.h"
#include "../CHeroHandler.h"
#include "../entities/building/TownFortifications.h"
#include "../filesystem/Filesystem.h"
#include "../mapObjects/CGTownInstance.h"
#include "../texts/CGeneralTextHandler.h"
#include "../BattleFieldHandler.h"
#include "../ObstacleHandler.h"

#include <vstd/RNG.h>

//TODO: remove
#include "../IGameCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

const SideInBattle & BattleInfo::getSide(BattleSide side) const
{
	return sides.at(side);
}

SideInBattle & BattleInfo::getSide(BattleSide side)
{
	return sides.at(side);
}

///BattleInfo
CStack * BattleInfo::generateNewStack(uint32_t id, const CStackInstance & base, BattleSide side, const SlotID & slot, BattleHex position)
{
	PlayerColor owner = getSide(side).color;
	assert(!owner.isValidPlayer() || (base.armyObj && base.armyObj->tempOwner == owner));

	auto * ret = new CStack(&base, owner, id, side, slot);
	ret->initialPosition = getAvailableHex(base.getCreatureID(), side, position); //TODO: what if no free tile on battlefield was found?
	stacks.push_back(ret);
	return ret;
}

CStack * BattleInfo::generateNewStack(uint32_t id, const CStackBasicDescriptor & base, BattleSide side, const SlotID & slot, BattleHex position)
{
	PlayerColor owner = getSide(side).color;
	auto * ret = new CStack(&base, owner, id, side, slot);
	ret->initialPosition = position;
	stacks.push_back(ret);
	return ret;
}

void BattleInfo::localInit()
{
	for(BattleSide i : { BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		auto * armyObj = battleGetArmyObject(i);
		armyObj->battle = this;
		armyObj->attachTo(*this);
	}

	for(CStack * s : stacks)
		s->localInit(this);

	exportBonuses();
}


//RNG that works like H3 one
struct RandGen
{
	ui32 seed;

	void srand(ui32 s)
	{
		seed = s;
	}
	void srand(const int3 & pos)
	{
		srand(110291 * static_cast<ui32>(pos.x) + 167801 * static_cast<ui32>(pos.y) + 81569);
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
		myRand(std::move(_myRand))
	{
	}

	int generateNumber() const
	{
		if(!remainingCount)
			throw ExhaustedPossibilities();
		if(remainingCount == 1)
			return 0;
		return myRand() % remainingCount;
	}

	//get number fulfilling predicate. Never gives the same number twice.
	int getSuchNumber(const std::function<bool(int)> & goodNumberPred = nullptr)
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

	int min;
	int remainingCount;
	std::vector<bool> remaining;
	std::function<int()> myRand;
};

BattleInfo * BattleInfo::setupBattle(const int3 & tile, TerrainId terrain, const BattleField & battlefieldType, BattleSideArray<const CArmedInstance *> armies, BattleSideArray<const CGHeroInstance *> heroes, const BattleLayout & layout, const CGTownInstance * town)
{
	CMP_stack cmpst;
	auto * curB = new BattleInfo(layout);

	for(auto i : { BattleSide::LEFT_SIDE, BattleSide::RIGHT_SIDE})
		curB->sides[i].init(heroes[i], armies[i]);

	std::vector<CStack*> & stacks = (curB->stacks);

	curB->tile = tile;
	curB->battlefieldType = battlefieldType;
	curB->round = -2;
	curB->activeStack = -1;
	curB->replayAllowed = false;

	if(town)
	{
		curB->town = town;
		curB->terrainType = town->getNativeTerrain();
	}
	else
	{
		curB->town = nullptr;
		curB->terrainType = terrain;
	}

	//setting up siege obstacles
	if (town && town->fortificationsLevel().wallsHealth != 0)
	{
		auto fortification = town->fortificationsLevel();

		curB->si.gateState = EGateState::CLOSED;

		curB->si.wallState[EWallPart::GATE] = EWallState::INTACT;

		for(const auto wall : {EWallPart::BOTTOM_WALL, EWallPart::BELOW_GATE, EWallPart::OVER_GATE, EWallPart::UPPER_WALL})
			curB->si.wallState[wall] = static_cast<EWallState>(fortification.wallsHealth);

		if (fortification.citadelHealth != 0)
			curB->si.wallState[EWallPart::KEEP] = static_cast<EWallState>(fortification.citadelHealth);

		if (fortification.upperTowerHealth != 0)
			curB->si.wallState[EWallPart::UPPER_TOWER] = static_cast<EWallState>(fortification.upperTowerHealth);

		if (fortification.lowerTowerHealth != 0)
			curB->si.wallState[EWallPart::BOTTOM_TOWER] = static_cast<EWallState>(fortification.lowerTowerHealth);
	}

	//randomize obstacles
	if (layout.obstaclesAllowed && !town)
 	{
		RandGen r{};
		auto ourRand = [&](){ return r.rand(); };
		r.srand(tile);
		r.rand(1,8); //battle sound ID to play... can't do anything with it here
		int tilesToBlock = r.rand(5,12);

		std::vector<BattleHex> blockedTiles;

		auto appropriateAbsoluteObstacle = [&](int id)
		{
			const auto * info = Obstacle(id).getInfo();
			return info && info->isAbsoluteObstacle && info->isAppropriate(curB->terrainType, battlefieldType);
		};
		auto appropriateUsualObstacle = [&](int id)
		{
			const auto * info = Obstacle(id).getInfo();
			return info && !info->isAbsoluteObstacle && info->isAppropriate(curB->terrainType, battlefieldType);
		};

		if(r.rand(1,100) <= 40) //put cliff-like obstacle
		{
			try
			{
				RangeGenerator obidgen(0, VLC->obstacleHandler->size() - 1, ourRand);
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
				logGlobal->debug("RangeGenerator::ExhaustedPossibilities exception occurred - cannot place absolute obstacle");
			}
		}

		try
		{
			while(tilesToBlock > 0)
			{
				RangeGenerator obidgen(0, VLC->obstacleHandler->size() - 1, ourRand);
				auto tileAccessibility = curB->getAccessibility();
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
			logGlobal->debug("RangeGenerator::ExhaustedPossibilities exception occurred - cannot place usual obstacle");
		}
	}

	//adding war machines
	//Checks if hero has artifact and create appropriate stack
	auto handleWarMachine = [&](BattleSide side, const ArtifactPosition & artslot, BattleHex hex)
	{
		const CArtifactInstance * warMachineArt = heroes[side]->getArt(artslot);

		if(nullptr != warMachineArt && hex.isValid())
		{
			CreatureID cre = warMachineArt->artType->getWarMachine();

			if(cre != CreatureID::NONE)
				curB->generateNewStack(curB->nextUnitId(), CStackBasicDescriptor(cre, 1), side, SlotID::WAR_MACHINES_SLOT, hex);
		}
	};

	if(heroes[BattleSide::ATTACKER])
	{
		auto warMachineHexes = layout.warMachines.at(BattleSide::ATTACKER);

		handleWarMachine(BattleSide::ATTACKER, ArtifactPosition::MACH1, warMachineHexes.at(0));
		handleWarMachine(BattleSide::ATTACKER, ArtifactPosition::MACH2, warMachineHexes.at(1));
		handleWarMachine(BattleSide::ATTACKER, ArtifactPosition::MACH3, warMachineHexes.at(2));
		if(town && town->fortificationsLevel().wallsHealth > 0)
			handleWarMachine(BattleSide::ATTACKER, ArtifactPosition::MACH4, warMachineHexes.at(3));
	}

	if(heroes[BattleSide::DEFENDER])
	{
		auto warMachineHexes = layout.warMachines.at(BattleSide::DEFENDER);

		if(!town) //defending hero shouldn't receive ballista (bug #551)
			handleWarMachine(BattleSide::DEFENDER, ArtifactPosition::MACH1, warMachineHexes.at(0));
		handleWarMachine(BattleSide::DEFENDER, ArtifactPosition::MACH2, warMachineHexes.at(1));
		handleWarMachine(BattleSide::DEFENDER, ArtifactPosition::MACH3, warMachineHexes.at(2));
	}
	//war machines added

	//battleStartpos read
	for(BattleSide side : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		int formationNo = armies[side]->stacksCount() - 1;
		vstd::abetween(formationNo, 0, GameConstants::ARMY_SIZE - 1);

		int k = 0; //stack serial
		for(auto i = armies[side]->Slots().begin(); i != armies[side]->Slots().end(); i++, k++)
		{
			const BattleHex & pos = layout.units.at(side).at(k);

			if (pos.isValid())
				curB->generateNewStack(curB->nextUnitId(), *i->second, side, i->first, pos);
		}
	}

	//adding commanders
	for(BattleSide i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		if (heroes[i] && heroes[i]->commander && heroes[i]->commander->alive)
		{
			curB->generateNewStack(curB->nextUnitId(), *heroes[i]->commander, i, SlotID::COMMANDER_SLOT_PLACEHOLDER, layout.commanders.at(i));
		}
	}

	if (curB->town)
	{
		if (curB->town->fortificationsLevel().citadelHealth != 0)
			curB->generateNewStack(curB->nextUnitId(), CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), BattleSide::DEFENDER, SlotID::ARROW_TOWERS_SLOT, BattleHex::CASTLE_CENTRAL_TOWER);

		if (curB->town->fortificationsLevel().upperTowerHealth != 0)
			curB->generateNewStack(curB->nextUnitId(), CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), BattleSide::DEFENDER, SlotID::ARROW_TOWERS_SLOT, BattleHex::CASTLE_UPPER_TOWER);

		if (curB->town->fortificationsLevel().lowerTowerHealth != 0)
			curB->generateNewStack(curB->nextUnitId(), CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), BattleSide::DEFENDER, SlotID::ARROW_TOWERS_SLOT, BattleHex::CASTLE_BOTTOM_TOWER);

		//Moat generating is done on server
	}

	std::stable_sort(stacks.begin(),stacks.end(),cmpst);

	auto neutral = std::make_shared<CreatureAlignmentLimiter>(EAlignment::NEUTRAL);
	auto good = std::make_shared<CreatureAlignmentLimiter>(EAlignment::GOOD);
	auto evil = std::make_shared<CreatureAlignmentLimiter>(EAlignment::EVIL);

	const auto * bgInfo = VLC->battlefields()->getById(battlefieldType);

	for(const std::shared_ptr<Bonus> & bonus : bgInfo->bonuses)
	{
		curB->addNewBonus(bonus);
	}

	//native terrain bonuses
	auto nativeTerrain = std::make_shared<CreatureTerrainLimiter>();
	
	curB->addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::STACKS_SPEED, BonusSource::TERRAIN_NATIVE, 1,  BonusSourceID())->addLimiter(nativeTerrain));
	curB->addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::PRIMARY_SKILL, BonusSource::TERRAIN_NATIVE, 1, BonusSourceID(), BonusSubtypeID(PrimarySkill::ATTACK))->addLimiter(nativeTerrain));
	curB->addNewBonus(std::make_shared<Bonus>(BonusDuration::ONE_BATTLE, BonusType::PRIMARY_SKILL, BonusSource::TERRAIN_NATIVE, 1, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE))->addLimiter(nativeTerrain));
	//////////////////////////////////////////////////////////////////////////

	//tactics
	BattleSideArray<int> battleRepositionHex = {};
	BattleSideArray<int> battleRepositionHexBlock = {};
	for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		if(heroes[i])
		{
			battleRepositionHex[i] += heroes[i]->valOfBonuses(Selector::type()(BonusType::BEFORE_BATTLE_REPOSITION));
			battleRepositionHexBlock[i] += heroes[i]->valOfBonuses(Selector::type()(BonusType::BEFORE_BATTLE_REPOSITION_BLOCK));
		}
	}
	int tacticsSkillDiffAttacker = battleRepositionHex[BattleSide::ATTACKER] - battleRepositionHexBlock[BattleSide::DEFENDER];
	int tacticsSkillDiffDefender = battleRepositionHex[BattleSide::DEFENDER] - battleRepositionHexBlock[BattleSide::ATTACKER];

	/* for current tactics, we need to choose one side, so, we will choose side when first - second > 0, and ignore sides
	   when first - second <= 0. If there will be situations when both > 0, attacker will be chosen. Anyway, in OH3 this
	   will not happen because tactics block opposite tactics on same value.
	   TODO: For now, it is an error to use BEFORE_BATTLE_REPOSITION bonus without counterpart, but it can be changed if
	   double tactics will be implemented.
	*/

	if(layout.tacticsAllowed)
	{
		if(tacticsSkillDiffAttacker > 0 && tacticsSkillDiffDefender > 0)
			logGlobal->warn("Double tactics is not implemented, only attacker will have tactics!");
		if(tacticsSkillDiffAttacker > 0)
		{
			curB->tacticsSide = BattleSide::ATTACKER;
			//bonus specifies distance you can move beyond base row; this allows 100% compatibility with HMM3 mechanics
			curB->tacticDistance = 1 + tacticsSkillDiffAttacker;
		}
		else if(tacticsSkillDiffDefender > 0)
		{
			curB->tacticsSide = BattleSide::DEFENDER;
			//bonus specifies distance you can move beyond base row; this allows 100% compatibility with HMM3 mechanics
			curB->tacticDistance = 1 + tacticsSkillDiffDefender;
		}
		else
			curB->tacticDistance = 0;
	}

	return curB;
}

const CGHeroInstance * BattleInfo::getHero(const PlayerColor & player) const
{
	for(const auto & side : sides)
		if(side.color == player)
			return side.hero;

	logGlobal->error("Player %s is not in battle!", player.toString());
	return nullptr;
}

BattleSide BattleInfo::whatSide(const PlayerColor & player) const
{
	for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
		if(sides[i].color == player)
			return i;

	logGlobal->warn("BattleInfo::whatSide: Player %s is not in battle!", player.toString());
	return BattleSide::NONE;
}

CStack * BattleInfo::getStack(int stackID, bool onlyAlive)
{
	return const_cast<CStack *>(battleGetStackByID(stackID, onlyAlive));
}

BattleInfo::BattleInfo(const BattleLayout & layout):
	BattleInfo()
{
	*this->layout = layout;
}

BattleInfo::BattleInfo():
	layout(std::make_unique<BattleLayout>()),
	round(-1),
	activeStack(-1),
	town(nullptr),
	tile(-1,-1,-1),
	battlefieldType(BattleField::NONE),
	tacticsSide(BattleSide::NONE),
	tacticDistance(0)
{
	setNodeType(BATTLE);
}

BattleLayout BattleInfo::getLayout() const
{
	return *layout;
}

BattleID BattleInfo::getBattleID() const
{
	return battleID;
}

const IBattleInfo * BattleInfo::getBattle() const
{
	return this;
}

std::optional<PlayerColor> BattleInfo::getPlayerID() const
{
	return std::nullopt;
}

BattleInfo::~BattleInfo()
{
	for (auto & elem : stacks)
		delete elem;

	for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
		if(auto * _armyObj = battleGetArmyObject(i))
			_armyObj->battle = nullptr;
}

int32_t BattleInfo::getActiveStackID() const
{
	return activeStack;
}

TStacks BattleInfo::getStacksIf(const TStackFilter & predicate) const
{
	TStacks ret;
	vstd::copy_if(stacks, std::back_inserter(ret), predicate);
	return ret;
}

battle::Units BattleInfo::getUnitsIf(const battle::UnitFilter & predicate) const
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

	for(const auto & obstacle : obstacles)
		ret.push_back(obstacle);

	return ret;
}

PlayerColor BattleInfo::getSidePlayer(BattleSide side) const
{
	return getSide(side).color;
}

const CArmedInstance * BattleInfo::getSideArmy(BattleSide side) const
{
	return getSide(side).armyObject;
}

const CGHeroInstance * BattleInfo::getSideHero(BattleSide side) const
{
	return getSide(side).hero;
}

uint8_t BattleInfo::getTacticDist() const
{
	return tacticDistance;
}

BattleSide BattleInfo::getTacticsSide() const
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

uint32_t BattleInfo::getCastSpells(BattleSide side) const
{
	return getSide(side).castSpellsCount;
}

int32_t BattleInfo::getEnchanterCounter(BattleSide side) const
{
	return getSide(side).enchanterCounter;
}

const IBonusBearer * BattleInfo::getBonusBearer() const
{
	return this;
}

int64_t BattleInfo::getActualDamage(const DamageRange & damage, int32_t attackerCount, vstd::RNG & rng) const
{
	if(damage.min != damage.max)
	{
		int64_t sum = 0;

		auto howManyToAv = std::min<int32_t>(10, attackerCount);

		for(int32_t g = 0; g < howManyToAv; ++g)
			sum += rng.nextInt64(damage.min, damage.max);

		return sum / howManyToAv;
	}
	else
	{
		return damage.min;
	}
}

int3 BattleInfo::getLocation() const
{
	return tile;
}

std::vector<SpellID> BattleInfo::getUsedSpells(BattleSide side) const
{
	return getSide(side).usedSpellsHistory;
}

void BattleInfo::nextRound()
{
	for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		sides.at(i).castSpellsCount = 0;
		vstd::amax(--sides.at(i).enchanterCounter, 0);
	}
	round += 1;

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

	auto * ret = new CStack(&base, owner, info.id, info.side, SlotID::SUMMONED_SLOT_PLACEHOLDER);
	ret->initialPosition = info.position;
	stacks.push_back(ret);
	ret->localInit(this);
	ret->summoned = info.summoned;
}

void BattleInfo::moveUnit(uint32_t id, BattleHex destination)
{
	auto * sta = getStack(id);
	if(!sta)
	{
		logGlobal->error("Cannot find stack %d", id);
		return;
	}
	sta->position = destination;
	//Bonuses can be limited by unit placement, so, change tree version 
	//to force updating a bonus. TODO: update version only when such bonuses are present
	CBonusSystemNode::treeHasChanged();
}

void BattleInfo::setUnitState(uint32_t id, const JsonNode & data, int64_t healthDelta)
{
	CStack * changedStack = getStack(id, false);
	if(!changedStack)
		throw std::runtime_error("Invalid unit id in BattleInfo update");

	if(!changedStack->alive() && healthDelta > 0)
	{
		//checking if we resurrect a stack that is under a living stack
		auto accessibility = getAccessibility();

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
			return b->source == BonusSource::SPELL_EFFECT && b->sid.as<SpellID>() != SpellID::DISRUPTING_RAY;
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
		auto * toRemove = getStack(toRemoveId, false);

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
			for(auto * s : stacks)
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
			&& one.effectRange == b->effectRange;
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
	if(forceAdd || !sta->hasBonus(Selector::source(BonusSource::SPELL_EFFECT, value.sid).And(Selector::typeSubtypeValueType(value.type, value.subtype, value.valType))))
	{
		//no such effect or cumulative - add new
		logBonus->trace("%s receives a new bonus: %s", sta->nodeName(), value.Description());
		sta->addNewBonus(std::make_shared<Bonus>(value));
	}
	else
	{
		logBonus->trace("%s updated bonus: %s", sta->nodeName(), value.Description());

		for(const auto & stackBonus : sta->getExportedBonusList()) //TODO: optimize
		{
			if(stackBonus->source == value.source && stackBonus->sid == value.sid && stackBonus->type == value.type && stackBonus->subtype == value.subtype && stackBonus->valType == value.valType)
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
	auto obstacle = std::make_shared<SpellCreatedObstacle>();
	obstacle->fromInfo(changes);
	obstacles.push_back(obstacle);
}

void BattleInfo::updateObstacle(const ObstacleChanges& changes)
{
	auto changedObstacle = std::make_shared<SpellCreatedObstacle>();
	changedObstacle->fromInfo(changes);

	for(auto & obstacle : obstacles)
	{
		if(obstacle->uniqueID == changes.id) // update this obstacle
		{
			auto * spellObstacle = dynamic_cast<SpellCreatedObstacle *>(obstacle.get());
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

CArmedInstance * BattleInfo::battleGetArmyObject(BattleSide side) const
{
	return const_cast<CArmedInstance*>(CBattleInfoEssentials::battleGetArmyObject(side));
}

CGHeroInstance * BattleInfo::battleGetFightingHero(BattleSide side) const
{
	return const_cast<CGHeroInstance*>(CBattleInfoEssentials::battleGetFightingHero(side));
}

#if SCRIPTING_ENABLED
scripting::Pool * BattleInfo::getContextPool() const
{
	//this is real battle, use global scripting context pool
	//TODO: make this line not ugly
	return battleGetFightingHero(BattleSide::ATTACKER)->cb->getGlobalContextPool();
}
#endif

bool CMP_stack::operator()(const battle::Unit * a, const battle::Unit * b) const
{
	switch(phase)
	{
	case 0: //catapult moves after turrets
		return a->creatureIndex() > b->creatureIndex(); //catapult is 145 and turrets are 149
	case 1:
	case 2:
	case 3:
		{
			int as = a->getInitiative(turn);
			int bs = b->getInitiative(turn);

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

CMP_stack::CMP_stack(int Phase, int Turn, BattleSide Side):
	phase(Phase), 
	turn(Turn), 
	side(Side) 
{
}

VCMI_LIB_NAMESPACE_END

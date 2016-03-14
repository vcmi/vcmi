/*
 * BattleState.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BattleState.h"

#include <numeric>
#include "VCMI_Lib.h"
#include "mapObjects/CObjectHandler.h"
#include "CHeroHandler.h"
#include "CCreatureHandler.h"
#include "spells/CSpellHandler.h"
#include "CTownHandler.h"
#include "NetPacks.h"
#include "JsonNode.h"
#include "filesystem/Filesystem.h"
#include "CRandomGenerator.h"
#include "mapObjects/CGTownInstance.h"

const CStack * BattleInfo::getNextStack() const
{
	std::vector<const CStack *> hlp;
	battleGetStackQueue(hlp, 1, -1);

	if(hlp.size())
		return hlp[0];
	else
		return nullptr;
}

int BattleInfo::getAvaliableHex(CreatureID creID, bool attackerOwned, int initialPos) const
{
	bool twoHex = VLC->creh->creatures[creID]->isDoubleWide();
	//bool flying = VLC->creh->creatures[creID]->isFlying();

	int pos;
	if (initialPos > -1)
		pos = initialPos;
	else //summon elementals depending on player side
	{
 		if (attackerOwned)
	 		pos = 0; //top left
 		else
 			pos = GameConstants::BFIELD_WIDTH - 1; //top right
 	}

	auto accessibility = getAccesibility();

	std::set<BattleHex> occupyable;
	for(int i = 0; i < accessibility.size(); i++)
		if(accessibility.accessible(i, twoHex, attackerOwned))
			occupyable.insert(i);

	if (occupyable.empty())
	{
		return BattleHex::INVALID; //all tiles are covered
	}

	return BattleHex::getClosestTile(attackerOwned, pos, occupyable);
}

std::pair< std::vector<BattleHex>, int > BattleInfo::getPath(BattleHex start, BattleHex dest, const CStack *stack)
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

ui32 BattleInfo::calculateDmg( const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero,
	bool shooting, ui8 charge, bool lucky, bool unlucky, bool deathBlow, bool ballistaDoubleDmg, CRandomGenerator & rand )
{
	TDmgRange range = calculateDmgRange(attacker, defender, shooting, charge, lucky, unlucky, deathBlow, ballistaDoubleDmg);

	if(range.first != range.second)
	{
		int valuesToAverage[10];
		int howManyToAv = std::min<ui32>(10, attacker->count);
		for (int g=0; g<howManyToAv; ++g)
		{
			valuesToAverage[g] = rand.nextInt(range.first, range.second);
		}

		return std::accumulate(valuesToAverage, valuesToAverage + howManyToAv, 0) / howManyToAv;
	}
	else
		return range.first;
}

void BattleInfo::calculateCasualties( std::map<ui32,si32> *casualties ) const
{
	for(auto & elem : stacks)//setting casualties
	{
		const CStack * const st = elem;
		si32 killed = (st->alive() ? (st->baseAmount - st->count + st->resurrected) : st->baseAmount);
		vstd::amax(killed, 0);
		if(killed)
			casualties[!st->attackerOwned][st->getCreature()->idNumber] += killed;
	}
}

CStack * BattleInfo::generateNewStack(const CStackInstance &base, bool attackerOwned, SlotID slot, BattleHex position) const
{
	int stackID = getIdForNewStack();
	PlayerColor owner = sides[attackerOwned ? 0 : 1].color;
	assert((owner >= PlayerColor::PLAYER_LIMIT)  ||
		   (base.armyObj && base.armyObj->tempOwner == owner));

	auto  ret = new CStack(&base, owner, stackID, attackerOwned, slot);
	ret->position = getAvaliableHex (base.getCreatureID(), attackerOwned, position); //TODO: what if no free tile on battlefield was found?
	ret->state.insert(EBattleStackState::ALIVE);  //alive state indication
	return ret;
}

CStack * BattleInfo::generateNewStack(const CStackBasicDescriptor &base, bool attackerOwned, SlotID slot, BattleHex position) const
{
	int stackID = getIdForNewStack();
	PlayerColor owner = sides[attackerOwned ? 0 : 1].color;
	auto  ret = new CStack(&base, owner, stackID, attackerOwned, slot);
	ret->position = position;
	ret->state.insert(EBattleStackState::ALIVE);  //alive state indication
	return ret;
}

const CGHeroInstance * BattleInfo::battleGetOwner(const CStack * stack) const
{
	return sides[!stack->attackerOwned].hero;
}

void BattleInfo::localInit()
{
	for(int i = 0; i < 2; i++)
	{
		auto armyObj = battleGetArmyObject(i);
		armyObj->battle = this;
		armyObj->attachTo(this);
	}

	for(CStack *s : stacks)
		localInitStack(s);

	exportBonuses();
}

void BattleInfo::localInitStack(CStack * s)
{
	s->exportBonuses();
	if(s->base) //stack originating from "real" stack in garrison -> attach to it
	{
		s->attachTo(const_cast<CStackInstance*>(s->base));
	}
	else //attach directly to obj to which stack belongs and creature type
	{
		CArmedInstance *army = battleGetArmyObject(!s->attackerOwned);
		s->attachTo(army);
		assert(s->type);
		s->attachTo(const_cast<CCreature*>(s->type));
	}
	s->postInit();
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
				pom.push_back(value.Float());
			}

			dest.push_back(pom);
		}
	}
}

//RNG that works like H3 one
struct RandGen
{
	int seed;

	void srand(int s)
	{
		seed = s;
	}
	void srand(int3 pos)
	{
		srand(110291 * pos.x + 167801 * pos.y + 81569);
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

BattleInfo * BattleInfo::setupBattle( int3 tile, ETerrainType terrain, BFieldType battlefieldType, const CArmedInstance *armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance *town )
{
	CMP_stack cmpst;
	auto curB = new BattleInfo;

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
		curB->terrainType = VLC->townh->factions[town->subID]->nativeTerrain;
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

		for (int b = 0; b < curB->si.wallState.size(); ++b)
		{
			curB->si.wallState[b] = EWallState::INTACT;
		}

		if (!town->hasBuilt(BuildingID::CITADEL))
		{
			curB->si.wallState[EWallPart::KEEP] = EWallState::NONE;
		}

		if (!town->hasBuilt(BuildingID::CASTLE))
		{
			curB->si.wallState[EWallPart::UPPER_TOWER] = EWallState::NONE;
			curB->si.wallState[EWallPart::BOTTOM_TOWER] = EWallState::NONE;
		}
	}

	//randomize obstacles
 	if (town == nullptr && !creatureBank) //do it only when it's not siege and not creature bank
 	{
		const int ABSOLUTE_OBSTACLES_COUNT = 34, USUAL_OBSTACLES_COUNT = 91; //shouldn't be changes if we want H3-like obstacle placement

		RandGen r;
		auto ourRand = [&]{ return r.rand(); };
		r.srand(tile);
		r.rand(1,8); //battle sound ID to play... can't do anything with it here
		int tilesToBlock = r.rand(5,12);
		const int specialBattlefield = battlefieldTypeToBI(battlefieldType);

		std::vector<BattleHex> blockedTiles;

		auto appropriateAbsoluteObstacle = [&](int id)
		{
			return VLC->heroh->absoluteObstacles[id].isAppropriate(curB->terrainType, specialBattlefield);
		};
		auto appropriateUsualObstacle = [&](int id) -> bool
		{
			return VLC->heroh->obstacles[id].isAppropriate(curB->terrainType, specialBattlefield);
		};

		if(r.rand(1,100) <= 40) //put cliff-like obstacle
		{
			RangeGenerator obidgen(0, ABSOLUTE_OBSTACLES_COUNT-1, ourRand);

			try
			{
				auto obstPtr = std::make_shared<CObstacleInstance>();
				obstPtr->obstacleType = CObstacleInstance::ABSOLUTE_OBSTACLE;
				obstPtr->ID = obidgen.getSuchNumber(appropriateAbsoluteObstacle);
				obstPtr->uniqueID = curB->obstacles.size();
				curB->obstacles.push_back(obstPtr);

				for(BattleHex blocked : obstPtr->getBlockedTiles())
					blockedTiles.push_back(blocked);
				tilesToBlock -= VLC->heroh->absoluteObstacles[obstPtr->ID].blockedTiles.size() / 2;
			}
			catch(RangeGenerator::ExhaustedPossibilities &)
			{
				//silently ignore, if we can't place absolute obstacle, we'll go with the usual ones
			}
		}

		RangeGenerator obidgen(0, USUAL_OBSTACLES_COUNT-1, ourRand);
		try
		{
			while(tilesToBlock > 0)
			{
				const int obid = obidgen.getSuchNumber(appropriateUsualObstacle);
				const CObstacleInfo &obi = VLC->heroh->obstacles[obid];

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
				obstPtr->uniqueID = curB->obstacles.size();
				curB->obstacles.push_back(obstPtr);

				for(BattleHex blocked : obstPtr->getBlockedTiles())
					blockedTiles.push_back(blocked);
				tilesToBlock -= obi.blockedTiles.size();
			}
		}
		catch(RangeGenerator::ExhaustedPossibilities &)
		{
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
		commanderField.push_back (position.Float());
	}
	for (auto position : config["commanderPositions"]["creBank"].Vector())
	{
		commanderBank.push_back (position.Float());
	}


	//adding war machines
	if(!creatureBank)
	{
		//Checks if hero has artifact and create appropriate stack
		auto handleWarMachine= [&](int side, ArtifactPosition artslot, CreatureID cretype, BattleHex hex)
		{
			if(heroes[side] && heroes[side]->getArt(artslot))
				stacks.push_back(curB->generateNewStack(CStackBasicDescriptor(cretype, 1), !side, SlotID::WAR_MACHINES_SLOT, hex));
		};

		handleWarMachine(0, ArtifactPosition::MACH1, CreatureID::BALLISTA, 52);
		handleWarMachine(0, ArtifactPosition::MACH2, CreatureID::AMMO_CART, 18);
		handleWarMachine(0, ArtifactPosition::MACH3, CreatureID::FIRST_AID_TENT, 154);
		if(town && town->hasFort())
			handleWarMachine(0, ArtifactPosition::MACH4, CreatureID::CATAPULT, 120);

		if(!town) //defending hero shouldn't receive ballista (bug #551)
			handleWarMachine(1, ArtifactPosition::MACH1, CreatureID::BALLISTA, 66);
		handleWarMachine(1, ArtifactPosition::MACH2, CreatureID::AMMO_CART, 32);
		handleWarMachine(1, ArtifactPosition::MACH3, CreatureID::FIRST_AID_TENT, 168);
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

			CStack * stack = curB->generateNewStack(*i->second, !side, i->first, pos);
			stacks.push_back(stack);
		}
	}

	//adding commanders
	for (int i = 0; i < 2; ++i)
	{
		if (heroes[i] && heroes[i]->commander)
		{
			CStack * stack = curB->generateNewStack (*heroes[i]->commander, !i, SlotID::COMMANDER_SLOT_PLACEHOLDER,
				creatureBank ? commanderBank[i] : commanderField[i]);
			stacks.push_back(stack);
		}

	}

	if (curB->town && curB->town->fortLevel() >= CGTownInstance::CITADEL)
	{
		// keep tower
		CStack * stack = curB->generateNewStack(CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), false, SlotID::ARROW_TOWERS_SLOT, -2);
		stacks.push_back(stack);

		if (curB->town->fortLevel() >= CGTownInstance::CASTLE)
		{
			// lower tower + upper tower
			CStack * stack = curB->generateNewStack(CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), false, SlotID::ARROW_TOWERS_SLOT, -4);
			stacks.push_back(stack);
			stack = curB->generateNewStack(CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), false, SlotID::ARROW_TOWERS_SLOT, -3);
			stacks.push_back(stack);
		}

		//moat
		auto moat = std::make_shared<MoatObstacle>();
		moat->ID = curB->town->subID;
		moat->obstacleType = CObstacleInstance::MOAT;
		moat->uniqueID = curB->obstacles.size();
		curB->obstacles.push_back(moat);
	}

	std::stable_sort(stacks.begin(),stacks.end(),cmpst);

	//spell level limiting bonus
	curB->addNewBonus(new Bonus(Bonus::ONE_BATTLE, Bonus::LEVEL_SPELL_IMMUNITY, Bonus::OTHER,
		0, -1, -1, Bonus::INDEPENDENT_MAX));

	//giving terrain overalay premies
	int bonusSubtype = -1;
	switch(battlefieldType)
	{
	case BFieldType::MAGIC_PLAINS:
		{
			bonusSubtype = 0;
		}
	case BFieldType::FIERY_FIELDS:
		{
			if(bonusSubtype == -1) bonusSubtype = 1;
		}
	case BFieldType::ROCKLANDS:
		{
			if(bonusSubtype == -1) bonusSubtype = 8;
		}
	case BFieldType::MAGIC_CLOUDS:
		{
			if(bonusSubtype == -1) bonusSubtype = 2;
		}
	case BFieldType::LUCID_POOLS:
		{
			if(bonusSubtype == -1) bonusSubtype = 4;
		}

		{ //common part for cases 9, 14, 15, 16, 17
			curB->addNewBonus(new Bonus(Bonus::ONE_BATTLE, Bonus::MAGIC_SCHOOL_SKILL, Bonus::TERRAIN_OVERLAY, 3, -1, "", bonusSubtype));
			break;
		}

	case BFieldType::HOLY_GROUND:
		{
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, +1, Bonus::TERRAIN_OVERLAY)->addLimiter(std::make_shared<CreatureAlignmentLimiter>(EAlignment::GOOD)));
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, -1, Bonus::TERRAIN_OVERLAY)->addLimiter(std::make_shared<CreatureAlignmentLimiter>(EAlignment::EVIL)));
			break;
		}
	case BFieldType::CLOVER_FIELD:
		{ //+2 luck bonus for neutral creatures
			curB->addNewBonus(makeFeature(Bonus::LUCK, Bonus::ONE_BATTLE, 0, +2, Bonus::TERRAIN_OVERLAY)->addLimiter(std::make_shared<CreatureAlignmentLimiter>(EAlignment::NEUTRAL)));
			break;
		}
	case BFieldType::EVIL_FOG:
		{
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, -1, Bonus::TERRAIN_OVERLAY)->addLimiter(std::make_shared<CreatureAlignmentLimiter>(EAlignment::GOOD)));
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, +1, Bonus::TERRAIN_OVERLAY)->addLimiter(std::make_shared<CreatureAlignmentLimiter>(EAlignment::EVIL)));
			break;
		}
	case BFieldType::CURSED_GROUND:
		{
			curB->addNewBonus(makeFeature(Bonus::NO_MORALE, Bonus::ONE_BATTLE, 0, 0, Bonus::TERRAIN_OVERLAY));
			curB->addNewBonus(makeFeature(Bonus::NO_LUCK, Bonus::ONE_BATTLE, 0, 0, Bonus::TERRAIN_OVERLAY));
			Bonus * b = makeFeature(Bonus::LEVEL_SPELL_IMMUNITY, Bonus::ONE_BATTLE, GameConstants::SPELL_LEVELS, 1, Bonus::TERRAIN_OVERLAY);
			b->valType = Bonus::INDEPENDENT_MAX;
			curB->addNewBonus(b);
			break;
		}
	}
	//overlay premies given

	//native terrain bonuses
	auto nativeTerrain = std::make_shared<CreatureNativeTerrainLimiter>(curB->terrainType);
	curB->addNewBonus(makeFeature(Bonus::STACKS_SPEED, Bonus::ONE_BATTLE, 0, 1, Bonus::TERRAIN_NATIVE)->addLimiter(nativeTerrain));
	curB->addNewBonus(makeFeature(Bonus::PRIMARY_SKILL, Bonus::ONE_BATTLE, PrimarySkill::ATTACK, 1, Bonus::TERRAIN_NATIVE)->addLimiter(nativeTerrain));
	curB->addNewBonus(makeFeature(Bonus::PRIMARY_SKILL, Bonus::ONE_BATTLE, PrimarySkill::DEFENSE, 1, Bonus::TERRAIN_NATIVE)->addLimiter(nativeTerrain));
	//////////////////////////////////////////////////////////////////////////

	//tactics
	bool isTacticsAllowed = !creatureBank; //no tactics in creature banks

	int tacticLvls[2] = {0};
	for(int i = 0; i < ARRAY_COUNT(tacticLvls); i++)
	{
		if(heroes[i])
			tacticLvls[i] += heroes[i]->getSecSkillLevel(SecondarySkill::TACTICS);
	}
	int tacticsSkillDiff = tacticLvls[0] - tacticLvls[1];

	if(tacticsSkillDiff && isTacticsAllowed)
	{
		curB->tacticsSide = tacticsSkillDiff < 0;
		curB->tacticDistance = std::abs(tacticsSkillDiff)*2 + 1;
	}
	else
		curB->tacticDistance = 0;


	// workaround  bonuses affecting only enemy - DOES NOT WORK
	for(int i = 0; i < 2; i++)
	{
		TNodes nodes;
		curB->battleGetArmyObject(i)->getRedAncestors(nodes);
		for(CBonusSystemNode *n : nodes)
		{
			for(Bonus *b : n->getExportedBonusList())
			{
				if(b->effectRange == Bonus::ONLY_ENEMY_ARMY/* && b->propagator && b->propagator->shouldBeAttached(curB)*/)
				{
					auto bCopy = new Bonus(*b);
					bCopy->effectRange = Bonus::NO_LIMIT;
					bCopy->propagator.reset();
					bCopy->limiter.reset(new StackOwnerLimiter(curB->sides[!i].color));
					curB->addNewBonus(bCopy);
				}
			}
		}
	}

	return curB;
}

const CGHeroInstance * BattleInfo::getHero( PlayerColor player ) const
{
	for(int i = 0; i < sides.size(); i++)
		if(sides[i].color == player)
			return sides[i].hero;

	logGlobal->errorStream() << "Player " << player << " is not in battle!";
	return nullptr;
}

PlayerColor BattleInfo::theOtherPlayer(PlayerColor player) const
{
	return sides[!whatSide(player)].color;
}

ui8 BattleInfo::whatSide(PlayerColor player) const
{
	for(int i = 0; i < sides.size(); i++)
		if(sides[i].color == player)
			return i;

	logGlobal->warnStream() << "BattleInfo::whatSide: Player " << player << " is not in battle!";
	return -1;
}

int BattleInfo::getIdForNewStack() const
{
	if(stacks.size())
	{
		//stacks vector may be sorted not by ID and they may be not contiguous -> find stack with max ID
		auto highestIDStack = *std::max_element(stacks.begin(), stacks.end(),
								[](const CStack *a, const CStack *b) { return a->ID < b->ID; });

		return highestIDStack->ID + 1;
	}

	return 0;
}

std::shared_ptr<CObstacleInstance> BattleInfo::getObstacleOnTile(BattleHex tile) const
{
	for(auto &obs : obstacles)
		if(vstd::contains(obs->getAffectedTiles(), tile))
			return obs;

	return std::shared_ptr<CObstacleInstance>();
}

BattlefieldBI::BattlefieldBI BattleInfo::battlefieldTypeToBI(BFieldType bfieldType)
{
	static const std::map<BFieldType, BattlefieldBI::BattlefieldBI> theMap =
	{
		{BFieldType::CLOVER_FIELD, BattlefieldBI::CLOVER_FIELD},
		{BFieldType::CURSED_GROUND, BattlefieldBI::CURSED_GROUND},
		{BFieldType::EVIL_FOG, BattlefieldBI::EVIL_FOG},
		{BFieldType::FAVORABLE_WINDS, BattlefieldBI::NONE},
		{BFieldType::FIERY_FIELDS, BattlefieldBI::FIERY_FIELDS},
		{BFieldType::HOLY_GROUND, BattlefieldBI::HOLY_GROUND},
		{BFieldType::LUCID_POOLS, BattlefieldBI::LUCID_POOLS},
		{BFieldType::MAGIC_CLOUDS, BattlefieldBI::MAGIC_CLOUDS},
		{BFieldType::MAGIC_PLAINS, BattlefieldBI::MAGIC_PLAINS},
		{BFieldType::ROCKLANDS, BattlefieldBI::ROCKLANDS},
		{BFieldType::SAND_SHORE, BattlefieldBI::COASTAL}
	};

	auto itr = theMap.find(bfieldType);
	if(itr != theMap.end())
		return itr->second;

	return BattlefieldBI::NONE;
}

CStack * BattleInfo::getStack(int stackID, bool onlyAlive /*= true*/)
{
	return const_cast<CStack *>(battleGetStackByID(stackID, onlyAlive));
}

BattleInfo::BattleInfo()
{
	setBattle(this);
	setNodeType(BATTLE);
}

CArmedInstance * BattleInfo::battleGetArmyObject(ui8 side) const
{
	return const_cast<CArmedInstance*>(CBattleInfoEssentials::battleGetArmyObject(side));
}

CGHeroInstance * BattleInfo::battleGetFightingHero(ui8 side) const
{
	return const_cast<CGHeroInstance*>(CBattleInfoEssentials::battleGetFightingHero(side));
}

CStack::CStack(const CStackInstance *Base, PlayerColor O, int I, bool AO, SlotID S)
	: base(Base), ID(I), owner(O), slot(S), attackerOwned(AO),
	counterAttacksPerformed(0),counterAttacksTotalCache(0), cloneID(-1)
{
	assert(base);
	type = base->type;
	count = baseAmount = base->count;
	setNodeType(STACK_BATTLE);
}
CStack::CStack()
{
	init();
	setNodeType(STACK_BATTLE);
}
CStack::CStack(const CStackBasicDescriptor *stack, PlayerColor O, int I, bool AO, SlotID S)
	: base(nullptr), ID(I), owner(O), slot(S), attackerOwned(AO), counterAttacksPerformed(0),
	 cloneID(-1)
{
	type = stack->type;
	count = baseAmount = stack->count;
	setNodeType(STACK_BATTLE);
}

void CStack::init()
{
	base = nullptr;
	type = nullptr;
	ID = -1;
	count = baseAmount = -1;
	firstHPleft = -1;
	owner = PlayerColor::NEUTRAL;
	slot = SlotID(255);
	attackerOwned = false;
	position = BattleHex();
	counterAttacksPerformed = 0;
	counterAttacksTotalCache = 0;
	cloneID = -1;
}

void CStack::postInit()
{
	assert(type);
	assert(getParentNodes().size());

	firstHPleft = MaxHealth();
	shots = getCreature()->valOfBonuses(Bonus::SHOTS);
	counterAttacksPerformed = 0;
	counterAttacksTotalCache = 0;
	casts = valOfBonuses(Bonus::CASTS);
	resurrected = 0;
	cloneID = -1;
}

ui32 CStack::level() const
{
	if (base)
		return base->getLevel(); //creatture or commander
	else
		return std::max(1, (int)getCreature()->level); //war machine, clone etc
}

si32 CStack::magicResistance() const
{
	si32 magicResistance;
	if (base) //TODO: make war machines receive aura of magic resistance
	{
		magicResistance = base->magicResistance();
		int auraBonus = 0;
		for (const CStack * stack : base->armyObj->battle-> batteAdjacentCreatures(this))
	{
		if (stack->owner == owner)
		{
			vstd::amax(auraBonus, stack->valOfBonuses(Bonus::SPELL_RESISTANCE_AURA)); //max value
		}
	}
		magicResistance += auraBonus;
		vstd::amin (magicResistance, 100);
	}
	else
		magicResistance = type->magicResistance();
	return magicResistance;
}

void CStack::stackEffectToFeature(std::vector<Bonus> & sf, const Bonus & sse)
{
	const CSpell * sp = SpellID(sse.sid).toSpell();

	std::vector<Bonus> tmp;
	sp->getEffects(tmp, sse.val);

	for(Bonus& b : tmp)
	{
		if(b.turnsRemain == 0)
			b.turnsRemain = sse.turnsRemain;
		sf.push_back(b);
	}
}

bool CStack::willMove(int turn /*= 0*/) const
{
	return ( turn ? true : !vstd::contains(state, EBattleStackState::DEFENDING) )
		&& !moved(turn)
		&& canMove(turn);
}

bool CStack::canMove( int turn /*= 0*/ ) const
{
	return alive()
		&& !hasBonus(Selector::type(Bonus::NOT_ACTIVE).And(Selector::turns(turn))); //eg. Ammo Cart or blinded creature
}

bool CStack::moved( int turn /*= 0*/ ) const
{
	if(!turn)
		return vstd::contains(state, EBattleStackState::MOVED);
	else
		return false;
}

bool CStack::waited(int turn /*= 0*/) const
{
	if(!turn)
		return vstd::contains(state, EBattleStackState::WAITING);
	else
		return false;
}

bool CStack::doubleWide() const
{
	return getCreature()->doubleWide;
}

BattleHex CStack::occupiedHex() const
{
	return occupiedHex(position);
}

BattleHex CStack::occupiedHex(BattleHex assumedPos) const
{
	if (doubleWide())
	{
		if (attackerOwned)
			return assumedPos - 1;
		else
			return assumedPos + 1;
	}
	else
	{
		return BattleHex::INVALID;
	}
}

std::vector<BattleHex> CStack::getHexes() const
{
	return getHexes(position);
}

std::vector<BattleHex> CStack::getHexes(BattleHex assumedPos) const
{
	return getHexes(assumedPos, doubleWide(), attackerOwned);
}

std::vector<BattleHex> CStack::getHexes(BattleHex assumedPos, bool twoHex, bool AttackerOwned)
{
	std::vector<BattleHex> hexes;
	hexes.push_back(assumedPos);

	if (twoHex)
	{
		if (AttackerOwned)
			hexes.push_back(assumedPos - 1);
		else
			hexes.push_back(assumedPos + 1);
	}

	return hexes;
}

bool CStack::coversPos(BattleHex pos) const
{
	return vstd::contains(getHexes(), pos);
}

std::vector<BattleHex> CStack::getSurroundingHexes(BattleHex attackerPos) const
{
	BattleHex hex = (attackerPos != BattleHex::INVALID) ? attackerPos : position; //use hypothetical position
	std::vector<BattleHex> hexes;
	if (doubleWide())
	{
		const int WN = GameConstants::BFIELD_WIDTH;
		if(attackerOwned)
		{ //position is equal to front hex
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN+2 : WN+1 ), hexes);
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN+1 : WN ), hexes);
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN : WN-1 ), hexes);
			BattleHex::checkAndPush(hex - 2, hexes);
			BattleHex::checkAndPush(hex + 1, hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN-2 : WN-1 ), hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN-1 : WN ), hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN : WN+1 ), hexes);
		}
		else
		{
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN+1 : WN ), hexes);
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN : WN-1 ), hexes);
			BattleHex::checkAndPush(hex - ( (hex/WN)%2 ? WN-1 : WN-2 ), hexes);
			BattleHex::checkAndPush(hex + 2, hexes);
			BattleHex::checkAndPush(hex - 1, hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN-1 : WN ), hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN : WN+1 ), hexes);
			BattleHex::checkAndPush(hex + ( (hex/WN)%2 ? WN+1 : WN+2 ), hexes);
		}
		return hexes;
	}
	else
	{
		return hex.neighbouringTiles();
	}
}

std::vector<si32> CStack::activeSpells() const
{
	std::vector<si32> ret;

	TBonusListPtr spellEffects = getSpellBonuses();
	for(const Bonus *it : *spellEffects)
	{
		if (!vstd::contains(ret, it->sid)) //do not duplicate spells with multiple effects
			ret.push_back(it->sid);
	}

	return ret;
}

CStack::~CStack()
{
	detachFromAll();
}

const CGHeroInstance * CStack::getMyHero() const
{
	if(base)
		return dynamic_cast<const CGHeroInstance *>(base->armyObj);
	else //we are attached directly?
		for(const CBonusSystemNode *n : getParentNodes())
			if(n->getNodeType() == HERO)
				return dynamic_cast<const CGHeroInstance *>(n);

	return nullptr;
}

ui32 CStack::totalHelth() const
{
	return (MaxHealth() * (count-1)) + firstHPleft;
}

std::string CStack::nodeName() const
{
	std::ostringstream oss;
	oss << "Battle stack [" << ID << "]: " << count << " creatures of ";
	if(type)
		oss << type->namePl;
	else
		oss << "[UNDEFINED TYPE]";

	oss << " from slot " << slot;
	if(base && base->armyObj)
		oss << " of armyobj=" << base->armyObj->id.getNum();
	return oss.str();
}

std::pair<int,int> CStack::countKilledByAttack(int damageReceived) const
{
	int killedCount = 0;
	int newRemainingHP = 0;

	killedCount = damageReceived / MaxHealth();
	unsigned damageFirst = damageReceived % MaxHealth();

	if (damageReceived && vstd::contains(state, EBattleStackState::CLONED)) // block ability should not kill clone (0 damage)
	{
		killedCount = count;
	}
	else
	{
		if( firstHPleft <= damageFirst )
		{
			killedCount++;
			newRemainingHP = firstHPleft + MaxHealth() - damageFirst;
		}
		else
		{
			newRemainingHP = firstHPleft - damageFirst;
		}
	}

	return std::make_pair(killedCount, newRemainingHP);
}

void CStack::prepareAttacked(BattleStackAttacked &bsa, CRandomGenerator & rand, boost::optional<int> customCount /*= boost::none*/) const
{
	auto afterAttack = countKilledByAttack(bsa.damageAmount);

	bsa.killedAmount = afterAttack.first;
	bsa.newHP = afterAttack.second;


	if(bsa.damageAmount && vstd::contains(state, EBattleStackState::CLONED)) // block ability should not kill clone (0 damage)
	{
		bsa.flags |= BattleStackAttacked::CLONE_KILLED;
		return; // no rebirth I believe
	}

	const int countToUse = customCount ? *customCount : count;

	if(countToUse <= bsa.killedAmount) //stack killed
	{
		bsa.newAmount = 0;
		bsa.flags |= BattleStackAttacked::KILLED;
		bsa.killedAmount = countToUse; //we cannot kill more creatures than we have

		int resurrectFactor = valOfBonuses(Bonus::REBIRTH);
		if(resurrectFactor > 0 && casts) //there must be casts left
		{
			int resurrectedStackCount = base->count * resurrectFactor / 100;

			// last stack has proportional chance to rebirth
			auto diff = base->count * resurrectFactor / 100.0 - resurrectedStackCount;
			if (diff > rand.nextDouble(0, 0.99))
			{
				resurrectedStackCount += 1;
			}

			if(hasBonusOfType(Bonus::REBIRTH, 1))
			{
				// resurrect at least one Sacred Phoenix
				vstd::amax(resurrectedStackCount, 1);
			}

			if(resurrectedStackCount > 0)
			{
				bsa.flags |= BattleStackAttacked::REBIRTH;
				bsa.newAmount = resurrectedStackCount; //risky?
				bsa.newHP = MaxHealth(); //resore full health
			}
		}
	}
	else
	{
		bsa.newAmount = countToUse - bsa.killedAmount;
	}
}

bool CStack::isMeleeAttackPossible(const CStack * attacker, const CStack * defender, BattleHex attackerPos /*= BattleHex::INVALID*/, BattleHex defenderPos /*= BattleHex::INVALID*/)
{
	if (!attackerPos.isValid())
	{
		attackerPos = attacker->position;
	}
	if (!defenderPos.isValid())
	{
		defenderPos = defender->position;
	}

	return
		(BattleHex::mutualPosition(attackerPos, defenderPos) >= 0)						//front <=> front
		|| (attacker->doubleWide()									//back <=> front
		&& BattleHex::mutualPosition(attackerPos + (attacker->attackerOwned ? -1 : 1), defenderPos) >= 0)
		|| (defender->doubleWide()									//front <=> back
		&& BattleHex::mutualPosition(attackerPos, defenderPos + (defender->attackerOwned ? -1 : 1)) >= 0)
		|| (defender->doubleWide() && attacker->doubleWide()//back <=> back
		&& BattleHex::mutualPosition(attackerPos + (attacker->attackerOwned ? -1 : 1), defenderPos + (defender->attackerOwned ? -1 : 1)) >= 0);

}

bool CStack::ableToRetaliate() const //FIXME: crash after clone is killed
{
	return alive()
		&& (counterAttacksPerformed < counterAttacksTotal() || hasBonusOfType(Bonus::UNLIMITED_RETALIATIONS))
		&& !hasBonusOfType(Bonus::SIEGE_WEAPON)
		&& !hasBonusOfType(Bonus::HYPNOTIZED)
		&& !hasBonusOfType(Bonus::NO_RETALIATION);
}

ui8 CStack::counterAttacksTotal() const
{
	//after dispell bonus should remain during current round
	ui8 val = 1 + valOfBonuses(Bonus::ADDITIONAL_RETALIATION);
	vstd::amax(counterAttacksTotalCache, val);
	return counterAttacksTotalCache;
}

si8 CStack::counterAttacksRemaining() const
{
	return counterAttacksTotal() - counterAttacksPerformed;
}

std::string CStack::getName() const
{
	return (count > 1) ? type->namePl : type->nameSing; //War machines can't use base
}

bool CStack::isValidTarget(bool allowDead/* = false*/) const
{
	return (alive() || (allowDead && isDead())) && position.isValid() && !isTurret();
}

bool CStack::isDead() const
{
	return !alive() && !isGhost();
}

bool CStack::isGhost() const
{
	return vstd::contains(state,EBattleStackState::GHOST);
}

bool CStack::isTurret() const
{
	return type->idNumber == CreatureID::ARROW_TOWERS;
}

bool CStack::canBeHealed() const
{
	return firstHPleft < MaxHealth()
		&& isValidTarget()
		&& !hasBonusOfType(Bonus::SIEGE_WEAPON);
}

void CStack::makeGhost()
{
	state.erase(EBattleStackState::ALIVE);
	state.insert(EBattleStackState::GHOST_PENDING);
}

ui32 CStack::calculateHealedHealthPoints(ui32 toHeal, const bool resurrect) const
{
	if(!resurrect && !alive())
	{
		logGlobal->warnStream() <<"Attempt to heal corpse detected.";
		return 0;
	}

	return std::min<ui32>(toHeal, MaxHealth() - firstHPleft + (resurrect ? (baseAmount - count) * MaxHealth() : 0));
}

ui8 CStack::getSpellSchoolLevel(const CSpell * spell, int * outSelectedSchool) const
{
	int skill = valOfBonuses(Selector::typeSubtype(Bonus::SPELLCASTER, spell->id));

	vstd::abetween(skill, 0, 3);

	return skill;
}

ui32 CStack::getSpellBonus(const CSpell * spell, ui32 base, const CStack * affectedStack) const
{
	//stacks does not have sorcery-like bonuses (yet?)
	return base;
}

int CStack::getEffectLevel(const CSpell * spell) const
{
	return getSpellSchoolLevel(spell);
}

int CStack::getEffectPower(const CSpell * spell) const
{
	return valOfBonuses(Bonus::CREATURE_SPELL_POWER) * count / 100;
}

int CStack::getEnchantPower(const CSpell * spell) const
{
	int res = valOfBonuses(Bonus::CREATURE_ENCHANT_POWER);
	if(res<=0)
		res = 3;//default for creatures
	return res;
}

int CStack::getEffectValue(const CSpell * spell) const
{
	return valOfBonuses(Bonus::SPECIFIC_SPELL_POWER, spell->id.toEnum()) * count;
}

const PlayerColor CStack::getOwner() const
{
	return owner;
}


bool CMP_stack::operator()( const CStack* a, const CStack* b )
{
	switch(phase)
	{
	case 0: //catapult moves after turrets
		return a->getCreature()->idNumber > b->getCreature()->idNumber; //catapult is 145 and turrets are 149
	case 1: //fastest first, upper slot first
		{
			int as = a->Speed(turn), bs = b->Speed(turn);
			if(as != bs)
				return as > bs;
			else
				return a->slot < b->slot;
		}
	case 2: //fastest last, upper slot first
		//TODO: should be replaced with order of receiving morale!
	case 3: //fastest last, upper slot first
		{
			int as = a->Speed(turn), bs = b->Speed(turn);
			if(as != bs)
				return as < bs;
			else
				return a->slot < b->slot;
		}
	default:
		assert(0);
		return false;
	}

}

CMP_stack::CMP_stack( int Phase /*= 1*/, int Turn )
{
	phase = Phase;
	turn = Turn;
}


SideInBattle::SideInBattle()
{
	hero = nullptr;
	armyObject = nullptr;
	castSpellsCount = 0;
	enchanterCounter = 0;
}

void SideInBattle::init(const CGHeroInstance *Hero, const CArmedInstance *Army)
{
	hero = Hero;
	armyObject = Army;
	color = armyObject->getOwner();

	if(color == PlayerColor::UNFLAGGABLE)
		color = PlayerColor::NEUTRAL;
}

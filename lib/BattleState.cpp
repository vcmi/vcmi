#include "StdInc.h"
#include "BattleState.h"

#include <numeric>
#include <boost/random/linear_congruential.hpp>
#include "VCMI_Lib.h"
#include "CObjectHandler.h"
#include "CHeroHandler.h"
#include "CCreatureHandler.h"
#include "CSpellHandler.h"
#include "CTownHandler.h"
#include "NetPacks.h"
#include "JsonNode.h"
#include "Filesystem/CResourceLoader.h"


/*
 * BattleState.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
extern boost::rand48 ran;

const CStack * BattleInfo::getNextStack() const
{
	std::vector<const CStack *> hlp;
	battleGetStackQueue(hlp, 1, -1);

	if(hlp.size())
		return hlp[0];
	else
		return NULL;
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

	if (!occupyable.size())
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
	bool shooting, ui8 charge, bool lucky, bool deathBlow, bool ballistaDoubleDmg )
{
	TDmgRange range = calculateDmgRange(attacker, defender, shooting, charge, lucky, deathBlow, ballistaDoubleDmg);

	if(range.first != range.second)
	{
		int valuesToAverage[10];
		int howManyToAv = std::min<ui32>(10, attacker->count);
		for (int g=0; g<howManyToAv; ++g)
		{
			valuesToAverage[g] = range.first  +  rand() % (range.second - range.first + 1);
		}

		return std::accumulate(valuesToAverage, valuesToAverage + howManyToAv, 0) / howManyToAv;
	}
	else
		return range.first;
}

void BattleInfo::calculateCasualties( std::map<ui32,si32> *casualties ) const
{
	for(ui32 i=0; i<stacks.size();i++)//setting casualties
	{
		const CStack * const st = stacks[i];
		si32 killed = (st->alive() ? st->baseAmount - st->count : st->baseAmount);
		vstd::amax(killed, 0);
		if(killed)
			casualties[!st->attackerOwned][st->getCreature()->idNumber] += killed;
	}
}

int BattleInfo::calculateSpellDuration( const CSpell * spell, const CGHeroInstance * caster, int usedSpellPower)
{
	if(!caster)
	{
		if (!usedSpellPower)
			return 3; //default duration of all creature spells
		else
			return usedSpellPower; //use creature spell power
	}
	switch(spell->id)
	{
	case SpellID::FRENZY:
		return 1;
	default: //other spells
		return caster->getPrimSkillLevel(PrimarySkill::SPELL_POWER) + caster->valOfBonuses(Bonus::SPELL_DURATION);
	}
}

CStack * BattleInfo::generateNewStack(const CStackInstance &base, bool attackerOwned, int slot, BattleHex position) const
{
	int stackID = getIdForNewStack();
	int owner = attackerOwned ? sides[0] : sides[1];
	assert((owner >= GameConstants::PLAYER_LIMIT)  ||
		   (base.armyObj && base.armyObj->tempOwner == owner));

	CStack * ret = new CStack(&base, owner, stackID, attackerOwned, slot);
	ret->position = getAvaliableHex (base.getCreatureID(), attackerOwned, position); //TODO: what if no free tile on battlefield was found?
	ret->state.insert(EBattleStackState::ALIVE);  //alive state indication
	return ret;
}

CStack * BattleInfo::generateNewStack(const CStackBasicDescriptor &base, bool attackerOwned, int slot, BattleHex position) const
{
	int stackID = getIdForNewStack();
	int owner = attackerOwned ? sides[0] : sides[1];
	CStack * ret = new CStack(&base, owner, stackID, attackerOwned, slot);
	ret->position = position;
	ret->state.insert(EBattleStackState::ALIVE);  //alive state indication
	return ret;
}

//All spells casted by hero 9resurrection, cure, sacrifice)
ui32 BattleInfo::calculateHealedHP(const CGHeroInstance * caster, const CSpell * spell, const CStack * stack, const CStack * sacrificedStack) const
{
	bool resurrect = resurrects(spell->id);
	int healedHealth;
	if (spell->id == SpellID::SACRIFICE && sacrificedStack)
		healedHealth = (caster->getPrimSkillLevel(PrimarySkill::SPELL_POWER) + sacrificedStack->MaxHealth() + spell->powers[caster->getSpellSchoolLevel(spell)]) * sacrificedStack->count;
	else
		healedHealth = caster->getPrimSkillLevel(PrimarySkill::SPELL_POWER) * spell->power + spell->powers[caster->getSpellSchoolLevel(spell)];
	healedHealth = calculateSpellBonus(healedHealth, spell, caster, stack);
	return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft + (resurrect ? stack->baseAmount * stack->MaxHealth() : 0));
}
//Archangel
ui32 BattleInfo::calculateHealedHP(int healedHealth, const CSpell * spell, const CStack * stack) const
{
	bool resurrect = resurrects(spell->id);
	return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft + (resurrect ? stack->baseAmount * stack->MaxHealth() : 0));
}
//Casted by stack, no hero bonus applied
ui32 BattleInfo::calculateHealedHP(const CSpell * spell, int usedSpellPower, int spellSchoolLevel, const CStack * stack) const
{
	bool resurrect = resurrects(spell->id);
	int healedHealth = usedSpellPower * spell->power + spell->powers[spellSchoolLevel];
	return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft + (resurrect ? stack->baseAmount * stack->MaxHealth() : 0));
}
bool BattleInfo::resurrects(SpellID spellid) const
{
	return spellid.toSpell()->isRisingSpell();
}

const CStack * BattleInfo::battleGetStack(BattleHex pos, bool onlyAlive)
{
	CStack * stack = NULL;
	for(ui32 g=0; g<stacks.size(); ++g)
	{
		if(stacks[g]->position == pos
			|| (stacks[g]->doubleWide()
			&&( (stacks[g]->attackerOwned && stacks[g]->position-1 == pos)
			||	(!stacks[g]->attackerOwned && stacks[g]->position+1 == pos)	)
			) )
		{
			if (stacks[g]->alive())
				return stacks[g]; //we prefer living stacks - there cna be only one stack on te tile, so return it imediately
			else if (!onlyAlive)
				stack = stacks[g]; //dead stacks are only accessible when there's no alive stack on this tile
		}
	}
	return stack;
}

const CGHeroInstance * BattleInfo::battleGetOwner(const CStack * stack) const
{
	return heroes[!stack->attackerOwned];
}

void BattleInfo::localInit()
{
	belligerents[0]->battle = belligerents[1]->battle = this;

	BOOST_FOREACH(CArmedInstance *b, belligerents)
		b->attachTo(this);

	BOOST_FOREACH(CStack *s, stacks)
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
		CArmedInstance *army = belligerents[!s->attackerOwned];
		s->attachTo(army);
		assert(s->type);
		s->attachTo(const_cast<CCreature*>(s->type));
	}
	s->postInit();
}

namespace CGH
{
	using namespace std;

	static void readBattlePositions(const JsonNode &node, vector< vector<int> > & dest)
	{
		BOOST_FOREACH(const JsonNode &level, node.Vector())
		{
			std::vector<int> pom;
			BOOST_FOREACH(const JsonNode &value, level.Vector())
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

	RangeGenerator(int _min, int _max, boost::function<int()> _myRand)
	{
		myRand = _myRand;
		min = _min;
		remainingCount = _max - _min + 1;
		remaining.resize(remainingCount, true);
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
	int getSuchNumber(boost::function<bool(int)> goodNumberPred = 0)
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
	boost::function<int()> myRand;
};

BattleInfo * BattleInfo::setupBattle( int3 tile, ETerrainType terrain, BFieldType battlefieldType, const CArmedInstance *armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance *town )
{
	CMP_stack cmpst;
	BattleInfo *curB = new BattleInfo;
	curB->castSpells[0] = curB->castSpells[1] = 0;
	curB->sides[0] = armies[0]->tempOwner;
	curB->sides[1] = armies[1]->tempOwner;
	if(curB->sides[1] == GameConstants::UNFLAGGABLE_PLAYER)
		curB->sides[1] = GameConstants::NEUTRAL_PLAYER;

	std::vector<CStack*> & stacks = (curB->stacks);

	curB->tile = tile;
	curB->battlefieldType = battlefieldType;
	curB->belligerents[0] = const_cast<CArmedInstance*>(armies[0]);
	curB->belligerents[1] = const_cast<CArmedInstance*>(armies[1]);
	curB->heroes[0] = const_cast<CGHeroInstance*>(heroes[0]);
	curB->heroes[1] = const_cast<CGHeroInstance*>(heroes[1]);
	curB->round = -2;
	curB->activeStack = -1;
	curB->enchanterCounter[0] = curB->enchanterCounter[1] = 0; //ready to cast

	if(town)
	{
		curB->town = town;
		curB->siege = town->fortLevel();
		curB->terrainType = VLC->townh->factions[town->town->typeID].nativeTerrain;
	}
	else
	{
		curB->town = NULL;
		curB->siege = CGTownInstance::NONE;
		curB->terrainType = terrain;
	}

	//setting up siege obstacles
	if (town && town->hasFort())
	{
		for (int b = 0; b < ARRAY_COUNT (curB->si.wallState); ++b)
		{
			curB->si.wallState[b] = 1;
		}
	}

	//randomize obstacles
 	if (town == NULL && !creatureBank) //do it only when it's not siege and not creature bank
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
				auto obstPtr = make_shared<CObstacleInstance>();
				obstPtr->obstacleType = CObstacleInstance::ABSOLUTE_OBSTACLE;
				obstPtr->ID = obidgen.getSuchNumber(appropriateAbsoluteObstacle);
				obstPtr->uniqueID = curB->obstacles.size();
				curB->obstacles.push_back(obstPtr);

				BOOST_FOREACH(BattleHex blocked, obstPtr->getBlockedTiles())
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

					BOOST_FOREACH(BattleHex blocked, obi.getBlocked(pos))
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

				auto obstPtr = make_shared<CObstacleInstance>();
				obstPtr->ID = obid;
				obstPtr->pos = posgenerator.getSuchNumber(validPosition);
				obstPtr->uniqueID = curB->obstacles.size();
				curB->obstacles.push_back(obstPtr);

				BOOST_FOREACH(BattleHex blocked, obstPtr->getBlockedTiles())
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

	BOOST_FOREACH (auto position, config["commanderPositions"]["field"].Vector())
	{
		commanderField.push_back (position.Float());
	}
	BOOST_FOREACH (auto position, config["commanderPositions"]["creBank"].Vector())
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
				stacks.push_back(curB->generateNewStack(CStackBasicDescriptor(cretype, 1), !side, 255, hex));
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
		for(TSlots::const_iterator i = armies[side]->Slots().begin(); i != armies[side]->Slots().end(); i++, k++)
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
			CStack * stack = curB->generateNewStack (*heroes[i]->commander, !i, -2, //TODO: use COMMANDER_SLOT_PLACEHOLDER
				creatureBank ? commanderBank[i] : commanderField[i]);
			stacks.push_back(stack);
		}

	}

	if (curB->siege == CGTownInstance::CITADEL || curB->siege == CGTownInstance::CASTLE)
	{
		// keep tower
		CStack * stack = curB->generateNewStack(CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), false, 255, -2);
		stacks.push_back(stack);

		if (curB->siege == CGTownInstance::CASTLE)
		{
			// lower tower + upper tower
			CStack * stack = curB->generateNewStack(CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), false, 255, -4);
			stacks.push_back(stack);
			stack = curB->generateNewStack(CStackBasicDescriptor(CreatureID::ARROW_TOWERS, 1), false, 255, -3);
			stacks.push_back(stack);
		}

		//moat
		auto moat = make_shared<MoatObstacle>();
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
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, +1, Bonus::TERRAIN_OVERLAY)->addLimiter(make_shared<CreatureAlignmentLimiter>(EAlignment::GOOD)));
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, -1, Bonus::TERRAIN_OVERLAY)->addLimiter(make_shared<CreatureAlignmentLimiter>(EAlignment::EVIL)));
			break;
		}
	case BFieldType::CLOVER_FIELD:
		{ //+2 luck bonus for neutral creatures
			curB->addNewBonus(makeFeature(Bonus::LUCK, Bonus::ONE_BATTLE, 0, +2, Bonus::TERRAIN_OVERLAY)->addLimiter(make_shared<CreatureFactionLimiter>(-1)));
			break;
		}
	case BFieldType::EVIL_FOG:
		{
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, -1, Bonus::TERRAIN_OVERLAY)->addLimiter(make_shared<CreatureAlignmentLimiter>(EAlignment::GOOD)));
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, +1, Bonus::TERRAIN_OVERLAY)->addLimiter(make_shared<CreatureAlignmentLimiter>(EAlignment::EVIL)));
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
	auto nativeTerrain = make_shared<CreatureNativeTerrainLimiter>(curB->terrainType);
	curB->addNewBonus(makeFeature(Bonus::STACKS_SPEED, Bonus::ONE_BATTLE, 0, 1, Bonus::TERRAIN_NATIVE)->addLimiter(nativeTerrain));
	curB->addNewBonus(makeFeature(Bonus::PRIMARY_SKILL, Bonus::ONE_BATTLE, PrimarySkill::ATTACK, 1, Bonus::TERRAIN_NATIVE)->addLimiter(nativeTerrain));
	curB->addNewBonus(makeFeature(Bonus::PRIMARY_SKILL, Bonus::ONE_BATTLE, PrimarySkill::DEFENSE, 1, Bonus::TERRAIN_NATIVE)->addLimiter(nativeTerrain));
	//////////////////////////////////////////////////////////////////////////

	//tactics
	bool isTacticsAllowed = !creatureBank; //no tactics in crebanks

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


	// workaround  bonuses affecting only enemy
	for(int i = 0; i < 2; i++)
	{
		TNodes nodes;
		curB->belligerents[i]->getRedAncestors(nodes);
		BOOST_FOREACH(CBonusSystemNode *n, nodes)
		{
			BOOST_FOREACH(Bonus *b, n->getExportedBonusList())
			{
				if(b->effectRange == Bonus::ONLY_ENEMY_ARMY/* && b->propagator && b->propagator->shouldBeAttached(curB)*/)
				{
					Bonus *bCopy = new Bonus(*b);
					bCopy->effectRange = Bonus::NO_LIMIT;
					bCopy->propagator.reset();
					bCopy->limiter.reset(new StackOwnerLimiter(curB->sides[!i]));
					curB->addNewBonus(bCopy);
				}
			}
		}
	}

	return curB;
}

const CGHeroInstance * BattleInfo::getHero( TPlayerColor player ) const
{
	assert(sides[0] == player || sides[1] == player);
	if(heroes[0] && heroes[0]->getOwner() == player)
		return heroes[0];

	return heroes[1];
}

std::vector<ui32> BattleInfo::calculateResistedStacks(const CSpell * sp, const CGHeroInstance * caster, const CGHeroInstance * hero2, const std::set<const CStack*> affectedCreatures, int casterSideOwner, ECastingMode::ECastingMode mode, int usedSpellPower, int spellLevel) const
{
	std::vector<ui32> ret;
	for(auto it = affectedCreatures.begin(); it != affectedCreatures.end(); ++it)
	{
		if(battleIsImmune(caster, sp, mode, (*it)->position) != ESpellCastProblem::OK) //FIXME: immune stacks should not display resisted animation
		{
			ret.push_back((*it)->ID);
			continue;
		}

		//non-negative spells should always succeed, unless immune
		if(!sp->isNegative())// && (*it)->owner == casterSideOwner)
			continue;

		/*
		const CGHeroInstance * bonusHero; //hero we should take bonuses from
		if((*it)->owner == casterSideOwner)
			bonusHero = caster;
		else
			bonusHero = hero2;*/

		int prob = (*it)->magicResistance(); //probability of resistance in %

		if(prob > 100) prob = 100;

		if(rand()%100 < prob) //immunity from resistance
			ret.push_back((*it)->ID);

	}

	if(sp->id == SpellID::HYPNOTIZE) //hypnotize
	{
		for(auto it = affectedCreatures.begin(); it != affectedCreatures.end(); ++it)
		{
			if( (*it)->hasBonusOfType(Bonus::SPELL_IMMUNITY, sp->id) //100% sure spell immunity
				|| ( (*it)->count - 1 ) * (*it)->MaxHealth() + (*it)->firstHPleft
		>
		calculateSpellBonus (usedSpellPower * 25 + sp->powers[spellLevel], sp, caster, *it) //apply 'damage' bonus for hypnotize, including hero specialty
			)
			{
				ret.push_back((*it)->ID);
			}
		}
	}

	return ret;
}

TPlayerColor BattleInfo::theOtherPlayer(TPlayerColor player) const
{
	return sides[!whatSide(player)];
}

ui8 BattleInfo::whatSide(TPlayerColor player) const
{
	for(int i = 0; i < ARRAY_COUNT(sides); i++)
		if(sides[i] == player)
			return i;

	tlog1 << "BattleInfo::whatSide: Player " << player << " is not in battle!\n";
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

shared_ptr<CObstacleInstance> BattleInfo::getObstacleOnTile(BattleHex tile) const
{
	BOOST_FOREACH(auto &obs, obstacles)
		if(vstd::contains(obs->getAffectedTiles(), tile))
			return obs;

	return shared_ptr<CObstacleInstance>();
}

BattlefieldBI::BattlefieldBI BattleInfo::battlefieldTypeToBI(BFieldType bfieldType)
{
	static const std::map<BFieldType, BattlefieldBI::BattlefieldBI> theMap = boost::assign::map_list_of
		(BFieldType::CLOVER_FIELD, BattlefieldBI::CLOVER_FIELD)
		(BFieldType::CURSED_GROUND, BattlefieldBI::CURSED_GROUND)
		(BFieldType::EVIL_FOG, BattlefieldBI::EVIL_FOG)
		(BFieldType::FAVOURABLE_WINDS, BattlefieldBI::NONE)
		(BFieldType::FIERY_FIELDS, BattlefieldBI::FIERY_FIELDS)
		(BFieldType::HOLY_GROUND, BattlefieldBI::HOLY_GROUND)
		(BFieldType::LUCID_POOLS, BattlefieldBI::LUCID_POOLS)
		(BFieldType::MAGIC_CLOUDS, BattlefieldBI::MAGIC_CLOUDS)
		(BFieldType::MAGIC_PLAINS, BattlefieldBI::MAGIC_PLAINS)
		(BFieldType::ROCKLANDS, BattlefieldBI::ROCKLANDS)
		(BFieldType::SAND_SHORE, BattlefieldBI::COASTAL);

	auto itr = theMap.find(bfieldType);
	if(itr != theMap.end())
		return itr->second;

	return BattlefieldBI::NONE;
}

CStack * BattleInfo::getStack(int stackID, bool onlyAlive /*= true*/)
{
	return const_cast<CStack *>(battleGetStackByID(stackID, onlyAlive));
}

CStack * BattleInfo::getStackT(BattleHex tileID, bool onlyAlive /*= true*/)
{
	return const_cast<CStack *>(battleGetStackByPos(tileID, onlyAlive));
}

BattleInfo::BattleInfo()
{
	setBattle(this);
	setNodeType(BATTLE);
}

CStack::CStack(const CStackInstance *Base, int O, int I, bool AO, int S)
	: base(Base), ID(I), owner(O), slot(S), attackerOwned(AO),
	counterAttacks(1)
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
CStack::CStack(const CStackBasicDescriptor *stack, int O, int I, bool AO, int S)
	: base(NULL), ID(I), owner(O), slot(S), attackerOwned(AO), counterAttacks(1)
{
	type = stack->type;
	count = baseAmount = stack->count;
	setNodeType(STACK_BATTLE);
}

void CStack::init()
{
	base = NULL;
	type = NULL;
	ID = -1;
	count = baseAmount = -1;
	firstHPleft = -1;
	owner = 255;
	slot = 255;
	attackerOwned = false;
	position = BattleHex();
	counterAttacks = -1;
}

void CStack::postInit()
{
	assert(type);
	assert(getParentNodes().size());

	//FIXME: the following should take into account ONLY_ENEMY_ARMY bonus range

	firstHPleft = MaxHealth();
	shots = getCreature()->valOfBonuses(Bonus::SHOTS);
	counterAttacks = 1 + valOfBonuses(Bonus::ADDITIONAL_RETALIATION);
	casts = valOfBonuses(Bonus::CASTS);
}

ui32 CStack::Speed( int turn /*= 0*/ , bool useBind /* = false*/) const
{
	if(hasBonus(Selector::type(Bonus::SIEGE_WEAPON) && Selector::turns(turn))) //war machines cannot move
		return 0;

	int speed = valOfBonuses(Selector::type(Bonus::STACKS_SPEED) && Selector::turns(turn));

	int percentBonus = 0;
	BOOST_FOREACH(const Bonus *b, getBonusList())
	{
		if(b->type == Bonus::STACKS_SPEED)
		{
			percentBonus += b->additionalInfo;
		}
	}

	speed = ((100 + percentBonus) * speed)/100;

	//bind effect check - doesn't influence stack initiative
	if (useBind && getEffect (SpellID::BIND))
	{
		return 0;
	}

	return speed;
}

si32 CStack::magicResistance() const
{
	si32 magicResistance;
	if (base) //TODO: make war machines receive aura of magic resistance
	{
		magicResistance = base->magicResistance();
		int auraBonus = 0;
		BOOST_FOREACH (const CStack * stack, base->armyObj->battle-> batteAdjacentCreatures(this))
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

	BOOST_FOREACH(Bonus& b, tmp)
	{
		b.turnsRemain =  sse.turnsRemain;
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
		&& !hasBonus(Selector::type(Bonus::NOT_ACTIVE) && Selector::turns(turn)); //eg. Ammo Cart or blinded creature
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
	BOOST_FOREACH(const Bonus *it, *spellEffects)
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
		BOOST_FOREACH(const CBonusSystemNode *n, getParentNodes())
			if(n->getNodeType() == HERO)
				return dynamic_cast<const CGHeroInstance *>(n);

	return NULL;
}

std::string CStack::nodeName() const
{
	std::ostringstream oss;
	oss << "Battle stack [" << ID << "]: " << count << " creatures of ";
	if(type)
		oss << type->namePl;
	else
		oss << "[UNDEFINED TYPE]";

	oss << " from slot " << (int)slot;
	if(base && base->armyObj)
		oss << " of armyobj=" << base->armyObj->id.getNum();
	return oss.str();
}

void CStack::prepareAttacked(BattleStackAttacked &bsa) const
{
	bsa.killedAmount = bsa.damageAmount / MaxHealth();
	unsigned damageFirst = bsa.damageAmount % MaxHealth();

	if (bsa.damageAmount && vstd::contains(state, EBattleStackState::CLONED)) // block ability should not kill clone (0 damage)
	{
		bsa.killedAmount = count;
		bsa.flags |= BattleStackAttacked::CLONE_KILLED;
		return; // no rebirth I believe
	}

	if( firstHPleft <= damageFirst )
	{
		bsa.killedAmount++;
		bsa.newHP = firstHPleft + MaxHealth() - damageFirst;
	}
	else
	{
		bsa.newHP = firstHPleft - damageFirst;
	}

	if(count <= bsa.killedAmount) //stack killed
	{
		bsa.newAmount = 0;
		bsa.flags |= BattleStackAttacked::KILLED;
		bsa.killedAmount = count; //we cannot kill more creatures than we have

		int resurrectFactor = valOfBonuses(Bonus::REBIRTH);
		if (resurrectFactor > 0 && casts) //there must be casts left
		{
			int resurrectedCount = base->count * resurrectFactor / 100;
			if (resurrectedCount)
				resurrectedCount += ((base->count * resurrectFactor / 100.0 - resurrectedCount) > ran()%100 / 100.0) ? 1 : 0; //last stack has proportional chance to rebirth
			else //only one unit
				resurrectedCount += ((base->count * resurrectFactor / 100.0) > ran()%100 / 100.0) ? 1 : 0;
			if (hasBonusOfType(Bonus::REBIRTH, 1))
				vstd::amax (resurrectedCount, 1); //resurrect at least one Sacred Phoenix
			if (resurrectedCount)
			{
				bsa.flags |= BattleStackAttacked::REBIRTH;
				bsa.newAmount = resurrectedCount; //risky?
				bsa.newHP = MaxHealth(); //resore full health
			}
		}
	}
	else
	{
		bsa.newAmount = count - bsa.killedAmount;
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

bool CStack::ableToRetaliate() const
{
	return alive()
		&& (counterAttacks > 0 || hasBonusOfType(Bonus::UNLIMITED_RETALIATIONS))
		&& !hasBonusOfType(Bonus::SIEGE_WEAPON)
		&& !hasBonusOfType(Bonus::HYPNOTIZED)
		&& !hasBonusOfType(Bonus::NO_RETALIATION);
}

std::string CStack::getName() const
{
	return (count > 1) ? type->namePl : type->nameSing; //War machines can't use base
}

bool CStack::isValidTarget(bool allowDead/* = false*/) const /*alive non-turret stacks (can be attacked or be object of magic effect) */
{
	return (alive() || allowDead) && position.isValid();
}

bool CStack::canBeHealed() const
{
	return firstHPleft < MaxHealth()
		&& isValidTarget()
		&& !hasBonusOfType(Bonus::SIEGE_WEAPON);
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


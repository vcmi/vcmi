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
#include "../lib/JsonNode.h"


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
	getStackQueue(hlp, 1, -1);

	if(hlp.size())
		return hlp[0];
	else
		return NULL;
}

static const CStack *takeStack(std::vector<const CStack *> &st, int &curside, int turn)
{
	const CStack *ret = NULL;
	unsigned i, //fastest stack
		j; //fastest stack of the other side
	for(i = 0; i < st.size(); i++)
		if(st[i])
			break;

	//no stacks left
	if(i == st.size())
		return NULL;

	const CStack *fastest = st[i], *other = NULL;
	int bestSpeed = fastest->Speed(turn);

	if(fastest->attackerOwned != curside)
	{
		ret = fastest;
	}
	else
	{
		for(j = i + 1; j < st.size(); j++)
		{
			if(!st[j]) continue;
			if(st[j]->attackerOwned != curside || st[j]->Speed(turn) != bestSpeed)
				break;
		}

		if(j >= st.size())
		{
			ret = fastest;
		}
		else
		{
			other = st[j];
			if(other->Speed(turn) != bestSpeed)
				ret = fastest;
			else
				ret = other;
		}
	}

	assert(ret);
	if(ret == fastest)
		st[i] = NULL;
	else
		st[j] = NULL;

	curside = ret->attackerOwned;
	return ret;
}

CStack * BattleInfo::getStack(int stackID, bool onlyAlive)
{
	for(ui32 g=0; g<stacks.size(); ++g)
	{
		if(stacks[g]->ID == stackID && (!onlyAlive || stacks[g]->alive()))
			return stacks[g];
	}
	return NULL;
}

const CStack * BattleInfo::getStack(int stackID, bool onlyAlive) const
{
	return const_cast<BattleInfo * const>(this)->getStack(stackID, onlyAlive);
}

CStack * BattleInfo::getStackT(BattleHex tileID, bool onlyAlive)
{
	for(ui32 g=0; g<stacks.size(); ++g)
	{
		if(stacks[g]->position == tileID 
			|| (stacks[g]->doubleWide() && stacks[g]->attackerOwned && stacks[g]->position-1 == tileID)
			|| (stacks[g]->doubleWide() && !stacks[g]->attackerOwned && stacks[g]->position+1 == tileID))
		{
			if(!onlyAlive || stacks[g]->alive())
			{
				return stacks[g];
			}
		}
	}
	return NULL;
}

const CStack * BattleInfo::getStackT(BattleHex tileID, bool onlyAlive) const
{
	return const_cast<BattleInfo * const>(this)->getStackT(tileID, onlyAlive);
}

void BattleInfo::getAccessibilityMap(bool *accessibility, bool twoHex, bool attackerOwned, bool addOccupiable, std::set<BattleHex> & occupyable, bool flying, const CStack * stackToOmmit) const
{
	memset(accessibility, 1, GameConstants::BFIELD_SIZE); //initialize array with trues

	//removing accessibility for side columns of hexes
	for(int v = 0; v < GameConstants::BFIELD_SIZE; ++v)
	{
		if( v % GameConstants::BFIELD_WIDTH == 0 || v % GameConstants::BFIELD_WIDTH == (GameConstants::BFIELD_WIDTH - 1) )
			accessibility[v] = false;
	}

	for(ui32 g=0; g<stacks.size(); ++g)
	{
		if(!stacks[g]->alive() || (stackToOmmit && stacks[g]->ID==stackToOmmit->ID) || stacks[g]->position < 0) //we don't want to lock position of this stack (eg. if it's a turret)
			continue;

		accessibility[stacks[g]->position] = false;
		if(stacks[g]->doubleWide()) //if it's a double hex creature
		{
			if(stacks[g]->attackerOwned)
				accessibility[stacks[g]->position-1] = false;
			else
				accessibility[stacks[g]->position+1] = false;
		}
	}
	//obstacles
	for(ui32 b=0; b<obstacles.size(); ++b)
	{
		std::vector<BattleHex> blocked = VLC->heroh->obstacles[obstacles[b].ID].getBlocked(obstacles[b].pos);
		for(ui32 c=0; c<blocked.size(); ++c)
		{
			if(blocked[c] >=0 && blocked[c] < GameConstants::BFIELD_SIZE)
				accessibility[blocked[c]] = false;
		}
	}

	//walls
	if(siege > 0)
	{
		static const int permanentlyLocked[] = {12, 45, 78, 112, 147, 165};
		for(int b=0; b<ARRAY_COUNT(permanentlyLocked); ++b)
		{
			accessibility[permanentlyLocked[b]] = false;
		}

		static const std::pair<int, BattleHex> lockedIfNotDestroyed[] = //(which part of wall, which hex is blocked if this part of wall is not destroyed
			{std::make_pair(2, BattleHex(182)), std::make_pair(3, BattleHex(130)),
			std::make_pair(4, BattleHex(62)), std::make_pair(5, BattleHex(29))};
		for(int b=0; b<ARRAY_COUNT(lockedIfNotDestroyed); ++b)
		{
			if(si.wallState[lockedIfNotDestroyed[b].first] < 3)
			{
				accessibility[lockedIfNotDestroyed[b].second] = false;
			}
		}

		//gate
		if(attackerOwned && si.wallState[7] < 3) //if it attacker's unit and gate is not destroyed
		{
			accessibility[95] = accessibility[96] = false; //block gate's hexes
		}
	}

	//occupyability
	if(addOccupiable && twoHex)
	{
		std::set<BattleHex> rem; //tiles to unlock
		for(int h=0; h<GameConstants::BFIELD_HEIGHT; ++h)
		{
			for(int w=1; w<GameConstants::BFIELD_WIDTH-1; ++w)
			{
				BattleHex hex(w, h);
				if(!isAccessible(hex, accessibility, twoHex, attackerOwned, flying, true)
					&& (attackerOwned ? isAccessible(hex+1, accessibility, twoHex, attackerOwned, flying, true) : isAccessible(hex-1, accessibility, twoHex, attackerOwned, flying, true) )
					)
					rem.insert(hex);
			}
		}
		occupyable = rem;
		/*for(std::set<int>::const_iterator it = rem.begin(); it != rem.end(); ++it)
		{
			accessibility[*it] = true;
		}*/
	}
}

bool BattleInfo::isAccessible(BattleHex hex, bool * accessibility, bool twoHex, bool attackerOwned, bool flying, bool lastPos)
{
	if(flying && !lastPos)
		return true;

	if(twoHex)
	{
		//if given hex is accessible and appropriate adjacent one is free too
		return accessibility[hex] && accessibility[hex + (attackerOwned ? -1 : 1 )];
	}
	else
	{
		return accessibility[hex];
	}
}

void BattleInfo::makeBFS(BattleHex start, bool *accessibility, BattleHex *predecessor, int *dists, bool twoHex, bool attackerOwned, bool flying, bool fillPredecessors) const //both pointers must point to the at least 187-elements int arrays
{
	//inits
	for(int b=0; b<GameConstants::BFIELD_SIZE; ++b)
		predecessor[b] = -1;
	for(int g=0; g<GameConstants::BFIELD_SIZE; ++g)
		dists[g] = 100000000;	
	
	std::queue< std::pair<BattleHex, bool> > hexq; //bfs queue <hex, accessible> (second filed used only if fillPredecessors is true)
	hexq.push(std::make_pair(start, true));
	dists[hexq.front().first] = 0;
	int curNext = -1; //for bfs loop only (helper var)
	while(!hexq.empty()) //bfs loop
	{
		std::pair<BattleHex, bool> curHex = hexq.front();
		std::vector<BattleHex> neighbours = curHex.first.neighbouringTiles();
		hexq.pop();
		for(ui32 nr=0; nr<neighbours.size(); nr++)
		{
			curNext = neighbours[nr]; //if(!accessibility[curNext] || (dists[curHex]+1)>=dists[curNext])
			bool accessible = isAccessible(curNext, accessibility, twoHex, attackerOwned, flying, dists[curHex.first]+1 == dists[curNext]);
			if( dists[curHex.first]+1 >= dists[curNext] )
				continue;
			if(accessible && curHex.second)
			{
				hexq.push(std::make_pair(curNext, true));
				dists[curNext] = dists[curHex.first] + 1;
			}
			else if(fillPredecessors && !(accessible && !curHex.second))
			{
				hexq.push(std::make_pair(curNext, false));
				dists[curNext] = dists[curHex.first] + 1;
			}
			predecessor[curNext] = curHex.first;
		}
	}
};

std::vector<BattleHex> BattleInfo::getAccessibility( const CStack * stack, bool addOccupiable, std::vector<BattleHex> * attackable /*= NULL*/, bool forPassingBy /*= false*/ ) const
{
	std::vector<BattleHex> ret;
	bool ac[GameConstants::BFIELD_SIZE];

	if(stack->position < 0) //turrets
		return std::vector<BattleHex>();

	std::set<BattleHex> occupyable;

	getAccessibilityMap(ac, stack->doubleWide(), stack->attackerOwned, addOccupiable, occupyable, stack->hasBonusOfType(Bonus::FLYING), stack);

	BattleHex pr[GameConstants::BFIELD_SIZE];
	int dist[GameConstants::BFIELD_SIZE];
	makeBFS(stack->position, ac, pr, dist, stack->doubleWide(), stack->attackerOwned, stack->hasBonusOfType(Bonus::FLYING), false);

	if(stack->doubleWide())
	{
		if(!addOccupiable)
		{
			std::vector<BattleHex> rem;
			for(int b=0; b<GameConstants::BFIELD_SIZE; ++b)
			{
				//don't take into account most left and most right columns of hexes
				if( b % GameConstants::BFIELD_WIDTH == 0 || b % GameConstants::BFIELD_WIDTH == GameConstants::BFIELD_WIDTH - 1 )
					continue;

				if( ac[b] && !(stack->attackerOwned ? ac[b-1] : ac[b+1]) )
				{
					rem.push_back(b);
				}
			}

			for(ui32 g=0; g<rem.size(); ++g)
			{
				ac[rem[g]] = false;
			}

			//removing accessibility for side hexes
			for(int v=0; v<GameConstants::BFIELD_SIZE; ++v)
				if(stack->attackerOwned ? (v%GameConstants::BFIELD_WIDTH)==1 : (v%GameConstants::BFIELD_WIDTH)==(GameConstants::BFIELD_WIDTH - 2))
					ac[v] = false;
		}
	}
	
	for (int i = 0; i < GameConstants::BFIELD_SIZE; ++i)
	{
		bool rangeFits;
		if (tacticDistance)
		{
			rangeFits = pr[i] >= 0; //reachable in terms of obstacles
			if(!forPassingBy) //only if we're passing through, we may step out of the tactic range -> otherwise check range
				rangeFits = rangeFits && isInTacticRange(i);
		}
		else
			rangeFits = dist[i] <= stack->Speed(0, true); //we can reach the stack

		if(	( !addOccupiable && rangeFits && ac[i] ) 
			|| ( addOccupiable && rangeFits && isAccessible(i, ac, stack->doubleWide(), stack->attackerOwned, stack->hasBonusOfType(Bonus::FLYING), true) )//we can reach it
			|| (vstd::contains(occupyable, i) && (!tacticDistance && dist[ i + (stack->attackerOwned ? 1 : -1 ) ] <= stack->Speed(0, true) ) && ac[i + (stack->attackerOwned ? 1 : -1 )] ) //it's occupyable and we can reach adjacent hex
			)
		{
			ret.push_back(i);
		}
	}

	if(attackable)
	{
		struct HLP
		{
			static bool meleeAttackable(BattleHex hex, const std::vector<BattleHex> & baseRng)
			{
				BOOST_FOREACH(BattleHex h, baseRng)
				{
					if(BattleHex::mutualPosition(h, hex) > 0)
						return true;
				}

				return false;
			}
		};
		BOOST_FOREACH(const CStack * otherSt, stacks)
		{
			if(otherSt->owner == stack->owner || !otherSt->isValidTarget(false))
				continue;

			std::vector<BattleHex> occupiedBySecond;
			occupiedBySecond.push_back(otherSt->position);
			if(otherSt->doubleWide())
				occupiedBySecond.push_back(otherSt->occupiedHex());

			if(battleCanShoot(stack, otherSt->position))
			{
				attackable->insert(attackable->end(), occupiedBySecond.begin(), occupiedBySecond.end());

				continue;
			}
			

			BOOST_FOREACH(BattleHex he, occupiedBySecond)
			{
				if(HLP::meleeAttackable(he, ret))
					attackable->push_back(he);
			}
				
		}
	}

	return ret;
}

BattleHex BattleInfo::getClosestTile (bool attackerOwned, int initialPos, std::set<BattleHex> & possibilities) const
{
	std::vector<BattleHex> sortedTiles (possibilities.begin(), possibilities.end()); //set can't be sorted properly :(

	BattleHex initialHex = BattleHex(initialPos);
	auto compareDistance = [initialPos, initialHex](const BattleHex left, const BattleHex right) -> bool
	{
		return initialHex.getDistance (initialHex, left) < initialHex.getDistance (initialHex, right);
	};

	boost::sort (sortedTiles, compareDistance); //closest tiles at front

	int closestDistance = initialHex.getDistance(initialPos, sortedTiles.front()); //sometimes closest tiles can be many hexes away

	auto notClosest = [closestDistance, initialPos](const BattleHex here) -> bool
	{
		return closestDistance < here.getDistance (initialPos, here);
	};

	boost::remove_if (sortedTiles, notClosest); //only closest tiles are interesting

	auto compareHorizontal = [attackerOwned](const BattleHex left, const BattleHex right) -> bool
	{
		if (attackerOwned)
			return left.getX() > right.getX(); //find furthest right
		else
			return left.getX() < right.getX(); //find furthest left
	};

	boost::sort (sortedTiles, compareHorizontal);

	return sortedTiles.front();
}

int BattleInfo::getAvaliableHex(TCreature creID, bool attackerOwned, int initialPos) const
{
	int pos;
	if (initialPos > -1)
		pos = initialPos;
	else
	{
		if (attackerOwned)
			pos = 0; //top left
		else
			pos = GameConstants::BFIELD_WIDTH; //top right
	}

	bool ac[GameConstants::BFIELD_SIZE];
	std::set<BattleHex> occupyable;

	bool twoHex = VLC->creh->creatures[creID]->isDoubleWide();
	bool flying = VLC->creh->creatures[creID]->isFlying();// vstd::contains(VLC->creh->creatures[creID]->bonuses, Bonus::FLYING);
	getAccessibilityMap (ac, twoHex, attackerOwned, true, occupyable, flying);

	if (!occupyable.size())
		return -1; //all tiles are covered

	return getClosestTile (attackerOwned, initialPos, occupyable);
}

bool BattleInfo::isStackBlocked(const CStack * stack) const
{
	if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON)) //siege weapons cannot be blocked
		return false;

	BOOST_FOREACH(CStack * s, getAdjacentCreatures(stack))
	{
		if (s->owner != stack->owner) //blocked by enemy stack
			return true;
	}
	return false;
}

std::pair< std::vector<BattleHex>, int > BattleInfo::getPath(BattleHex start, BattleHex dest, bool*accessibility, bool flyingCreature, bool twoHex, bool attackerOwned)
{
	BattleHex predecessor[GameConstants::BFIELD_SIZE]; //for getting the Path
	int dist[GameConstants::BFIELD_SIZE]; //calculated distances

	makeBFS(start, accessibility, predecessor, dist, twoHex, attackerOwned, flyingCreature, false);
	
	if(predecessor[dest] == -1) //cannot reach destination
	{
		return std::make_pair(std::vector<BattleHex>(), 0);
	}

	//making the Path
	std::vector<BattleHex> path;
	BattleHex curElem = dest;
	while(curElem != start)
	{
		path.push_back(curElem);
		curElem = predecessor[curElem];
	}

	return std::make_pair(path, dist[dest]);
}

TDmgRange BattleInfo::calculateDmgRange( const CStack* attacker, const CStack* defender, TQuantity attackerCount, TQuantity defenderCount, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero,
	bool shooting, ui8 charge, bool lucky, bool deathBlow, bool ballistaDoubleDmg ) const
{
	double additiveBonus = 1.0, multBonus = 1.0,
		minDmg = attacker->getMinDamage() * attackerCount, 
		maxDmg = attacker->getMaxDamage() * attackerCount;

	if(attacker->getCreature()->idNumber == 149) //arrow turret
	{
		switch(attacker->position)
		{
		case -2: //keep
			minDmg = 15;
			maxDmg = 15;
			break;
		case -3: case -4: //turrets
			minDmg = 7.5;
			maxDmg = 7.5;
			break;
		}
	}

	if(attacker->hasBonusOfType(Bonus::SIEGE_WEAPON) && attacker->getCreature()->idNumber != 149) //any siege weapon, but only ballista can attack (second condition - not arrow turret)
	{ //minDmg and maxDmg are multiplied by hero attack + 1
		minDmg *= attackerHero->getPrimSkillLevel(0) + 1; 
		maxDmg *= attackerHero->getPrimSkillLevel(0) + 1; 
	}

	int attackDefenceDifference = 0;
	if(attacker->hasBonusOfType(Bonus::GENERAL_ATTACK_REDUCTION))
	{
		double multAttackReduction = attacker->valOfBonuses(Bonus::GENERAL_ATTACK_REDUCTION, -1024) / 100.0;
		attackDefenceDifference = attacker->Attack() * multAttackReduction;
	}
	else
	{
		attackDefenceDifference = attacker->Attack();
	}

	if(attacker->hasBonusOfType(Bonus::ENEMY_DEFENCE_REDUCTION))
	{
		double multDefenceReduction = (100 - attacker->valOfBonuses(Bonus::ENEMY_DEFENCE_REDUCTION, -1024)) / 100.0;
		attackDefenceDifference -= defender->Defense() * multDefenceReduction;
	}
	else
	{
		attackDefenceDifference -= defender->Defense();
	}

	//calculating total attack/defense skills modifier

	if(shooting) //precision handling (etc.)
		attackDefenceDifference += attacker->getBonuses(Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), Selector::effectRange(Bonus::ONLY_DISTANCE_FIGHT))->totalValue();
	else //bloodlust handling (etc.)
		attackDefenceDifference += attacker->getBonuses(Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK), Selector::effectRange(Bonus::ONLY_MELEE_FIGHT))->totalValue();


	if(attacker->getEffect(55)) //slayer handling
	{
		std::vector<int> affectedIds;
		int spLevel = attacker->getEffect(55)->val;

		for(int g = 0; g < VLC->creh->creatures.size(); ++g)
		{
			BOOST_FOREACH(const Bonus *b, VLC->creh->creatures[g]->getBonusList())
			{
				if ( (b->type == Bonus::KING3 && spLevel >= 3) || //expert
					(b->type == Bonus::KING2 && spLevel >= 2) || //adv +
					(b->type == Bonus::KING1 && spLevel >= 0) ) //none or basic +
				{
					affectedIds.push_back(g);
					break;
				}
			}
		}

		for(ui32 g=0; g<affectedIds.size(); ++g)
		{
			if(defender->getCreature()->idNumber == affectedIds[g])
			{
				attackDefenceDifference += VLC->spellh->spells[55]->powers[attacker->getEffect(55)->val];
				break;
			}
		}
	}

	//bonus from attack/defense skills
	if(attackDefenceDifference < 0) //decreasing dmg
	{
		double dec = 0.025 * (-attackDefenceDifference);
		if(dec > 0.7)
		{
			multBonus *= 0.3; //1.0 - 0.7
		}
		else
		{
			multBonus *= 1.0 - dec;
		}
	}
	else //increasing dmg
	{
		double inc = 0.05 * attackDefenceDifference;
		if(inc > 4.0)
		{
			additiveBonus += 4.0;
		}
		else
		{
			additiveBonus += inc;
		}
	}


	//applying jousting bonus
	if( attacker->hasBonusOfType(Bonus::JOUSTING) && !defender->hasBonusOfType(Bonus::CHARGE_IMMUNITY) )
		additiveBonus += charge * 0.05;


	//handling secondary abilities and artifacts giving premies to them
	if(attackerHero)
	{
		if(shooting)
		{
			additiveBonus += attackerHero->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, CGHeroInstance::ARCHERY) / 100.0;
		}
		else
		{
			additiveBonus += attackerHero->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, CGHeroInstance::OFFENCE) / 100.0;
		}
	}

	if(defendingHero)
	{
		multBonus *= (std::max(0, 100-defendingHero->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, CGHeroInstance::ARMORER))) / 100.0;
	}

	//handling hate effect
	additiveBonus += attacker->valOfBonuses(Bonus::HATE, defender->getCreature()->idNumber) / 100.;

	//luck bonus
	if (lucky)
	{
		additiveBonus += 1.0;
	}

	//ballista double dmg
	if(ballistaDoubleDmg)
	{
		additiveBonus += 1.0;
	}

	if (deathBlow) //Dread Knight and many WoGified creatures
	{
		additiveBonus += 1.0;
	}

	//handling spell effects
	if(!shooting && defender->hasBonusOfType(Bonus::GENERAL_DAMAGE_REDUCTION, 0)) //eg. shield
	{
		multBonus *= defender->valOfBonuses(Bonus::GENERAL_DAMAGE_REDUCTION, 0) / 100.0;
	}
	else if(shooting && defender->hasBonusOfType(Bonus::GENERAL_DAMAGE_REDUCTION, 1)) //eg. air shield
	{
		multBonus *= defender->valOfBonuses(Bonus::GENERAL_DAMAGE_REDUCTION, 1) / 100.0;
	}

	TBonusListPtr curseEffects = attacker->getBonuses(Selector::type(Bonus::ALWAYS_MINIMUM_DAMAGE)); //attacker->getEffect(42);
	TBonusListPtr blessEffects = attacker->getBonuses(Selector::type(Bonus::ALWAYS_MAXIMUM_DAMAGE)); //attacker->getEffect(43);
	int curseBlessAdditiveModifier = blessEffects->totalValue() - curseEffects->totalValue();
	double curseMultiplicativePenalty = curseEffects->size() ? (*std::max_element(curseEffects->begin(), curseEffects->end(), &Bonus::compareByAdditionalInfo))->additionalInfo : 0;

	if(curseMultiplicativePenalty) //curse handling (partial, the rest is below)
	{
		multBonus *= 1.0 - curseMultiplicativePenalty/100;
	}

	class HLP
	{
	public:
		static bool hasAdvancedAirShield(const CStack * stack)
		{

			BOOST_FOREACH(const Bonus *it, stack->getBonusList())
			{
				if (it->source == Bonus::SPELL_EFFECT && it->sid == 28 && it->val >= 2)
				{
					return true;
				}
			}
			return false;
		}
	};

	//wall / distance penalty + advanced air shield
	bool distPenalty = !NBonus::hasOfType(attackerHero, Bonus::NO_DISTANCE_PENALTY) &&
		hasDistancePenalty(attacker, defender->position);
	bool obstaclePenalty = hasWallPenalty(attacker, defender->position);
	if (shooting)
	{
		if (distPenalty || HLP::hasAdvancedAirShield(defender))
		{
			multBonus *= 0.5;
		}
		if (obstaclePenalty)
		{
			multBonus *= 0.5; //cumulative
		}
	}
	if (!shooting && attacker->hasBonusOfType(Bonus::SHOOTER) && !attacker->hasBonusOfType(Bonus::NO_MELEE_PENALTY))
	{
		multBonus *= 0.5;
	}

	minDmg *= additiveBonus * multBonus;
	maxDmg *= additiveBonus * multBonus;

	TDmgRange returnedVal;

	if(curseEffects->size()) //curse handling (rest)
	{
		minDmg += curseBlessAdditiveModifier;
		returnedVal = std::make_pair(int(minDmg), int(minDmg));
	}
	else if(blessEffects->size()) //bless handling
	{
		maxDmg += curseBlessAdditiveModifier;
		returnedVal =  std::make_pair(int(maxDmg), int(maxDmg));
	}
	else
	{
		returnedVal =  std::make_pair(int(minDmg), int(maxDmg));
	}

	//damage cannot be less than 1
	vstd::amax(returnedVal.first, 1);
	vstd::amax(returnedVal.second, 1);

	return returnedVal;
}

TDmgRange BattleInfo::calculateDmgRange(const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero,
	bool shooting, ui8 charge, bool lucky, bool deathBlow, bool ballistaDoubleDmg) const
{
	return calculateDmgRange(attacker, defender, attacker->count, defender->count, attackerHero, defendingHero, shooting, charge, lucky, deathBlow, ballistaDoubleDmg);
}

ui32 BattleInfo::calculateDmg( const CStack* attacker, const CStack* defender, const CGHeroInstance * attackerHero, const CGHeroInstance * defendingHero,
	bool shooting, ui8 charge, bool lucky, bool deathBlow, bool ballistaDoubleDmg )
{
	TDmgRange range = calculateDmgRange(attacker, defender, attackerHero, defendingHero, shooting, charge, lucky, deathBlow, ballistaDoubleDmg);

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

std::set<CStack*> BattleInfo::getAttackedCreatures(const CSpell * s, int skillLevel, ui8 attackerOwner, BattleHex destinationTile )
{
	std::set<ui16> attackedHexes = s->rangeInHexes(destinationTile, skillLevel);
	std::set<CStack*> attackedCres; /*std::set to exclude multiple occurrences of two hex creatures*/

	bool onlyAlive = s->id != 38 && s->id != 39; //when casting resurrection or animate dead we should be allow to select dead stack

	if(s->id == Spells::DEATH_RIPPLE || s->id == Spells::DESTROY_UNDEAD || s->id == Spells::ARMAGEDDON)
	{
		for(int it=0; it<stacks.size(); ++it)
		{
			if((s->id == Spells::DEATH_RIPPLE && !stacks[it]->getCreature()->isUndead()) //death ripple
				|| (s->id == Spells::DESTROY_UNDEAD && stacks[it]->getCreature()->isUndead()) //destroy undead
				|| (s->id == Spells::ARMAGEDDON) //Armageddon
				)
			{
				if(stacks[it]->isValidTarget())
					attackedCres.insert(stacks[it]);
			}
		}
	}
	else if (s->range[skillLevel].size() > 1) //custom many-hex range
	{
		for(std::set<ui16>::iterator it = attackedHexes.begin(); it != attackedHexes.end(); ++it)
		{
			CStack * st = getStackT(*it, onlyAlive);
			if(st)
			{
				if (s->id == 76) //Death Cloud //TODO: fireball and fire immunity
				{
					if (st->isLiving() || st->coversPos(destinationTile)) //directly hit or alive
					{
						attackedCres.insert(st);
					}
				}
				else
					attackedCres.insert(st);
			}
		}
	}
	else if(s->getTargetType() == CSpell::CREATURE_EXPERT_MASSIVE)
	{
		if(skillLevel < 3)  /*not expert */
		{
			CStack * st = getStackT(destinationTile, onlyAlive);
			if(st)
				attackedCres.insert(st);
		}
		else
		{
			for(int it=0; it<stacks.size(); ++it)
			{
				/*if it's non negative spell and our unit or non positive spell and hostile unit */
				if((!s->isNegative() && stacks[it]->owner == attackerOwner)
					||(!s->isPositive() && stacks[it]->owner != attackerOwner )
					)
				{
					if(stacks[it]->isValidTarget(!onlyAlive))
						attackedCres.insert(stacks[it]);
				}
			}
		} //if(caster->getSpellSchoolLevel(s) < 3)
	}
	else if(s->getTargetType() == CSpell::CREATURE)
	{
		CStack * st = getStackT(destinationTile, onlyAlive);
		if(st)
			attackedCres.insert(st);
	}
	else //custom range from attackedHexes
	{
		for(std::set<ui16>::iterator it = attackedHexes.begin(); it != attackedHexes.end(); ++it)
		{
			CStack * st = getStackT(*it, onlyAlive);
			if(st)
				attackedCres.insert(st);
		}
	}
	return attackedCres;
}
void BattleInfo::getPotentiallyAttackableHexes(AttackableTiles &at, const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos)
{
	const int WN = GameConstants::BFIELD_WIDTH;
	ui16 hex = (attackerPos != BattleHex::INVALID) ? attackerPos.hex : attacker->position.hex; //real or hypothetical (cursor) position
	if (attacker->hasBonusOfType(Bonus::ATTACKS_ALL_ADJACENT))
	{
		std::vector<BattleHex> hexes = attacker->getSurroundingHexes(attackerPos);
		BOOST_FOREACH (BattleHex tile, hexes)
		{
			at.hostileCreaturePositions.insert(tile);
		}
	}
	if (attacker->hasBonusOfType(Bonus::THREE_HEADED_ATTACK))
	{
		std::vector<BattleHex> hexes = attacker->getSurroundingHexes(attackerPos);
		BOOST_FOREACH (BattleHex tile, hexes)
		{
			if ((BattleHex::mutualPosition(tile, destinationTile) > -1 && BattleHex::mutualPosition(tile, hex) > -1) //adjacent both to attacker's head and attacked tile
				|| tile == destinationTile) //or simply attacked directly
			{
				CStack * st = getStackT(tile, true);
				if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
				{
					at.hostileCreaturePositions.insert(tile);
				}
			}
		}
	}
	if (attacker->hasBonusOfType(Bonus::TWO_HEX_ATTACK_BREATH))
	{
		std::vector<BattleHex> hexes; //only one, in fact
		int pseudoVector = destinationTile.hex - hex;
		switch (pseudoVector)
		{
			case 1:
			case -1:
				BattleHex::checkAndPush(destinationTile.hex + pseudoVector, hexes);
				break;
			case WN: //17
			case WN + 1: //18
			case -WN: //-17
			case -WN + 1: //-16
				BattleHex::checkAndPush(destinationTile.hex + pseudoVector + ((hex/WN)%2 ? 1 : -1 ), hexes);
				break;
			case WN-1: //16
			case -WN-1: //-18
				BattleHex::checkAndPush(destinationTile.hex + pseudoVector + ((hex/WN)%2 ? 1 : 0), hexes);
				break;
		}
		BOOST_FOREACH (BattleHex tile, hexes)
		{
			CStack * st = getStackT(tile, true);
			if(st) //friendly stacks can also be damaged by Dragon Breath
			{
				at.friendlyCreaturePositions.insert(tile);
			}
		}
	}
}
std::set<CStack*> BattleInfo::getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos)
{ //TODO: caching?
	AttackableTiles at;
	getPotentiallyAttackableHexes(at, attacker, destinationTile, attackerPos);
	std::set<CStack*> attackedCres;
	BOOST_FOREACH (BattleHex tile, at.hostileCreaturePositions) //all around & three-headed attack
	{
		CStack * st = getStackT(tile, true);
		if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
		{
			attackedCres.insert(st);
		}
	}
	BOOST_FOREACH (BattleHex tile, at.friendlyCreaturePositions)
	{
		CStack * st = getStackT(tile, true);
		if(st) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedCres.insert(st);
		}
	}
	return attackedCres;
}

std::set<BattleHex> BattleInfo::getAttackedHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos)
{
	AttackableTiles at;
	getPotentiallyAttackableHexes(at, attacker, destinationTile, attackerPos);
	std::set<BattleHex> attackedHexes;
	BOOST_FOREACH (BattleHex tile, at.hostileCreaturePositions)
	{
		CStack * st = getStackT(tile, true);
		if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
		{
			attackedHexes.insert(tile);
		}
	}
	BOOST_FOREACH (BattleHex tile, at.friendlyCreaturePositions)
	{
		CStack * st = getStackT(tile, true);
		if(st) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedHexes.insert(tile);
		}
	}
	return attackedHexes;
}

std::set<CStack*> BattleInfo::getAdjacentCreatures (const CStack * stack) const
{
	std::set<CStack*> stacks;

	CStack * localStack;
	BOOST_FOREACH (BattleHex hex, stack->getSurroundingHexes())
	{
		localStack = const_cast<CStack*>(getStackT(hex, true)); //only alive?
		if (localStack)
			stacks.insert(localStack);
	}
	return stacks;
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
	case Spells::FRENZY:
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
	ret->position = position;
	return ret;
}
CStack * BattleInfo::generateNewStack(const CStackBasicDescriptor &base, bool attackerOwned, int slot, BattleHex position) const
{
	int stackID = getIdForNewStack();
	int owner = attackerOwned ? sides[0] : sides[1];
	CStack * ret = new CStack(&base, owner, stackID, attackerOwned, slot);
	ret->position = position;
	return ret;
}

ui32 BattleInfo::getSpellCost(const CSpell * sp, const CGHeroInstance * caster) const
{
	ui32 ret = caster->getSpellCost(sp);

	//checking for friendly stacks reducing cost of the spell and
	//enemy stacks increasing it
	si32 manaReduction = 0;
	si32 manaIncrease = 0;
	for(int g=0; g<stacks.size(); ++g)
	{
		if( stacks[g]->owner == caster->tempOwner && stacks[g]->hasBonusOfType(Bonus::CHANGES_SPELL_COST_FOR_ALLY) )
		{
			vstd::amax(manaReduction, stacks[g]->valOfBonuses(Bonus::CHANGES_SPELL_COST_FOR_ALLY));
		}
		if( stacks[g]->owner != caster->tempOwner && stacks[g]->hasBonusOfType(Bonus::CHANGES_SPELL_COST_FOR_ENEMY) )
		{
			vstd::amax(manaIncrease, stacks[g]->valOfBonuses(Bonus::CHANGES_SPELL_COST_FOR_ENEMY));
		}
	}

	return ret - manaReduction + manaIncrease;
}

int BattleInfo::hexToWallPart(BattleHex hex) const
{
	if(siege == 0) //there is no battle!
		return -1;

	static const std::pair<int, int> attackable[] = //potentially attackable parts of wall
	{std::make_pair(50, 0), std::make_pair(183, 1), std::make_pair(182, 2), std::make_pair(130, 3),
	std::make_pair(62, 4), std::make_pair(29, 5), std::make_pair(12, 6), std::make_pair(95, 7), std::make_pair(96, 7),
	std::make_pair(45, -2), std::make_pair(78, -2), std::make_pair(112, -2), std::make_pair(147, -2)}; // -2 - indestructible walls

	for(int g = 0; g < ARRAY_COUNT(attackable); ++g)
	{
		if(attackable[g].first == hex)
			return attackable[g].second;
	}

	return -1; //not found!
}

int BattleInfo::lineToWallHex( int line ) const
{
	static const int lineToHex[] = {12, 29, 45, 62, 78, 95, 112, 130, 147, 165, 182};

	return lineToHex[line];
}

std::pair<const CStack *, BattleHex> BattleInfo::getNearestStack(const CStack * closest, boost::logic::tribool attackerOwned) const
{	
	bool ac[GameConstants::BFIELD_SIZE];
	std::set<BattleHex> occupyable;

	getAccessibilityMap(ac, closest->doubleWide(), closest->attackerOwned, false, occupyable, closest->hasBonusOfType(Bonus::FLYING), closest);

	BattleHex predecessor[GameConstants::BFIELD_SIZE];
	int dist[GameConstants::BFIELD_SIZE];
	makeBFS(closest->position, ac, predecessor, dist, closest->doubleWide(), closest->attackerOwned, closest->hasBonusOfType(Bonus::FLYING), true);

	std::vector< std::pair< std::pair<int, int>, const CStack *> > stackPairs; //pairs <<distance, hex>, stack>
	for(int g=0; g<GameConstants::BFIELD_SIZE; ++g)
	{
		const CStack * atG = getStackT(g);
		if(!atG || atG->ID == closest->ID) //if there is not stack or we are the closest one
			continue;
		if(boost::logic::indeterminate(attackerOwned) || atG->attackerOwned == attackerOwned)
		{
			if(predecessor[g] == -1) //TODO: is it really the best solution?
				continue;
			stackPairs.push_back( std::make_pair( std::make_pair(dist[predecessor[g]], g), atG) );
		}
	}

	if(stackPairs.size() > 0)
	{
		std::vector< std::pair< std::pair<int, int>, const CStack *> > minimalPairs;
		minimalPairs.push_back(stackPairs[0]);

		for(int b=1; b<stackPairs.size(); ++b)
		{
			if(stackPairs[b].first.first < minimalPairs[0].first.first)
			{
				minimalPairs.clear();
				minimalPairs.push_back(stackPairs[b]);
			}
			else if(stackPairs[b].first.first == minimalPairs[0].first.first)
			{
				minimalPairs.push_back(stackPairs[b]);
			}
		}

		std::pair< std::pair<int, int>, const CStack *> minPair = minimalPairs[minimalPairs.size()/2];

		return std::make_pair(minPair.second, predecessor[minPair.first.second]);
	}

	return std::make_pair<const CStack * , BattleHex>(NULL, BattleHex::INVALID);
}
ui32 BattleInfo::calculateSpellBonus(ui32 baseDamage, const CSpell * sp, const CGHeroInstance * caster, const CStack * affectedCreature) const
{
	ui32 ret = baseDamage;
	//applying sorcery secondary skill
	if(caster)
	{
		ret *= (100.0 + caster->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, CGHeroInstance::SORCERY)) / 100.0;
		ret *= (100.0 + caster->valOfBonuses(Bonus::SPELL_DAMAGE) + caster->valOfBonuses(Bonus::SPECIFIC_SPELL_DAMAGE, sp->id)) / 100.0;

		if(sp->air)
			ret *= (100.0 + caster->valOfBonuses(Bonus::AIR_SPELL_DMG_PREMY)) / 100.0;
		else if(sp->fire) //only one type of bonus for Magic Arrow
			ret *= (100.0 + caster->valOfBonuses(Bonus::FIRE_SPELL_DMG_PREMY)) / 100.0;
		else if(sp->water)
			ret *= (100.0 + caster->valOfBonuses(Bonus::WATER_SPELL_DMG_PREMY)) / 100.0;
		else if(sp->earth)
			ret *= (100.0 + caster->valOfBonuses(Bonus::EARTH_SPELL_DMG_PREMY)) / 100.0;

		if (affectedCreature && affectedCreature->getCreature()->level) //Hero specials like Solmyr, Deemer
			ret *= (100. + ((caster->valOfBonuses(Bonus::SPECIAL_SPELL_LEV, sp->id) * caster->level) / affectedCreature->getCreature()->level)) / 100.0;
	}
	return ret;
}

ui32 BattleInfo::calculateSpellDmg( const CSpell * sp, const CGHeroInstance * caster, const CStack * affectedCreature, int spellSchoolLevel, int usedSpellPower ) const
{
	ui32 ret = 0; //value to return

	//15 - magic arrows, 16 - ice bolt, 17 - lightning bolt, 18 - implosion, 20 - frost ring, 21 - fireball, 22 - inferno, 23 - meteor shower,
	//24 - death ripple, 25 - destroy undead, 26 - armageddon, 77 - thunderbolt

	//check if spell really does damage - if not, return 0
	if(VLC->spellh->damageSpells.find(sp->id) == VLC->spellh->damageSpells.end())
		return 0;

	ret = usedSpellPower * sp->power;
	ret += sp->powers[spellSchoolLevel];

	//affected creature-specific part
	if(affectedCreature)
	{
		//applying protections - when spell has more then one elements, only one protection should be applied (I think)
		if(sp->air && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 0)) //air spell & protection from air
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 0);
			ret /= 100;
		}
		else if(sp->fire && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 1)) //fire spell & protection from fire
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 1);
			ret /= 100;
		}
		else if(sp->water && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 2)) //water spell & protection from water
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 2);
			ret /= 100;
		}
		else if (sp->earth && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, 3)) //earth spell & protection from earth
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, 3);
			ret /= 100;
		}
		//general spell dmg reduction
		if(sp->air && affectedCreature->hasBonusOfType(Bonus::SPELL_DAMAGE_REDUCTION, -1)) //air spell & protection from air
		{
			ret *= affectedCreature->valOfBonuses(Bonus::SPELL_DAMAGE_REDUCTION, -1);
			ret /= 100;
		}
		//dmg increasing
		if( affectedCreature->hasBonusOfType(Bonus::MORE_DAMAGE_FROM_SPELL, sp->id) )
		{
			ret *= 100 + affectedCreature->valOfBonuses(Bonus::MORE_DAMAGE_FROM_SPELL, sp->id);
			ret /= 100;
		}
	}
	ret = calculateSpellBonus(ret, sp, caster, affectedCreature);
	return ret;
}

ui32 BattleInfo::calculateHealedHP(const CGHeroInstance * caster, const CSpell * spell, const CStack * stack) const
{
	bool resurrect = resurrects(spell->id);
	int healedHealth = caster->getPrimSkillLevel(2) * spell->power + spell->powers[caster->getSpellSchoolLevel(spell)];
	healedHealth = calculateSpellBonus(healedHealth, spell, caster, stack);
	return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft + (resurrect ? stack->baseAmount * stack->MaxHealth() : 0));
}
ui32 BattleInfo::calculateHealedHP(int healedHealth, const CSpell * spell, const CStack * stack) const
{
	bool resurrect = resurrects(spell->id);
	return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft + (resurrect ? stack->baseAmount * stack->MaxHealth() : 0));
}
ui32 BattleInfo::calculateHealedHP(const CSpell * spell, int usedSpellPower, int spellSchoolLevel, const CStack * stack) const
{
	bool resurrect = resurrects(spell->id);
	int healedHealth = usedSpellPower * spell->power + spell->powers[spellSchoolLevel];
	return std::min<ui32>(healedHealth, stack->MaxHealth() - stack->firstHPleft + (resurrect ? stack->baseAmount * stack->MaxHealth() : 0));
}
bool BattleInfo::resurrects(TSpell spellid) const
{
	return vstd::contains(VLC->spellh->risingSpells, spellid);
}

void BattleInfo::getStackQueue( std::vector<const CStack *> &out, int howMany, int turn /*= 0*/, int lastMoved /*= -1*/ ) const
{
	//we'll split creatures with remaining movement to 4 parts
	std::vector<const CStack *> phase[4]; //0 - turrets/catapult, 1 - normal (unmoved) creatures, other war machines, 2 - waited cres that had morale, 3 - rest of waited cres
	int toMove = 0; //how many stacks still has move
	const CStack *active = getStack(activeStack);

	//active stack hasn't taken any action yet - must be placed at the beginning of queue, no matter what
	if(!turn && active && active->willMove() && !vstd::contains(active->state, EBattleStackState::WAITING))
	{
		out.push_back(active);
		if(out.size() == howMany)
			return;
	}


	for(ui32 i=0; i<stacks.size(); ++i)
	{
		const CStack * const s = stacks[i];
		if((turn <= 0 && !s->willMove()) //we are considering current round and stack won't move
		   || (turn > 0 && !s->canMove(turn)) //stack won't be able to move in later rounds
		   || (turn <= 0 && s == active && out.size() && s == out.front())) //it's active stack already added at the beginning of queue
		{
			continue;
		}

		int p = -1; //in which phase this tack will move?
		if(turn <= 0 && vstd::contains(s->state, EBattleStackState::WAITING)) //consider waiting state only for ongoing round
		{
			if(vstd::contains(s->state, EBattleStackState::HAD_MORALE))
				p = 2;
			else
				p = 3;
		}
		else if(s->getCreature()->idNumber == 145  ||  s->getCreature()->idNumber == 149) //catapult and turrets are first
		{
			p = 0;
		}
		else
		{
			p = 1;
		}

		phase[p].push_back(s);
		toMove++;
	}

	for(int i = 0; i < 4; i++)
		std::sort(phase[i].begin(), phase[i].end(), CMP_stack(i, turn > 0 ? turn : 0));

	for(size_t i = 0; i < phase[0].size() && i < howMany; i++)
		out.push_back(phase[0][i]);

	if(out.size() == howMany)
		return;

	if(lastMoved == -1)
	{
		if(active)
		{
			if(out.size() && out.front() == active)
				lastMoved = active->attackerOwned;
			else
				lastMoved = active->attackerOwned;
		}
		else
		{
			lastMoved = 0;
		}
	}

	int pi = 1;
	while(out.size() < howMany)
	{
		const CStack *hlp = takeStack(phase[pi], lastMoved, turn);
		if(!hlp)
		{
			pi++;
			if(pi > 3)
			{
				//if(turn != 2)
				getStackQueue(out, howMany, turn + 1, lastMoved);
				return;
			}
		}
		else
		{
			out.push_back(hlp);
		}
	}
}

si8 BattleInfo::hasDistancePenalty( const CStack * stack, BattleHex destHex ) const
{
	struct HLP
	{
		static bool lowerAnalyze(const CStack * stack, BattleHex hex)
		{
			int distance = BattleHex::getDistance(hex, stack->position);

			//I hope it's approximately correct
			return distance > 10 && !stack->hasBonusOfType(Bonus::NO_DISTANCE_PENALTY);
		}
	};
	const CStack * dstStack = getStackT(destHex, false);

	if (dstStack->doubleWide())
		return HLP::lowerAnalyze(stack, destHex) && HLP::lowerAnalyze(stack, dstStack->occupiedHex());
	else
		return HLP::lowerAnalyze(stack, destHex);

}

si8 BattleInfo::sameSideOfWall(int pos1, int pos2) const
{
	int wallInStackLine = lineToWallHex(pos1/GameConstants::BFIELD_WIDTH);
	int wallInDestLine = lineToWallHex(pos2/GameConstants::BFIELD_WIDTH);

	bool stackLeft = pos1 < wallInStackLine;
	bool destLeft = pos2 < wallInDestLine;

	return stackLeft != destLeft;
}

si8 BattleInfo::hasWallPenalty( const CStack* stack, BattleHex destHex ) const
{
	if (!siege || stack->hasBonusOfType(Bonus::NO_WALL_PENALTY))
	{
		return false;
	}

	int wallInStackLine = lineToWallHex(stack->position/GameConstants::BFIELD_WIDTH);
	int wallInDestLine = lineToWallHex(destHex/GameConstants::BFIELD_WIDTH);

	bool stackLeft = stack->position < wallInStackLine;
	bool destRight = destHex > wallInDestLine;

	if (stackLeft && destRight) //shooting from outside to inside
	{
		int row = (stack->position + destHex) / (2 * GameConstants::BFIELD_WIDTH);
		if (stack->position > destHex && ((destHex % GameConstants::BFIELD_WIDTH - stack->position % GameConstants::BFIELD_WIDTH) < 2)) //shooting up high
			row -= 2;
		int wallPos = lineToWallHex(row);
		if (hexToWallPart(wallPos) != -1) //wall still exists or is indestructible
			return true;
	}
	return false;
}

si8 BattleInfo::canTeleportTo(const CStack * stack, BattleHex destHex, int telportLevel) const
{
	bool ac[GameConstants::BFIELD_SIZE];

	std::set<BattleHex> occupyable;

	getAccessibilityMap(ac, stack->doubleWide(), stack->attackerOwned, false, occupyable, stack->hasBonusOfType(Bonus::FLYING), stack);

	if (siege && telportLevel < 2) //check for wall
	{
		return ac[destHex] && sameSideOfWall(stack->position, destHex);
	}
	else
	{
		return ac[destHex];
	}

}

bool BattleInfo::battleCanShoot(const CStack * stack, BattleHex dest) const
{
	if(tacticDistance) //no shooting during tactics
		return false; 

	const CStack *dst = getStackT(dest);

	if(!stack || !dst) return false;

	const CGHeroInstance * stackHero = battleGetOwner(stack);

	if(stack->hasBonusOfType(Bonus::FORGETFULL)) //forgetfulness
		return false;

	if(stack->getCreature()->idNumber == 145 && dst) //catapult cannot attack creatures
		return false;

	if(stack->hasBonusOfType(Bonus::SHOOTER)//it's shooter
		&& stack->owner != dst->owner
		&& dst->alive()
		&& (!isStackBlocked(stack)  ||  NBonus::hasOfType(stackHero, Bonus::FREE_SHOOTING))
		&& stack->shots
		)
		return true;
	return false;
}

bool BattleInfo::battleCanFlee(int player) const
{
	if (player == sides[0])
	{
		if (!heroes[0])
			return false;//current player have no hero
	}
	else
	{
		if (!heroes[1])
			return false;
	}

	if( ( heroes[0] && heroes[0]->hasBonusOfType(Bonus::ENEMY_CANT_ESCAPE) ) //eg. one of heroes is wearing shakles of war
		|| ( heroes[1] && heroes[1]->hasBonusOfType(Bonus::ENEMY_CANT_ESCAPE)))
		return false;

	if (player == sides[1] && siege //defender in siege
		&& !(town->subID == 6 && vstd::contains(town->builtBuildings, 17)))//without escape tunnel
		return false;

	return true;
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

si8 BattleInfo::battleMinSpellLevel() const
{
	si8 levelLimit = 0;

	if(const CGHeroInstance *h1 =  heroes[0])
	{
		vstd::amax(levelLimit, h1->valOfBonuses(Bonus::LEVEL_SPELL_IMMUNITY));
	}

	if(const CGHeroInstance *h2 = heroes[1])
	{
		vstd::amax(levelLimit, h2->valOfBonuses(Bonus::LEVEL_SPELL_IMMUNITY));
	}

	return levelLimit;
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

BattleInfo * BattleInfo::setupBattle( int3 tile, int terrain, int terType, const CArmedInstance *armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance *town )
{
	CMP_stack cmpst;
	BattleInfo *curB = new BattleInfo;
	curB->castSpells[0] = curB->castSpells[1] = 0;
	curB->sides[0] = armies[0]->tempOwner;
	curB->sides[1] = armies[1]->tempOwner;
	if(curB->sides[1] == 254) 
		curB->sides[1] = 255;

	std::vector<CStack*> & stacks = (curB->stacks);

	curB->tile = tile;
	curB->battlefieldType = terType;
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
	}
	else
	{
		curB->town = NULL;
		curB->siege = 0;
	}

	//reading battleStartpos
	std::vector< std::vector<int> > attackerLoose, defenderLoose, attackerTight, defenderTight, attackerCreBank, defenderCreBank;
	const JsonNode config(GameConstants::DATA_DIR + "/config/battleStartpos.json");
	const JsonVector &positions = config["battle_positions"].Vector();

	CGH::readBattlePositions(positions[0]["levels"], attackerLoose);
	CGH::readBattlePositions(positions[1]["levels"], defenderLoose);
	CGH::readBattlePositions(positions[2]["levels"], attackerTight);
	CGH::readBattlePositions(positions[3]["levels"], defenderTight);
	CGH::readBattlePositions(positions[4]["levels"], attackerCreBank);
	CGH::readBattlePositions(positions[5]["levels"], defenderCreBank);

	//battleStartpos read
	int k = 0; //stack serial 
	for(TSlots::const_iterator i = armies[0]->Slots().begin(); i!=armies[0]->Slots().end(); i++, k++)
	{
		int pos;
		if(creatureBank)
			pos = attackerCreBank[armies[0]->stacksCount()-1][k];
		else if(armies[0]->formation)
			pos = attackerTight[armies[0]->stacksCount()-1][k];
		else
			pos = attackerLoose[armies[0]->stacksCount()-1][k];

		CStack * stack = curB->generateNewStack(*i->second, true, i->first, pos);
		stacks.push_back(stack);
	}

	k = 0;
	for(TSlots::const_iterator i = armies[1]->Slots().begin(); i!=armies[1]->Slots().end(); i++, k++)
	{
		int pos;
		if(creatureBank)
			pos = defenderCreBank[armies[1]->stacksCount()-1][k];
		else if(armies[1]->formation)
			pos = defenderTight[armies[1]->stacksCount()-1][k];
		else
			pos = defenderLoose[armies[1]->stacksCount()-1][k];

		CStack * stack = curB->generateNewStack(*i->second, false, i->first, pos);
		stacks.push_back(stack);
	}

	for(unsigned g=0; g<stacks.size(); ++g) //shifting positions of two-hex creatures
	{
		//we should do that for creature bank too
		if(stacks[g]->doubleWide() && stacks[g]->attackerOwned)
		{
			stacks[g]->position += BattleHex::RIGHT;
		}
		else if(stacks[g]->doubleWide() && !stacks[g]->attackerOwned)
		{
			if (stacks[g]->position.getX() > 1)
				stacks[g]->position += BattleHex::LEFT;
		}
	}

	//adding war machines
	if(!creatureBank)
	{
		if(heroes[0])
		{
			if(heroes[0]->getArt(13)) //ballista
			{
				CStack * stack = curB->generateNewStack(CStackBasicDescriptor(146, 1), true, 255, 52);
				stacks.push_back(stack);
			}
			if(heroes[0]->getArt(14)) //ammo cart
			{
				CStack * stack = curB->generateNewStack(CStackBasicDescriptor(148, 1), true, 255, 18);
				stacks.push_back(stack);
			}
			if(heroes[0]->getArt(15)) //first aid tent
			{
				CStack * stack = curB->generateNewStack(CStackBasicDescriptor(147, 1), true, 255, 154);
				stacks.push_back(stack);
			}
		}
		if(heroes[1])
		{
			//defending hero shouldn't receive ballista (bug #551)
			if(heroes[1]->getArt(13) && !town) //ballista
			{
				CStack * stack = curB->generateNewStack(CStackBasicDescriptor(146, 1),  false, 255, 66);
				stacks.push_back(stack);
			}
			if(heroes[1]->getArt(14)) //ammo cart
			{
				CStack * stack = curB->generateNewStack(CStackBasicDescriptor(148, 1), false, 255, 32);
				stacks.push_back(stack);
			}
			if(heroes[1]->getArt(15)) //first aid tent
			{
				CStack * stack = curB->generateNewStack(CStackBasicDescriptor(147, 1), false, 255, 168);
				stacks.push_back(stack);
			}
		}
		if(town && heroes[0] && town->hasFort()) //catapult
		{
			CStack * stack = curB->generateNewStack(CStackBasicDescriptor(145, 1), true, 255, 120);
			stacks.push_back(stack);
		}
	}
	//war machines added

	if (curB->siege == 2 || curB->siege == 3)
	{
		// keep tower
		CStack * stack = curB->generateNewStack(CStackBasicDescriptor(149, 1), false, 255, -2);
		stacks.push_back(stack);

		if (curB->siege == 3)
		{
			// lower tower + upper tower
			CStack * stack = curB->generateNewStack(CStackBasicDescriptor(149, 1), false, 255, -4);
			stacks.push_back(stack);
			stack = curB->generateNewStack(CStackBasicDescriptor(149, 1), false, 255, -3);
			stacks.push_back(stack);
		}
	}

	std::stable_sort(stacks.begin(),stacks.end(),cmpst);

	//setting up siege
	if(town && town->hasFort())
	{
		for(int b=0; b<ARRAY_COUNT(curB->si.wallState); ++b)
		{
			curB->si.wallState[b] = 1;
		}
	}

	//randomize obstacles
	if(town == NULL && !creatureBank) //do it only when it's not siege and not creature bank
	{
		bool obAv[GameConstants::BFIELD_SIZE]; //availability of hexes for obstacles;
		std::vector<int> possibleObstacles;

		for(int i=0; i<GameConstants::BFIELD_SIZE; ++i)
		{
			if(i%17 < 4 || i%17 > 12)
			{
				obAv[i] = false;
			}
			else
			{
				obAv[i] = true;
			}
		}

		for(std::map<int, CObstacleInfo>::const_iterator g=VLC->heroh->obstacles.begin(); g!=VLC->heroh->obstacles.end(); ++g)
		{
			if(g->second.allowedTerrains[terType-1] == '1') //we need to take terType with -1 because terrain ids start from 1 and allowedTerrains array is indexed from 0
			{
				possibleObstacles.push_back(g->first);
			}
		}

		srand(time(NULL));
		if(possibleObstacles.size() > 0) //we cannot place any obstacles when we don't have them
		{
			int toBlock = rand()%6 + 6; //how many hexes should be blocked by obstacles
			while(toBlock>0)
			{
				CObstacleInstance coi;
				coi.uniqueID = curB->obstacles.size();
				coi.ID = possibleObstacles[rand()%possibleObstacles.size()];
				coi.pos = rand()%GameConstants::BFIELD_SIZE;
				std::vector<BattleHex> block = VLC->heroh->obstacles[coi.ID].getBlocked(coi.pos);
				bool badObstacle = false;
				for(int b=0; b<block.size(); ++b)
				{
					if(block[b] < 0 || block[b] >= GameConstants::BFIELD_SIZE || !obAv[block[b]])
					{
						badObstacle = true;
						break;
					}
				}
				if(badObstacle) continue;
				//obstacle can be placed
				curB->obstacles.push_back(coi);
				for(int b=0; b<block.size(); ++b)
				{
					if(block[b] >= 0 && block[b] < GameConstants::BFIELD_SIZE)
						obAv[block[b]] = false;
				}
				toBlock -= block.size();
			}
		}
	}

	//spell level limiting bonus
	curB->addNewBonus(new Bonus(Bonus::ONE_BATTLE, Bonus::LEVEL_SPELL_IMMUNITY, Bonus::OTHER,
		0, -1, -1, Bonus::INDEPENDENT_MAX));

	//giving terrain overalay premies
	int bonusSubtype = -1;
	switch(terType)
	{
	case 9: //magic plains
		{
			bonusSubtype = 0;
		}
	case 14: //fiery fields
		{
			if(bonusSubtype == -1) bonusSubtype = 1;
		}
	case 15: //rock lands
		{
			if(bonusSubtype == -1) bonusSubtype = 8;
		}
	case 16: //magic clouds
		{
			if(bonusSubtype == -1) bonusSubtype = 2;
		}
	case 17: //lucid pools
		{
			if(bonusSubtype == -1) bonusSubtype = 4;
		}

		{ //common part for cases 9, 14, 15, 16, 17
			curB->addNewBonus(new Bonus(Bonus::ONE_BATTLE, Bonus::MAGIC_SCHOOL_SKILL, Bonus::TERRAIN_OVERLAY, 3, -1, "", bonusSubtype));
			break;
		}

	case 18: //holy ground
		{
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, +1, Bonus::TERRAIN_OVERLAY)->addLimiter(make_shared<CreatureAlignmentLimiter>(EAlignment::GOOD)));
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, -1, Bonus::TERRAIN_OVERLAY)->addLimiter(make_shared<CreatureAlignmentLimiter>(EAlignment::EVIL)));
			break;
		}
	case 19: //clover field
		{ //+2 luck bonus for neutral creatures
			curB->addNewBonus(makeFeature(Bonus::LUCK, Bonus::ONE_BATTLE, 0, +2, Bonus::TERRAIN_OVERLAY)->addLimiter(make_shared<CreatureFactionLimiter>(-1)));
			break;
		}
	case 20: //evil fog
		{
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, -1, Bonus::TERRAIN_OVERLAY)->addLimiter(make_shared<CreatureAlignmentLimiter>(EAlignment::GOOD)));
			curB->addNewBonus(makeFeature(Bonus::MORALE, Bonus::ONE_BATTLE, 0, +1, Bonus::TERRAIN_OVERLAY)->addLimiter(make_shared<CreatureAlignmentLimiter>(EAlignment::EVIL)));
			break;
		}
	case 22: //cursed ground
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
	if(town) //during siege always take premies for native terrain of faction
		terrain = VLC->heroh->nativeTerrains[town->town->typeID];

	auto nativeTerrain = make_shared<CreatureNativeTerrainLimiter>(terrain);
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
			tacticLvls[i] += heroes[i]->getSecSkillLevel(CGHeroInstance::TACTICS);
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

bool BattleInfo::isInTacticRange( BattleHex dest ) const
{

	return ((!tacticsSide && dest.getX() > 0 && dest.getX() <= tacticDistance)
		|| (tacticsSide && dest.getX() < GameConstants::BFIELD_WIDTH - 1 && dest.getX() >= GameConstants::BFIELD_WIDTH - tacticDistance - 1));
}

ESpellCastProblem::ESpellCastProblem BattleInfo::battleCanCastSpell(int player, ECastingMode::ECastingMode mode) const
{
	int side = sides[0] == player ? 0 : 1;

	switch (mode)
	{
		case ECastingMode::HERO_CASTING:
		{
			if(tacticDistance)
				return ESpellCastProblem::ONGOING_TACTIC_PHASE;
			if(castSpells[side] > 0)
				return ESpellCastProblem::ALREADY_CASTED_THIS_TURN;
			if(!heroes[side])
				return ESpellCastProblem::NO_HERO_TO_CAST_SPELL;
			if(!heroes[side]->getArt(17))
				return ESpellCastProblem::NO_SPELLBOOK;
		}
		break;
	}

	return ESpellCastProblem::OK;
}

ESpellCastProblem::ESpellCastProblem BattleInfo::battleCanCastThisSpell( int player, const CSpell * spell, ECastingMode::ECastingMode mode ) const
{
	ESpellCastProblem::ESpellCastProblem genProblem = battleCanCastSpell(player, mode);
	if(genProblem != ESpellCastProblem::OK)
		return genProblem;

	int cside = sides[0] == player ? 0 : 1; //caster's side
	switch(mode)
	{
		case ECastingMode::HERO_CASTING:
		{
			const CGHeroInstance * caster = heroes[cside];
			if(!caster->canCastThisSpell(spell))
				return ESpellCastProblem::HERO_DOESNT_KNOW_SPELL;

			if(caster->mana < getSpellCost(spell, caster)) //not enough mana
				return ESpellCastProblem::NOT_ENOUGH_MANA;
		}
		break;
	}
	

	if(spell->id < 10) //it's adventure spell (not combat))
		return ESpellCastProblem::ADVMAP_SPELL_INSTEAD_OF_BATTLE_SPELL;

	if(NBonus::hasOfType(heroes[1-cside], Bonus::SPELL_IMMUNITY, spell->id)) //non - casting hero provides immunity for this spell 
		return ESpellCastProblem::SECOND_HEROS_SPELL_IMMUNITY;

	if(battleMinSpellLevel() > spell->level) //non - casting hero stops caster from casting this spell
		return ESpellCastProblem::SPELL_LEVEL_LIMIT_EXCEEDED;
	
	int spellIDs[] = {66, 67, 68, 69}; //IDs of summon elemental spells (fire, earth, water, air)
	int creIDs[] = {114, 113, 115, 112}; //(fire, earth, water, air)

	int * idp = std::find(spellIDs, spellIDs + ARRAY_COUNT(spellIDs), spell->id);
	int arpos = idp - spellIDs;
	if(arpos < ARRAY_COUNT(spellIDs))
	{
		//check if there are summoned elementals of other type
		BOOST_FOREACH ( const CStack * st, stacks)
		{
			if (vstd::contains(st->state, EBattleStackState::SUMMONED) && st->getCreature()->idNumber != creIDs[arpos])
			{
				return ESpellCastProblem::ANOTHER_ELEMENTAL_SUMMONED;
			}
		}
	}

	//checking if there exists an appropriate target
	switch(spell->getTargetType())
	{
	case CSpell::CREATURE:
	case CSpell::CREATURE_EXPERT_MASSIVE:
		if(mode == ECastingMode::HERO_CASTING)
		{
			const CGHeroInstance * caster = getHero(player);
			bool targetExists = false;
			BOOST_FOREACH(const CStack * stack, stacks)
			{
				switch (spell->positiveness)
				{
				case CSpell::POSITIVE:
					if(stack->owner == caster->getOwner())
					{
						if(battleIsImmune(caster, spell, mode, stack->position) == ESpellCastProblem::OK)
						{
							targetExists = true;
							break;
						}
					}
					break;
				case CSpell::NEUTRAL:
					if(battleIsImmune(caster, spell, mode, stack->position) == ESpellCastProblem::OK)
					{
						targetExists = true;
						break;
					}
					break;
				case CSpell::NEGATIVE:
					if(stack->owner != caster->getOwner())
					{
						if(battleIsImmune(caster, spell, mode, stack->position) == ESpellCastProblem::OK)
						{
							targetExists = true;
							break;
						}
					}
					break;
				}
			}
			if(!targetExists)
			{
				return ESpellCastProblem::NO_APPROPRIATE_TARGET;
			}
		}
		break;
	case CSpell::OBSTACLE:
		break;
	}

	return ESpellCastProblem::OK;
}

ESpellCastProblem::ESpellCastProblem BattleInfo::battleCanCastThisSpellHere( int player, const CSpell * spell, ECastingMode::ECastingMode mode, BattleHex dest ) const
{
	ESpellCastProblem::ESpellCastProblem moreGeneralProblem = battleCanCastThisSpell(player, spell, mode);
	if(moreGeneralProblem != ESpellCastProblem::OK)
		return moreGeneralProblem;

	if(spell->getTargetType() == CSpell::OBSTACLE  &&  !isObstacleOnTile(dest))
		return ESpellCastProblem::NO_APPROPRIATE_TARGET;


	//get dead stack if we cast resurrection or animate dead
	const CStack *deadStack = getStackIf([dest](const CStack *s) { return !s->alive() && s->position == dest; });
	const CStack *aliveStack = getStackIf([dest](const CStack *s) { return s->alive() && s->position == dest; });


	if(spell->isRisingSpell())
	{
		if(!deadStack || aliveStack)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
		if(spell->id == Spells::ANIMATE_DEAD  &&  !deadStack->hasBonusOfType(Bonus::UNDEAD)) 
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
		if(deadStack->owner != player) //you can resurrect only your own stacks
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}
	else if(spell->getTargetType() == CSpell::CREATURE  ||  spell->getTargetType() == CSpell::CREATURE_EXPERT_MASSIVE)
	{
		if(!aliveStack)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
		if(spell->isNegative() && aliveStack->owner == player)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
		if(spell->isPositive() && aliveStack->owner != player)
			return ESpellCastProblem::NO_APPROPRIATE_TARGET;
	}


	if (mode != ECastingMode::CREATURE_ACTIVE_CASTING && mode != ECastingMode::ENCHANTER_CASTING)
		return battleIsImmune(getHero(player), spell, mode, dest);
	else
		return battleIsImmune(NULL, spell, mode, dest);
}

TSpell BattleInfo::getRandomBeneficialSpell(const CStack * subject) const
{
	std::vector<TSpell> possibleSpells;
	CSpell * spell;
	for (int i = 0; i < GameConstants::SPELLS_QUANTITY; ++i) //should not use future spells added by mods
	{
		spell = VLC->spellh->spells[i];
		if (spell->isPositive()) //only positive
		{
			if (subject->hasBonusFrom(Bonus::SPELL_EFFECT, i) ||
				battleCanCastThisSpellHere(subject->owner, spell, ECastingMode::CREATURE_ACTIVE_CASTING, subject->position) != ESpellCastProblem::OK)
				continue;
			switch (i)
			{
				case Spells::SHIELD:
				case Spells::FIRE_SHIELD: // not if all enemy units are shooters
				{
					bool walkerPresent = false;
					BOOST_FOREACH (CStack * stack, stacks)
					{
						 if ((stack->owner != subject->owner) && !stack->shots)
						 {
							 walkerPresent = true;
							 break;
						 }
					}
					if (!walkerPresent)
						continue;
				}
					break;
				case Spells::AIR_SHIELD: //only against active shooters
				{
					bool shooterPresent = false;
					BOOST_FOREACH (CStack * stack, stacks)
					{
						 if ((stack->owner != subject->owner) && stack->hasBonusOfType(Bonus::SHOOTER) && stack->shots)
						 {
							 shooterPresent = true;
							 break;
						 }
					}
					if (!shooterPresent)
						continue;
					break;
				}
				case Spells::ANTI_MAGIC:
				case Spells::MAGIC_MIRROR:
				{
					if (!heroes[whatSide(theOtherPlayer(subject->owner))]) //only if there is enemy hero
						continue;
				}
					break;
				case Spells::CURE: //only damaged units - what about affected by curse?
				{
					if (subject->firstHPleft >= subject->MaxHealth())
						continue;
				}
					break;
				case Spells::BLOODLUST:
				{
					if (subject->shots) //if can shoot - only if enemy uits are adjacent
						continue;
				}
					break;
				case Spells::PRECISION:
				{
					if (!(subject->hasBonusOfType(Bonus::SHOOTER) && subject->shots))
						continue;
				}
					break;
				case Spells::SLAYER://only if monsters are present
				{
					bool monsterPresent = false;
					BOOST_FOREACH (CStack * stack, stacks)
					{
						 if ((stack->owner != subject->owner) &&
							 (stack->hasBonus(Selector::type(Bonus::KING1) || Selector::type(Bonus::KING2) || Selector::type(Bonus::KING3))))
						 {
							 monsterPresent = true;
							 break;
						 }
					}
					if (!monsterPresent)
						continue;
				}
					break;
				case Spells::CLONE: //not allowed
					continue;
					break;
			}
			possibleSpells.push_back(i);
		}
	}
	if (possibleSpells.size())
		return possibleSpells[ran() % possibleSpells.size()];
	else
		return -1;
}
TSpell BattleInfo::getRandomCastedSpell(const CStack * caster) const
{
	TBonusListPtr bl = caster->getBonuses(Selector::type(Bonus::SPELLCASTER));
	if (!bl->size())
		return -1;
	int totalWeight = 0;
	BOOST_FOREACH(Bonus * b, *bl)
	{
		totalWeight += std::max(b->additionalInfo, 1); //minimal chance to cast is 1
	}
	int randomPos = ran() % totalWeight;
	BOOST_FOREACH(Bonus * b, *bl)
	{
		randomPos -= std::max(b->additionalInfo, 1);
		if(randomPos < 0)
		{
			return b->subtype;
		}
	}

	return -1;
}

const CGHeroInstance * BattleInfo::getHero( int player ) const
{
	assert(sides[0] == player || sides[1] == player);
	if(heroes[0] && heroes[0]->getOwner() == player)
		return heroes[0];

	return heroes[1];
}

bool NegateRemover(const Bonus* b)
{
	return b->source == Bonus::CREATURE_ABILITY;
}

bool BattleInfo::battleTestElementalImmunity(const CStack * subject, const CSpell * spell, Bonus::BonusType element, bool damageSpell) const //helper for battleisImmune
{
	if (!spell->isPositive()) //negative or indifferent
	{
		if ((damageSpell && subject->hasBonusOfType(element, 2)) || subject->hasBonusOfType(element, 1))
			return true;
	}
	else if (spell->isPositive()) //positive
	{
		if (subject->hasBonusOfType(element, 0)) //must be immune to all spells
			return true;
	}
	return false;
}

ESpellCastProblem::ESpellCastProblem BattleInfo::battleIsImmune(const CGHeroInstance * caster, const CSpell * spell, ECastingMode::ECastingMode mode, BattleHex dest) const
{
	const CStack * subject;
	bool risingSpell = vstd::contains(VLC->spellh->risingSpells, spell->id);
	if (risingSpell)
		subject = getStackT(dest, false); //including dead stacks
	else
		subject = getStackT(dest, true); //only alive

	if(subject)
	{
		if (spell->isPositive() && subject->hasBonusOfType(Bonus::RECEPTIVE)) //accept all positive spells
			return ESpellCastProblem::OK;

		switch (spell->id) //TODO: more general logic for new spells?
		{
			case Spells::DESTROY_UNDEAD:
				if (!subject->hasBonusOfType(Bonus::UNDEAD))
					return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
				break;
			case Spells::DEATH_RIPPLE:
				if (subject->hasBonusOfType(Bonus::SIEGE_WEAPON))
					return ESpellCastProblem::STACK_IMMUNE_TO_SPELL; //don't break here - undeads and war machines are immune, non-living are not
			case Spells::BLESS:
			case Spells::CURSE: //undeads are immune to bless & curse
				if (subject->hasBonusOfType(Bonus::UNDEAD))
					return ESpellCastProblem::STACK_IMMUNE_TO_SPELL; 
				break;
			case Spells::HASTE:
			case Spells::SLOW:
			case Spells::TELEPORT:
			case Spells::CLONE:
				if (subject->hasBonusOfType(Bonus::SIEGE_WEAPON))
					return ESpellCastProblem::STACK_IMMUNE_TO_SPELL; //war machines are immune to some spells than involve movement
				if (spell->id == Spells::CLONE && caster) //TODO: how about stacks casting Clone?
				{
					if (vstd::contains(subject->state, EBattleStackState::CLONED))
						return ESpellCastProblem::STACK_IMMUNE_TO_SPELL; //can't clone already cloned creature
					int maxLevel = (std::max(caster->getSpellSchoolLevel(spell), (ui8)1) + 4);
					int creLevel = subject->getCreature()->level;
					if (maxLevel < creLevel) //tier 1-5 for basic, 1-6 for advanced, 1-7 for expert
						return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
				}
				break;
			case Spells::FORGETFULNESS:
				if (!subject->hasBonusOfType(Bonus::SHOOTER))
					return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
				break;
			case Spells::DISPEL_HELPFUL_SPELLS:
			{
				TBonusListPtr spellBon = subject->getSpellBonuses();
				bool hasPositiveSpell = false;
				BOOST_FOREACH(const Bonus * b, *spellBon)
				{
					if(VLC->spellh->spells[b->sid]->isPositive())
					{
						hasPositiveSpell = true;
						break;
					}
				}
				if(!hasPositiveSpell)
				{
					return ESpellCastProblem::NO_SPELLS_TO_DISPEL;
				}
			}
				break;
		}
		
		bool damageSpell = (vstd::contains(VLC->spellh->damageSpells, spell->id));

		if (damageSpell && subject->hasBonusOfType(Bonus::DIRECT_DAMAGE_IMMUNITY))
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;

		if (spell->fire)
		{
			if (battleTestElementalImmunity(subject, spell, Bonus::FIRE_IMMUNITY, damageSpell))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
		if (spell->water)
		{
			if (battleTestElementalImmunity(subject, spell, Bonus::WATER_IMMUNITY, damageSpell))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
		if (spell->earth)
		{
			if (battleTestElementalImmunity(subject, spell, Bonus::EARTH_IMMUNITY, damageSpell))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
		if (spell->air)
		{
			if (battleTestElementalImmunity(subject, spell, Bonus::AIR_IMMUNITY, damageSpell))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
		if (vstd::contains(VLC->spellh->mindSpells, spell->id))
		{
			if (subject->hasBonusOfType(Bonus::MIND_IMMUNITY))
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}

		if (risingSpell)
		{
			if (subject->count >= subject->baseAmount) //TODO: calculate potential hp raised
				return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}

		TBonusListPtr immunities = subject->getBonuses(Selector::type(Bonus::LEVEL_SPELL_IMMUNITY));
		if(subject->hasBonusOfType(Bonus::NEGATE_ALL_NATURAL_IMMUNITIES))
		{
			//std::remove_if(immunities->begin(), immunities->end(), NegateRemover);
			immunities->remove_if(NegateRemover);
		}

		if(subject->hasBonusOfType(Bonus::SPELL_IMMUNITY, spell->id) ||
			( immunities->size() > 0 && immunities->totalValue() >= spell->level && spell->level))
		{ 
			return ESpellCastProblem::STACK_IMMUNE_TO_SPELL;
		}
	}
	else //no target stack on this tile
	{
		if(spell->getTargetType() == CSpell::CREATURE ||
			(spell->getTargetType() == CSpell::CREATURE_EXPERT_MASSIVE && mode == ECastingMode::HERO_CASTING && caster && caster->getSpellSchoolLevel(spell) < 3))
		{
			return ESpellCastProblem::WRONG_SPELL_TARGET;
		}
	}

	return ESpellCastProblem::OK;
}

std::vector<ui32> BattleInfo::calculateResistedStacks(const CSpell * sp, const CGHeroInstance * caster, const CGHeroInstance * hero2, const std::set<CStack*> affectedCreatures, int casterSideOwner, ECastingMode::ECastingMode mode, int usedSpellPower, int spellLevel) const
{
	std::vector<ui32> ret;
	for(std::set<CStack*>::const_iterator it = affectedCreatures.begin(); it != affectedCreatures.end(); ++it)
	{
		if(battleIsImmune(caster, sp, mode, (*it)->position) != ESpellCastProblem::OK)
		{
			ret.push_back((*it)->ID);
			continue;
		}

		//non-negative spells on friendly stacks should always succeed, unless immune
		if(!sp->isNegative() && (*it)->owner == casterSideOwner)
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

	if(sp->id == 60) //hypnotize
	{
		for(std::set<CStack*>::const_iterator it = affectedCreatures.begin(); it != affectedCreatures.end(); ++it)
		{
			if( (*it)->hasBonusOfType(Bonus::SPELL_IMMUNITY, sp->id) //100% sure spell immunity
				|| ( (*it)->count - 1 ) * (*it)->MaxHealth() + (*it)->firstHPleft 
		> 
		usedSpellPower * 25 + sp->powers[spellLevel]
			)
			{
				ret.push_back((*it)->ID);
			}
		}
	}

	return ret;
}

int BattleInfo::getSurrenderingCost(int player) const
{
	if(!battleCanFlee(player)) //to surrender, conditions of fleeing must be fulfilled
		return -1;
	if(!getHero(theOtherPlayer(player))) //additionally, there must be an enemy hero
		return -2;

	int ret = 0;
	double discount = 0;
	BOOST_FOREACH(const CStack *s, stacks)
		if(s->owner == player  &&  s->base) //we pay for our stack that comes from our army (the last condition eliminates summoned cres and war machines)
			ret += s->getCreature()->cost[Res::GOLD] * s->count;

	if(const CGHeroInstance *h = getHero(player))
		discount += h->valOfBonuses(Bonus::SURRENDER_DISCOUNT);

	ret *= (100.0 - discount) / 100.0;
	vstd::amax(ret, 0); //no negative costs for >100% discounts (impossible in original H3 mechanics, but some day...)
	return ret;
}

int BattleInfo::theOtherPlayer(int player) const
{
	return sides[!whatSide(player)];
}

ui8 BattleInfo::whatSide(int player) const
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

bool BattleInfo::isObstacleOnTile(BattleHex tile) const
{
	std::set<BattleHex> coveredHexes;
	BOOST_FOREACH(const CObstacleInstance &obs, obstacles)
	{
		std::vector<BattleHex> blocked = VLC->heroh->obstacles.find(obs.ID)->second.getBlocked(obs.pos);
		for(size_t w = 0; w < blocked.size(); ++w)
			coveredHexes.insert(blocked[w]);
	}
	return vstd::contains(coveredHexes, tile);
}

const CStack * BattleInfo::getStackIf(boost::function<bool(const CStack*)> pred) const
{
	auto stackItr = range::find_if(stacks, pred);
	return stackItr == stacks.end() 
		? NULL
		: *stackItr;
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

	firstHPleft = MaxHealth();
	shots = getCreature()->valOfBonuses(Bonus::SHOTS);
	counterAttacks = 1 + valOfBonuses(Bonus::ADDITIONAL_RETALIATION);
	casts = valOfBonuses(Bonus::CASTS); //TODO: set them in cr_abils.txt
	state.insert(EBattleStackState::ALIVE);  //alive state indication
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
	if (useBind && getEffect(72)) 
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
		BOOST_FOREACH (CStack * stack, base->armyObj->battle->getAdjacentCreatures(this))
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

const Bonus * CStack::getEffect( ui16 id, int turn /*= 0*/ ) const
{
	BOOST_FOREACH(Bonus *it, getBonusList())
	{
		if(it->source == Bonus::SPELL_EFFECT && it->sid == id)
		{
			if(!turn || it->turnsRemain > turn)
				return &(*it);
		}
	}
	return NULL;
}

void CStack::stackEffectToFeature(std::vector<Bonus> & sf, const Bonus & sse)
{	
	si32 power = VLC->spellh->spells[sse.sid]->powers[sse.val];
	switch(sse.sid)
	{
	case 27: //shield 
	 	sf.push_back(featureGenerator(Bonus::GENERAL_DAMAGE_REDUCTION, 0, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 28: //air shield
	 	sf.push_back(featureGenerator(Bonus::GENERAL_DAMAGE_REDUCTION, 1, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 29: //fire shield
	 	sf.push_back(featureGenerator(Bonus::FIRE_SHIELD, 0, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 30: //protection from air
	 	sf.push_back(featureGenerator(Bonus::SPELL_DAMAGE_REDUCTION, 0, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 31: //protection from fire
	 	sf.push_back(featureGenerator(Bonus::SPELL_DAMAGE_REDUCTION, 1, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 32: //protection from water
	 	sf.push_back(featureGenerator(Bonus::SPELL_DAMAGE_REDUCTION, 2, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 33: //protection from earth
	 	sf.push_back(featureGenerator(Bonus::SPELL_DAMAGE_REDUCTION, 3, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 34: //anti-magic
	 	sf.push_back(featureGenerator(Bonus::LEVEL_SPELL_IMMUNITY, GameConstants::SPELL_LEVELS, power - 1, sse.turnsRemain));
		sf.back().valType = Bonus::INDEPENDENT_MAX;
	 	sf.back().sid = sse.sid;
	 	break;
	case 36: //magic mirror
		sf.push_back(featureGenerator(Bonus::MAGIC_MIRROR, -1, power, sse.turnsRemain));
		sf.back().valType = Bonus::INDEPENDENT_MAX;
	 	sf.back().sid = sse.sid;
	case 41: //bless
		sf.push_back(featureGenerator(Bonus::ALWAYS_MAXIMUM_DAMAGE, -1, power, sse.turnsRemain));
		sf.back().valType = Bonus::INDEPENDENT_MAX;
	 	sf.back().sid = sse.sid;
	 	break;
	case 42: //curse
	 	sf.push_back(featureGenerator(Bonus::ALWAYS_MINIMUM_DAMAGE, -1, power, sse.turnsRemain, sse.val >= 2 ? 20 : 0));
		sf.back().valType = Bonus::INDEPENDENT_MAX;
	 	sf.back().sid = sse.sid;
	 	break;
	case 43: //bloodlust
	 	sf.push_back(featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK, power, sse.turnsRemain, 0, Bonus::ONLY_MELEE_FIGHT));
	 	sf.back().sid = sse.sid;
	 	break;
	case 44: //precision
	 	sf.push_back(featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK, power, sse.turnsRemain, 0, Bonus::ONLY_DISTANCE_FIGHT));
	 	sf.back().sid = sse.sid;
	 	break;
	case 45: //weakness
	 	sf.push_back(featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK, -1 * power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 46: //stone skin
	 	sf.push_back(featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 47: //disrupting ray
	 	sf.push_back(featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE, -1 * power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
		sf.back().valType = Bonus::ADDITIVE_VALUE;
	 	break;
	case 48: //prayer
	 	sf.push_back(featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	sf.push_back(featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	sf.push_back(featureGenerator(Bonus::STACKS_SPEED, 0, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 49: //mirth
	 	sf.push_back(featureGenerator(Bonus::MORALE, 0, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 50: //sorrow
	 	sf.push_back(featureGenerator(Bonus::MORALE, 0, -1 * power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 51: //fortune
	 	sf.push_back(featureGenerator(Bonus::LUCK, 0, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 52: //misfortune
	 	sf.push_back(featureGenerator(Bonus::LUCK, 0, -1 * power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 53: //haste
	 	sf.push_back(featureGenerator(Bonus::STACKS_SPEED, 0, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 54: //slow
	 	sf.push_back(featureGeneratorVT(Bonus::STACKS_SPEED, 0, -1 * ( 100 - power ), sse.turnsRemain, Bonus::PERCENT_TO_ALL));
	 	sf.back().sid = sse.sid;
	 	break;
	case 55: //slayer
	 	sf.push_back(featureGenerator(Bonus::SLAYER, 0, sse.val, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 56: //frenzy
	 	sf.push_back(featureGenerator(Bonus::IN_FRENZY, 0, VLC->spellh->spells[56]->powers[sse.val]/100.0, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 58: //counterstrike
	 	sf.push_back(featureGenerator(Bonus::ADDITIONAL_RETALIATION, 0, power, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 59: //bersek
	 	sf.push_back(featureGenerator(Bonus::ATTACKS_NEAREST_CREATURE, 0, sse.val, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 60: //hypnotize
	 	sf.push_back(featureGenerator(Bonus::HYPNOTIZED, 0, sse.val, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case 61: //forgetfulness
	 	sf.push_back(featureGenerator(Bonus::FORGETFULL, 0, sse.val, sse.turnsRemain));
	 	sf.back().sid = sse.sid;
	 	break;
	case Spells::BLIND: //blind
		sf.push_back(makeFeatureVal(Bonus::NOT_ACTIVE, Bonus::UNITL_BEING_ATTACKED | Bonus::N_TURNS, sse.sid, 0, Bonus::SPELL_EFFECT, sse.turnsRemain));
		sf.back().sid = sse.sid;
		sf.push_back(makeFeatureVal(Bonus::GENERAL_ATTACK_REDUCTION, Bonus::UNTIL_ATTACK | Bonus::N_TURNS, 0, power, Bonus::SPELL_EFFECT, sse.turnsRemain));
		sf.back().sid = sse.sid;
		sf.push_back(makeFeatureVal(Bonus::NO_RETALIATION, Bonus::UNITL_BEING_ATTACKED, 0, 0, Bonus::SPELL_EFFECT, 0)); // don't retaliate after basilisk / unicorn attack
		sf.back().sid = sse.sid;
	 	break;
	case Spells::STONE_GAZE: //Stone Gaze
	case Spells::PARALYZE: //Paralyze
		sf.push_back(makeFeatureVal(Bonus::NOT_ACTIVE, Bonus::UNITL_BEING_ATTACKED | Bonus::N_TURNS, sse.sid, 0, Bonus::SPELL_EFFECT, sse.turnsRemain));
		sf.back().sid = sse.sid;
		sf.push_back(makeFeatureVal(Bonus::NO_RETALIATION, Bonus::UNITL_BEING_ATTACKED, 0, 0, Bonus::SPELL_EFFECT, 0)); // don't retaliate after basilisk / unicorn attack
		sf.back().sid = sse.sid;
		break;
	case 71: //Poison
		sf.push_back(featureGeneratorVT(Bonus::POISON, 0, 30, sse.turnsRemain, Bonus::INDEPENDENT_MAX)); //max hp penalty from this source
		sf.back().sid = sse.sid;
		sf.push_back(featureGeneratorVT(Bonus::STACK_HEALTH, 0, -10, sse.turnsRemain, Bonus::PERCENT_TO_ALL));
		sf.back().sid = sse.sid;
		break;
	case 72: //Bind
		sf.push_back(featureGenerator(Bonus::BIND_EFFECT, 0, 0, 1)); //marker
		sf.back().duration = Bonus::PERMANENT;
	 	sf.back().sid = sse.sid;
		break;
	case 73: //Disease
		sf.push_back(featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK, -2 , sse.turnsRemain));
	 	sf.back().sid = sse.sid;
		sf.push_back(featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE, -2 , sse.turnsRemain));
	 	sf.back().sid = sse.sid;
		break;
	case 75: //Age
		sf.push_back(featureGeneratorVT(Bonus::STACK_HEALTH, 0, -50, sse.turnsRemain, Bonus::PERCENT_TO_ALL));
		sf.back().sid = sse.sid;
		break;
	case 80: //Acid Breath
		sf.push_back(featureGenerator(Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE, -sse.turnsRemain, 1));
	 	sf.back().sid = sse.sid;
		sf.back().duration = Bonus::PERMANENT;
		sf.back().valType = Bonus::ADDITIVE_VALUE;
		break;
	}
}

ui8 CStack::howManyEffectsSet(ui16 id) const
{
	ui8 ret = 0;
	BOOST_FOREACH(const Bonus *it, getBonusList())
		if(it->source == Bonus::SPELL_EFFECT && it->sid == id) //effect found
		{
			++ret;
		}
		return ret;
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

bool CStack::doubleWide() const
{
	return getCreature()->doubleWide;
}

BattleHex CStack::occupiedHex() const
{
	if (doubleWide())
	{
		if (attackerOwned)
			return position - 1;
		else
			return position + 1;
	} 
	else
	{
		return BattleHex::INVALID;
	}
}

std::vector<BattleHex> CStack::getHexes() const
{
	std::vector<BattleHex> hexes;
	hexes.push_back(BattleHex(position));
	BattleHex occupied = occupiedHex();
	if(occupied.isValid())
		hexes.push_back(occupied);

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
			dynamic_cast<const CGHeroInstance *>(n);

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
		oss << " of armyobj=" << base->armyObj->id;
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
	return firstHPleft != MaxHealth()
		&& alive()
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


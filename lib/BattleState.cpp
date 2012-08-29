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


// const CStack * BattleInfo::getStackT(BattleHex tileID, bool onlyAlive) const
// {
// 	return const_cast<BattleInfo * const>(this)->getStackT(tileID, onlyAlive);
// }

// void BattleInfo::getAccessibilityMap(bool *accessibility, bool twoHex, bool attackerOwned, bool addOccupiable, std::set<BattleHex> & occupyable, bool flying, const CStack * stackToOmmit) const
// {
// 	memset(accessibility, 1, GameConstants::BFIELD_SIZE); //initialize array with trues
//
// 	//removing accessibility for side columns of hexes
// 	for(int v = 0; v < GameConstants::BFIELD_SIZE; ++v)
// 	{
// 		if( v % GameConstants::BFIELD_WIDTH == 0 || v % GameConstants::BFIELD_WIDTH == (GameConstants::BFIELD_WIDTH - 1) )
// 			accessibility[v] = false;
// 	}
//
// 	for(ui32 g=0; g<stacks.size(); ++g)
// 	{
// 		if(!stacks[g]->alive() || (stackToOmmit && stacks[g]->ID==stackToOmmit->ID) || stacks[g]->position < 0) //we don't want to lock position of this stack (eg. if it's a turret)
// 			continue;
//
// 		accessibility[stacks[g]->position] = false;
// 		if(stacks[g]->doubleWide()) //if it's a double hex creature
// 		{
// 			if(stacks[g]->attackerOwned)
// 				accessibility[stacks[g]->position-1] = false;
// 			else
// 				accessibility[stacks[g]->position+1] = false;
// 		}
// 	}
// 	//obstacles
// 	BOOST_FOREACH(const auto &obstacle, obstacles)
// 	{
// 		BOOST_FOREACH(BattleHex hex, obstacle->getBlockedTiles())
// 		{
// 			assert(hex.isValid());
// 			accessibility[hex] = false;
// 		}
// 	}
//
// 	//walls
// 	if(siege > 0)
// 	{
// 		static const int permanentlyLocked[] = {12, 45, 78, 112, 147, 165};
// 		for(int b=0; b<ARRAY_COUNT(permanentlyLocked); ++b)
// 		{
// 			accessibility[permanentlyLocked[b]] = false;
// 		}
//
// 		static const std::pair<int, BattleHex> lockedIfNotDestroyed[] = //(which part of wall, which hex is blocked if this part of wall is not destroyed
// 			{std::make_pair(2, BattleHex(182)), std::make_pair(3, BattleHex(130)),
// 			std::make_pair(4, BattleHex(62)), std::make_pair(5, BattleHex(29))};
// 		for(int b=0; b<ARRAY_COUNT(lockedIfNotDestroyed); ++b)
// 		{
// 			if(si.wallState[lockedIfNotDestroyed[b].first] < 3)
// 			{
// 				accessibility[lockedIfNotDestroyed[b].second] = false;
// 			}
// 		}
//
// 		//gate
// 		if(attackerOwned && si.wallState[7] < 3) //if it attacker's unit and gate is not destroyed
// 		{
// 			accessibility[95] = accessibility[96] = false; //block gate's hexes
// 		}
// 	}
//
// 	//occupyability
// 	if(addOccupiable && twoHex)
// 	{
// 		std::set<BattleHex> rem; //tiles to unlock
// 		for(int h=0; h<GameConstants::BFIELD_HEIGHT; ++h)
// 		{
// 			for(int w=1; w<GameConstants::BFIELD_WIDTH-1; ++w)
// 			{
// 				BattleHex hex(w, h);
// 				if(!isAccessible(hex, accessibility, twoHex, attackerOwned, flying, true)
// 					&& (attackerOwned ? isAccessible(hex+1, accessibility, twoHex, attackerOwned, flying, true) : isAccessible(hex-1, accessibility, twoHex, attackerOwned, flying, true) )
// 					)
// 					rem.insert(hex);
// 			}
// 		}
// 		occupyable = rem;
// 		/*for(std::set<int>::const_iterator it = rem.begin(); it != rem.end(); ++it)
// 		{
// 			accessibility[*it] = true;
// 		}*/
// 	}
// }

// bool BattleInfo::isAccessible(BattleHex hex, bool * accessibility, bool twoHex, bool attackerOwned, bool flying, bool lastPos)
// {
// 	if(flying && !lastPos)
// 		return true;
//
// 	if(twoHex)
// 	{
// 		//if given hex is accessible and appropriate adjacent one is free too
// 		return accessibility[hex] && accessibility[hex + (attackerOwned ? -1 : 1 )];
// 	}
// 	else
// 	{
// 		return accessibility[hex];
// 	}
// }

// void BattleInfo::makeBFS(BattleHex start, bool *accessibility, BattleHex *predecessor, int *dists, bool twoHex, bool attackerOwned, bool flying, bool fillPredecessors) const //both pointers must point to the at least 187-elements int arrays
// {
// 	std::set<BattleHex> quicksands = getStoppers(!attackerOwned);
//
// 	//inits
// 	for(int b=0; b<GameConstants::BFIELD_SIZE; ++b)
// 		predecessor[b] = -1;
// 	for(int g=0; g<GameConstants::BFIELD_SIZE; ++g)
// 		dists[g] = 100000000;
//
// 	std::queue< std::pair<BattleHex, bool> > hexq; //bfs queue <hex, accessible> (second filed used only if fillPredecessors is true)
// 	hexq.push(std::make_pair(start, true));
// 	dists[hexq.front().first] = 0;
// 	int curNext = -1; //for bfs loop only (helper var)
// 	while(!hexq.empty()) //bfs loop
// 	{
// 		std::pair<BattleHex, bool> curHex = hexq.front();
// 		std::vector<BattleHex> neighbours = curHex.first.neighbouringTiles();
// 		hexq.pop();
// 		if(curHex.first != start && !flying && vstd::contains(quicksands, curHex.first)) //walking stack can't step past the quicksands
// 			continue;
//
// 		for(ui32 nr=0; nr<neighbours.size(); nr++)
// 		{
// 			curNext = neighbours[nr]; //if(!accessibility[curNext] || (dists[curHex]+1)>=dists[curNext])
// 			bool accessible = isAccessible(curNext, accessibility, twoHex, attackerOwned, flying, dists[curHex.first]+1 == dists[curNext]);
// 			if( dists[curHex.first]+1 >= dists[curNext] )
// 				continue;
// 			if(accessible && curHex.second)
// 			{
// 				hexq.push(std::make_pair(curNext, true));
// 				dists[curNext] = dists[curHex.first] + 1;
// 			}
// 			else if(fillPredecessors && !(accessible && !curHex.second))
// 			{
// 				hexq.push(std::make_pair(curNext, false));
// 				dists[curNext] = dists[curHex.first] + 1;
// 			}
// 			predecessor[curNext] = curHex.first;
// 		}
// 	}
// };
//

BattleHex BattleInfo::getClosestTile(bool attackerOwned, BattleHex initialPos, std::set<BattleHex> & possibilities) const
{
	std::vector<BattleHex> sortedTiles (possibilities.begin(), possibilities.end()); //set can't be sorted properly :(

	BattleHex initialHex = BattleHex(initialPos);
	auto compareDistance = [initialHex](const BattleHex left, const BattleHex right) -> bool
	{
		return initialHex.getDistance (initialHex, left) < initialHex.getDistance (initialHex, right);
	};

	boost::sort (sortedTiles, compareDistance); //closest tiles at front

	int closestDistance = initialHex.getDistance(initialPos, sortedTiles.front()); //sometimes closest tiles can be many hexes away

	auto notClosest = [closestDistance, initialPos](const BattleHex here) -> bool
	{
		return closestDistance < here.getDistance (initialPos, here);
	};

	vstd::erase_if(sortedTiles, notClosest); //only closest tiles are interesting

	auto compareHorizontal = [attackerOwned, initialPos](const BattleHex left, const BattleHex right) -> bool
	{
		if(left.getX() != right.getX())
		{
			if (attackerOwned)
				return left.getX() > right.getX(); //find furthest right
			else
				return left.getX() < right.getX(); //find furthest left
		}
		else
		{
			//Prefer tiles in the same row.
			return std::abs(left.getY() - initialPos.getY()) < std::abs(right.getY() - initialPos.getY());
		}
	};

	boost::sort (sortedTiles, compareHorizontal);

	return sortedTiles.front();
}

int BattleInfo::getAvaliableHex(TCreature creID, bool attackerOwned, int initialPos) const
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

	return getClosestTile(attackerOwned, pos, occupyable);
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

std::set<const CStack*> BattleInfo::getAttackedCreatures(const CSpell * s, int skillLevel, ui8 attackerOwner, BattleHex destinationTile)
{
	std::set<const CStack*> attackedCres; /*std::set to exclude multiple occurrences of two hex creatures*/

	const ui8 attackerSide = sides[1] == attackerOwner;
	const auto attackedHexes = s->rangeInHexes(destinationTile, skillLevel, attackerSide);
	const bool onlyAlive = s->id != Spells::RESURRECTION && s->id != Spells::ANIMATE_DEAD; //when casting resurrection or animate dead we should be allow to select dead stack
	//fixme: what about other rising spells (Sacrifice) ?
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
	else if (s->id == Spells::CHAIN_LIGHTNING)
	{
		std::set<BattleHex> possibleHexes;
		BOOST_FOREACH (auto stack, stacks)
		{
			if (stack->isValidTarget())
			{
				BOOST_FOREACH (auto hex, stack->getHexes())
				{
					possibleHexes.insert (hex);
				}
			}
		}
		BattleHex lightningHex =  destinationTile;
		for (int i = 0; i < 5; ++i) //TODO: depends on spell school level
		{
			auto stack = battleGetStackByPos (lightningHex, true);
			if (!stack)
				break;
			attackedCres.insert (stack);
			BOOST_FOREACH (auto hex, stack->getHexes())
			{
				possibleHexes.erase (hex); //can't hit same place twice
			}
			lightningHex = getClosestTile (attackerOwner, destinationTile, possibleHexes);
		}
	}
	else if (s->range[skillLevel].size() > 1) //custom many-hex range
	{
		BOOST_FOREACH(BattleHex hex, attackedHexes)
		{
			if(const CStack * st = battleGetStackByPos(hex, onlyAlive))
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
			const CStack * st = battleGetStackByPos(destinationTile, onlyAlive);
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
		if(const CStack * st = battleGetStackByPos(destinationTile, onlyAlive))
			attackedCres.insert(st);
	}
	else //custom range from attackedHexes
	{
		BOOST_FOREACH(BattleHex hex, attackedHexes)
		{
			if(const CStack * st = battleGetStackByPos(hex, onlyAlive))
				attackedCres.insert(st);
		}
	}
	return attackedCres;
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

// std::pair<const CStack *, BattleHex> BattleInfo::getNearestStack(const CStack * closest, boost::logic::tribool attackerOwned) const
// {
// 	bool ac[GameConstants::BFIELD_SIZE];
// 	std::set<BattleHex> occupyable;
//
// 	getAccessibilityMap(ac, closest->doubleWide(), closest->attackerOwned, false, occupyable, closest->hasBonusOfType(Bonus::FLYING), closest);
//
// 	BattleHex predecessor[GameConstants::BFIELD_SIZE];
// 	int dist[GameConstants::BFIELD_SIZE];
// 	makeBFS(closest->position, ac, predecessor, dist, closest->doubleWide(), closest->attackerOwned, closest->hasBonusOfType(Bonus::FLYING), true);
//
// 	std::vector< std::pair< std::pair<int, int>, const CStack *> > stackPairs; //pairs <<distance, hex>, stack>
// 	for(int g=0; g<GameConstants::BFIELD_SIZE; ++g)
// 	{
// 		const CStack * atG = getStackT(g);
// 		if(!atG || atG->ID == closest->ID) //if there is not stack or we are the closest one
// 			continue;
// 		if(boost::logic::indeterminate(attackerOwned) || atG->attackerOwned == attackerOwned)
// 		{
// 			if(predecessor[g] == -1) //TODO: is it really the best solution?
// 				continue;
// 			stackPairs.push_back( std::make_pair( std::make_pair(dist[predecessor[g]], g), atG) );
// 		}
// 	}
//
// 	if(stackPairs.size() > 0)
// 	{
// 		std::vector< std::pair< std::pair<int, int>, const CStack *> > minimalPairs;
// 		minimalPairs.push_back(stackPairs[0]);
//
// 		for(int b=1; b<stackPairs.size(); ++b)
// 		{
// 			if(stackPairs[b].first.first < minimalPairs[0].first.first)
// 			{
// 				minimalPairs.clear();
// 				minimalPairs.push_back(stackPairs[b]);
// 			}
// 			else if(stackPairs[b].first.first == minimalPairs[0].first.first)
// 			{
// 				minimalPairs.push_back(stackPairs[b]);
// 			}
// 		}
//
// 		std::pair< std::pair<int, int>, const CStack *> minPair = minimalPairs[minimalPairs.size()/2];
//
// 		return std::make_pair(minPair.second, predecessor[minPair.first.second]);
// 	}
//
// 	return std::make_pair<const CStack * , BattleHex>(NULL, BattleHex::INVALID);
// }
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

ui32 BattleInfo::calculateHealedHP(const CGHeroInstance * caster, const CSpell * spell, const CStack * stack, const CStack * sacrificedStack) const
{
	bool resurrect = resurrects(spell->id);
	int healedHealth;
	if (spell->id == Spells::SACRIFICE && sacrificedStack)
		healedHealth = (caster->getPrimSkillLevel(2) + sacrificedStack->MaxHealth() + spell->powers[caster->getSpellSchoolLevel(spell)]) * sacrificedStack->count;
	else
		healedHealth = caster->getPrimSkillLevel(2) * spell->power + spell->powers[caster->getSpellSchoolLevel(spell)];
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

BattleInfo * BattleInfo::setupBattle( int3 tile, int terrain, int battlefieldType, const CArmedInstance *armies[2], const CGHeroInstance * heroes[2], bool creatureBank, const CGTownInstance *town )
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
		curB->terrainType = VLC->heroh->nativeTerrains[town->town->typeID];
	}
	else
	{
		curB->town = NULL;
		curB->siege = 0;
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
	std::vector< std::vector<int> > attackerLoose, defenderLoose, attackerTight, defenderTight, attackerCreBank, defenderCreBank;
	std::vector <int> commanderField, commanderBank;
	const JsonNode config(ResourceID("config/battleStartpos.json"));
	const JsonVector &positions = config["battle_positions"].Vector();

	CGH::readBattlePositions(positions[0]["levels"], attackerLoose);
	CGH::readBattlePositions(positions[1]["levels"], defenderLoose);
	CGH::readBattlePositions(positions[2]["levels"], attackerTight);
	CGH::readBattlePositions(positions[3]["levels"], defenderTight);
	CGH::readBattlePositions(positions[4]["levels"], attackerCreBank);
	CGH::readBattlePositions(positions[5]["levels"], defenderCreBank);

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
		auto handleWarMachine= [&](int side, int artslot, int cretype, int hex)
		{
			if(heroes[side] && heroes[side]->getArt(artslot))
				stacks.push_back(curB->generateNewStack(CStackBasicDescriptor(cretype, 1), true, 255, hex));
		};

		handleWarMachine(0, 13, 146, 52); //ballista
		handleWarMachine(0, 14, 148, 18); //ammo cart
		handleWarMachine(0, 15, 147, 154);//first aid tent
		if(town && town->hasFort())
			handleWarMachine(0, 16, 145, 120);//catapult

		if(!town) //defending hero shouldn't receive ballista (bug #551)
			handleWarMachine(1, 13, 146, 66); //ballista
		handleWarMachine(1, 14, 148, 32); //ammo cart
		handleWarMachine(1, 15, 147, 168); //first aid tent
	}
	//war machines added

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
	
// 	//shifting positions of two-hex creatures
// 	for(unsigned g=0; g<stacks.size(); ++g)
// 	{
// 		//we should do that for creature bank too
// 		if(stacks[g]->doubleWide() && stacks[g]->attackerOwned)
// 		{
// 			stacks[g]->position += BattleHex::RIGHT;
// 		}
// 		else if(stacks[g]->doubleWide() && !stacks[g]->attackerOwned)
// 		{
// 			if (stacks[g]->position.getX() > 1)
// 				stacks[g]->position += BattleHex::LEFT;
// 		}
// 	}


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

const CGHeroInstance * BattleInfo::getHero( int player ) const
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
		for(auto it = affectedCreatures.begin(); it != affectedCreatures.end(); ++it)
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

shared_ptr<CObstacleInstance> BattleInfo::getObstacleOnTile(BattleHex tile) const
{
	BOOST_FOREACH(auto &obs, obstacles)
		if(vstd::contains(obs->getAffectedTiles(), tile))
			return obs;

	return shared_ptr<CObstacleInstance>();
}

int BattleInfo::battlefieldTypeToBI(int bfieldType)
{
	static const std::map<int, int> theMap = boost::assign::map_list_of(19, BattlefieldBI::CLOVER_FIELD)
		(22, BattlefieldBI::CURSED_GROUND)(20, BattlefieldBI::EVIL_FOG)(21, BattlefieldBI::NONE)
		(14, BattlefieldBI::FIERY_FIELDS)(18, BattlefieldBI::HOLY_GROUND)(17, BattlefieldBI::LUCID_POOLS)
		(16, BattlefieldBI::MAGIC_CLOUDS)(9, BattlefieldBI::MAGIC_PLAINS)(15, BattlefieldBI::ROCKLANDS)
		(1, BattlefieldBI::COASTAL);

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
	if (useBind && getEffect (Spells::BIND))
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


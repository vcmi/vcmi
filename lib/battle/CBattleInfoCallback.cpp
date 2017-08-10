/*
 * CBattleInfoCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleInfoCallback.h"
#include "../CStack.h"
#include "BattleInfo.h"
#include "../NetPacks.h"
#include "../spells/CSpellHandler.h"
#include "../mapObjects/CGTownInstance.h"

namespace SiegeStuffThatShouldBeMovedToHandlers // <=== TODO
{
static void retreiveTurretDamageRange(const CGTownInstance * town, const CStack * turret, double & outMinDmg, double & outMaxDmg)
{
	assert(turret->getCreature()->idNumber == CreatureID::ARROW_TOWERS);
	assert(town);
	assert(turret->position >= -4 && turret->position <= -2);

	float multiplier = (turret->position == -2) ? 1 : 0.5;

	int baseMin = 6;
	int baseMax = 10;

	outMinDmg = multiplier * (baseMin + town->getTownLevel() * 2);
	outMaxDmg = multiplier * (baseMax + town->getTownLevel() * 3);
}

static BattleHex lineToWallHex(int line) //returns hex with wall in given line (y coordinate)
{
	static const BattleHex lineToHex[] = {12, 29, 45, 62, 78, 95, 112, 130, 147, 165, 182};

	return lineToHex[line];
}

static bool sameSideOfWall(BattleHex pos1, BattleHex pos2)
{
	const int wallInStackLine = lineToWallHex(pos1.getY());
	const int wallInDestLine = lineToWallHex(pos2.getY());

	const bool stackLeft = pos1 < wallInStackLine;
	const bool destLeft = pos2 < wallInDestLine;

	return stackLeft == destLeft;
}

// parts of wall
static const std::pair<int, EWallPart::EWallPart> wallParts[] =
{
	std::make_pair(50, EWallPart::KEEP),
	std::make_pair(183, EWallPart::BOTTOM_TOWER),
	std::make_pair(182, EWallPart::BOTTOM_WALL),
	std::make_pair(130, EWallPart::BELOW_GATE),
	std::make_pair(78, EWallPart::OVER_GATE),
	std::make_pair(29, EWallPart::UPPER_WALL),
	std::make_pair(12, EWallPart::UPPER_TOWER),
	std::make_pair(95, EWallPart::INDESTRUCTIBLE_PART_OF_GATE),
	std::make_pair(96, EWallPart::GATE),
	std::make_pair(45, EWallPart::INDESTRUCTIBLE_PART),
	std::make_pair(62, EWallPart::INDESTRUCTIBLE_PART),
	std::make_pair(112, EWallPart::INDESTRUCTIBLE_PART),
	std::make_pair(147, EWallPart::INDESTRUCTIBLE_PART),
	std::make_pair(165, EWallPart::INDESTRUCTIBLE_PART)
};

static EWallPart::EWallPart hexToWallPart(BattleHex hex)
{
	for(auto & elem : wallParts)
	{
		if(elem.first == hex)
			return elem.second;
	}

	return EWallPart::INVALID; //not found!
}

static BattleHex WallPartToHex(EWallPart::EWallPart part)
{
	for(auto & elem : wallParts)
	{
		if(elem.second == part)
			return elem.first;
	}

	return BattleHex::INVALID; //not found!
}
}

using namespace SiegeStuffThatShouldBeMovedToHandlers;

ESpellCastProblem::ESpellCastProblem CBattleInfoCallback::battleCanCastSpell(const ISpellCaster * caster, ECastingMode::ECastingMode mode) const
{
	RETURN_IF_NOT_BATTLE(ESpellCastProblem::INVALID);
	if(caster == nullptr)
	{
		logGlobal->error("CBattleInfoCallback::battleCanCastSpell: no spellcaster.");
		return ESpellCastProblem::INVALID;
	}
	const PlayerColor player = caster->getOwner();
	const auto side = playerToSide(player);
	if(!side)
		return ESpellCastProblem::INVALID;
	if(!battleDoWeKnowAbout(side.get()))
	{
		logGlobal->warn("You can't check if enemy can cast given spell!");
		return ESpellCastProblem::INVALID;
	}

	if(battleTacticDist())
		return ESpellCastProblem::ONGOING_TACTIC_PHASE;

	switch (mode)
	{
	case ECastingMode::HERO_CASTING:
	{
		if(battleCastSpells(side.get()) > 0)
			return ESpellCastProblem::ALREADY_CASTED_THIS_TURN;

		auto hero = dynamic_cast<const CGHeroInstance *>(caster);

		if(!hero)
			return ESpellCastProblem::NO_HERO_TO_CAST_SPELL;
		if(hero->hasBonusOfType(Bonus::BLOCK_ALL_MAGIC))
			return ESpellCastProblem::MAGIC_IS_BLOCKED;
	}
		break;
	default:
		break;
	}

	return ESpellCastProblem::OK;
}

si8 CBattleInfoCallback::battleHasWallPenalty(const CStack * stack, BattleHex destHex) const
{
	return battleHasWallPenalty(stack, stack->position, destHex);
}

si8 CBattleInfoCallback::battleHasWallPenalty(const IBonusBearer * bonusBearer, BattleHex shooterPosition, BattleHex destHex) const
{
	RETURN_IF_NOT_BATTLE(false);
	if (!battleGetSiegeLevel() || bonusBearer->hasBonusOfType(Bonus::NO_WALL_PENALTY))
		return false;

	const int wallInStackLine = lineToWallHex(shooterPosition.getY());
	const int wallInDestLine = lineToWallHex(destHex.getY());

	const bool stackLeft = shooterPosition < wallInStackLine;
	const bool destRight = destHex > wallInDestLine;

	if (stackLeft && destRight) //shooting from outside to inside
	{
		int row = (shooterPosition + destHex) / (2 * GameConstants::BFIELD_WIDTH);
		if (shooterPosition > destHex && ((destHex % GameConstants::BFIELD_WIDTH - shooterPosition % GameConstants::BFIELD_WIDTH) < 2)) //shooting up high
			row -= 2;
		const int wallPos = lineToWallHex(row);
		if (!isWallPartPotentiallyAttackable(battleHexToWallPart(wallPos))) return true;
	}

	return false;
}

si8 CBattleInfoCallback::battleCanTeleportTo(const CStack * stack, BattleHex destHex, int telportLevel) const
{
	RETURN_IF_NOT_BATTLE(false);
	if (!getAccesibility(stack).accessible(destHex, stack))
		return false;

	const ui8 siegeLevel = battleGetSiegeLevel();

	//check for wall
	//advanced teleport can pass wall of fort|citadel, expert - of castle
	if ((siegeLevel > CGTownInstance::NONE && telportLevel < 2) || (siegeLevel >= CGTownInstance::CASTLE && telportLevel < 3))
		return sameSideOfWall(stack->position, destHex);

	return true;
}

std::set<BattleHex> CBattleInfoCallback::battleGetAttackedHexes(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos) const
{
	std::set<BattleHex> attackedHexes;
	RETURN_IF_NOT_BATTLE(attackedHexes);

	AttackableTiles at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);

	for (BattleHex tile : at.hostileCreaturePositions)
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
		{
			attackedHexes.insert(tile);
		}
	}
	for (BattleHex tile : at.friendlyCreaturePositions)
	{
		if(battleGetStackByPos(tile, true)) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedHexes.insert(tile);
		}
	}
	return attackedHexes;
}

SpellID CBattleInfoCallback::battleGetRandomStackSpell(CRandomGenerator & rand, const CStack * stack, ERandomSpell mode) const
{
	switch (mode)
	{
	case RANDOM_GENIE:
		return getRandomBeneficialSpell(rand, stack); //target
		break;
	case RANDOM_AIMED:
		return getRandomCastedSpell(rand, stack); //caster
		break;
	default:
		logGlobal->errorStream() << "Incorrect mode of battleGetRandomSpell (" << mode <<")";
		return SpellID::NONE;
	}
}

const CStack* CBattleInfoCallback::battleGetStackByPos(BattleHex pos, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	for(auto s : battleGetAllStacks(true))
		if(vstd::contains(s->getHexes(), pos) && (!onlyAlive || s->alive()))
			return s;

	return nullptr;
}

void CBattleInfoCallback::battleGetStackQueue(std::vector<const CStack *> &out, const int howMany, const int turn, int lastMoved) const
{
	RETURN_IF_NOT_BATTLE();

	//let's define a huge lambda
	auto takeStack = [&](std::vector<const CStack *> &st) -> const CStack*
	{
		const CStack * ret = nullptr;
		unsigned i, //fastest stack
				j=0; //fastest stack of the other side
		for(i = 0; i < st.size(); i++)
			if(st[i])
				break;

		//no stacks left
		if(i == st.size())
			return nullptr;

		const CStack * fastest = st[i], *other = nullptr;
		int bestSpeed = fastest->Speed(turn);

		//FIXME: comparison between bool and integer. Logic does not makes sense either
		if(fastest->side != lastMoved)
		{
			ret = fastest;
		}
		else
		{
			for(j = i + 1; j < st.size(); j++)
			{
				if(!st[j]) continue;
				if(st[j]->side != lastMoved || st[j]->Speed(turn) != bestSpeed)
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
			st[i] = nullptr;
		else
			st[j] = nullptr;

		lastMoved = ret->side;
		return ret;
	};

	//We'll split creatures with remaining movement to 4 buckets
	// [0] - turrets/catapult,
	// [1] - normal (unmoved) creatures, other war machines,
	// [2] - waited cres that had morale,
	// [3] - rest of waited cres
	std::vector<const CStack *> phase[4];
	int toMove = 0; //how many stacks still has move
	const CStack * active = battleActiveStack();

	//active stack hasn't taken any action yet - must be placed at the beginning of queue, no matter what
	if(!turn && active && active->willMove() && !active->waited())
	{
		out.push_back(active);
		if(out.size() == howMany)
			return;
	}

	auto allStacks = battleGetAllStacks(true);
	if(!vstd::contains_if(allStacks, [](const CStack *stack) { return stack->willMove(100000); })) //little evil, but 100000 should be enough for all effects to disappear
	{
		//No stack will be able to move, battle is over.
		out.clear();
		return;
	}

	for(auto s : battleGetAllStacks(true))
	{
		if((turn <= 0 && !s->willMove()) //we are considering current round and stack won't move
		|| (turn > 0 && !s->canMove(turn)) //stack won't be able to move in later rounds
		|| (turn <= 0 && s == active && out.size() && s == out.front())) //it's active stack already added at the beginning of queue
		{
			continue;
		}

		int p = -1; //in which phase this tack will move?
		if(turn <= 0 && s->waited()) //consider waiting state only for ongoing round
		{
			if(vstd::contains(s->state, EBattleStackState::HAD_MORALE))
				p = 2;
			else
				p = 3;
		}
		else if(s->getCreature()->idNumber == CreatureID::CATAPULT || s->getCreature()->idNumber == CreatureID::ARROW_TOWERS) //catapult and turrets are first
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
		boost::sort(phase[i], CMP_stack(i, turn > 0 ? turn : 0));

	for(size_t i = 0; i < phase[0].size() && i < howMany; i++)
		out.push_back(phase[0][i]);

	if(out.size() == howMany)
		return;

	if(lastMoved == -1)
	{
		if(active)
		{
			//FIXME: both branches contain same code!!!
			if(out.size() && out.front() == active)
				lastMoved = active->side;
			else
				lastMoved = active->side;
		}
		else
		{
			lastMoved = 0;
		}
	}

	int pi = 1;
	while(out.size() < howMany)
	{
		const CStack * hlp = takeStack(phase[pi]);
		if(!hlp)
		{
			pi++;
			if(pi > 3)
			{
				//if(turn != 2)
				battleGetStackQueue(out, howMany, turn + 1, lastMoved);
				return;
			}
		}
		else
		{
			out.push_back(hlp);
		}
	}
}

void CBattleInfoCallback::battleGetStackCountOutsideHexes(bool *ac) const
{
	RETURN_IF_NOT_BATTLE();
	auto accessibility = getAccesibility();

	for(int i = 0; i < accessibility.size(); i++)
		ac[i] = (accessibility[i] == EAccessibility::ACCESSIBLE);
}

std::vector<BattleHex> CBattleInfoCallback::battleGetAvailableHexes(const CStack * stack, bool addOccupiable, std::vector<BattleHex> * attackable) const
{
	std::vector<BattleHex> ret;

	RETURN_IF_NOT_BATTLE(ret);
	if(!stack->position.isValid()) //turrets
		return ret;

	auto reachability = getReachability(stack);

	for (int i = 0; i < GameConstants::BFIELD_SIZE; ++i)
	{
		// If obstacles or other stacks makes movement impossible, it can't be helped.
		if(!reachability.isReachable(i))
			continue;

		if(battleTacticDist() && battleGetTacticsSide() == stack->side)
		{
			//Stack has to perform tactic-phase movement -> can enter any reachable tile within given range
			if(!isInTacticRange(i))
				continue;
		}
		else
		{
			//Not tactics phase -> destination must be reachable and within stack range.
			if(reachability.distances[i] > stack->Speed(0, true))
				continue;
		}

		ret.push_back(i);

		if(addOccupiable && stack->doubleWide())
		{
			//If two-hex stack can stand on hex i then obviously it can occupy its second hex from that position
			ret.push_back(stack->occupiedHex(i));
		}
	}


	if(attackable)
	{
		auto meleeAttackable = [&](BattleHex hex) -> bool
		{
			// Return true if given hex has at least one available neighbour.
			// Available hexes are already present in ret vector.
			auto availableNeighbor = boost::find_if(ret, [=] (BattleHex availableHex)
			{
				return BattleHex::mutualPosition(hex, availableHex) >= 0;
			});
			return availableNeighbor != ret.end();
		};
		for(const CStack * otherSt : battleAliveStacks(1-stack->side))
		{
			if(!otherSt->isValidTarget(false))
				continue;

			std::vector<BattleHex> occupied = otherSt->getHexes();

			if(battleCanShoot(stack, otherSt->position))
			{
				attackable->insert(attackable->end(), occupied.begin(), occupied.end());
				continue;
			}

			for(BattleHex he : occupied)
			{
				if(meleeAttackable(he))
					attackable->push_back(he);
			}
		}
	}

	//adding occupiable likely adds duplicates to ret -> clean it up
	boost::sort(ret);
	ret.erase(boost::unique(ret).end(), ret.end());
	return ret;
}

bool CBattleInfoCallback::battleCanAttack(const CStack * stack, const CStack * target, BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(battleTacticDist())
		return false;

	if (!stack || !target)
		return false;

	if(!battleMatchOwner(stack, target))
		return false;

	auto &id = stack->getCreature()->idNumber;
	if (id == CreatureID::FIRST_AID_TENT || id == CreatureID::CATAPULT)
		return false;

	if (!target->alive())
		return false;

	return true;
}

bool CBattleInfoCallback::battleCanShoot(const CStack * stack, BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(battleTacticDist()) //no shooting during tactics
		return false;

	const CStack * dst = battleGetStackByPos(dest);

	if(!stack || !dst)
		return false;

	//forgetfulness
	TBonusListPtr forgetfulList = stack->getBonuses(Selector::type(Bonus::FORGETFULL),"");
	if(!forgetfulList->empty())
	{
		int forgetful = forgetfulList->valOfBonuses(Selector::type(Bonus::FORGETFULL));

		//advanced+ level
		if(forgetful > 1)
			return false;
	}

	if(stack->getCreature()->idNumber == CreatureID::CATAPULT && dst) //catapult cannot attack creatures
		return false;

	if(stack->canShoot()
		&& battleMatchOwner(stack, dst)
		&& dst->alive()
		&& (!battleIsStackBlocked(stack) || stack->hasBonusOfType(Bonus::FREE_SHOOTING)))
		return true;
	return false;
}

TDmgRange CBattleInfoCallback::calculateDmgRange(const BattleAttackInfo & info) const
{
	auto battleBonusValue = [&](const IBonusBearer * bearer, CSelector selector) -> int
	{
		auto noLimit = Selector::effectRange(Bonus::NO_LIMIT);
		auto limitMatches = info.shooting
							? Selector::effectRange(Bonus::ONLY_DISTANCE_FIGHT)
							: Selector::effectRange(Bonus::ONLY_MELEE_FIGHT);

		//any regular bonuses or just ones for melee/ranged
		return bearer->getBonuses(selector, noLimit.Or(limitMatches))->totalValue();
	};

	double additiveBonus = 1.0, multBonus = 1.0,
			minDmg = info.attackerBonuses->getMinDamage() * info.attackerHealth.getCount(),//TODO: ONLY_MELEE_FIGHT / ONLY_DISTANCE_FIGHT
			maxDmg = info.attackerBonuses->getMaxDamage() * info.attackerHealth.getCount();

	const CCreature *attackerType = info.attacker->getCreature(),
			*defenderType = info.defender->getCreature();

	if(attackerType->idNumber == CreatureID::ARROW_TOWERS)
	{
		SiegeStuffThatShouldBeMovedToHandlers::retreiveTurretDamageRange(battleGetDefendedTown(), info.attacker, minDmg, maxDmg);
	}

	if(info.attackerBonuses->hasBonusOfType(Bonus::SIEGE_WEAPON) && attackerType->idNumber != CreatureID::ARROW_TOWERS) //any siege weapon, but only ballista can attack (second condition - not arrow turret)
	{ //minDmg and maxDmg are multiplied by hero attack + 1
		auto retreiveHeroPrimSkill = [&](int skill) -> int
		{
			const std::shared_ptr<Bonus> b = info.attackerBonuses->getBonus(Selector::sourceTypeSel(Bonus::HERO_BASE_SKILL).And(Selector::typeSubtype(Bonus::PRIMARY_SKILL, skill)));
			return b ? b->val : 0; //if there is no hero or no info on his primary skill, return 0
		};


		minDmg *= retreiveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
		maxDmg *= retreiveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
	}

	int attackDefenceDifference = 0;

	double multAttackReduction = (100 - battleBonusValue (info.attackerBonuses, Selector::type(Bonus::GENERAL_ATTACK_REDUCTION))) / 100.0;
	attackDefenceDifference += battleBonusValue (info.attackerBonuses, Selector::typeSubtype(Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK)) * multAttackReduction;

	double multDefenceReduction = (100 - battleBonusValue (info.attackerBonuses, Selector::type(Bonus::ENEMY_DEFENCE_REDUCTION))) / 100.0;
	attackDefenceDifference -= info.defenderBonuses->Defense() * multDefenceReduction;

	if(const std::shared_ptr<Bonus> slayerEffect = info.attackerBonuses->getBonus(Selector::type(Bonus::SLAYER))) //slayer handling //TODO: apply only ONLY_MELEE_FIGHT / DISTANCE_FIGHT?
	{
		std::vector<int> affectedIds;
		int spLevel = slayerEffect->val;

		//FIXME: do not check all creatures
		for(int g = 0; g < VLC->creh->creatures.size(); ++g)
		{
			for(const std::shared_ptr<Bonus> b : VLC->creh->creatures[g]->getBonusList())
			{
				if ((b->type == Bonus::KING3 && spLevel >= 3) || //expert
					 (b->type == Bonus::KING2 && spLevel >= 2) || //adv +
					 (b->type == Bonus::KING1 && spLevel >= 0)) //none or basic +
				{
					affectedIds.push_back(g);
					break;
				}
			}
		}

		for(auto & affectedId : affectedIds)
		{
			if(defenderType->idNumber == affectedId)
			{
				attackDefenceDifference += SpellID(SpellID::SLAYER).toSpell()->getPower(spLevel);
				break;
			}
		}
	}

	//bonus from attack/defense skills
	if(attackDefenceDifference < 0) //decreasing dmg
	{
		const double dec = std::min(0.025 * (-attackDefenceDifference), 0.7);
		multBonus *= 1.0 - dec;
	}
	else //increasing dmg
	{
		const double inc = std::min(0.05 * attackDefenceDifference, 4.0);
		additiveBonus += inc;
	}

	//applying jousting bonus
	if(info.attackerBonuses->hasBonusOfType(Bonus::JOUSTING) && !info.defenderBonuses->hasBonusOfType(Bonus::CHARGE_IMMUNITY))
		additiveBonus += info.chargedFields * 0.05;

	//handling secondary abilities and artifacts giving premies to them
	if(info.shooting)
		additiveBonus += info.attackerBonuses->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::ARCHERY) / 100.0;
	else
		additiveBonus += info.attackerBonuses->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::OFFENCE) / 100.0;

	if(info.defenderBonuses)
		multBonus *= (std::max(0, 100 - info.defenderBonuses->valOfBonuses(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::ARMORER))) / 100.0;

	//handling hate effect
	additiveBonus += info.attackerBonuses->valOfBonuses(Bonus::HATE, defenderType->idNumber.toEnum()) / 100.;

	//luck bonus
	if (info.luckyHit)
	{
		additiveBonus += 1.0;
	}
	//unlucky hit, used only if negative luck is enabled
	if (info.unluckyHit)
	{
		additiveBonus -= 0.5; // FIXME: how bad (and luck in general) should work with following bonuses?
	}

	//ballista double dmg
	if(info.ballistaDoubleDamage)
	{
		additiveBonus += 1.0;
	}

	if (info.deathBlow) //Dread Knight and many WoGified creatures
	{
		additiveBonus += 1.0;
	}

	//handling spell effects
	if(!info.shooting) //eg. shield
	{
		multBonus *= (100 - info.defenderBonuses->valOfBonuses(Bonus::GENERAL_DAMAGE_REDUCTION, 0)) / 100.0;
	}
	else if(info.shooting) //eg. air shield
	{
		multBonus *= (100 - info.defenderBonuses->valOfBonuses(Bonus::GENERAL_DAMAGE_REDUCTION, 1)) / 100.0;
	}

	if(info.shooting)
	{
		//todo: set actual percentage in spell bonus configuration instead of just level; requires non trivial backward compatibility handling

		//get list first, total value of 0 also counts
		TBonusListPtr forgetfulList = info.attackerBonuses->getBonuses(Selector::type(Bonus::FORGETFULL),"");

		if(!forgetfulList->empty())
		{
			int forgetful = forgetfulList->valOfBonuses(Selector::type(Bonus::FORGETFULL));

			//none of basic level
			if(forgetful == 0 || forgetful == 1)
				multBonus *= 0.5;
			else
				logGlobal->warn("Attempt to calculate shooting damage with adv+ FORGETFULL effect");
		}
	}

	TBonusListPtr curseEffects = info.attackerBonuses->getBonuses(Selector::type(Bonus::ALWAYS_MINIMUM_DAMAGE));
	TBonusListPtr blessEffects = info.attackerBonuses->getBonuses(Selector::type(Bonus::ALWAYS_MAXIMUM_DAMAGE));
	int curseBlessAdditiveModifier = blessEffects->totalValue() - curseEffects->totalValue();
	double curseMultiplicativePenalty = curseEffects->size() ? (*std::max_element(curseEffects->begin(), curseEffects->end(), &Bonus::compareByAdditionalInfo<std::shared_ptr<Bonus>>))->additionalInfo : 0;

	if(curseMultiplicativePenalty) //curse handling (partial, the rest is below)
	{
		multBonus *= 1.0 - curseMultiplicativePenalty/100;
	}

	auto isAdvancedAirShield = [](const Bonus* bonus)
	{
		return bonus->source == Bonus::SPELL_EFFECT
				&& bonus->sid == SpellID::AIR_SHIELD
				&& bonus->val >= SecSkillLevel::ADVANCED;
	};

	//wall / distance penalty + advanced air shield
	const bool distPenalty = !info.attackerBonuses->hasBonusOfType(Bonus::NO_DISTANCE_PENALTY) && battleHasDistancePenalty(info.attackerBonuses, info.attackerPosition, info.defenderPosition);
	const bool obstaclePenalty = battleHasWallPenalty(info.attackerBonuses, info.attackerPosition, info.defenderPosition);

	if(info.shooting)
	{
		if (distPenalty || info.defenderBonuses->hasBonus(isAdvancedAirShield))
		{
			multBonus *= 0.5;
		}
		if (obstaclePenalty)
		{
			multBonus *= 0.5; //cumulative
		}
	}
	if(!info.shooting && info.attackerBonuses->hasBonusOfType(Bonus::SHOOTER) && !info.attackerBonuses->hasBonusOfType(Bonus::NO_MELEE_PENALTY))
	{
		multBonus *= 0.5;
	}

	// psychic elementals versus mind immune units 50%
	if(attackerType->idNumber == CreatureID::PSYCHIC_ELEMENTAL
	&& info.defenderBonuses->hasBonusOfType(Bonus::MIND_IMMUNITY))
	{
		multBonus *= 0.5;
	}

	// TODO attack on petrified unit 50%
	// blinded unit retaliates

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
		returnedVal = std::make_pair(int(maxDmg), int(maxDmg));
	}
	else
	{
		returnedVal = std::make_pair(int(minDmg), int(maxDmg));
	}

	//damage cannot be less than 1
	vstd::amax(returnedVal.first, 1);
	vstd::amax(returnedVal.second, 1);

	return returnedVal;
}

TDmgRange CBattleInfoCallback::battleEstimateDamage(CRandomGenerator & rand, const CStack * attacker, const CStack * defender, TDmgRange * retaliationDmg) const
{
	RETURN_IF_NOT_BATTLE(std::make_pair(0, 0));
	const bool shooting = battleCanShoot(attacker, defender->position);
	const BattleAttackInfo bai(attacker, defender, shooting);
	return battleEstimateDamage(rand, bai, retaliationDmg);
}

std::pair<ui32, ui32> CBattleInfoCallback::battleEstimateDamage(CRandomGenerator & rand, const BattleAttackInfo & bai, std::pair<ui32, ui32> * retaliationDmg) const
{
	RETURN_IF_NOT_BATTLE(std::make_pair(0, 0));

	//const bool shooting = battleCanShoot(bai.attacker, bai.defenderPosition); //TODO handle bonus bearer

	TDmgRange ret = calculateDmgRange(bai);

	if(retaliationDmg)
	{
		if(bai.shooting)
		{
			retaliationDmg->first = retaliationDmg->second = 0;
		}
		else
		{
			ui32 TDmgRange::* pairElems[] = {&TDmgRange::first, &TDmgRange::second};
			for (int i=0; i<2; ++i)
			{
				BattleStackAttacked bsa;
				bsa.damageAmount = ret.*pairElems[i];
				bai.defender->prepareAttacked(bsa, rand, bai.defenderHealth);

				auto retaliationAttack = bai.reverse();
				retaliationAttack.attackerHealth = retaliationAttack.attacker->healthAfterAttacked(bsa.damageAmount);
				retaliationDmg->*pairElems[!i] = calculateDmgRange(retaliationAttack).*pairElems[!i];
			}
		}
	}

	return ret;
}

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoCallback::battleGetAllObstaclesOnPos(BattleHex tile, bool onlyBlocking) const
{
	std::vector<std::shared_ptr<const CObstacleInstance>> obstacles = std::vector<std::shared_ptr<const CObstacleInstance>>();
	RETURN_IF_NOT_BATTLE(obstacles);
	for(auto & obs : battleGetAllObstacles())
	{
		if(vstd::contains(obs->getBlockedTiles(), tile)
				|| (!onlyBlocking && vstd::contains(obs->getAffectedTiles(), tile)))
		{
			obstacles.push_back(obs);
		}
	}
	return obstacles;
}

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoCallback::getAllAffectedObstaclesByStack(const CStack * stack) const
{
	std::vector<std::shared_ptr<const CObstacleInstance>> affectedObstacles = std::vector<std::shared_ptr<const CObstacleInstance>>();
	RETURN_IF_NOT_BATTLE(affectedObstacles);
	if(stack->alive())
	{
		affectedObstacles = battleGetAllObstaclesOnPos(stack->position, false);
		if(stack->doubleWide())
		{
			BattleHex otherHex = stack->occupiedHex(stack->position);
			if(otherHex.isValid())
				for(auto & i : battleGetAllObstaclesOnPos(otherHex, false))
					affectedObstacles.push_back(i);
		}
		for(auto hex : stack->getHexes())
			if(hex == ESiegeHex::GATE_BRIDGE)
				if(battleGetGateState() == EGateState::OPENED || battleGetGateState() == EGateState::DESTROYED)
					for(int i=0; i<affectedObstacles.size(); i++)
						if(affectedObstacles.at(i)->obstacleType == CObstacleInstance::MOAT)
							affectedObstacles.erase(affectedObstacles.begin()+i);
	}
	return affectedObstacles;
}

AccessibilityInfo CBattleInfoCallback::getAccesibility() const
{
	AccessibilityInfo ret;
	ret.fill(EAccessibility::ACCESSIBLE);

	//removing accessibility for side columns of hexes
	for(int y = 0; y < GameConstants::BFIELD_HEIGHT; y++)
	{
		ret[BattleHex(GameConstants::BFIELD_WIDTH - 1, y)] = EAccessibility::SIDE_COLUMN;
		ret[BattleHex(0, y)] = EAccessibility::SIDE_COLUMN;
	}

	//special battlefields with logically unavailable tiles
	std::vector<BattleHex> impassableHexes;
	if(battleGetBattlefieldType().num == BFieldType::SHIP_TO_SHIP)
	{
		impassableHexes =
		{
			6, 7, 8, 9,
			24, 25, 26,
			58, 59, 60,
			75, 76, 77,
			92, 93, 94,
			109, 110, 111,
			126, 127, 128,
			159, 160, 161, 162, 163,
			176, 177, 178, 179, 180
		};
	}
	for(auto hex : impassableHexes)
		ret[hex] = EAccessibility::UNAVAILABLE;

	//gate -> should be before stacks
	if(battleGetSiegeLevel() > 0)
	{
		EAccessibility accessability = EAccessibility::ACCESSIBLE;
		switch(battleGetGateState())
		{
		case EGateState::CLOSED:
			accessability = EAccessibility::GATE;
			break;

		case EGateState::BLOCKED:
			accessability = EAccessibility::UNAVAILABLE;
			break;
		}
		ret[ESiegeHex::GATE_OUTER] = ret[ESiegeHex::GATE_INNER] = accessability;
	}

	//tiles occupied by standing stacks
	for(auto stack : battleAliveStacks())
	{
		for(auto hex : stack->getHexes())
			if(hex.isAvailable()) //towers can have <0 pos; we don't also want to overwrite side columns
				ret[hex] = EAccessibility::ALIVE_STACK;
	}

	//obstacles
	for(const auto &obst : battleGetAllObstacles())
	{
		for(auto hex : obst->getBlockedTiles())
			ret[hex] = EAccessibility::OBSTACLE;
	}

	//walls
	if(battleGetSiegeLevel() > 0)
	{
		static const int permanentlyLocked[] = {12, 45, 62, 112, 147, 165};
		for(auto hex : permanentlyLocked)
			ret[hex] = EAccessibility::UNAVAILABLE;

		//TODO likely duplicated logic
		static const std::pair<int, BattleHex> lockedIfNotDestroyed[] =
		{
			//which part of wall, which hex is blocked if this part of wall is not destroyed
			std::make_pair(2, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_4)),
			std::make_pair(3, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_3)),
			std::make_pair(4, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_2)),
			std::make_pair(5, BattleHex(ESiegeHex::DESTRUCTIBLE_WALL_1))
		};

		for(auto & elem : lockedIfNotDestroyed)
		{
			if(battleGetWallState(elem.first) != EWallState::DESTROYED)
				ret[elem.second] = EAccessibility::DESTRUCTIBLE_WALL;
		}
	}

	return ret;
}

AccessibilityInfo CBattleInfoCallback::getAccesibility(const CStack * stack) const
{
	return getAccesibility(stack->getHexes());
}

AccessibilityInfo CBattleInfoCallback::getAccesibility(const std::vector<BattleHex> & accessibleHexes) const
{
	auto ret = getAccesibility();
	for(auto hex : accessibleHexes)
		if(hex.isValid())
			ret[hex] = EAccessibility::ACCESSIBLE;

	return ret;
}

ReachabilityInfo CBattleInfoCallback::makeBFS(const AccessibilityInfo &accessibility, const ReachabilityInfo::Parameters & params) const
{
	ReachabilityInfo ret;
	ret.accessibility = accessibility;
	ret.params = params;

	ret.predecessors.fill(BattleHex::INVALID);
	ret.distances.fill(ReachabilityInfo::INFINITE_DIST);

	if(!params.startPosition.isValid()) //if got call for arrow turrets
		return ret;

	const std::set<BattleHex> quicksands = getStoppers(params.perspective);
	//const bool twoHexCreature = params.doubleWide;

	std::queue<BattleHex> hexq; //bfs queue

	//first element
	hexq.push(params.startPosition);
	ret.distances[params.startPosition] = 0;

	while(!hexq.empty()) //bfs loop
	{
		const BattleHex curHex = hexq.front();
		hexq.pop();

		//walking stack can't step past the quicksands
		//TODO what if second hex of two-hex creature enters quicksand
		if(curHex != params.startPosition && vstd::contains(quicksands, curHex))
			continue;

		const int costToNeighbour = ret.distances[curHex] + 1;
		for(BattleHex neighbour : curHex.neighbouringTiles())
		{
			const bool accessible = accessibility.accessible(neighbour, params.doubleWide, params.side);
			const int costFoundSoFar = ret.distances[neighbour];

			if(accessible && costToNeighbour < costFoundSoFar)
			{
				hexq.push(neighbour);
				ret.distances[neighbour] = costToNeighbour;
				ret.predecessors[neighbour] = curHex;
			}
		}
	}

	return ret;
}

ReachabilityInfo CBattleInfoCallback::makeBFS(const CStack * stack) const
{
	return makeBFS(getAccesibility(stack), ReachabilityInfo::Parameters(stack));
}

std::set<BattleHex> CBattleInfoCallback::getStoppers(BattlePerspective::BattlePerspective whichSidePerspective) const
{
	std::set<BattleHex> ret;
	RETURN_IF_NOT_BATTLE(ret);

	for(auto &oi : battleGetAllObstacles(whichSidePerspective))
	{
		if(battleIsObstacleVisibleForSide(*oi, whichSidePerspective))
		{
			range::copy(oi->getStoppingTile(), vstd::set_inserter(ret));
		}
	}

	return ret;
}

std::pair<const CStack *, BattleHex> CBattleInfoCallback::getNearestStack(const CStack * closest, BattleSideOpt side) const
{
	auto reachability = getReachability(closest);
	auto avHexes = battleGetAvailableHexes(closest, false);

	// I hate std::pairs with their undescriptive member names first / second
	struct DistStack
	{
		int distanceToPred;
		BattleHex destination;
		const CStack * stack;
	};

	std::vector<DistStack> stackPairs;

	std::vector<const CStack *> possibleStacks = battleGetStacksIf([=](const CStack * s)
	{
		return s->isValidTarget(false) && s != closest && (!side || side.get() == s->side);
	});

	for(const CStack * st : possibleStacks)
		for(BattleHex hex : avHexes)
			if(CStack::isMeleeAttackPossible(closest, st, hex))
			{
				DistStack hlp = {reachability.distances[hex], hex, st};
				stackPairs.push_back(hlp);
			}

	if (stackPairs.size())
	{
		auto comparator = [](DistStack lhs, DistStack rhs) { return lhs.distanceToPred < rhs.distanceToPred; };
		auto minimal = boost::min_element(stackPairs, comparator);
		return std::make_pair(minimal->stack, minimal->destination);
	}
	else
		return std::make_pair<const CStack * , BattleHex>(nullptr, BattleHex::INVALID);
}

si8 CBattleInfoCallback::battleGetTacticDist() const
{
	RETURN_IF_NOT_BATTLE(0);

	//TODO get rid of this method
	if(battleDoWeKnowAbout(battleGetTacticsSide()))
		return battleTacticDist();

	return 0;
}

bool CBattleInfoCallback::isInTacticRange(BattleHex dest) const
{
	RETURN_IF_NOT_BATTLE(false);
	auto side = battleGetTacticsSide();
	auto dist = battleGetTacticDist();

	return ((!side && dest.getX() > 0 && dest.getX() <= dist)
			|| (side && dest.getX() < GameConstants::BFIELD_WIDTH - 1 && dest.getX() >= GameConstants::BFIELD_WIDTH - dist - 1));
}

ReachabilityInfo CBattleInfoCallback::getReachability(const CStack *stack) const
{
	ReachabilityInfo::Parameters params(stack);

	if(!battleDoWeKnowAbout(stack->side))
	{
		//Stack is held by enemy, we can't use his perspective to check for reachability.
		// Happens ie. when hovering enemy stack for its range. The arg could be set properly, but it's easier to fix it here.
		params.perspective = battleGetMySide();
	}

	return getReachability(params);
}

ReachabilityInfo CBattleInfoCallback::getReachability(const ReachabilityInfo::Parameters &params) const
{
	if(params.flying)
		return getFlyingReachability(params);
	else
		return makeBFS(getAccesibility(params.knownAccessible), params);
}

ReachabilityInfo CBattleInfoCallback::getFlyingReachability(const ReachabilityInfo::Parameters &params) const
{
	ReachabilityInfo ret;
	ret.accessibility = getAccesibility(params.knownAccessible);

	for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
	{
		if(ret.accessibility.accessible(i, params.doubleWide, params.side))
		{
			ret.predecessors[i] = params.startPosition;
			ret.distances[i] = BattleHex::getDistance(params.startPosition, i);
		}
	}

	return ret;
}

AttackableTiles CBattleInfoCallback::getPotentiallyAttackableHexes (const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos) const
{
	//does not return hex attacked directly
	//TODO: apply rotation to two-hex attackers
	bool isAttacker = attacker->side == BattleSide::ATTACKER;

	AttackableTiles at;
	RETURN_IF_NOT_BATTLE(at);

	const int WN = GameConstants::BFIELD_WIDTH;
	BattleHex hex = (attackerPos != BattleHex::INVALID) ? attackerPos.hex : attacker->position.hex; //real or hypothetical (cursor) position

	//FIXME: dragons or cerbers can rotate before attack, making their base hex different (#1124)
	bool reverse = isToReverse (hex, destinationTile, isAttacker, attacker->doubleWide(), isAttacker);
	if (reverse && attacker->doubleWide())
	{
		hex = attacker->occupiedHex(hex); //the other hex stack stands on
	}
	if (attacker->hasBonusOfType(Bonus::ATTACKS_ALL_ADJACENT))
	{
		boost::copy (attacker->getSurroundingHexes (attackerPos), vstd::set_inserter (at.hostileCreaturePositions));
	}
	if (attacker->hasBonusOfType(Bonus::THREE_HEADED_ATTACK))
	{
		std::vector<BattleHex> hexes = attacker->getSurroundingHexes(attackerPos);
		for (BattleHex tile : hexes)
		{
			if ((BattleHex::mutualPosition(tile, destinationTile) > -1 && BattleHex::mutualPosition (tile, hex) > -1)) //adjacent both to attacker's head and attacked tile
			{
				const CStack * st = battleGetStackByPos(tile, true);
				if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
				{
					at.hostileCreaturePositions.insert(tile);
				}
			}
		}
	}
	if (attacker->hasBonusOfType(Bonus::TWO_HEX_ATTACK_BREATH) && BattleHex::mutualPosition (destinationTile.hex, hex) > -1) //only adjacent hexes are subject of dragon breath calculation
	{
		std::vector<BattleHex> hexes; //only one, in fact
		int pseudoVector = destinationTile.hex - hex;
		switch (pseudoVector)
		{
		case 1:
		case -1:
			BattleHex::checkAndPush (destinationTile.hex + pseudoVector, hexes);
			break;
		case WN: //17 //left-down or right-down
		case -WN: //-17 //left-up or right-up
		case WN + 1: //18 //right-down
		case -WN + 1: //-16 //right-up
			BattleHex::checkAndPush (destinationTile.hex + pseudoVector + (((hex/WN)%2) ? 1 : -1), hexes);
			break;
		case WN-1: //16 //left-down
		case -WN-1: //-18 //left-up
			BattleHex::checkAndPush (destinationTile.hex + pseudoVector + (((hex/WN)%2) ? 1 : 0), hexes);
			break;
		}
		for (BattleHex tile : hexes)
		{
			//friendly stacks can also be damaged by Dragon Breath
			if (battleGetStackByPos (tile, true))
				at.friendlyCreaturePositions.insert (tile);
		}
	}

	return at;
}

std::set<const CStack*> CBattleInfoCallback::getAttackedCreatures(const CStack* attacker, BattleHex destinationTile, BattleHex attackerPos) const
{
	std::set<const CStack*> attackedCres;
	RETURN_IF_NOT_BATTLE(attackedCres);

	AttackableTiles at = getPotentiallyAttackableHexes(attacker, destinationTile, attackerPos);
	for (BattleHex tile : at.hostileCreaturePositions) //all around & three-headed attack
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st && st->owner != attacker->owner) //only hostile stacks - does it work well with Berserk?
		{
			attackedCres.insert(st);
		}
	}
	for (BattleHex tile : at.friendlyCreaturePositions)
	{
		const CStack * st = battleGetStackByPos(tile, true);
		if(st) //friendly stacks can also be damaged by Dragon Breath
		{
			attackedCres.insert(st);
		}
	}
	return attackedCres;
}

//TODO: this should apply also to mechanics and cursor interface
bool CBattleInfoCallback::isToReverseHlp (BattleHex hexFrom, BattleHex hexTo, bool curDir) const
{
	int fromX = hexFrom.getX();
	int fromY = hexFrom.getY();
	int toX = hexTo.getX();
	int toY = hexTo.getY();

	if (curDir) // attacker, facing right
	{
		if (fromX < toX)
			return false;
		if (fromX > toX)
			return true;

		if (fromY % 2 == 0 && toY % 2 == 1)

			return true;
		return false;
	}
	else // defender, facing left
	{
		if(fromX < toX)
			return true;
		if(fromX > toX)
			return false;

		if (fromY % 2 == 1 && toY % 2 == 0)
			return true;
		return false;
	}
}

//TODO: this should apply also to mechanics and cursor interface
bool CBattleInfoCallback::isToReverse (BattleHex hexFrom, BattleHex hexTo, bool curDir, bool toDoubleWide, bool toDir) const
{
	if (hexTo < 0 || hexFrom < 0) //turret
		return false;

	if (toDoubleWide)
	{
		if (isToReverseHlp (hexFrom, hexTo, curDir))
		{
			if (toDir)
				return isToReverseHlp (hexFrom, hexTo-1, curDir);
			else
				return isToReverseHlp (hexFrom, hexTo+1, curDir);
		}
		return false;
	}
	else
	{
		return isToReverseHlp(hexFrom, hexTo, curDir);
	}
}

ReachabilityInfo::TDistances CBattleInfoCallback::battleGetDistances(const CStack * stack, BattleHex hex, BattleHex * predecessors) const
{
	ReachabilityInfo::TDistances ret;
	ret.fill(-1);
	RETURN_IF_NOT_BATTLE(ret);

	ReachabilityInfo::Parameters params(stack);
	params.perspective = battleGetMySide();
	params.startPosition = hex.isValid() ? hex : stack->position;
	auto reachability = getReachability(params);

	boost::copy(reachability.distances, ret.begin());

	if(predecessors)
		for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
			predecessors[i] = reachability.predecessors[i];

	return ret;
}

si8 CBattleInfoCallback::battleHasDistancePenalty(const CStack * stack, BattleHex destHex) const
{
	return battleHasDistancePenalty(stack, stack->position, destHex);
}

si8 CBattleInfoCallback::battleHasDistancePenalty(const IBonusBearer *bonusBearer, BattleHex shooterPosition, BattleHex destHex) const
{
	RETURN_IF_NOT_BATTLE(false);
	if(bonusBearer->hasBonusOfType(Bonus::NO_DISTANCE_PENALTY))
		return false;

	if(const CStack * dstStack = battleGetStackByPos(destHex, false))
	{
		//If any hex of target creature is within range, there is no penalty
		for(auto hex : dstStack->getHexes())
			if(BattleHex::getDistance(shooterPosition, hex) <= GameConstants::BATTLE_PENALTY_DISTANCE)
				return false;
		//TODO what about two-hex shooters?
	}
	else
	{
		if (BattleHex::getDistance(shooterPosition, destHex) <= GameConstants::BATTLE_PENALTY_DISTANCE)
			return false;
	}

	return true;
}

BattleHex CBattleInfoCallback::wallPartToBattleHex(EWallPart::EWallPart part) const
{
	RETURN_IF_NOT_BATTLE(BattleHex::INVALID);
	return WallPartToHex(part);
}

EWallPart::EWallPart CBattleInfoCallback::battleHexToWallPart(BattleHex hex) const
{
	RETURN_IF_NOT_BATTLE(EWallPart::INVALID);
	return hexToWallPart(hex);
}

bool CBattleInfoCallback::isWallPartPotentiallyAttackable(EWallPart::EWallPart wallPart) const
{
	RETURN_IF_NOT_BATTLE(false);
	return wallPart != EWallPart::INDESTRUCTIBLE_PART && wallPart != EWallPart::INDESTRUCTIBLE_PART_OF_GATE &&
																	wallPart != EWallPart::INVALID;
}

std::vector<BattleHex> CBattleInfoCallback::getAttackableBattleHexes() const
{
	std::vector<BattleHex> attackableBattleHexes;
	RETURN_IF_NOT_BATTLE(attackableBattleHexes);

	for(auto & wallPartPair : wallParts)
	{
		if(isWallPartPotentiallyAttackable(wallPartPair.second))
		{
			auto wallState = static_cast<EWallState::EWallState>(battleGetWallState(static_cast<int>(wallPartPair.second)));
			if(wallState == EWallState::INTACT || wallState == EWallState::DAMAGED)
			{
				attackableBattleHexes.push_back(BattleHex(wallPartPair.first));
			}
		}
	}

	return attackableBattleHexes;
}

ui32 CBattleInfoCallback::battleGetSpellCost(const CSpell * sp, const CGHeroInstance * caster) const
{
	RETURN_IF_NOT_BATTLE(-1);
	//TODO should be replaced using bonus system facilities (propagation onto battle node)

	ui32 ret = caster->getSpellCost(sp);

	//checking for friendly stacks reducing cost of the spell and
	//enemy stacks increasing it
	si32 manaReduction = 0;
	si32 manaIncrease = 0;

	for(auto stack : battleAliveStacks())
	{
		if(stack->owner == caster->tempOwner && stack->hasBonusOfType(Bonus::CHANGES_SPELL_COST_FOR_ALLY))
		{
			vstd::amax(manaReduction, stack->valOfBonuses(Bonus::CHANGES_SPELL_COST_FOR_ALLY));
		}
		if(stack->owner != caster->tempOwner && stack->hasBonusOfType(Bonus::CHANGES_SPELL_COST_FOR_ENEMY))
		{
			vstd::amax(manaIncrease, stack->valOfBonuses(Bonus::CHANGES_SPELL_COST_FOR_ENEMY));
		}
	}

	return ret - manaReduction + manaIncrease;
}

const CStack * CBattleInfoCallback::getStackIf(std::function<bool(const CStack*)> pred) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	auto stacks = battleGetAllStacks();
	auto stackItr = range::find_if(stacks, pred);
	return stackItr == stacks.end() ? nullptr : *stackItr;
}

si8 CBattleInfoCallback::battleHasShootingPenalty(const CStack * stack, BattleHex destHex)
{
	return battleHasDistancePenalty(stack, destHex) || battleHasWallPenalty(stack, destHex);
}

bool CBattleInfoCallback::battleIsStackBlocked(const CStack * stack) const
{
	RETURN_IF_NOT_BATTLE(false);

	if(stack->hasBonusOfType(Bonus::SIEGE_WEAPON)) //siege weapons cannot be blocked
		return false;

	for(const CStack * s : batteAdjacentCreatures(stack))
	{
		if (s->owner != stack->owner) //blocked by enemy stack
			return true;
	}
	return false;
}

std::set<const CStack*> CBattleInfoCallback:: batteAdjacentCreatures(const CStack * stack) const
{
	std::set<const CStack*> stacks;
	RETURN_IF_NOT_BATTLE(stacks);

	for (BattleHex hex : stack->getSurroundingHexes())
		if(const CStack * neighbour = battleGetStackByPos(hex, true))
			stacks.insert(neighbour);

	return stacks;
}

SpellID CBattleInfoCallback::getRandomBeneficialSpell(CRandomGenerator & rand, const CStack * subject) const
{
	RETURN_IF_NOT_BATTLE(SpellID::NONE);
	//This is complete list. No spells from mods.
	//todo: this should be Spellbook of caster Stack
	static const std::set<SpellID> allPossibleSpells =
	{
		SpellID::AIR_SHIELD,
		SpellID::ANTI_MAGIC,
		SpellID::BLESS,
		SpellID::BLOODLUST,
		SpellID::COUNTERSTRIKE,
		SpellID::CURE,
		SpellID::FIRE_SHIELD,
		SpellID::FORTUNE,
		SpellID::HASTE,
		SpellID::MAGIC_MIRROR,
		SpellID::MIRTH,
		SpellID::PRAYER,
		SpellID::PRECISION,
		SpellID::PROTECTION_FROM_AIR,
		SpellID::PROTECTION_FROM_EARTH,
		SpellID::PROTECTION_FROM_FIRE,
		SpellID::PROTECTION_FROM_WATER,
		SpellID::SHIELD,
		SpellID::SLAYER,
		SpellID::STONE_SKIN
	};
	std::vector<SpellID> beneficialSpells;

	auto getAliveEnemy = [=](const std::function<bool(const CStack *)> & pred)
	{
		return getStackIf([=](const CStack * stack)
		{
			return pred(stack) && stack->owner != subject->owner && stack->alive();
		});
	};

	for(const SpellID spellID : allPossibleSpells)
	{
		std::stringstream cachingStr;
		cachingStr << "source_" << Bonus::SPELL_EFFECT << "id_" << spellID.num;

		if(subject->hasBonus(Selector::source(Bonus::SPELL_EFFECT, spellID), Selector::all, cachingStr.str())
		 //TODO: this ability has special limitations
		|| spellID.toSpell()->canBeCast(this, ECastingMode::CREATURE_ACTIVE_CASTING, subject) != ESpellCastProblem::OK)
			continue;

		switch (spellID)
		{
		case SpellID::SHIELD:
		case SpellID::FIRE_SHIELD: // not if all enemy units are shooters
		{
			auto walker = getAliveEnemy([&](const CStack * stack) //look for enemy, non-shooting stack
			{
				return !stack->canShoot();
			});

			if (!walker)
				continue;
		}
			break;
		case SpellID::AIR_SHIELD: //only against active shooters
		{
			auto shooter = getAliveEnemy([&](const CStack * stack) //look for enemy, non-shooting stack
			{
				return stack->canShoot();
			});
			if (!shooter)
				continue;
		}
			break;
		case SpellID::ANTI_MAGIC:
		case SpellID::MAGIC_MIRROR:
		case SpellID::PROTECTION_FROM_AIR:
		case SpellID::PROTECTION_FROM_EARTH:
		case SpellID::PROTECTION_FROM_FIRE:
		case SpellID::PROTECTION_FROM_WATER:
		{
			const ui8 enemySide = 1 - subject->side;
			//todo: only if enemy has spellbook
			if (!battleHasHero(enemySide)) //only if there is enemy hero
				continue;
		}
			break;
		case SpellID::CURE: //only damaged units
		{
			//do not cast on affected by debuffs
			if(!subject->canBeHealed())
				continue;
		}
			break;
		case SpellID::BLOODLUST:
		{
			if(subject->canShoot()) //TODO: if can shoot - only if enemy units are adjacent
				continue;
		}
			break;
		case SpellID::PRECISION:
		{
			if(!subject->canShoot())
				continue;
		}
			break;
		case SpellID::SLAYER://only if monsters are present
		{
			auto kingMonster = getAliveEnemy([&](const CStack * stack) -> bool //look for enemy, non-shooting stack
			{
				const auto isKing = Selector::type(Bonus::KING1)
									.Or(Selector::type(Bonus::KING2))
									.Or(Selector::type(Bonus::KING3));

				return stack->hasBonus(isKing);
			});

			if (!kingMonster)
				continue;
		}
			break;
		}
		beneficialSpells.push_back(spellID);
	}

	if(!beneficialSpells.empty())
	{
		return *RandomGeneratorUtil::nextItem(beneficialSpells, rand);
	}
	else
	{
		return SpellID::NONE;
	}
}

SpellID CBattleInfoCallback::getRandomCastedSpell(CRandomGenerator & rand,const CStack * caster) const
{
	RETURN_IF_NOT_BATTLE(SpellID::NONE);

	TBonusListPtr bl = caster->getBonuses(Selector::type(Bonus::SPELLCASTER));
	if (!bl->size())
		return SpellID::NONE;
	int totalWeight = 0;
	for(auto b : *bl)
	{
		totalWeight += std::max(b->additionalInfo, 1); //minimal chance to cast is 1
	}
	int randomPos = rand.nextInt(totalWeight - 1);
	for(auto b : *bl)
	{
		randomPos -= std::max(b->additionalInfo, 1);
		if(randomPos < 0)
		{
			return SpellID(b->subtype);
		}
	}

	return SpellID::NONE;
}

int CBattleInfoCallback::battleGetSurrenderCost(PlayerColor Player) const
{
	RETURN_IF_NOT_BATTLE(-3);
	if(!battleCanSurrender(Player))
		return -1;

	const auto side = playerToSide(Player);
	if(!side)
		return -1;

	int ret = 0;
	double discount = 0;
	for(const CStack * s : battleAliveStacks(side.get()))
		if(s->base) //we pay for our stack that comes from our army slots - condition eliminates summoned cres and war machines
			ret += s->getCreature()->cost[Res::GOLD] * s->getCount(); //todo: extract CStack method

	if(const CGHeroInstance * h = battleGetFightingHero(side.get()))
		discount += h->valOfBonuses(Bonus::SURRENDER_DISCOUNT);

	ret *= (100.0 - discount) / 100.0;
	vstd::amax(ret, 0); //no negative costs for >100% discounts (impossible in original H3 mechanics, but some day...)
	return ret;
}

si8 CBattleInfoCallback::battleMaxSpellLevel(ui8 side) const
{
	const IBonusBearer *node = nullptr;
	if(const CGHeroInstance * h = battleGetFightingHero(side))
		node = h;
	else
		node = getBattleNode();

	if(!node)
		return GameConstants::SPELL_LEVELS;

	//We can't "just get value" - it'd be 0 if there are bonuses (and all would be blocked)
	auto b = node->getBonuses(Selector::type(Bonus::BLOCK_MAGIC_ABOVE));
	if(b->size())
		return b->totalValue();

	return GameConstants::SPELL_LEVELS;
}

boost::optional<int> CBattleInfoCallback::battleIsFinished() const
{
	auto stacks = battleGetAllStacks();

	//checking winning condition
	bool hasStack[2]; //hasStack[0] - true if attacker has a living stack; defender similarly
	hasStack[0] = hasStack[1] = false;

	for(auto & stack : stacks)
	{
		if(stack->alive() && !stack->hasBonusOfType(Bonus::SIEGE_WEAPON))
		{
			hasStack[stack->side] = true;
		}
	}

	if(!hasStack[0] && !hasStack[1])
		return 2;
	if(!hasStack[1])
		return 0;
	if(!hasStack[0])
		return 1;
	return boost::none;
}

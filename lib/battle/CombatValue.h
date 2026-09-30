/*
 * CombatValue.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Creature.h>

#include "BattleSide.h"

class CCreature;
class CGHeroInstance;
class CBattleInfoCallback;

namespace battle
{
class Unit;
}

/// Data of units that a unit fights against. Bonuses useful only against a particular enemy are weighted
/// by this, so the same unit can have different value in different battles. When the enemy is unknown,
/// units are evaluated against CombatValue::averageBattle.
class DLL_LINKAGE CombatValueContext
{
public:
	/// Slayer mastery levels, from none to expert
	using MasteryShares = std::array<double, 4>;

	/// Identifies a context, so that values computed against it can be cached
	int32_t id() const;

	/// Fraction of enemy units that attack in melee rather than shoot
	double meleeShare = 1;
	/// Magic strength of the enemy, with 1 being a hero of about five knowledge and five spell power
	double magicPower = 1;
	/// Fraction of enemy units that a slayer can target, indexed by mastery level
	MasteryShares kingShare = {};
	/// Chance that an allied stack other than the attacker is present and can be hit
	double allyCrowding = 0;

	/// Data of units opposing the given side
	static CombatValueContext against(const CBattleInfoCallback & battle, BattleSide side);
};

/// Computes AI value of provided unit or creature. All computation is done at runtime and accounts
/// for any bonuses affecting the unit
class DLL_LINKAGE CombatValue
{
public:
	CombatValue();

	/// Value of a single creature of given type, modified by bonuses of its bearer
	int64_t getAIValue(const ACreature & bearer, const Creature * type) const;
	int64_t getAIValue(const ACreature & bearer, const Creature * type, const CombatValueContext & context) const;

	int64_t getAIValue(const battle::Unit * unit) const;
	int64_t getAIValue(const battle::Unit * unit, const CombatValueContext & context) const;

	/// Same, but takes distance to enemy units into account
	int64_t getAIValue(const battle::Unit * unit, const CBattleInfoCallback & battle) const;

	int64_t getAIValue(const Creature * creature) const;

	/// Context used to evaluate creatures when the actual enemy is unknown
	const CombatValueContext & averageBattle() const;

	/// Number of attacks per round, with additional attacks counted at reduced weight
	static double attacksPerRound(const ACreature & creature);
	static double targetsPerAttack(const ACreature & creature);
	/// Number of retaliations that a creature performs per round
	static double retaliationsPerRound(const ACreature & creature);
	/// Damage multiplier from bonuses that are useful against any enemy
	static double offenseMultiplier(const ACreature & creature);
	/// Effective hit points multiplier from bonuses that are useful against any enemy
	static double survivalMultiplier(const ACreature & creature);

	/// Same two, for bonuses that are only useful against a particular enemy
	static double situationalOffense(const ACreature & creature, const CombatValueContext & context);
	static double situationalSurvival(const ACreature & creature, const CombatValueContext & context);
	/// Hit points that regeneration restores over a battle, per single creature in a stack
	static double regeneratedHitPoints(const ACreature & creature, int count);
	/// Stack size that value of a whole stack scales with, discounted for wounds
	static double stackScale(const battle::Unit & unit);
	/// Stack size that a creature is valued at, so that value depends only on bonuses of the unit
	static int referenceCount(const Creature * creature);
	/// Fraction of a battle that a creature spends attacking rather than approaching the enemy
	static double uptimeOf(const ACreature & creature);
	static double uptimeOf(const ACreature & creature, int hexesToEnemy);
	/// Defense skill, including expected contribution of defending
	static int effectiveDefense(const ACreature & creature);
	/// Fraction of dealt damage that a creature receives back as retaliation
	static double retaliationSuffered(const ACreature & creature);
	static double combine(double output, double effectiveHitPoints, double uptime);
	static double median(std::vector<double> values);

	/// Distance in hexes between the two sides at the start of a battle
	static int startingDistance();

private:
	/// Fraction of its damage that an attack of given skill deals to an average creature
	double offenseAt(int attack) const;
	/// Fraction of an average attack that given defense skill lets through
	double defenseAt(int defense) const;

	double valueOf(const ACreature & creature, double uptime, int count, const CombatValueContext & context) const;

	void buildCurves(const std::vector<const CCreature *> & builtinCreatures);
	void pinScale(const std::vector<const CCreature *> & builtinCreatures);
	void tabulateCreatures();

	/// Value of every creature by index. Bonuses of a creature type never change, so this never goes stale
	std::vector<int64_t> creatureValues;

	std::vector<double> offenseCurve;
	std::vector<double> defenseCurve;

	double averageDefense = 0;
	/// Average battle, computed from creatures of the base game
	CombatValueContext defaultContext;
	/// Factor that maps computed values onto the H3 aiValue range
	double scale = 1;
};

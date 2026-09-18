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

/// What a unit is up against. Bonuses that only pay off against a particular enemy are weighted by
/// this, so the same unit can be worth more in one battle than in another. The average battle is
/// held by CombatValue::averageBattle, and is what a unit is worth before its enemy is known - the
/// member defaults here are only a starting point for building one.
class DLL_LINKAGE CombatValueContext
{
public:
	/// Slayer masteries a bonus can name, from none through expert
	using MasteryShares = std::array<double, 4>;

	/// Tells one context from another, so that what was weighed against it can be remembered.
	/// Contexts that describe the same enemy answer the same, however each of them was built.
	int32_t id() const;

	/// Share of the enemy that closes in to strike rather than shooting
	double meleeShare = 1;
	/// Hostile magic the enemy can bring, with 1 standing for a hero of about five knowledge and
	/// five spell power at full mana. A spellbook alone brings none of it and counts as zero.
	double magicPower = 1;
	/// Share of the enemy that a slayer of each mastery reaches, indexed by that mastery
	MasteryShares kingShare = {};
	/// Chance that a blow aimed at an allied stack reaches yet another one, which is what a unit
	/// turned against its own side costs the army it stands in
	double allyCrowding = 0;

	/// What the other side of a battle presents to the given one
	static CombatValueContext against(const CBattleInfoCallback & battle, BattleSide side);
};

/// Computes AI value of provided unit or creature. All computation is done in runtime and accounts
/// for any bonuses affecting the unit
class DLL_LINKAGE CombatValue
{
public:
	CombatValue();

	/// Value of a single creature of given type, as modified by the bonuses that its bearer carries
	int64_t getAIValue(const ACreature & bearer, const Creature * type) const;
	int64_t getAIValue(const ACreature & bearer, const Creature * type, const CombatValueContext & context) const;

	int64_t getAIValue(const battle::Unit * unit) const;

	/// Same, but takes distance to enemy units in account
	int64_t getAIValue(const battle::Unit * unit, const CBattleInfoCallback & battle) const;

	int64_t getAIValue(const Creature * creature) const;

	/// The enemy that creature values are measured against when no actual one is known
	const CombatValueContext & averageBattle() const;

	/// Number of attacks per round, with repeated attacks counted at a discount
	static double attacksPerRound(const ACreature & creature);
	static double targetsPerAttack(const ACreature & creature);
	/// Number of retaliations that creature manages to use per round
	static double retaliationsPerRound(const ACreature & creature);
	/// Damage multiplier from bonuses that improve damage output whatever the unit is up against
	static double offenseMultiplier(const ACreature & creature);
	/// Effective hit points multiplier from bonuses that help whatever the unit is up against
	static double survivalMultiplier(const ACreature & creature);

	/// Same two, for bonuses that are only worth something against a particular enemy - magic
	/// defenses against a spellcasting hero, a shield against the kind of blow it turns aside
	static double situationalOffense(const ACreature & creature, const CombatValueContext & context);
	static double situationalSurvival(const ACreature & creature, const CombatValueContext & context);
	/// Hit points that regeneration restores over a battle, per single creature in a stack
	static double regeneratedHitPoints(const ACreature & creature, int count);
	/// Stack size a creature is valued at. Always this rather than the size of an actual stack, so
	/// that value reads only the bonuses a unit carries and stays proportional to its size.
	static int referenceCount(const Creature * creature);
	/// Share of a battle that creature spends attacking rather than approaching enemy
	static double uptimeOf(const ACreature & creature);
	static double uptimeOf(const ACreature & creature, int hexesToEnemy);
	/// Defense skill, including expected contribution of defending
	static int effectiveDefense(const ACreature & creature);
	/// Share of dealt damage that creature receives back as retaliation
	static double retaliationSuffered(const ACreature & creature);
	static double combine(double output, double effectiveHitPoints, double uptime);
	static double median(std::vector<double> values);

	/// Hexes between the two sides of a battle at the start of one
	static int startingDistance();

private:
	/// Share of its damage that an attack of given skill deals to an average creature
	double offenseAt(int attack) const;
	/// Share of an average attack that given defense skill lets through
	double defenseAt(int defense) const;

	double valueOf(const ACreature & creature, double uptime, int count, const CombatValueContext & context) const;

	void buildCurves(const std::vector<const CCreature *> & builtinCreatures);
	void pinScale(const std::vector<const CCreature *> & builtinCreatures);
	void tabulateCreatures();

	/// Value of every creature by index. Creature bonuses never change, so asking for one costs
	/// nothing - which matters, since the AI asks constantly while it searches.
	std::vector<int64_t> creatureValues;

	std::vector<double> offenseCurve;
	std::vector<double> defenseCurve;

	double averageDefense = 0;
	/// The average battle, measured off the creatures the game ships with
	CombatValueContext defaultContext;
	/// Factor that maps computed values onto the H3 aiValue range
	double scale = 1;
};

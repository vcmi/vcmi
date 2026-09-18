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

class CCreature;
class CBattleInfoCallback;

namespace battle
{
class Unit;
}

/// Computes AI value or fight value of provided unit or creature. All computation is done in
/// runtime and accounts for any bonuses affecting the unit
class DLL_LINKAGE CombatValue
{
public:
	CombatValue();

	/// Value of a single creature of given type, as modified by the bonuses that its bearer carries
	int64_t getAIValue(const ACreature & bearer, const Creature * type) const;

	int64_t getAIValue(const battle::Unit * unit) const;

	/// Same, but takes distance to enemy units in account
	int64_t getAIValue(const battle::Unit * unit, const CBattleInfoCallback & battle) const;

	int64_t getAIValue(const Creature * creature) const;
	int64_t getFightValue(const Creature * creature) const;

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
	/// defenses against a spellcasting hero, a shield against the kind of blow it turns aside.
	/// Valued here at what an average battle gives them, since a unit carrying one is better than a
	/// unit without it even before it is known what the unit will face.
	static double situationalOffense(const ACreature & creature);
	double situationalSurvival(const ACreature & creature) const;
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

	double valueOf(const ACreature & creature, double uptime, int count) const;

	void buildCurves(const std::vector<const CCreature *> & builtinCreatures);
	void pinScale(const std::vector<const CCreature *> & builtinCreatures);
	void tabulateCreatures();

	/// Value of every creature by index. Creature bonuses never change, so asking for one costs
	/// nothing - which matters, since the AI asks constantly while it searches.
	std::vector<int64_t> creatureValues;

	std::vector<double> offenseCurve;
	std::vector<double> defenseCurve;

	double averageDefense = 0;
	/// Share of creatures that approach instead of shooting, i.e. how often retaliation is possible
	double meleeAttackerShare = 1;
	/// Factors that map computed values onto H3 aiValue and fightValue ranges
	double scale = 1;
	double fightScale = 1;
};

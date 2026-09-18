/*
 * CreatureValueEstimator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/battle/BattleHex.h"
#include "../lib/battle/CombatValue.h"
#include "../lib/battle/BattleSide.h"

class CCreature;
class CGameState;
class CGHeroInstance;
class CStack;
class BattleInfo;
class IMapService;

/// Computes fight value and AI value of every loaded creature by simulating battles between each
/// possible pair of creatures. H3 creatures serve as a baseline, modded creatures are valued by
/// their performance against that baseline. Diagnostic tool, results are written to the log
class CreatureValueEstimator
{
public:
	/// Measures every loaded creature and logs derived values next to configured ones
	static void run();

	/// Casts every combat spell on a few creatures and logs what it does to what they are worth
	static void runSpells();

private:
	/// Initial positions of creatures to be tested
	static constexpr int attackerHex = 8 * GameConstants::BFIELD_WIDTH + 3;
	static constexpr int defenderHex = 8 * GameConstants::BFIELD_WIDTH + 12;

	/// Total hit points of each measured stack, so that every pair is an even exchange
	static constexpr int stackHitPoints = 2000;

	/// Spell power the measuring hero casts with, midway through what a hero reaches in a game
	static constexpr int spellPower = 15;

	/// How far a derived value may sit from the configured one, as a factor either way round
	static constexpr double agreementTolerance = 1.1;
	static constexpr double disagreementTolerance = 2.0;

	/// One creature and everything measured about it
	struct Entry
	{
		const CCreature * creature = nullptr;
		/// Hit points destroyed per round, averaged over the baseline
		double output = 0;
		/// Hit points needed to kill it, once its defense is accounted for
		double effectiveHitPoints = 0;
		/// Share of a battle that it spends attacking rather than approaching
		double uptime = 0;
		double fightValue = 0;
		double aiValue = 0;
		/// The same two values as computed by CombatValue, without simulating any battle
		double formulaFightValue = 0;
		double formulaAiValue = 0;
		bool baseline = false;
	};

	class BattlegroundMapService;

	std::shared_ptr<CGameState> gameState;
	std::unique_ptr<IMapService> mapService;
	CGHeroInstance * attackerSideHero = nullptr;
	CGHeroInstance * defenderSideHero = nullptr;

	/// Every creature that is valued, H3 creatures first - averages are taken over those alone
	std::vector<const CCreature *> subjects;
	size_t baselineCount = 0;

	/// Damage dealt and taken against every baseline creature, indexed by subject and by opponent
	std::vector<std::vector<double>> dealtBySubject;
	std::vector<std::vector<double>> retaliatedByOpponent;
	std::vector<std::vector<double>> dealtOnSubject;
	std::vector<std::vector<double>> retaliatedBySubject;

	std::vector<Entry> entries;

	/// Formula-based model that measurements are compared against
	CombatValue values;

	/// Builds a game just complete enough to hold a battle, with nothing that could scale damage
	void startGame();
	void startBattle();
	void collectSubjects();
	/// Fills damage tables by asking the engine for damage of a single attack in every pair
	void measureDamage();
	void deriveValues();
	/// Computes the same values as deriveValues, but using CombatValue instead of a battle
	void applyFormula();
	/// Checks the different ways of querying CombatValue against one another
	void checkModelPaths();
	void report() const;

	/// Applies spell packs straight to the game state, which is all a cast needs when no player is
	/// watching the battle
	class LocalSpellEnvironment;

	/// Creatures a spell is measured on: one that closes in, one that shoots, one that flies
	std::vector<const CCreature *> archetypes();
	void measureSpells();

	BattleInfo * battle() const;
	CStack * placeStack(BattleSide side, const CCreature * creature, const BattleHex & hex);
	CStack * placeStack(BattleSide side, const CCreature * creature, const BattleHex & hex, int count);
	void removeStack(const CStack * stack);
	/// Measures one ordered pair, returning damage dealt and taken back, per single creature
	void measurePair(const CStack * attacker, const CStack * defender, double & dealt, double & retaliated) const;

	/// Strips everything from a hero that could modify battle results
	static void stripHeroBonuses(CGHeroInstance * hero);
};

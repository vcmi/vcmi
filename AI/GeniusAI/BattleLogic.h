#ifndef __BATTLE_LOGIC_H__
#define __BATTLE_LOGIC_H__

#include "Common.h"
#include "BattleHelper.h"

#pragma warning (disable: 4100 4251 4245 4018 4081)
#include "../../global.h"
#include "../../CCallback.h"
#include "../../lib/CCreatureHandler.h"
#include "../../lib/CObjectHandler.h"
#pragma warning (default: 4100 4251 4245 4018 4081)

#pragma warning (disable: 4100)

using namespace std;

namespace geniusai { namespace BattleAI {

/**
 * Class is responsible for making decision during the battle.
 */
class CBattleLogic
{
private:
	enum EMainStrategyType
	{
		strategy_super_aggresive,
		strategy_aggresive,
		strategy_neutral,
		strategy_defensive,
		strategy_super_defensive,
		strategy_berserk_attack /** cause max damage, usually when creatures fight against overwhelming army*/
	};
	enum ECreatureRoleInBattle
	{
		creature_role_shooter,
		creature_role_defenser,
		creature_role_fast_attacker,
		creature_role_attacker
	};
	enum EActionType
	{
		action_cancel = 0,				// Cancel BattleAction
		action_cast_spell = 1,			// Hero cast a spell
		action_walk = 2,				// Walk
		action_defend = 3,				// Defend
		action_retreat = 4,				// Retreat from the battle
		action_surrender = 5,			// Surrender
		action_walk_and_attack = 6,		// Walk and Attack
		action_shoot = 7,				// Shoot
		action_wait = 8,				// Wait
		action_catapult = 9,			// Catapult
		action_monster_casts_spell = 10 // Monster casts a spell (i.e. Faerie Dragons)
	};
	struct SCreatureCasualties
	{
		int amount_max; // amount of creatures that will be dead
		int amount_min;
		int damage_max; // number of hit points that creature will lost
		int damage_min;
		int leftHitPoints_for_max; // number of hit points that will remain (for last unity)
		int leftHitPoint_for_min; // scenario
	};
public:
	CBattleLogic(ICallback *cb, const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side);
	~CBattleLogic();

	void SetCurrentTurn(int turn);
	/**
	 * Get the final decision.
	 */
	BattleAction MakeDecision(int stackID);
private:
	CBattleHelper m_battleHelper;
	//BattleInfo m_battleInfo;
	int m_iCurrentTurn;
	bool m_bIsAttacker;
	ICallback *m_cb;
	const CCreatureSet *m_army1;
	const CCreatureSet *m_army2;
	int3 m_tile;
	const CGHeroInstance *m_hero1;
	const CGHeroInstance *m_hero2;
	bool m_side;

	// statistics
	typedef std::vector<std::pair<int, int> > creature_stat; // first - creature id, second - value
	creature_stat m_statMaxDamage;
	creature_stat m_statMinDamage;
	//
	creature_stat m_statMaxSpeed;
	creature_stat m_statDistance;
	creature_stat m_statDistanceFromShooters;
	creature_stat m_statHitPoints;

	typedef std::vector<std::pair<int, SCreatureCasualties> > creature_stat_casualties;
	creature_stat_casualties m_statCasualties;

	bool m_bEnemyDominates;
	/**
	 * Before decision we have to make some calculation and simulation.
	 */
	void MakeStatistics(int currentCreatureId);
	/**
	 * Helper function. It's used for performing an attack action.
	 */
	std::vector<int> GetAvailableHexesForAttacker(const CStack *defender, const CStack *attacker = NULL);
	/**
	 * Make an attack action if it's possible.
	 * If it's not possible then function returns defend action.
	 */
	BattleAction MakeAttack(int attackerID, int destinationID);
	/**
	 * Berserk mode - do maximum casualties.
	 * Return vector wiht IDs of creatures to attack,
	 * additional info: -2 means wait, -1 - defend, 0 - make attack
	 */
	list<int> PerformBerserkAttack(int stackID, int &additionalInfo);
	/**
	 * Normal mode - equilibrium between casualties and yields.
	 * Return vector wiht IDs of creatures to attack,
	 * additional info: -2 means wait, -1 - defend, 0 - make attack
	 */
	list<int> PerformDefaultAction(int stackID, int &additionalInfo);
	/**
	 * Only for debug purpose.
	 */
	void PrintBattleAction(const BattleAction &action);
};

}}

#endif/*__BATTLE_LOGIC_H__*/
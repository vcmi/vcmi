#ifndef __CGENIUSAI_H__
#define __CGENIUSAI_H__

#define VCMI_DLL

#pragma warning (disable: 4100 4251 4245 4018 4081)
#include "../../global.h"
#include "../../AI_Base.h"
#include "../../CCallback.h"
#include "../../hch/CCreatureHandler.h"
#include "../../hch/CObjectHandler.h"

#pragma warning (default: 4100 4251 4245 4018 4081)

#pragma warning (disable: 4100)

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

using namespace std;

namespace GeniusAI {

class CBattleHelper
{
public:
	CBattleHelper();
	~CBattleHelper();

	int GetBattleFieldPosition(int x, int y);
	int DecodeXPosition(int battleFieldPosition);
	int DecodeYPosition(int battleFieldPosition);

	int GetShortestDistance(int pointA, int pointB);
	int GetDistanceWithObstacles(int pointA, int pointB);

	int GetVoteForMaxDamage() const { return m_voteForMaxDamage; }
	int GetVoteForMinDamage() const { return m_voteForMinDamage; }
	int GetVoteForMaxSpeed() const { return m_voteForMaxSpeed; }
	int GetVoteForDistance() const { return m_voteForDistance; }
	int GetVoteForDistanceFromShooters() const { return m_voteForDistanceFromShooters; }
	int GetVoteForHitPoints() const { return m_voteForHitPoints; }

	const int InfiniteDistance;
	const int BattlefieldWidth;
	const int BattlefieldHeight;
private:
	int m_voteForMaxDamage;
	int m_voteForMinDamage;
	int m_voteForMaxSpeed;
	int m_voteForDistance;
	int m_voteForDistanceFromShooters;
	int m_voteForHitPoints;

	CBattleHelper(const CBattleHelper &);
	CBattleHelper &operator=(const CBattleHelper &);
};

/**
 * Maybe it is a additional proxy, but i've separated the game IA for transparent.
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
	CBattleLogic(ICallback *cb, CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side);
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
	CCreatureSet *m_army1;
	CCreatureSet *m_army2; 
	int3 m_tile; 
	CGHeroInstance *m_hero1;
	CGHeroInstance *m_hero2; 
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
	std::vector<int> GetAvailableHexesForAttacker(CStack *defender, CStack *attacker = NULL);
	/**
	 * Just make defend action.
	 */
	BattleAction MakeDefend(int stackID);
	/**
	 * Just make wait action.
	 */
	BattleAction MakeWait(int stackID);
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
	void CBattleLogic::PrintBattleAction(const BattleAction &action);
};

class CGeniusAI : public CGlobalAI
{
private:
	ICallback *m_cb;
	CBattleLogic *m_battleLogic;
public:
	virtual void init(ICallback * CB);
	virtual void yourTurn();
	virtual void heroKilled(const CGHeroInstance *);
	virtual void heroCreated(const CGHeroInstance *);
	virtual void heroMoved(const HeroMoveDetails &);
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val) {};
	virtual void showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID){};
	virtual void tileRevealed(int3 pos){};
	virtual void tileHidden(int3 pos){};
	virtual void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback);
	// battle
	virtual void actionFinished(const BattleAction *action);//occurs AFTER every action taken by any stack or by the hero
	virtual void actionStarted(const BattleAction *action);//occurs BEFORE every action taken by any stack or by the hero
	virtual void battleAttack(BattleAttack *ba); //called when stack is performing attack
	virtual void battleStackAttacked(BattleStackAttacked * bsa); //called when stack receives damage (after battleAttack())
	virtual void battleEnd(BattleResult *br);
	virtual void battleNewRound(int round); //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	virtual void battleStackMoved(int ID, int dest);
	virtual void battleSpellCasted(SpellCasted *sc);
	virtual void battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side); //called by engine when battle starts; side=0 - left, side=1 - right
	virtual void battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles); //called when battlefield is prepared, prior the battle beginning
	//
	virtual void battleStackMoved(int ID, int dest, bool startMoving, bool endMoving);
	virtual void battleStackAttacking(int ID, int dest);
	virtual void battleStackIsAttacked(int ID, int dmg, int killed, int IDby, bool byShooting);
	virtual BattleAction activeStack(int stackID);
};

}

#endif // __CGENIUSAI_H__

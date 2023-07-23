/*
 * BattleProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"
#include "../../lib/NetPacks.h"

VCMI_LIB_NAMESPACE_BEGIN

class CStack;
struct SideInBattle;

namespace battle {
class CUnitState;
}

VCMI_LIB_NAMESPACE_END

class CBattleQuery;

struct CasualtiesAfterBattle
{
	using TStackAndItsNewCount = std::pair<StackLocation, int>;
	using TSummoned = std::map<CreatureID, TQuantity>;
	enum {ERASE = -1};
	const CArmedInstance * army;
	std::vector<TStackAndItsNewCount> newStackCounts;
	std::vector<ArtifactLocation> removedWarMachines;
	TSummoned summoned;
	ObjectInstanceID heroWithDeadCommander; //TODO: unify stack locations

	CasualtiesAfterBattle(const SideInBattle & battleSide, const BattleInfo * bat);
	void updateArmy(CGameHandler *gh);
};

struct FinishingBattleHelper
{
	FinishingBattleHelper();
	FinishingBattleHelper(std::shared_ptr<const CBattleQuery> Query, int RemainingBattleQueriesCount);

	inline bool isDraw() const {return winnerSide == 2;}

	const CGHeroInstance *winnerHero, *loserHero;
	PlayerColor victor, loser;
	ui8 winnerSide;

	int remainingBattleQueriesCount;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & winnerHero;
		h & loserHero;
		h & victor;
		h & loser;
		h & winnerSide;
		h & remainingBattleQueriesCount;
	}
};

using FireShieldInfo = std::vector<std::pair<const CStack *, int64_t>>;

class BattleProcessor : boost::noncopyable
{
	////used only in endBattle - don't touch elsewhere
	bool visitObjectAfterVictory;

	std::unique_ptr<boost::thread> battleThread;
	std::unique_ptr<FinishingBattleHelper> finishingBattle;

	void removeObstacle(const CObstacleInstance &obstacle);
	void makeStackDoNothing(const CStack * next);
	void updateGateState();
	bool makeAutomaticAction(const CStack *stack, BattleAction &ba); //used when action is taken by stack without volition of player (eg. unguided catapult attack)
	void stackEnchantedTrigger(const CStack * stack);
	void stackTurnTrigger(const CStack *stack);
	void engageIntoBattle( PlayerColor player );
	void handleAttackBeforeCasting(bool ranged, const CStack * attacker, const CStack * defender);
	void handleAfterAttackCasting(bool ranged, const CStack * attacker, const CStack * defender);
	void attackCasting(bool ranged, BonusType attackMode, const battle::Unit * attacker, const battle::Unit * defender);

	int moveStack(int stack, BattleHex dest); //returned value - travelled distance
	void runBattle();

	void makeAttack(const CStack * attacker, const CStack * defender, int distance, BattleHex targetHex, bool first, bool ranged, bool counter);

	// damage, drain life & fire shield; returns amount of drained life
	int64_t applyBattleEffects(BattleAttack & bat, std::shared_ptr<battle::CUnitState> attackerState, FireShieldInfo & fireShield, const CStack * def, int distance, bool secondary);

	void sendGenericKilledLog(const CStack * defender, int32_t killed, bool multiple);
	void addGenericKilledLog(BattleLogMessage & blm, const CStack * defender, int32_t killed, bool multiple);

	void checkBattleStateChanges();
	void setupBattle(int3 tile, const CArmedInstance *armies[2], const CGHeroInstance *heroes[2], bool creatureBank, const CGTownInstance *town);
	void setBattleResult(BattleResult::EResult resultType, int victoriusSide);

public:
	CGameHandler * gameHandler;

	BattleProcessor(CGameHandler * gameHandler);
	BattleProcessor();

	~BattleProcessor();

	void startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, const CGTownInstance *town = nullptr); //use hero=nullptr for no hero
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank = false); //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank = false); //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle

	void battleAfterLevelUp(const BattleResult &result);

	bool makeBattleAction(BattleAction &ba);
	bool makeCustomAction(BattleAction &ba);

	void endBattle(int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2); //ends battle
	void endBattleConfirm(const BattleInfo * battleInfo);

	template <typename Handler> void serialize(Handler &h, const int version)
	{

	}

};


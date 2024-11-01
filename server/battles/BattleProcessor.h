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
#include "../../lib/battle/BattleSide.h"

VCMI_LIB_NAMESPACE_BEGIN
class CGHeroInstance;
class CGTownInstance;
class CArmedInstance;
class BattleAction;
class int3;
class CBattleInfoCallback;
struct BattleResult;
struct BattleLayout;
class BattleID;
VCMI_LIB_NAMESPACE_END

class CGameHandler;
class CBattleQuery;
class BattleActionProcessor;
class BattleFlowProcessor;
class BattleResultProcessor;

/// Main class for battle handling. Contains all public interface for battles that is accessible from outside, e.g. for CGameHandler
class BattleProcessor : boost::noncopyable
{
	friend class BattleActionProcessor;
	friend class BattleFlowProcessor;
	friend class BattleResultProcessor;

	CGameHandler * gameHandler;
	std::unique_ptr<BattleActionProcessor> actionsProcessor;
	std::unique_ptr<BattleFlowProcessor> flowProcessor;
	std::unique_ptr<BattleResultProcessor> resultProcessor;

	void updateGateState(const CBattleInfoCallback & battle);
	void engageIntoBattle(PlayerColor player);

	bool checkBattleStateChanges(const CBattleInfoCallback & battle);
	BattleID setupBattle(int3 tile, BattleSideArray<const CArmedInstance *> armies, BattleSideArray<const CGHeroInstance *> heroes, const BattleLayout & layout, const CGTownInstance *town);

	bool makeAutomaticBattleAction(const CBattleInfoCallback & battle, const BattleAction & ba);

	void setBattleResult(const CBattleInfoCallback & battle, EBattleResult resultType, BattleSide victoriusSide);

public:
	explicit BattleProcessor(CGameHandler * gameHandler);
	~BattleProcessor();

	/// Starts battle with specified parameters
	void startBattle(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, const BattleLayout & layout, const CGTownInstance *town);
	/// Starts battle between two armies (which can also be heroes) at position of 2nd object
	void startBattle(const CArmedInstance *army1, const CArmedInstance *army2);
	/// Restart ongoing battle and end previous battle
	void restartBattle(const BattleID & battleID, const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, const BattleLayout & layout, const CGTownInstance *town);

	/// Processing of incoming battle action netpack
	bool makePlayerBattleAction(const BattleID & battleID, PlayerColor player, const BattleAction & ba);

	/// Applies results of a battle once player agrees to them
	void endBattleConfirm(const BattleID & battleID);
	/// Applies results of a battle after potential levelup
	void battleAfterLevelUp(const BattleID & battleID, const BattleResult & result);

	template <typename Handler> void serialize(Handler &h)
	{

	}
};


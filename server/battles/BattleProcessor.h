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

VCMI_LIB_NAMESPACE_BEGIN
class CGHeroInstance;
class CGTownInstance;
class CArmedInstance;
class BattleAction;
class int3;
class BattleInfo;
struct BattleResult;
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

	void updateGateState();
	void engageIntoBattle(PlayerColor player);

	bool checkBattleStateChanges();
	void setupBattle(int3 tile, const CArmedInstance *armies[2], const CGHeroInstance *heroes[2], bool creatureBank, const CGTownInstance *town);

	bool makeBattleAction(const BattleAction & ba);

	void setBattleResult(EBattleResult resultType, int victoriusSide);

public:
	explicit BattleProcessor(CGameHandler * gameHandler);
	BattleProcessor();
	~BattleProcessor();

	void setGameHandler(CGameHandler * gameHandler);

	/// Starts battle with specified parameters
	void startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, const CGTownInstance *town = nullptr);
	/// Starts battle between two armies (which can also be heroes) at specified tile
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank = false);
	/// Starts battle between two armies (which can also be heroes) at position of 2nd object
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank = false);

	/// Processing of incoming battle action netpack
	bool makeBattleAction(PlayerColor player, BattleAction & ba);

	/// Applies results of a battle once player agrees to them
	void endBattleConfirm(const BattleInfo * battleInfo);
	/// Applies results of a battle after potential levelup
	void battleAfterLevelUp(const BattleResult & result);

	template <typename Handler> void serialize(Handler &h, const int version)
	{

	}

};


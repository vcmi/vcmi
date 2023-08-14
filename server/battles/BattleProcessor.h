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
	void engageIntoBattle( PlayerColor player );

	void checkBattleStateChanges();
	void setupBattle(int3 tile, const CArmedInstance *armies[2], const CGHeroInstance *heroes[2], bool creatureBank, const CGTownInstance *town);

	bool makeBattleAction(BattleAction &ba);
	bool makeCustomAction(BattleAction &ba);

	void setBattleResult(EBattleResult resultType, int victoriusSide);
public:
	explicit BattleProcessor(CGameHandler * gameHandler);
	BattleProcessor();
	~BattleProcessor();

	void setGameHandler(CGameHandler * gameHandler);

	void startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank = false, const CGTownInstance *town = nullptr); //use hero=nullptr for no hero
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank = false); //if any of armies is hero, hero will be used
	void startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank = false); //if any of armies is hero, hero will be used, visitable tile of second obj is place of battle

	bool makeBattleAction(PlayerColor player, BattleAction &ba);
	bool makeCustomAction(PlayerColor player, BattleAction &ba);

	void endBattle(int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2); //ends battle
	void endBattleConfirm(const BattleInfo * battleInfo);
	void battleAfterLevelUp(const BattleResult &result);

	template <typename Handler> void serialize(Handler &h, const int version)
	{

	}

};


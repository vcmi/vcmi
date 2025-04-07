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
#include "../../lib/networkPacks/StackLocation.h"
#include "../../lib/networkPacks/ArtifactLocation.h"
#include "../../lib/battle/BattleSide.h"

VCMI_LIB_NAMESPACE_BEGIN
struct SideInBattle;
struct BattleResult;
class CBattleInfoCallback;
class CGHeroInstance;
class CArmedInstance;
VCMI_LIB_NAMESPACE_END

class CBattleQuery;
class BattleProcessor;
class CGameHandler;

struct CasualtiesAfterBattle
{
	using TStackAndItsNewCount = std::pair<StackLocation, int>;
	using TSummoned = std::map<CreatureID, TQuantity>;
	const CArmedInstance * army;
	std::vector<TStackAndItsNewCount> newStackCounts;
	std::vector<ArtifactLocation> removedWarMachines;
	TSummoned summoned;
	ObjectInstanceID heroWithDeadCommander; //TODO: unify stack locations

	CasualtiesAfterBattle(const CBattleInfoCallback & battle, BattleSide sideInBattle);
	void updateArmy(CGameHandler * gh);
};

struct FinishingBattleHelper
{
	FinishingBattleHelper(const CBattleInfoCallback & battle, const BattleResult & result, int RemainingBattleQueriesCount);

	inline bool isDraw() const {return winnerSide == BattleSide::NONE;}

	ObjectInstanceID winnerId;
	ObjectInstanceID loserId;
	PlayerColor victor, loser;
	BattleSide winnerSide;

	int remainingBattleQueriesCount;

	template <typename Handler> void serialize(Handler &h)
	{
		h & winnerId;
		h & loserId;
		h & victor;
		h & loser;
		h & winnerSide;
		h & remainingBattleQueriesCount;
	}
};

class BattleResultProcessor : boost::noncopyable
{
	//	BattleProcessor * owner;
	CGameHandler * gameHandler;

	std::map<BattleID, std::unique_ptr<BattleResult>> battleResults;
	std::map<BattleID, std::unique_ptr<FinishingBattleHelper>> finishingBattles;

public:
	explicit BattleResultProcessor(BattleProcessor * owner, CGameHandler * newGameHandler);

	bool battleIsEnding(const CBattleInfoCallback & battle) const;

	void setBattleResult(const CBattleInfoCallback & battle, EBattleResult resultType, BattleSide victoriusSide);
	void endBattle(const CBattleInfoCallback & battle); //ends battle
	void endBattleConfirm(const CBattleInfoCallback & battle);
	void battleFinalize(const BattleID & battleID, const BattleResult & result);
};

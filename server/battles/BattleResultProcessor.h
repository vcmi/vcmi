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
struct SideInBattle;
VCMI_LIB_NAMESPACE_END

class CBattleQuery;
class BattleProcessor;
class CGameHandler;

struct CasualtiesAfterBattle
{
	using TStackAndItsNewCount = std::pair<StackLocation, int>;
	using TSummoned = std::map<CreatureID, TQuantity>;
	//	enum {ERASE = -1};
	const CArmedInstance * army;
	std::vector<TStackAndItsNewCount> newStackCounts;
	std::vector<ArtifactLocation> removedWarMachines;
	TSummoned summoned;
	ObjectInstanceID heroWithDeadCommander; //TODO: unify stack locations

	CasualtiesAfterBattle(const SideInBattle & battleSide, const BattleInfo * bat);
	void updateArmy(CGameHandler * gh);
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

class BattleResultProcessor : boost::noncopyable
{
	//	BattleProcessor * owner;
	CGameHandler * gameHandler;

	std::unique_ptr<BattleResult> battleResult;
	std::unique_ptr<FinishingBattleHelper> finishingBattle;

public:
	explicit BattleResultProcessor(BattleProcessor * owner);
	void setGameHandler(CGameHandler * newGameHandler);

	bool battleIsEnding() const;

	void setupBattle();
	void setBattleResult(EBattleResult resultType, int victoriusSide);
	void endBattle(int3 tile, const CGHeroInstance * hero1, const CGHeroInstance * hero2); //ends battle
	void endBattleConfirm(const BattleInfo * battleInfo);
	void battleAfterLevelUp(const BattleResult & result);
};

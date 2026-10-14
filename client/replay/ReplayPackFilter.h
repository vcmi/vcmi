/*
 * ReplayPackFilter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/networkPacks/NetPackVisitor.h"

enum class EReplayPackKind : uint8_t
{
	/// Pack that only changes gamestate and/or plays adventure map animations - shown as is
	REGULAR,

	/// Pack that opens a dialog, expects an answer from the player or manipulates live session state.
	/// Applied on gamestate, but its client-side handler is skipped during a replay.
	INTERACTIVE,

	/// Pack that belongs to a combat. Its client-side handler runs only if the player asked to watch battles.
	BATTLE,
};

/// Decides how a recorded netpack has to be treated while it is being replayed
class ReplayPackFilter final : public ICPackVisitor
{
	EReplayPackKind kindValue = EReplayPackKind::REGULAR;

	void markInteractive()
	{
		kindValue = EReplayPackKind::INTERACTIVE;
	}

	void markBattle()
	{
		kindValue = EReplayPackKind::BATTLE;
	}

	// packs that would ask the player for input, or that would answer a request that we never sent
	void visitPackageApplied(PackageApplied & pack) override { markInteractive(); }
	void visitPackageReceived(PackageReceived & pack) override { markInteractive(); }
	void visitQueryResolved(QueryResolved & pack) override { markInteractive(); }
	void visitPlayerStartsTurn(PlayerStartsTurn & pack) override { markInteractive(); }
	void visitPlayerEndsGame(PlayerEndsGame & pack) override { markInteractive(); }
	void visitPlayerBlocked(PlayerBlocked & pack) override { markInteractive(); }
	void visitPlayerCheated(PlayerCheated & pack) override { markInteractive(); }
	void visitTurnTimeUpdate(TurnTimeUpdate & pack) override { markInteractive(); }
	void visitGamePause(GamePause & pack) override { markInteractive(); }
	void visitInfoWindow(InfoWindow & pack) override { markInteractive(); }
	void visitOpenWindow(OpenWindow & pack) override { markInteractive(); }
	void visitHeroLevelUp(HeroLevelUp & pack) override { markInteractive(); }
	void visitCommanderLevelUp(CommanderLevelUp & pack) override { markInteractive(); }
	void visitBlockingDialog(BlockingDialog & pack) override { markInteractive(); }
	void visitGarrisonDialog(GarrisonDialog & pack) override { markInteractive(); }
	void visitExchangeDialog(ExchangeDialog & pack) override { markInteractive(); }
	void visitTeleportDialog(TeleportDialog & pack) override { markInteractive(); }
	void visitMapObjectSelectDialog(MapObjectSelectDialog & pack) override { markInteractive(); }
	void visitShowWorldViewEx(ShowWorldViewEx & pack) override { markInteractive(); }
	void visitResponseStatistic(ResponseStatistic & pack) override { markInteractive(); }
	void visitAdvInterfaceReady(AdvInterfaceReady & pack) override { markInteractive(); }
	void visitSystemMessage(SystemMessage & pack) override { markInteractive(); }
	void visitPlayerMessageClient(PlayerMessageClient & pack) override { markInteractive(); }

	// combat packs - either all of them are shown, or none of them, so that the battle interface
	// is never left half-initialized
	void visitBattleStart(BattleStart & pack) override { markBattle(); }
	void visitBattleNextRound(BattleNextRound & pack) override { markBattle(); }
	void visitBattleSetActiveStack(BattleSetActiveStack & pack) override { markBattle(); }
	void visitBattleResult(BattleResult & pack) override { markBattle(); }
	void visitBattleResultAccepted(BattleResultAccepted & pack) override { markBattle(); }
	void visitBattleCancelled(BattleCancelled & pack) override { markBattle(); }
	void visitBattleLogMessage(BattleLogMessage & pack) override { markBattle(); }
	void visitBattleStackMoved(BattleStackMoved & pack) override { markBattle(); }
	void visitBattleUnitsChanged(BattleUnitsChanged & pack) override { markBattle(); }
	void visitBattleAttack(BattleAttack & pack) override { markBattle(); }
	void visitStartAction(StartAction & pack) override { markBattle(); }
	void visitEndAction(EndAction & pack) override { markBattle(); }
	void visitBattleSpellCast(BattleSpellCast & pack) override { markBattle(); }
	void visitSetStackEffect(SetStackEffect & pack) override { markBattle(); }
	void visitStacksInjured(StacksInjured & pack) override { markBattle(); }
	void visitBattleResultsApplied(BattleResultsApplied & pack) override { markBattle(); }
	void visitBattleEnded(BattleEnded & pack) override { markBattle(); }
	void visitBattleObstaclesChanged(BattleObstaclesChanged & pack) override { markBattle(); }
	void visitBattleSetStackProperty(BattleSetStackProperty & pack) override { markBattle(); }
	void visitBattleTriggerEffect(BattleTriggerEffect & pack) override { markBattle(); }
	void visitBattleUpdateGateState(BattleUpdateGateState & pack) override { markBattle(); }
	void visitCatapultAttack(CatapultAttack & pack) override { markBattle(); }

public:
	EReplayPackKind kind() const
	{
		return kindValue;
	}

	static EReplayPackKind classify(CPackForClient & pack)
	{
		ReplayPackFilter filter;
		pack.visit(filter);
		return filter.kind();
	}
};

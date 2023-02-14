/*
 * ClientNetPackVisitors.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/NetPackVisitor.h"

class CClient;

VCMI_LIB_NAMESPACE_BEGIN

class CGameState;

VCMI_LIB_NAMESPACE_END

class ApplyClientNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	CClient & cl;
	CGameState & gs;

public:
	ApplyClientNetPackVisitor(CClient & cl, CGameState & gs)
		:cl(cl), gs(gs)
	{
	}

	void visitSetResources(SetResources & pack) override;
	void visitSetPrimSkill(SetPrimSkill & pack) override;
	void visitSetSecSkill(SetSecSkill & pack) override;
	void visitHeroVisitCastle(HeroVisitCastle & pack) override;
	void visitSetMana(SetMana & pack) override;
	void visitSetMovePoints(SetMovePoints & pack) override;
	void visitFoWChange(FoWChange & pack) override;
	void visitChangeStackCount(ChangeStackCount & pack) override;
	void visitSetStackType(SetStackType & pack) override;
	void visitEraseStack(EraseStack & pack) override;
	void visitSwapStacks(SwapStacks & pack) override;
	void visitInsertNewStack(InsertNewStack & pack) override;
	void visitRebalanceStacks(RebalanceStacks & pack) override;
	void visitBulkRebalanceStacks(BulkRebalanceStacks & pack) override;
	void visitBulkSmartRebalanceStacks(BulkSmartRebalanceStacks & pack) override;
	void visitPutArtifact(PutArtifact & pack) override;
	void visitEraseArtifact(EraseArtifact & pack) override;
	void visitMoveArtifact(MoveArtifact & pack) override;
	void visitBulkMoveArtifacts(BulkMoveArtifacts & pack) override;
	void visitAssembledArtifact(AssembledArtifact & pack) override;
	void visitDisassembledArtifact(DisassembledArtifact & pack) override;
	void visitHeroVisit(HeroVisit & pack) override;
	void visitNewTurn(NewTurn & pack) override;
	void visitGiveBonus(GiveBonus & pack) override;
	void visitChangeObjPos(ChangeObjPos & pack) override;
	void visitPlayerEndsGame(PlayerEndsGame & pack) override;
	void visitPlayerReinitInterface(PlayerReinitInterface & pack) override;
	void visitRemoveBonus(RemoveBonus & pack) override;
	void visitRemoveObject(RemoveObject & pack) override;
	void visitTryMoveHero(TryMoveHero & pack) override;
	void visitNewStructures(NewStructures & pack) override;
	void visitRazeStructures(RazeStructures & pack) override;
	void visitSetAvailableCreatures(SetAvailableCreatures & pack) override;
	void visitSetHeroesInTown(SetHeroesInTown & pack) override;
	void visitHeroRecruited(HeroRecruited & pack) override;
	void visitGiveHero(GiveHero & pack) override;
	void visitInfoWindow(InfoWindow & pack) override;
	void visitSetObjectProperty(SetObjectProperty & pack) override;
	void visitHeroLevelUp(HeroLevelUp & pack) override;
	void visitCommanderLevelUp(CommanderLevelUp & pack) override;
	void visitBlockingDialog(BlockingDialog & pack) override;
	void visitGarrisonDialog(GarrisonDialog & pack) override;
	void visitExchangeDialog(ExchangeDialog & pack) override;
	void visitTeleportDialog(TeleportDialog & pack) override;
	void visitMapObjectSelectDialog(MapObjectSelectDialog & pack) override;
	void visitBattleStart(BattleStart & pack) override;
	void visitBattleNextRound(BattleNextRound & pack) override;
	void visitBattleSetActiveStack(BattleSetActiveStack & pack) override;
	void visitBattleLogMessage(BattleLogMessage & pack) override;
	void visitBattleTriggerEffect(BattleTriggerEffect & pack) override;
	void visitBattleAttack(BattleAttack & pack) override;
	void visitBattleSpellCast(BattleSpellCast & pack) override;
	void visitSetStackEffect(SetStackEffect & pack) override;
	void visitStacksInjured(StacksInjured & pack) override;
	void visitBattleResultsApplied(BattleResultsApplied & pack) override;
	void visitBattleUnitsChanged(BattleUnitsChanged & pack) override;
	void visitBattleObstaclesChanged(BattleObstaclesChanged & pack) override;
	void visitCatapultAttack(CatapultAttack & pack) override;
	void visitEndAction(EndAction & pack) override;
	void visitPackageApplied(PackageApplied & pack) override;
	void visitSystemMessage(SystemMessage & pack) override;
	void visitPlayerBlocked(PlayerBlocked & pack) override;
	void visitYourTurn(YourTurn & pack) override;
	void visitSaveGameClient(SaveGameClient & pack) override;
	void visitPlayerMessageClient(PlayerMessageClient & pack) override;
	void visitShowInInfobox(ShowInInfobox & pack) override;
	void visitAdvmapSpellCast(AdvmapSpellCast & pack) override;
	void visitShowWorldViewEx(ShowWorldViewEx & pack) override;	
	void visitOpenWindow(OpenWindow & pack) override;
	void visitCenterView(CenterView & pack) override;
	void visitNewObject(NewObject & pack) override;
	void visitSetAvailableArtifacts(SetAvailableArtifacts & pack) override;
	void visitEntitiesChanged(EntitiesChanged & pack) override;
};

class ApplyFirstClientNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	CClient & cl;
	CGameState & gs;

public:
	ApplyFirstClientNetPackVisitor(CClient & cl, CGameState & gs)
		:cl(cl), gs(gs)
	{
	}

	virtual void visitChangeObjPos(ChangeObjPos & pack) override;
	virtual void visitRemoveObject(RemoveObject & pack) override;
	virtual void visitTryMoveHero(TryMoveHero & pack) override;
	virtual void visitGiveHero(GiveHero & pack) override;
	virtual void visitBattleStart(BattleStart & pack) override;
	virtual void visitBattleNextRound(BattleNextRound & pack) override;
	virtual void visitBattleUpdateGateState(BattleUpdateGateState & pack) override;
	virtual void visitBattleResult(BattleResult & pack) override;
	virtual void visitBattleStackMoved(BattleStackMoved & pack) override;
	virtual void visitBattleAttack(BattleAttack & pack) override;
	virtual void visitStartAction(StartAction & pack) override;
};
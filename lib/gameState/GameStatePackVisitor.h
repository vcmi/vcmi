/*
 * GameStatePackVisitor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../networkPacks/NetPackVisitor.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGameState;

class GameStatePackVisitor final : public ICPackVisitor
{
	void restorePreBattleState(BattleID battleID);
private:
	CGameState & gs;

public:
	GameStatePackVisitor(CGameState & gs)
		: gs(gs)
	{
	}

	void visitSetResources(SetResources & pack) override;
	void visitSetPrimarySkill(SetPrimarySkill & pack) override;
	void visitSetHeroExperience(SetHeroExperience & pack) override;
	void visitGiveStackExperience(GiveStackExperience & pack) override;
	void visitSetSecSkill(SetSecSkill & pack) override;
	void visitHeroVisitCastle(HeroVisitCastle & pack) override;
	void visitSetMana(SetMana & pack) override;
	void visitSetMovePoints(SetMovePoints & pack) override;
	void visitSetResearchedSpells(SetResearchedSpells & pack) override;
	void visitFoWChange(FoWChange & pack) override;
	void visitChangeStackCount(ChangeStackCount & pack) override;
	void visitSetStackType(SetStackType & pack) override;
	void visitEraseStack(EraseStack & pack) override;
	void visitSwapStacks(SwapStacks & pack) override;
	void visitInsertNewStack(InsertNewStack & pack) override;
	void visitRebalanceStacks(RebalanceStacks & pack) override;
	void visitBulkRebalanceStacks(BulkRebalanceStacks & pack) override;
	void visitGrowUpArtifact(GrowUpArtifact & pack) override;
	void visitPutArtifact(PutArtifact & pack) override;
	void visitBulkEraseArtifacts(BulkEraseArtifacts & pack) override;
	void visitBulkMoveArtifacts(BulkMoveArtifacts & pack) override;
	void visitAssembledArtifact(AssembledArtifact & pack) override;
	void visitDisassembledArtifact(DisassembledArtifact & pack) override;
	void visitDischargeArtifact(DischargeArtifact & pack) override;
	void visitHeroVisit(HeroVisit & pack) override;
	void visitNewTurn(NewTurn & pack) override;
	void visitGiveBonus(GiveBonus & pack) override;
	void visitChangeObjPos(ChangeObjPos & pack) override;
	void visitPlayerEndsTurn(PlayerEndsTurn & pack) override;
	void visitPlayerEndsGame(PlayerEndsGame & pack) override;
	void visitRemoveBonus(RemoveBonus & pack) override;
	void visitRemoveObject(RemoveObject & pack) override;
	void visitTryMoveHero(TryMoveHero & pack) override;
	void visitNewStructures(NewStructures & pack) override;
	void visitRazeStructures(RazeStructures & pack) override;
	void visitSetAvailableCreatures(SetAvailableCreatures & pack) override;
	void visitSetHeroesInTown(SetHeroesInTown & pack) override;
	void visitHeroRecruited(HeroRecruited & pack) override;
	void visitGiveHero(GiveHero & pack) override;
	void visitSetObjectProperty(SetObjectProperty & pack) override;
	void visitHeroLevelUp(HeroLevelUp & pack) override;
	void visitCommanderLevelUp(CommanderLevelUp & pack) override;
	void visitBattleStart(BattleStart & pack) override;
	void visitBattleSetActiveStack(BattleSetActiveStack & pack) override;
	void visitBattleTriggerEffect(BattleTriggerEffect & pack) override;
	void visitBattleAttack(BattleAttack & pack) override;
	void visitBattleSpellCast(BattleSpellCast & pack) override;
	void visitSetStackEffect(SetStackEffect & pack) override;
	void visitStacksInjured(StacksInjured & pack) override;
	void visitBattleUnitsChanged(BattleUnitsChanged & pack) override;
	void visitBattleObstaclesChanged(BattleObstaclesChanged & pack) override;
	void visitBattleStackMoved(BattleStackMoved & pack) override;
	void visitCatapultAttack(CatapultAttack & pack) override;
	void visitPlayerStartsTurn(PlayerStartsTurn & pack) override;
	void visitNewObject(NewObject & pack) override;
	void visitSetAvailableArtifacts(SetAvailableArtifacts & pack) override;
	void visitEntitiesChanged(EntitiesChanged & pack) override;
	void visitSetCommanderProperty(SetCommanderProperty & pack) override;
	void visitAddQuest(AddQuest & pack) override;
	void visitChangeFormation(ChangeFormation & pack) override;
	void visitChangeTownName(ChangeTownName & pack) override;
	void visitChangeSpells(ChangeSpells & pack) override;
	void visitSetAvailableHero(SetAvailableHero & pack) override;
	void visitChangeObjectVisitors(ChangeObjectVisitors & pack) override;
	void visitChangeArtifactsCostume(ChangeArtifactsCostume & pack) override;
	void visitNewArtifact(NewArtifact & pack) override;
	void visitBattleUpdateGateState(BattleUpdateGateState & pack) override;
	void visitPlayerCheated(PlayerCheated & pack) override;
	void visitDaysWithoutTown(DaysWithoutTown & pack) override;
	void visitStartAction(StartAction & pack) override;
	void visitSetRewardableConfiguration(SetRewardableConfiguration & pack) override;
	void visitBattleSetStackProperty(BattleSetStackProperty & pack) override;
	void visitBattleNextRound(BattleNextRound & pack) override;
	void visitBattleCancelled(BattleCancelled & pack) override;
	void visitBattleResultsApplied(BattleResultsApplied & pack) override;
	void visitBattleEnded(BattleEnded & pack) override;
	void visitBattleResultAccepted(BattleResultAccepted & pack) override;
	void visitTurnTimeUpdate(TurnTimeUpdate & pack) override;
};

class DLL_LINKAGE BattleStatePackVisitor final : public ICPackVisitor
{
	IBattleState & battleState;
public:
	BattleStatePackVisitor(IBattleState & battleState)
		:battleState(battleState)
	{}

	void visitSetStackEffect(SetStackEffect & pack) override;
	void visitStacksInjured(StacksInjured & pack) override;
	void visitBattleUnitsChanged(BattleUnitsChanged & pack) override;
	void visitBattleObstaclesChanged(BattleObstaclesChanged & pack) override;
	void visitCatapultAttack(CatapultAttack & pack) override;
	void visitBattleStackMoved(BattleStackMoved & pack) override;
};

VCMI_LIB_NAMESPACE_END

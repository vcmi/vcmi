/*
 * CServerHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "NetPacks.h"
#include "NetPacksLobby.h"

VCMI_LIB_NAMESPACE_BEGIN

class ICPackVisitor
{
public:
	virtual bool callTyped() { return true; }

	virtual void visitForLobby(CPackForLobby & pack) {}
	virtual void visitForServer(CPackForServer & pack) {}
	virtual void visitForClient(CPackForClient & pack) {}
	virtual void visitPackageApplied(PackageApplied & pack) {}
	virtual void visitSystemMessage(SystemMessage & pack) {}
	virtual void visitPlayerBlocked(PlayerBlocked & pack) {}
	virtual void visitPlayerCheated(PlayerCheated & pack) {}
	virtual void visitYourTurn(YourTurn & pack) {}
	virtual void visitTurnTimeUpdate(TurnTimeUpdate & pack) {}
	virtual void visitEntitiesChanged(EntitiesChanged & pack) {}
	virtual void visitSetResources(SetResources & pack) {}
	virtual void visitSetPrimSkill(SetPrimSkill & pack) {}
	virtual void visitSetSecSkill(SetSecSkill & pack) {}
	virtual void visitHeroVisitCastle(HeroVisitCastle & pack) {}
	virtual void visitChangeSpells(ChangeSpells & pack) {}
	virtual void visitSetMana(SetMana & pack) {}
	virtual void visitSetMovePoints(SetMovePoints & pack) {}
	virtual void visitFoWChange(FoWChange & pack) {}
	virtual void visitSetAvailableHeroes(SetAvailableHero & pack) {}
	virtual void visitGiveBonus(GiveBonus & pack) {}
	virtual void visitChangeObjPos(ChangeObjPos & pack) {}
	virtual void visitPlayerEndsGame(PlayerEndsGame & pack) {}
	virtual void visitPlayerReinitInterface(PlayerReinitInterface & pack) {}
	virtual void visitRemoveBonus(RemoveBonus & pack) {}
	virtual void visitSetCommanderProperty(SetCommanderProperty & pack) {}
	virtual void visitAddQuest(AddQuest & pack) {}
	virtual void visitUpdateArtHandlerLists(UpdateArtHandlerLists & pack) {}
	virtual void visitUpdateMapEvents(UpdateMapEvents & pack) {}
	virtual void visitUpdateCastleEvents(UpdateCastleEvents & pack) {}
	virtual void visitChangeFormation(ChangeFormation & pack) {}
	virtual void visitRemoveObject(RemoveObject & pack) {}
	virtual void visitTryMoveHero(TryMoveHero & pack) {}
	virtual void visitNewStructures(NewStructures & pack) {}
	virtual void visitRazeStructures(RazeStructures & pack) {}
	virtual void visitSetAvailableCreatures(SetAvailableCreatures & pack) {}
	virtual void visitSetHeroesInTown(SetHeroesInTown & pack) {}
	virtual void visitHeroRecruited(HeroRecruited & pack) {}
	virtual void visitGiveHero(GiveHero & pack) {}
	virtual void visitCatapultAttack(CatapultAttack & pack) {}
	virtual void visitOpenWindow(OpenWindow & pack) {}
	virtual void visitNewObject(NewObject & pack) {}
	virtual void visitSetAvailableArtifacts(SetAvailableArtifacts & pack) {}
	virtual void visitNewArtifact(NewArtifact & pack) {}
	virtual void visitChangeStackCount(ChangeStackCount & pack) {}
	virtual void visitSetStackType(SetStackType & pack) {}
	virtual void visitEraseStack(EraseStack & pack) {}
	virtual void visitSwapStacks(SwapStacks & pack) {}
	virtual void visitInsertNewStack(InsertNewStack & pack) {}
	virtual void visitRebalanceStacks(RebalanceStacks & pack) {}
	virtual void visitBulkRebalanceStacks(BulkRebalanceStacks & pack) {}
	virtual void visitBulkSmartRebalanceStacks(BulkSmartRebalanceStacks & pack) {}
	virtual void visitPutArtifact(PutArtifact & pack) {}
	virtual void visitEraseArtifact(EraseArtifact & pack) {}
	virtual void visitMoveArtifact(MoveArtifact & pack) {}
	virtual void visitBulkMoveArtifacts(BulkMoveArtifacts & pack) {}
	virtual void visitAssembledArtifact(AssembledArtifact & pack) {}
	virtual void visitDisassembledArtifact(DisassembledArtifact & pack) {}
	virtual void visitHeroVisit(HeroVisit & pack) {}
	virtual void visitNewTurn(NewTurn & pack) {}
	virtual void visitInfoWindow(InfoWindow & pack) {}
	virtual void visitSetObjectProperty(SetObjectProperty & pack) {}
	virtual void visitChangeObjectVisitors(ChangeObjectVisitors & pack) {}
	virtual void visitPrepareHeroLevelUp(PrepareHeroLevelUp & pack) {}
	virtual void visitHeroLevelUp(HeroLevelUp & pack) {}
	virtual void visitCommanderLevelUp(CommanderLevelUp & pack) {}
	virtual void visitBlockingDialog(BlockingDialog & pack) {}
	virtual void visitGarrisonDialog(GarrisonDialog & pack) {}
	virtual void visitExchangeDialog(ExchangeDialog & pack) {}
	virtual void visitTeleportDialog(TeleportDialog & pack) {}
	virtual void visitMapObjectSelectDialog(MapObjectSelectDialog & pack) {}
	virtual void visitBattleStart(BattleStart & pack) {}
	virtual void visitBattleNextRound(BattleNextRound & pack) {}
	virtual void visitBattleSetActiveStack(BattleSetActiveStack & pack) {}
	virtual void visitBattleResult(BattleResult & pack) {}
	virtual void visitBattleLogMessage(BattleLogMessage & pack) {}
	virtual void visitBattleStackMoved(BattleStackMoved & pack) {}
	virtual void visitBattleUnitsChanged(BattleUnitsChanged & pack) {}
	virtual void visitBattleAttack(BattleAttack & pack) {}
	virtual void visitStartAction(StartAction & pack) {}
	virtual void visitEndAction(EndAction & pack) {}
	virtual void visitBattleSpellCast(BattleSpellCast & pack) {}
	virtual void visitSetStackEffect(SetStackEffect & pack) {}
	virtual void visitStacksInjured(StacksInjured & pack) {}
	virtual void visitBattleResultsApplied(BattleResultsApplied & pack) {}
	virtual void visitBattleObstaclesChanged(BattleObstaclesChanged & pack) {}
	virtual void visitBattleSetStackProperty(BattleSetStackProperty & pack) {}
	virtual void visitBattleTriggerEffect(BattleTriggerEffect & pack) {}
	virtual void visitBattleUpdateGateState(BattleUpdateGateState & pack) {}
	virtual void visitAdvmapSpellCast(AdvmapSpellCast & pack) {}
	virtual void visitShowWorldViewEx(ShowWorldViewEx & pack) {}
	virtual void visitEndTurn(EndTurn & pack) {}
	virtual void visitDismissHero(DismissHero & pack) {}
	virtual void visitMoveHero(MoveHero & pack) {}
	virtual void visitCastleTeleportHero(CastleTeleportHero & pack) {}
	virtual void visitArrangeStacks(ArrangeStacks & pack) {}
	virtual void visitBulkMoveArmy(BulkMoveArmy & pack) {}
	virtual void visitBulkSplitStack(BulkSplitStack & pack) {}
	virtual void visitBulkMergeStacks(BulkMergeStacks & pack) {}
	virtual void visitBulkSmartSplitStack(BulkSmartSplitStack & pack) {}
	virtual void visitDisbandCreature(DisbandCreature & pack) {}
	virtual void visitBuildStructure(BuildStructure & pack) {}
	virtual void visitRazeStructure(RazeStructure & pack) {}
	virtual void visitRecruitCreatures(RecruitCreatures & pack) {}
	virtual void visitUpgradeCreature(UpgradeCreature & pack) {}
	virtual void visitGarrisonHeroSwap(GarrisonHeroSwap & pack) {}
	virtual void visitExchangeArtifacts(ExchangeArtifacts & pack) {}
	virtual void visitBulkExchangeArtifacts(BulkExchangeArtifacts & pack) {}
	virtual void visitAssembleArtifacts(AssembleArtifacts & pack) {}
	virtual void visitEraseArtifactByClient(EraseArtifactByClient & pack) {}
	virtual void visitBuyArtifact(BuyArtifact & pack) {}
	virtual void visitTradeOnMarketplace(TradeOnMarketplace & pack) {}
	virtual void visitSetFormation(SetFormation & pack) {}
	virtual void visitHireHero(HireHero & pack) {}
	virtual void visitBuildBoat(BuildBoat & pack) {}
	virtual void visitQueryReply(QueryReply & pack) {}
	virtual void visitMakeAction(MakeAction & pack) {}
	virtual void visitDigWithHero(DigWithHero & pack) {}
	virtual void visitCastAdvSpell(CastAdvSpell & pack) {}
	virtual void visitSaveGame(SaveGame & pack) {}
	virtual void visitPlayerMessage(PlayerMessage & pack) {}
	virtual void visitPlayerMessageClient(PlayerMessageClient & pack) {}
	virtual void visitCenterView(CenterView & pack) {}
	virtual void visitLobbyClientConnected(LobbyClientConnected & pack) {}
	virtual void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) {}
	virtual void visitLobbyChatMessage(LobbyChatMessage & pack) {}
	virtual void visitLobbyGuiAction(LobbyGuiAction & pack) {}
	virtual void visitLobbyLoadProgress(LobbyLoadProgress & pack) {}
	virtual void visitLobbyEndGame(LobbyEndGame & pack) {}
	virtual void visitLobbyStartGame(LobbyStartGame & pack) {}
	virtual void visitLobbyChangeHost(LobbyChangeHost & pack) {}
	virtual void visitLobbyUpdateState(LobbyUpdateState & pack) {}
	virtual void visitLobbySetMap(LobbySetMap & pack) {}
	virtual void visitLobbySetCampaign(LobbySetCampaign & pack) {}
	virtual void visitLobbySetCampaignMap(LobbySetCampaignMap & pack) {}
	virtual void visitLobbySetCampaignBonus(LobbySetCampaignBonus & pack) {}
	virtual void visitLobbyChangePlayerOption(LobbyChangePlayerOption & pack) {}
	virtual void visitLobbySetPlayer(LobbySetPlayer & pack) {}
	virtual void visitLobbySetTurnTime(LobbySetTurnTime & pack) {}
	virtual void visitLobbySetDifficulty(LobbySetDifficulty & pack) {}
	virtual void visitLobbyForceSetPlayer(LobbyForceSetPlayer & pack) {}
	virtual void visitLobbyShowMessage(LobbyShowMessage & pack) {}
};

VCMI_LIB_NAMESPACE_END

/*
 * NetPacksLib.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "PacksForClient.h"
#include "PacksForClientBattle.h"
#include "PacksForServer.h"
#include "SaveLocalState.h"
#include "SetRewardableConfiguration.h"
#include "PacksForLobby.h"
#include "SetStackEffect.h"
#include "NetPackVisitor.h"

VCMI_LIB_NAMESPACE_BEGIN

void CPack::visit(ICPackVisitor & visitor)
{
	visitBasic(visitor);

	// visitBasic may destroy this and in such cases we do not want to call visitTyped
	if(visitor.callTyped())
	{
		visitTyped(visitor);
	}
}

void CPack::visitBasic(ICPackVisitor & visitor)
{
}

void CPack::visitTyped(ICPackVisitor & visitor)
{
	throw std::runtime_error(std::string("CPack::visitTyped called for class ") + typeid(*this).name());
}

void CPackForClient::visitBasic(ICPackVisitor & visitor)
{
	visitor.visitForClient(*this);
}

void CPackForServer::visitBasic(ICPackVisitor & visitor)
{
	visitor.visitForServer(*this);
}

void CPackForLobby::visitBasic(ICPackVisitor & visitor)
{
	visitor.visitForLobby(*this);
}

bool CPackForLobby::isForServer() const
{
	return false;
}

bool CLobbyPackToServer::isForServer() const
{
	return true;
}

void SaveLocalState::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSaveLocalState(*this);
}

void PackageApplied::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPackageApplied(*this);
}

void PackageReceived::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPackageReceived(*this);
}

void SystemMessage::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSystemMessage(*this);
}

void PlayerBlocked::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPlayerBlocked(*this);
}

void PlayerCheated::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPlayerCheated(*this);
}

void PlayerStartsTurn::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPlayerStartsTurn(*this);
}

void DaysWithoutTown::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitDaysWithoutTown(*this);
}

void EntitiesChanged::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitEntitiesChanged(*this);
}

void SetRewardableConfiguration::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetRewardableConfiguration(*this);
}

void SetResources::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetResources(*this);
}

void SetPrimarySkill::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetPrimarySkill(*this);
}

void SetHeroExperience::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetHeroExperience(*this);
}

void GiveStackExperience::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitGiveStackExperience(*this);
}

void SetSecSkill::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetSecSkill(*this);
}

void HeroVisitCastle::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitHeroVisitCastle(*this);
}

void ChangeSpells::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitChangeSpells(*this);
}

void SetResearchedSpells::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetResearchedSpells(*this);
}
void SetMana::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetMana(*this);
}

void SetMovePoints::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetMovePoints(*this);
}

void FoWChange::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitFoWChange(*this);
}

void SetAvailableHero::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetAvailableHero(*this);
}

void GiveBonus::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitGiveBonus(*this);
}

void ChangeObjPos::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitChangeObjPos(*this);
}

void PlayerEndsTurn::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPlayerEndsTurn(*this);
}

void PlayerEndsGame::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPlayerEndsGame(*this);
}

void RemoveBonus::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitRemoveBonus(*this);
}

void SetCommanderProperty::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetCommanderProperty(*this);
}

void AddQuest::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitAddQuest(*this);
}

void ChangeFormation::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitChangeFormation(*this);
}

void RemoveObject::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitRemoveObject(*this);
}

void TryMoveHero::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitTryMoveHero(*this);
}

void NewStructures::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitNewStructures(*this);
}

void RazeStructures::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitRazeStructures(*this);
}

void SetAvailableCreatures::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetAvailableCreatures(*this);
}

void SetHeroesInTown::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetHeroesInTown(*this);
}

void HeroRecruited::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitHeroRecruited(*this);
}

void GiveHero::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitGiveHero(*this);
}

void OpenWindow::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitOpenWindow(*this);
}

void NewObject::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitNewObject(*this);
}

void SetAvailableArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetAvailableArtifacts(*this);
}

void NewArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitNewArtifact(*this);
}

void ChangeStackCount::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitChangeStackCount(*this);
}

void SetStackType::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetStackType(*this);
}

void EraseStack::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitEraseStack(*this);
}

void SwapStacks::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSwapStacks(*this);
}

void InsertNewStack::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitInsertNewStack(*this);
}

void RebalanceStacks::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitRebalanceStacks(*this);
}

void BulkRebalanceStacks::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkRebalanceStacks(*this);
}

void GrowUpArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitGrowUpArtifact(*this);
}

void PutArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPutArtifact(*this);
}

void BulkEraseArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkEraseArtifacts(*this);
}

void BulkMoveArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkMoveArtifacts(*this);
}

void AssembledArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitAssembledArtifact(*this);
}

void DischargeArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitDischargeArtifact(*this);
}

void DisassembledArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitDisassembledArtifact(*this);
}

void HeroVisit::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitHeroVisit(*this);
}

void NewTurn::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitNewTurn(*this);
}

void InfoWindow::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitInfoWindow(*this);
}

void SetObjectProperty::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetObjectProperty(*this);
}

void ChangeObjectVisitors::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitChangeObjectVisitors(*this);
}

void ChangeArtifactsCostume::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitChangeArtifactsCostume(*this);
}

void HeroLevelUp::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitHeroLevelUp(*this);
}

void CommanderLevelUp::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitCommanderLevelUp(*this);
}

void BlockingDialog::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBlockingDialog(*this);
}

void GarrisonDialog::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitGarrisonDialog(*this);
}

void ExchangeDialog::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitExchangeDialog(*this);
}

void TeleportDialog::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitTeleportDialog(*this);
}

void MapObjectSelectDialog::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitMapObjectSelectDialog(*this);
}

void BattleStart::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleStart(*this);
}

void BattleNextRound::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleNextRound(*this);
}

void BattleSetActiveStack::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleSetActiveStack(*this);
}

void BattleResult::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleResult(*this);
}

void BattleLogMessage::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleLogMessage(*this);
}

void BattleStackMoved::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleStackMoved(*this);
}

void BattleUnitsChanged::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleUnitsChanged(*this);
}

void BattleAttack::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleAttack(*this);
}

void StartAction::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitStartAction(*this);
}

void EndAction::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitEndAction(*this);
}

void BattleSpellCast::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleSpellCast(*this);
}

void SetStackEffect::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetStackEffect(*this);
}

void StacksInjured::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitStacksInjured(*this);
}

void BattleResultsApplied::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleResultsApplied(*this);
}

void BattleObstaclesChanged::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleObstaclesChanged(*this);
}

void BattleSetStackProperty::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleSetStackProperty(*this);
}

void BattleTriggerEffect::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleTriggerEffect(*this);
}

void BattleUpdateGateState::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleUpdateGateState(*this);
}

void AdvmapSpellCast::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitAdvmapSpellCast(*this);
}

void ShowWorldViewEx::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitShowWorldViewEx(*this);
}

void EndTurn::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitEndTurn(*this);
}

void GamePause::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitGamePause(*this);
}

void DismissHero::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitDismissHero(*this);
}

void MoveHero::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitMoveHero(*this);
}

void CastleTeleportHero::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitCastleTeleportHero(*this);
}

void ArrangeStacks::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitArrangeStacks(*this);
}

void BulkMoveArmy::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkMoveArmy(*this);
}

void BulkSplitStack::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkSplitStack(*this);
}

void BulkMergeStacks::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkMergeStacks(*this);
}

void BulkSplitAndRebalanceStack::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkSplitAndRebalanceStack(*this);
}

void DisbandCreature::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitDisbandCreature(*this);
}

void BuildStructure::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBuildStructure(*this);
}

void VisitTownBuilding::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitVisitTownBuilding(*this);
}

void RazeStructure::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitRazeStructure(*this);
}

void SpellResearch::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSpellResearch(*this);
}

void RecruitCreatures::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitRecruitCreatures(*this);
}

void UpgradeCreature::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitUpgradeCreature(*this);
}

void GarrisonHeroSwap::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitGarrisonHeroSwap(*this);
}

void ExchangeArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitExchangeArtifacts(*this);
}

void BulkExchangeArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkExchangeArtifacts(*this);
}

void ManageBackpackArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitManageBackpackArtifacts(*this);
}

void ManageEquippedArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitManageEquippedArtifacts(*this);
}

void AssembleArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitAssembleArtifacts(*this);
}

void EraseArtifactByClient::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitEraseArtifactByClient(*this);
}

void BuyArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBuyArtifact(*this);
}

void TradeOnMarketplace::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitTradeOnMarketplace(*this);
}

void SetFormation::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetFormation(*this);
}

void HireHero::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitHireHero(*this);
}

void BuildBoat::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBuildBoat(*this);
}

void QueryReply::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitQueryReply(*this);
}

void MakeAction::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitMakeAction(*this);
}

void DigWithHero::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitDigWithHero(*this);
}

void CastAdvSpell::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitCastAdvSpell(*this);
}

void SaveGame::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSaveGame(*this);
}

void PlayerMessage::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPlayerMessage(*this);
}

void PlayerMessageClient::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPlayerMessageClient(*this);
}

void CenterView::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitCenterView(*this);
}

void LobbyClientConnected::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyClientConnected(*this);
}

void LobbyClientDisconnected::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyClientDisconnected(*this);
}

void LobbyChatMessage::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyChatMessage(*this);
}

void LobbyGuiAction::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyGuiAction(*this);
}

void LobbyLoadProgress::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyLoadProgress(*this);
}

void LobbyRestartGame::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyRestartGame(*this);
}

void LobbyStartGame::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyStartGame(*this);
}

void LobbyPrepareStartGame::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyPrepareStartGame(*this);
}

void LobbyChangeHost::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyChangeHost(*this);
}

void LobbyUpdateState::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyUpdateState(*this);
}

void LobbySetMap::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetMap(*this);
}

void LobbySetCampaign::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetCampaign(*this);
}

void LobbySetCampaignMap::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetCampaignMap(*this);
}

void LobbySetCampaignBonus::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetCampaignBonus(*this);
}

void LobbyChangePlayerOption::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyChangePlayerOption(*this);
}

void LobbySetPlayer::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetPlayer(*this);
}

void LobbySetPlayerName::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetPlayerName(*this);
}

void LobbySetPlayerHandicap::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetPlayerHandicap(*this);
}

void LobbySetSimturns::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetSimturns(*this);
}

void LobbySetTurnTime::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetTurnTime(*this);
}

void LobbySetExtraOptions::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetExtraOptions(*this);
}

void LobbySetDifficulty::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetDifficulty(*this);
}

void LobbyForceSetPlayer::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyForceSetPlayer(*this);
}

void LobbyShowMessage::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyShowMessage(*this);
}

void LobbyPvPAction::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyPvPAction(*this);
}

void LobbyDelete::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyDelete(*this);
}

void CatapultAttack::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitCatapultAttack(*this);
}

void BattleResultAccepted::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleResultAccepted(*this);
}

void BattleCancelled::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBattleCancelled(*this);
}

void TurnTimeUpdate::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitTurnTimeUpdate(*this);
}

VCMI_LIB_NAMESPACE_END

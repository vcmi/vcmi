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
#include "ArtifactUtils.h"
#include "PacksForClient.h"
#include "PacksForClientBattle.h"
#include "PacksForServer.h"
#include "SaveLocalState.h"
#include "SetRewardableConfiguration.h"
#include "StackLocation.h"
#include "PacksForLobby.h"
#include "SetStackEffect.h"
#include "NetPackVisitor.h"
#include "texts/CGeneralTextHandler.h"
#include "CArtHandler.h"
#include "GameLibrary.h"
#include "mapping/CMap.h"
#include "spells/CSpellHandler.h"
#include "CCreatureHandler.h"
#include "gameState/CGameState.h"
#include "gameState/TavernHeroesPool.h"
#include "CStack.h"
#include "battle/BattleInfo.h"
#include "mapping/CMapInfo.h"
#include "StartInfo.h"
#include "CPlayerState.h"
#include "TerrainHandler.h"
#include "entities/building/CBuilding.h"
#include "entities/building/TownFortifications.h"
#include "mapObjects/CGCreature.h"
#include "mapObjects/CGMarket.h"
#include "mapObjects/TownBuildingInstance.h"
#include "mapObjects/CGTownInstance.h"
#include "mapObjects/CQuest.h"
#include "mapObjects/MiscObjects.h"
#include "mapObjectConstructors/AObjectTypeHandler.h"
#include "mapObjectConstructors/CObjectClassesHandler.h"
#include "campaign/CampaignState.h"
#include "IGameSettings.h"
#include "mapObjects/FlaggableMapObject.h"

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

void SetPrimSkill::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSetPrimSkill(*this);
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
	visitor.visitSetAvailableHeroes(*this);
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

void PlayerReinitInterface::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPlayerReinitInterface(*this);
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

void UpdateArtHandlerLists::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitUpdateArtHandlerLists(*this);
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

void BulkSmartRebalanceStacks::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkSmartRebalanceStacks(*this);
}

void PutArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPutArtifact(*this);
}

void BulkEraseArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitEraseArtifact(*this);
}

void BulkMoveArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkMoveArtifacts(*this);
}

void AssembledArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitAssembledArtifact(*this);
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

void BulkSmartSplitStack::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitBulkSmartSplitStack(*this);
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

void SetResources::applyGs(CGameState *gs)
{
	assert(player.isValidPlayer());
	if(abs)
		gs->getPlayerState(player)->resources = res;
	else
		gs->getPlayerState(player)->resources += res;
	gs->getPlayerState(player)->resources.amin(GameConstants::PLAYER_RESOURCES_CAP);

	//just ensure that player resources are not negative
	//server is responsible to check if player can afford deal
	//but events on server side are allowed to take more than player have
	gs->getPlayerState(player)->resources.positive();
}

void SetPrimSkill::applyGs(CGameState *gs)
{
	CGHeroInstance * hero = gs->getHero(id);
	assert(hero);
	hero->setPrimarySkill(which, val, abs);
}

void SetSecSkill::applyGs(CGameState *gs)
{
	CGHeroInstance *hero = gs->getHero(id);
	hero->setSecSkillLevel(which, val, abs);
}

void SetCommanderProperty::applyGs(CGameState *gs)
{
	CCommanderInstance * commander = gs->getHero(heroid)->commander;
	assert (commander);

	switch (which)
	{
		case BONUS:
			commander->accumulateBonus (std::make_shared<Bonus>(accumulatedBonus));
			break;
		case SPECIAL_SKILL:
			commander->accumulateBonus (std::make_shared<Bonus>(accumulatedBonus));
			commander->specialSkills.insert (additionalInfo);
			break;
		case SECONDARY_SKILL:
			commander->secondarySkills[additionalInfo] = static_cast<ui8>(amount);
			break;
		case ALIVE:
			if (amount)
				commander->setAlive(true);
			else
				commander->setAlive(false);
			break;
		case EXPERIENCE:
			commander->giveStackExp(amount); //TODO: allow setting exp for stacks via netpacks
			commander->nodeHasChanged();
			break;
	}
}

void AddQuest::applyGs(CGameState *gs)
{
	assert (vstd::contains(gs->players, player));
	auto * vec = &gs->players[player].quests;
	if (!vstd::contains(*vec, quest))
		vec->push_back (quest);
	else
		logNetwork->warn("Warning! Attempt to add duplicated quest");
}

void UpdateArtHandlerLists::applyGs(CGameState *gs)
{
	gs->allocatedArtifacts = allocatedArtifacts;
}

void ChangeFormation::applyGs(CGameState *gs)
{
	gs->getHero(hid)->setFormation(formation);
}

void HeroVisitCastle::applyGs(CGameState *gs)
{
	CGHeroInstance *h = gs->getHero(hid);
	CGTownInstance *t = gs->getTown(tid);

	assert(h);
	assert(t);

	if(start())
		t->setVisitingHero(h);
	else
		t->setVisitingHero(nullptr);
}

void ChangeSpells::applyGs(CGameState *gs)
{
	CGHeroInstance *hero = gs->getHero(hid);

	if(learn)
		for(const auto & sid : spells)
			hero->addSpellToSpellbook(sid);
	else
		for(const auto & sid : spells)
			hero->removeSpellFromSpellbook(sid);
}

void SetResearchedSpells::applyGs(CGameState *gs)
{
	CGTownInstance *town = gs->getTown(tid);

	town->spells[level] = spells;
	town->spellResearchCounterDay++;
	if(accepted)
		town->spellResearchAcceptedCounter++;
}

void SetMana::applyGs(CGameState *gs)
{
	CGHeroInstance * hero = gs->getHero(hid);

	assert(hero);

	if(absolute)
		hero->mana = val;
	else
		hero->mana += val;

	vstd::amax(hero->mana, 0); //not less than 0
}

void SetMovePoints::applyGs(CGameState *gs)
{
	CGHeroInstance *hero = gs->getHero(hid);

	assert(hero);

	if(absolute)
		hero->setMovementPoints(val);
	else
		hero->setMovementPoints(hero->movementPointsRemaining() + val);
}

void FoWChange::applyGs(CGameState *gs)
{
	TeamState * team = gs->getPlayerTeam(player);
	auto & fogOfWarMap = team->fogOfWarMap;
	for(const int3 & t : tiles)
		fogOfWarMap[t.z][t.x][t.y] = mode != ETileVisibility::HIDDEN;

	if (mode == ETileVisibility::HIDDEN) //do not hide too much
	{
		std::unordered_set<int3> tilesRevealed;
		for (auto & elem : gs->getMap().objects)
		{
			const CGObjectInstance *o = elem;
			if (o)
			{
				switch(o->ID.toEnum())
				{
				case Obj::HERO:
				case Obj::MINE:
				case Obj::TOWN:
				case Obj::ABANDONED_MINE:
					if(vstd::contains(team->players, o->tempOwner)) //check owned observators
						gs->getTilesInRange(tilesRevealed, o->getSightCenter(), o->getSightRadius(), ETileVisibility::HIDDEN, o->tempOwner);
					break;
				}
			}
		}
		for(const int3 & t : tilesRevealed) //probably not the most optimal solution ever
			fogOfWarMap[t.z][t.x][t.y] = 1;
	}
}

void SetAvailableHero::applyGs(CGameState *gs)
{
	gs->heroesPool->setHeroForPlayer(player, slotID, hid, army, roleID, replenishPoints);
}

void GiveBonus::applyGs(CGameState *gs)
{
	CBonusSystemNode *cbsn = nullptr;
	switch(who)
	{
	case ETarget::OBJECT:
		cbsn = dynamic_cast<CBonusSystemNode*>(gs->getObjInstance(id.as<ObjectInstanceID>()));
		break;
	case ETarget::PLAYER:
		cbsn = gs->getPlayerState(id.as<PlayerColor>());
		break;
	case ETarget::BATTLE:
		assert(Bonus::OneBattle(&bonus));
		cbsn = dynamic_cast<CBonusSystemNode*>(gs->getBattle(id.as<BattleID>()));
		break;
	}

	assert(cbsn);

	if(Bonus::OneWeek(&bonus))
		bonus.turnsRemain = 8 - gs->getDate(Date::DAY_OF_WEEK); // set correct number of days before adding bonus

	auto b = std::make_shared<Bonus>(bonus);
	cbsn->addNewBonus(b);
}

void ChangeObjPos::applyGs(CGameState *gs)
{
	CGObjectInstance *obj = gs->getObjInstance(objid);
	if(!obj)
	{
		logNetwork->error("Wrong ChangeObjPos: object %d doesn't exist!", objid.getNum());
		return;
	}
	gs->getMap().removeBlockVisTiles(obj);
	obj->setAnchorPos(nPos + obj->getVisitableOffset());
	gs->getMap().addBlockVisTiles(obj);
}

void ChangeObjectVisitors::applyGs(CGameState *gs)
{
	switch (mode) {
		case VISITOR_ADD_HERO:
			gs->getPlayerTeam(gs->getHero(hero)->tempOwner)->scoutedObjects.insert(object);
			gs->getHero(hero)->visitedObjects.insert(object);
			gs->getPlayerState(gs->getHero(hero)->tempOwner)->visitedObjects.insert(object);
			break;
		case VISITOR_ADD_PLAYER:
			gs->getPlayerTeam(gs->getHero(hero)->tempOwner)->scoutedObjects.insert(object);
			for(const auto & color : gs->getPlayerTeam(gs->getHero(hero)->tempOwner)->players)
				gs->getPlayerState(color)->visitedObjects.insert(object);

			break;
		case VISITOR_CLEAR:
			// remove visit info from all heroes, including those that are not present on map
			for (CGHeroInstance * hero : gs->getMap().allHeroes)
				if (hero)
					hero->visitedObjects.erase(object);

			for(auto &elem : gs->players)
				elem.second.visitedObjects.erase(object);

			for(auto &elem : gs->teams)
				elem.second.scoutedObjects.erase(object);

			break;
		case VISITOR_SCOUTED:
			gs->getPlayerTeam(gs->getHero(hero)->tempOwner)->scoutedObjects.insert(object);

			break;
		case VISITOR_GLOBAL:
			{
				CGObjectInstance * objectPtr = gs->getObjInstance(object);
				gs->getPlayerState(gs->getHero(hero)->tempOwner)->visitedObjectsGlobal.insert({objectPtr->ID, objectPtr->subID});
				break;
			}
	}
}

void ChangeArtifactsCostume::applyGs(CGameState *gs)
{
	auto & allCostumes = gs->getPlayerState(player)->costumesArtifacts;
	if(const auto & costume = allCostumes.find(costumeIdx); costume != allCostumes.end())
		costume->second = costumeSet;
	else
		allCostumes.try_emplace(costumeIdx, costumeSet);
}

void PlayerEndsGame::applyGs(CGameState *gs)
{
	PlayerState *p = gs->getPlayerState(player);
	if(victoryLossCheckResult.victory())
	{
		p->status = EPlayerStatus::WINNER;

		// TODO: Campaign-specific code might as well go somewhere else
		// keep all heroes from the winning player
		if(p->human && gs->getStartInfo()->campState)
		{
			std::vector<CGHeroInstance *> crossoverHeroes;
			for (CGHeroInstance * hero : gs->getMap().heroesOnMap)
				if (hero->tempOwner == player)
					crossoverHeroes.push_back(hero);

			gs->getStartInfo()->campState->setCurrentMapAsConquered(crossoverHeroes);
		}
	}
	else
	{
		p->status = EPlayerStatus::LOSER;
	}

	// defeated player may be making turn right now
	gs->actingPlayers.erase(player);
}

void PlayerReinitInterface::applyGs(CGameState *gs)
{
	if(!gs || !gs->getStartInfo())
		return;
	
	//TODO: what does mean if more that one player connected?
	if(playerConnectionId == PlayerSettings::PLAYER_AI)
	{
		for(const auto & player : players)
			gs->getStartInfo()->getIthPlayersSettings(player).connectedPlayerIDs.clear();
	}
}

void RemoveBonus::applyGs(CGameState *gs)
{
	CBonusSystemNode *node = nullptr;
	switch(who)
	{
	case GiveBonus::ETarget::OBJECT:
		node = dynamic_cast<CBonusSystemNode*>(gs->getObjInstance(whoID.as<ObjectInstanceID>()));
		break;
	case GiveBonus::ETarget::PLAYER:
		node = gs->getPlayerState(whoID.as<PlayerColor>());
		break;
	case GiveBonus::ETarget::BATTLE:
		assert(Bonus::OneBattle(&bonus));
		node = dynamic_cast<CBonusSystemNode*>(gs->getBattle(whoID.as<BattleID>()));
		break;
	}

	BonusList &bonuses = node->getExportedBonusList();

	for(const auto & b : bonuses)
	{
		if(b->source == source && b->sid == id)
		{
			bonus = *b; //backup bonus (to show to interfaces later)
			node->removeBonus(b);
			break;
		}
	}
}

void RemoveObject::applyGs(CGameState *gs)
{
	CGObjectInstance *obj = gs->getObjInstance(objectID);
	logGlobal->debug("removing object id=%d; address=%x; name=%s", objectID, (intptr_t)obj, obj->getObjectName());
	//unblock tiles
	gs->getMap().removeBlockVisTiles(obj);

	if (initiator.isValidPlayer())
		gs->getPlayerState(initiator)->destroyedObjects.insert(objectID);

	if(obj->getOwner().isValidPlayer())
	{
		gs->getPlayerState(obj->getOwner())->removeOwnedObject(obj); //object removed via map event or hero got beaten

		FlaggableMapObject* flaggableObject = dynamic_cast<FlaggableMapObject*>(obj);
		if(flaggableObject)
		{
			flaggableObject->markAsDeleted();
		}
	}

	if(obj->ID == Obj::HERO) //remove beaten hero
	{
		auto * beatenHero = dynamic_cast<CGHeroInstance *>(obj);
		assert(beatenHero);
		gs->getMap().heroesOnMap -= beatenHero;

		auto * siegeNode = beatenHero->whereShouldBeAttachedOnSiege(gs);

		// FIXME: workaround:
		// hero should be attached to siegeNode after battle
		// however this code might also be called on dismissing hero while in town
		if (siegeNode && vstd::contains(beatenHero->getParentNodes(), siegeNode))
				beatenHero->detachFrom(*siegeNode);

		beatenHero->tempOwner = PlayerColor::NEUTRAL; //no one owns beaten hero
		vstd::erase_if(beatenHero->artifactsInBackpack, [](const ArtSlotInfo& asi)
		{
			return asi.artifact->getTypeId() == ArtifactID::GRAIL;
		});

		if(beatenHero->visitedTown)
		{
			if(beatenHero->visitedTown->garrisonHero == beatenHero)
				beatenHero->visitedTown->garrisonHero = nullptr;
			else
				beatenHero->visitedTown->visitingHero = nullptr;

			beatenHero->visitedTown = nullptr;
			beatenHero->inTownGarrison = false;
		}
		//return hero to the pool, so he may reappear in tavern

		gs->heroesPool->addHeroToPool(beatenHero);
		gs->getMap().objects[objectID.getNum()] = nullptr;

		//If hero on Boat is removed, the Boat disappears
		if(beatenHero->boat)
		{
			beatenHero->detachFrom(const_cast<CGBoat&>(*beatenHero->boat));
			gs->getMap().instanceNames.erase(beatenHero->boat->instanceName);
			gs->getMap().objects[beatenHero->boat->id.getNum()].dellNull();
			beatenHero->boat = nullptr;
		}
		return;
	}

	const auto * quest = dynamic_cast<const IQuestObject *>(obj);
	if (quest)
	{
		gs->getMap().quests[quest->quest->qid] = nullptr;
		for (auto &player : gs->players)
		{
			vstd::erase_if(player.second.quests, [obj](const QuestInfo & q){
				return q.obj == obj;
			});
		}
	}

	gs->getMap().instanceNames.erase(obj->instanceName);
	gs->getMap().objects[objectID.getNum()].dellNull();
	gs->getMap().calculateGuardingGreaturePositions();//FIXME: excessive, update only affected tiles
}

static int getDir(const int3 & src, const int3 & dst)
{
	int ret = -1;
	if(dst.x+1 == src.x && dst.y+1 == src.y) //tl
	{
		ret = 1;
	}
	else if(dst.x == src.x && dst.y+1 == src.y) //t
	{
		ret = 2;
	}
	else if(dst.x-1 == src.x && dst.y+1 == src.y) //tr
	{
		ret = 3;
	}
	else if(dst.x-1 == src.x && dst.y == src.y) //r
	{
		ret = 4;
	}
	else if(dst.x-1 == src.x && dst.y-1 == src.y) //br
	{
		ret = 5;
	}
	else if(dst.x == src.x && dst.y-1 == src.y) //b
	{
		ret = 6;
	}
	else if(dst.x+1 == src.x && dst.y-1 == src.y) //bl
	{
		ret = 7;
	}
	else if(dst.x+1 == src.x && dst.y == src.y) //l
	{
		ret = 8;
	}
	return ret;
}

void TryMoveHero::applyGs(CGameState *gs)
{
	CGHeroInstance *h = gs->getHero(id);
	if (!h)
	{
		logGlobal->error("Attempt ot move unavailable hero %d", id.getNum());
		return;
	}

	h->setMovementPoints(movePoints);

	if((result == SUCCESS || result == BLOCKING_VISIT || result == EMBARK || result == DISEMBARK) && start != end)
	{
		auto dir = getDir(start,end);
		if(dir > 0  &&  dir <= 8)
			h->moveDir = dir;
		//else don`t change move direction - hero might have traversed the subterranean gate, direction should be kept
	}

	if(result == EMBARK) //hero enters boat at destination tile
	{
		const TerrainTile &tt = gs->getMap().getTile(h->convertToVisitablePos(end));
		assert(tt.visitableObjects.size() >= 1  &&  tt.visitableObjects.back()->ID == Obj::BOAT); //the only visitable object at destination is Boat
		auto * boat = dynamic_cast<CGBoat *>(tt.visitableObjects.back());
		assert(boat);

		gs->getMap().removeBlockVisTiles(boat); //hero blockvis mask will be used, we don't need to duplicate it with boat
		h->boat = boat;
		h->attachTo(*boat);
		boat->hero = h;
	}
	else if(result == DISEMBARK) //hero leaves boat to destination tile
	{
		auto * b = const_cast<CGBoat *>(h->boat);
		b->direction = h->moveDir;
		b->pos = start;
		b->hero = nullptr;
		gs->getMap().addBlockVisTiles(b);
		h->detachFrom(*b);
		h->boat = nullptr;
	}

	if(start!=end && (result == SUCCESS || result == TELEPORTATION || result == EMBARK || result == DISEMBARK))
	{
		gs->getMap().removeBlockVisTiles(h);
		h->pos = end;
		if(auto * b = const_cast<CGBoat *>(h->boat))
			b->pos = end;
		gs->getMap().addBlockVisTiles(h);
	}

	auto & fogOfWarMap = gs->getPlayerTeam(h->getOwner())->fogOfWarMap;
	for(const int3 & t : fowRevealed)
		fogOfWarMap[t.z][t.x][t.y] = 1;
}

void NewStructures::applyGs(CGameState *gs)
{
	CGTownInstance *t = gs->getTown(tid);

	for(const auto & id : bid)
	{
		assert(t->getTown()->buildings.at(id) != nullptr);
		t->addBuilding(id);
	}
	t->updateAppearance();
	t->built = built;
	t->recreateBuildingsBonuses();
}

void RazeStructures::applyGs(CGameState *gs)
{
	CGTownInstance *t = gs->getTown(tid);
	for(const auto & id : bid)
	{
		t->removeBuilding(id);

		t->updateAppearance();
	}
	t->destroyed = destroyed; //yeaha
	t->recreateBuildingsBonuses();
}

void SetAvailableCreatures::applyGs(CGameState *gs)
{
	auto * dw = dynamic_cast<CGDwelling *>(gs->getObjInstance(tid));
	assert(dw);
	dw->creatures = creatures;
}

void SetHeroesInTown::applyGs(CGameState *gs)
{
	CGTownInstance *t = gs->getTown(tid);

	CGHeroInstance * v = gs->getHero(visiting);
	CGHeroInstance * g = gs->getHero(garrison);

	bool newVisitorComesFromGarrison = v && v == t->garrisonHero;
	bool newGarrisonComesFromVisiting = g && g == t->visitingHero;

	if(newVisitorComesFromGarrison)
		t->setGarrisonedHero(nullptr);
	if(newGarrisonComesFromVisiting)
		t->setVisitingHero(nullptr);
	if(!newGarrisonComesFromVisiting || v)
		t->setVisitingHero(v);
	if(!newVisitorComesFromGarrison || g)
		t->setGarrisonedHero(g);

	if(v)
	{
		gs->getMap().addBlockVisTiles(v);
	}
	if(g)
	{
		gs->getMap().removeBlockVisTiles(g);
	}
}

void HeroRecruited::applyGs(CGameState *gs)
{
	CGHeroInstance *h = gs->heroesPool->takeHeroFromPool(hid);
	CGTownInstance *t = gs->getTown(tid);
	PlayerState *p = gs->getPlayerState(player);

	if (boatId != ObjectInstanceID::NONE)
	{
		CGObjectInstance *obj = gs->getObjInstance(boatId);
		auto * boat = dynamic_cast<CGBoat *>(obj);
		if (boat)
		{
			gs->getMap().removeBlockVisTiles(boat);
			h->attachToBoat(boat);
		}
	}

	h->setOwner(player);
	h->pos = tile;
	h->updateAppearance();

	if(h->id == ObjectInstanceID())
	{
		h->id = ObjectInstanceID(static_cast<si32>(gs->getMap().objects.size()));
		gs->getMap().objects.emplace_back(h);
	}
	else
		gs->getMap().objects[h->id.getNum()] = h;

	gs->getMap().heroesOnMap.emplace_back(h);
	p->addOwnedObject(h);
	h->attachTo(*p);
	gs->getMap().addBlockVisTiles(h);

	if(t)
		t->setVisitingHero(h);
}

void GiveHero::applyGs(CGameState *gs)
{
	CGHeroInstance *h = gs->getHero(id);

	if (boatId != ObjectInstanceID::NONE)
	{
		CGObjectInstance *obj = gs->getObjInstance(boatId);
		auto * boat = dynamic_cast<CGBoat *>(obj);
		if (boat)
		{
			gs->getMap().removeBlockVisTiles(boat);
			h->attachToBoat(boat);
		}
	}

	//bonus system
	h->detachFrom(gs->globalEffects);
	h->attachTo(*gs->getPlayerState(player));

	auto oldVisitablePos = h->visitablePos();
	gs->getMap().removeBlockVisTiles(h,true);
	h->updateAppearance();

	h->setOwner(player);
	h->setMovementPoints(h->movementPointsLimit(true));
	h->setAnchorPos(h->convertFromVisitablePos(oldVisitablePos));
	gs->getMap().heroesOnMap.emplace_back(h);
	gs->getPlayerState(h->getOwner())->addOwnedObject(h);

	gs->getMap().addBlockVisTiles(h);
	h->inTownGarrison = false;
}

void NewObject::applyGs(CGameState *gs)
{
	newObject->id = ObjectInstanceID(static_cast<si32>(gs->getMap().objects.size()));

	gs->getMap().objects.emplace_back(newObject);
	gs->getMap().addBlockVisTiles(newObject);
	gs->getMap().calculateGuardingGreaturePositions();

	// attach newly spawned wandering monster to global bonus system node
	auto newArmy = dynamic_cast<CArmedInstance*>(newObject);
	if (newArmy)
		newArmy->whatShouldBeAttached().attachTo(newArmy->whereShouldBeAttached(gs));

	logGlobal->debug("Added object id=%d; name=%s", newObject->id, newObject->getObjectName());
}

void NewArtifact::applyGs(CGameState *gs)
{
	auto art = ArtifactUtils::createArtifact(artId, spellId);
	gs->getMap().addNewArtifactInstance(art);
	PutArtifact pa(art->getId(), ArtifactLocation(artHolder, pos), false);
	pa.applyGs(gs);
}

struct ObjectRetriever
{
	const CArmedInstance * operator()(const ConstTransitivePtr<CGHeroInstance> &h) const
	{
		return h;
	}
	const CArmedInstance * operator()(const ConstTransitivePtr<CStackInstance> &s) const
	{
		return s->armyObj;
	}
};
template<typename T>
struct GetBase
{
	template <typename TArg>
	T * operator()(TArg &arg) const
	{
		return arg;
	}
};

void ChangeStackCount::applyGs(CGameState *gs)
{
	auto * srcObj = gs->getArmyInstance(army);
	if(!srcObj)
		throw std::runtime_error("ChangeStackCount: invalid army object " + std::to_string(army.getNum()) + ", possible game state corruption.");

	if(absoluteValue)
		srcObj->setStackCount(slot, count);
	else
		srcObj->changeStackCount(slot, count);
}

void SetStackType::applyGs(CGameState *gs)
{
	auto * srcObj = gs->getArmyInstance(army);
	if(!srcObj)
		throw std::runtime_error("SetStackType: invalid army object " + std::to_string(army.getNum()) + ", possible game state corruption.");

	srcObj->setStackType(slot, type);
}

void EraseStack::applyGs(CGameState *gs)
{
	auto * srcObj = gs->getArmyInstance(army);
	if(!srcObj)
		throw std::runtime_error("EraseStack: invalid army object " + std::to_string(army.getNum()) + ", possible game state corruption.");

	srcObj->eraseStack(slot);
}

void SwapStacks::applyGs(CGameState *gs)
{
	auto * srcObj = gs->getArmyInstance(srcArmy);
	if(!srcObj)
		throw std::runtime_error("SwapStacks: invalid army object " + std::to_string(srcArmy.getNum()) + ", possible game state corruption.");

	auto * dstObj = gs->getArmyInstance(dstArmy);
	if(!dstObj)
		throw std::runtime_error("SwapStacks: invalid army object " + std::to_string(dstArmy.getNum()) + ", possible game state corruption.");

	CStackInstance * s1 = srcObj->detachStack(srcSlot);
	CStackInstance * s2 = dstObj->detachStack(dstSlot);

	srcObj->putStack(srcSlot, s2);
	dstObj->putStack(dstSlot, s1);
}

void InsertNewStack::applyGs(CGameState *gs)
{
	if(auto * obj = gs->getArmyInstance(army))
		obj->putStack(slot, new CStackInstance(type, count));
	else
		throw std::runtime_error("InsertNewStack: invalid army object " + std::to_string(army.getNum()) + ", possible game state corruption.");
}

void RebalanceStacks::applyGs(CGameState *gs)
{
	auto * srcObj = gs->getArmyInstance(srcArmy);
	if(!srcObj)
		throw std::runtime_error("RebalanceStacks: invalid army object " + std::to_string(srcArmy.getNum()) + ", possible game state corruption.");

	auto * dstObj = gs->getArmyInstance(dstArmy);
	if(!dstObj)
		throw std::runtime_error("RebalanceStacks: invalid army object " + std::to_string(dstArmy.getNum()) + ", possible game state corruption.");

	StackLocation src(srcObj->id, srcSlot);
	StackLocation dst(dstObj->id, dstSlot);

	const CCreature * srcType = srcObj->getCreature(src.slot);
	TQuantity srcCount = srcObj->getStackCount(src.slot);
	bool stackExp = gs->getSettings().getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE);

	if(srcCount == count) //moving whole stack
	{
		const auto c = dstObj->getCreature(dst.slot);

		if(c) //stack at dest -> merge
		{
			assert(c == srcType);
			
			const auto srcHero = dynamic_cast<CGHeroInstance*>(srcObj);
			const auto dstHero = dynamic_cast<CGHeroInstance*>(dstObj);
			auto srcStack = const_cast<CStackInstance*>(srcObj->getStackPtr(src.slot));
			auto dstStack = const_cast<CStackInstance*>(dstObj->getStackPtr(dst.slot));
			if(srcStack->getArt(ArtifactPosition::CREATURE_SLOT))
			{
				if(auto dstArt = dstStack->getArt(ArtifactPosition::CREATURE_SLOT))
				{
					auto dstSlot = ArtifactUtils::getArtBackpackPosition(srcHero, dstArt->getTypeId());
					if(srcHero && dstSlot != ArtifactPosition::PRE_FIRST)
					{
						gs->getMap().moveArtifactInstance(*dstStack, ArtifactPosition::CREATURE_SLOT, *srcHero, dstSlot);
					}
					//else - artifact can be lost :/
					else
					{
						BulkEraseArtifacts ea;
						ea.artHolder = dstHero->id;
						ea.posPack.emplace_back(ArtifactPosition::CREATURE_SLOT);
						ea.creature = dst.slot;
						ea.applyGs(gs);
						logNetwork->warn("Cannot move artifact! No free slots");
					}
					gs->getMap().moveArtifactInstance(*srcStack, ArtifactPosition::CREATURE_SLOT, *dstStack, ArtifactPosition::CREATURE_SLOT);
					//TODO: choose from dialog
				}
				else //just move to the other slot before stack gets erased
				{
					gs->getMap().moveArtifactInstance(*srcStack, ArtifactPosition::CREATURE_SLOT, *dstStack, ArtifactPosition::CREATURE_SLOT);
				}
			}
			if (stackExp)
			{
				ui64 totalExp = srcCount * srcObj->getStackExperience(src.slot) + dstObj->getStackCount(dst.slot) * dstObj->getStackExperience(dst.slot);
				srcObj->eraseStack(src.slot);
				dstObj->changeStackCount(dst.slot, count);
				dstObj->setStackExp(dst.slot, totalExp /(dstObj->getStackCount(dst.slot))); //mean
			}
			else
			{
				srcObj->eraseStack(src.slot);
				dstObj->changeStackCount(dst.slot, count);
			}
		}
		else //move stack to an empty slot, no exp change needed
		{
			CStackInstance *stackDetached = srcObj->detachStack(src.slot);
			dstObj->putStack(dst.slot, stackDetached);
		}
	}
	else
	{
		[[maybe_unused]] const CCreature *c = dstObj->getCreature(dst.slot);
		if(c) //stack at dest -> rebalance
		{
			assert(c == srcType);
			if (stackExp)
			{
				ui64 totalExp = srcCount * srcObj->getStackExperience(src.slot) + dstObj->getStackCount(dst.slot) * dstObj->getStackExperience(dst.slot);
				srcObj->changeStackCount(src.slot, -count);
				dstObj->changeStackCount(dst.slot, count);
				dstObj->setStackExp(dst.slot, totalExp /(srcObj->getStackCount(src.slot) + dstObj->getStackCount(dst.slot))); //mean
			}
			else
			{
				srcObj->changeStackCount(src.slot, -count);
				dstObj->changeStackCount(dst.slot, count);
			}
		}
		else //split stack to an empty slot
		{
			srcObj->changeStackCount(src.slot, -count);
			dstObj->addToSlot(dst.slot, srcType->getId(), count, false);
			if (stackExp)
				dstObj->setStackExp(dst.slot, srcObj->getStackExperience(src.slot));
		}
	}

	srcObj->nodeHasChanged();
	if (srcObj != dstObj)
		dstObj->nodeHasChanged();
}

void BulkRebalanceStacks::applyGs(CGameState *gs)
{
	for(auto & move : moves)
		move.applyGs(gs);
}

void BulkSmartRebalanceStacks::applyGs(CGameState *gs)
{
	for(auto & move : moves)
		move.applyGs(gs);

	for(auto & change : changes)
		change.applyGs(gs);
}

void PutArtifact::applyGs(CGameState *gs)
{
	auto art = gs->getArtInstance(id);
	assert(!art->getParentNodes().empty());
	auto hero = gs->getHero(al.artHolder);
	assert(hero);
	assert(art && art->canBePutAt(hero, al.slot));
	assert(ArtifactUtils::checkIfSlotValid(*hero, al.slot));
	gs->getMap().putArtifactInstance(*hero, art, al.slot);
}

void BulkEraseArtifacts::applyGs(CGameState *gs)
{
	const auto artSet = gs->getArtSet(artHolder);
	assert(artSet);

	std::sort(posPack.begin(), posPack.end(), [](const ArtifactPosition & slot0, const ArtifactPosition & slot1) -> bool
		{
			return slot0.num > slot1.num;
		});

	for(const auto & slot : posPack)
	{
		const auto slotInfo = artSet->getSlot(slot);
		if(slotInfo->locked)
		{
			logGlobal->debug("Erasing locked artifact: %s", slotInfo->artifact->getType()->getNameTranslated());
			DisassembledArtifact dis;
			dis.al.artHolder = artHolder;

			for(auto & slotInfoWorn : artSet->artifactsWorn)
			{
				auto art = slotInfoWorn.second.artifact;
				if(art->isCombined() && art->isPart(slotInfo->artifact))
				{
					dis.al.slot = artSet->getArtPos(art);
					break;
				}
			}
			assert((dis.al.slot != ArtifactPosition::PRE_FIRST) && "Failed to determine the assembly this locked artifact belongs to");
			logGlobal->debug("Found the corresponding assembly: %s", artSet->getArt(dis.al.slot)->getType()->getNameTranslated());
			dis.applyGs(gs);
		}
		else
		{
			logGlobal->debug("Erasing artifact %s", slotInfo->artifact->getType()->getNameTranslated());
		}
		gs->getMap().removeArtifactInstance(*artSet, slot);
	}
}

void BulkMoveArtifacts::applyGs(CGameState *gs)
{
	const auto bulkArtsRemove = [gs](std::vector<LinkedSlots> & artsPack, CArtifactSet & artSet)
	{
		std::vector<ArtifactPosition> packToRemove;
		for(const auto & slotsPair : artsPack)
			packToRemove.push_back(slotsPair.srcPos);
		std::sort(packToRemove.begin(), packToRemove.end(), [](const ArtifactPosition & slot0, const ArtifactPosition & slot1) -> bool
			{
				return slot0.num > slot1.num;
			});

		for(const auto & slot : packToRemove)
			gs->getMap().removeArtifactInstance(artSet, slot);
	};

	const auto bulkArtsPut = [gs](std::vector<LinkedSlots> & artsPack, CArtifactSet & initArtSet, CArtifactSet & dstArtSet)
	{
		for(const auto & slotsPair : artsPack)
		{
			auto * art = initArtSet.getArt(slotsPair.srcPos);
			assert(art);
			gs->getMap().putArtifactInstance(dstArtSet, art, slotsPair.dstPos);
		}
	};
	
	auto * leftSet = gs->getArtSet(ArtifactLocation(srcArtHolder, srcCreature));
	assert(leftSet);
	auto * rightSet = gs->getArtSet(ArtifactLocation(dstArtHolder, dstCreature));
	assert(rightSet);
	CArtifactFittingSet artInitialSetLeft(*leftSet);
	bulkArtsRemove(artsPack0, *leftSet);
	if(!artsPack1.empty())
	{
		CArtifactFittingSet artInitialSetRight(*rightSet);
		bulkArtsRemove(artsPack1, *rightSet);
		bulkArtsPut(artsPack1, artInitialSetRight, *leftSet);
	}
	bulkArtsPut(artsPack0, artInitialSetLeft, *rightSet);
}

void AssembledArtifact::applyGs(CGameState *gs)
{
	auto artSet = gs->getArtSet(al.artHolder);
	assert(artSet);
	const auto transformedArt = artSet->getArt(al.slot);
	assert(transformedArt);
	const auto builtArt = artId.toArtifact();
	assert(vstd::contains_if(ArtifactUtils::assemblyPossibilities(artSet, transformedArt->getTypeId()), [=](const CArtifact * art)->bool
		{
			return art->getId() == builtArt->getId();
		}));

	auto * combinedArt = new CArtifactInstance(builtArt);
	gs->getMap().addNewArtifactInstance(combinedArt);

	// Find slots for all involved artifacts
	std::set<ArtifactPosition, std::greater<>> slotsInvolved = { al.slot };
	CArtifactFittingSet fittingSet(*artSet);
	auto parts = builtArt->getConstituents();
	parts.erase(std::find(parts.begin(), parts.end(), transformedArt->getType()));
	for(const auto constituent : parts)
	{
		const auto slot = fittingSet.getArtPos(constituent->getId(), false, false);
		fittingSet.lockSlot(slot);
		assert(slot != ArtifactPosition::PRE_FIRST);
		slotsInvolved.insert(slot);
	}

	// Find a slot for combined artifact
	if(ArtifactUtils::isSlotEquipment(al.slot) && ArtifactUtils::isSlotBackpack(*slotsInvolved.begin()))
	{
		al.slot = ArtifactPosition::BACKPACK_START;
	}
	else if(ArtifactUtils::isSlotBackpack(al.slot))
	{
		for(const auto & slot : slotsInvolved)
			if(ArtifactUtils::isSlotBackpack(slot))
				al.slot = slot;
	}
	else
	{
		for(const auto & slot : slotsInvolved)
			if(!vstd::contains(builtArt->getPossibleSlots().at(artSet->bearerType()), al.slot)
				&& vstd::contains(builtArt->getPossibleSlots().at(artSet->bearerType()), slot))
			{
				al.slot = slot;
				break;
			}
	}

	// Delete parts from hero
	for(const auto & slot : slotsInvolved)
	{
		const auto constituentInstance = artSet->getArt(slot);
		gs->getMap().removeArtifactInstance(*artSet, slot);

		if(!combinedArt->getType()->isFused())
		{
			if(ArtifactUtils::isSlotEquipment(al.slot) && slot != al.slot)
				combinedArt->addPart(constituentInstance, slot);
			else
				combinedArt->addPart(constituentInstance, ArtifactPosition::PRE_FIRST);
		}
	}

	// Put new combined artifacts
	gs->getMap().putArtifactInstance(*artSet, combinedArt, al.slot);
}

void DisassembledArtifact::applyGs(CGameState *gs)
{
	auto hero = gs->getHero(al.artHolder);
	assert(hero);
	auto disassembledArt = hero->getArt(al.slot);
	assert(disassembledArt);

	const auto parts = disassembledArt->getPartsInfo();
	gs->getMap().removeArtifactInstance(*hero, al.slot);
	for(auto & part : parts)
	{
		// ArtifactPosition::PRE_FIRST is value of main part slot -> it'll replace combined artifact in its pos
		auto slot = (ArtifactUtils::isSlotEquipment(part.slot) ? part.slot : al.slot);
		disassembledArt->detachFrom(*part.art);
		gs->getMap().putArtifactInstance(*hero, part.art, slot);
	}
	gs->getMap().eraseArtifactInstance(disassembledArt);
}

void HeroVisit::applyGs(CGameState *gs)
{
}

void SetAvailableArtifacts::applyGs(CGameState *gs)
{
	if(id != ObjectInstanceID::NONE)
	{
		if(auto * bm = dynamic_cast<CGBlackMarket *>(gs->getObjInstance(id)))
		{
			bm->artifacts = arts;
		}
		else
		{
			logNetwork->error("Wrong black market id!");
		}
	}
	else
	{
		gs->getMap().townMerchantArtifacts = arts;
	}
}

void NewTurn::applyGs(CGameState *gs)
{
	gs->day = day;

	// Update bonuses before doing anything else so hero don't get more MP than needed
	gs->globalEffects.removeBonusesRecursive(Bonus::OneDay); //works for children -> all game objs
	gs->globalEffects.reduceBonusDurations(Bonus::NDays);
	gs->globalEffects.reduceBonusDurations(Bonus::OneWeek);
	//TODO not really a single root hierarchy, what about bonuses placed elsewhere? [not an issue with H3 mechanics but in the future...]

	for(auto & manaPack : heroesMana)
		manaPack.applyGs(gs);

	for(auto & movePack : heroesMovement)
		movePack.applyGs(gs);

	gs->heroesPool->onNewDay();

	for(auto & entry : playerIncome)
	{
		gs->getPlayerState(entry.first)->resources += entry.second;
		gs->getPlayerState(entry.first)->resources.amin(GameConstants::PLAYER_RESOURCES_CAP);
	}

	for(auto & creatureSet : availableCreatures) //set available creatures in towns
		creatureSet.applyGs(gs);

	for(CGTownInstance* t : gs->getMap().towns)
	{
		t->built = 0;
		t->spellResearchCounterDay = 0;
	}

	if(newRumor)
		gs->currentRumor = *newRumor;
}

void SetObjectProperty::applyGs(CGameState *gs)
{
	CGObjectInstance *obj = gs->getObjInstance(id);
	if(!obj)
	{
		logNetwork->error("Wrong object ID - property cannot be set!");
		return;
	}

	auto * cai = dynamic_cast<CArmedInstance *>(obj);

	if(what == ObjProperty::OWNER && obj->asOwnable())
	{
		PlayerColor oldOwner = obj->getOwner();
		PlayerColor newOwner = identifier.as<PlayerColor>();
		if(oldOwner.isValidPlayer())
			gs->getPlayerState(oldOwner)->removeOwnedObject(obj);

		if(newOwner.isValidPlayer())
			gs->getPlayerState(newOwner)->addOwnedObject(obj);
	}

	if(what == ObjProperty::OWNER && cai)
	{
		if(obj->ID == Obj::TOWN)
		{
			auto * t = dynamic_cast<CGTownInstance *>(obj);
			assert(t);

			PlayerColor oldOwner = t->tempOwner;
			if(oldOwner.isValidPlayer())
			{
				auto * state = gs->getPlayerState(oldOwner);
				if(state->getTowns().empty())
					state->daysWithoutCastle = 0;
			}
			if(identifier.as<PlayerColor>().isValidPlayer())
			{
				//reset counter before NewTurn to avoid no town message if game loaded at turn when one already captured
				PlayerState * p = gs->getPlayerState(identifier.as<PlayerColor>());
				if(p->daysWithoutCastle)
					p->daysWithoutCastle = std::nullopt;
			}
		}

		CBonusSystemNode & nodeToMove = cai->whatShouldBeAttached();
		nodeToMove.detachFrom(cai->whereShouldBeAttached(gs));
		obj->setProperty(what, identifier);
		nodeToMove.attachTo(cai->whereShouldBeAttached(gs));
	}
	else //not an armed instance
	{
		obj->setProperty(what, identifier);
	}
}

void HeroLevelUp::applyGs(CGameState *gs)
{
	auto * hero = gs->getHero(heroId);
	assert(hero);
	hero->levelUp(skills);
}

void CommanderLevelUp::applyGs(CGameState *gs)
{
	auto * hero = gs->getHero(heroId);
	assert(hero);
	auto commander = hero->commander;
	assert(commander);
	commander->levelUp();
}

void BattleStart::applyGs(CGameState *gs)
{
	assert(battleID == gs->nextBattleID);

	gs->currentBattles.emplace_back(info);

	info->battleID = gs->nextBattleID;
	info->localInit();

	gs->nextBattleID = BattleID(gs->nextBattleID.getNum() + 1);
}

void BattleNextRound::applyGs(CGameState *gs)
{
	gs->getBattle(battleID)->nextRound();
}

void BattleSetActiveStack::applyGs(CGameState *gs)
{
	gs->getBattle(battleID)->nextTurn(stack);
}

void BattleTriggerEffect::applyGs(CGameState *gs)
{
	CStack * st = gs->getBattle(battleID)->getStack(stackID);
	assert(st);
	switch(static_cast<BonusType>(effect))
	{
	case BonusType::HP_REGENERATION:
	{
		int64_t toHeal = val;
		st->heal(toHeal, EHealLevel::HEAL, EHealPower::PERMANENT);
		break;
	}
	case BonusType::MANA_DRAIN:
	{
		CGHeroInstance * h = gs->getHero(ObjectInstanceID(additionalInfo));
		st->drainedMana = true;
		h->mana -= val;
		vstd::amax(h->mana, 0);
		break;
	}
	case BonusType::POISON:
	{
		auto b = st->getLocalBonus(Selector::source(BonusSource::SPELL_EFFECT, SpellID(SpellID::POISON))
				.And(Selector::type()(BonusType::STACK_HEALTH)));
		if (b)
			b->val = val;
		break;
	}
	case BonusType::ENCHANTER:
	case BonusType::MORALE:
		break;
	case BonusType::FEAR:
		st->fear = true;
		break;
	default:
		logNetwork->error("Unrecognized trigger effect type %d", effect);
	}
}

void BattleUpdateGateState::applyGs(CGameState *gs)
{
	if(gs->getBattle(battleID))
		gs->getBattle(battleID)->si.gateState = state;
}

void BattleCancelled::applyGs(CGameState *gs)
{
	auto currentBattle = boost::range::find_if(gs->currentBattles, [&](const auto & battle)
	{
		return battle->battleID == battleID;
	});

	assert(currentBattle != gs->currentBattles.end());
	gs->currentBattles.erase(currentBattle);
}

void BattleResultAccepted::applyGs(CGameState *gs)
{
	// Remove any "until next battle" bonuses
	for(auto & res : heroResult)
	{
		if(res.hero)
			res.hero->removeBonusesRecursive(Bonus::OneBattle);
	}

	if(winnerSide != BattleSide::NONE)
	{
		// Grow up growing artifacts
		const auto hero = heroResult[winnerSide].hero;

		if (hero)
		{
			if(hero->commander && hero->commander->alive)
			{
				for(auto & art : hero->commander->artifactsWorn)
					art.second.artifact->growingUp();
			}
			for(auto & art : hero->artifactsWorn)
			{
				art.second.artifact->growingUp();
			}
		}
	}

	if(gs->getSettings().getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE))
	{
		if(heroResult[BattleSide::ATTACKER].army)
		{
			heroResult[BattleSide::ATTACKER].army->giveStackExp(heroResult[BattleSide::ATTACKER].exp);
			heroResult[BattleSide::ATTACKER].army->nodeHasChanged();
		}
		if(heroResult[BattleSide::DEFENDER].army)
		{
			heroResult[BattleSide::DEFENDER].army->giveStackExp(heroResult[BattleSide::DEFENDER].exp);
			heroResult[BattleSide::DEFENDER].army->nodeHasChanged();
		}

	}

	auto currentBattle = boost::range::find_if(gs->currentBattles, [&](const auto & battle)
	{
		return battle->battleID == battleID;
	});

	assert(currentBattle != gs->currentBattles.end());
	gs->currentBattles.erase(currentBattle);
}

void BattleLogMessage::applyGs(CGameState *gs)
{
	//nothing
}

void BattleLogMessage::applyBattle(IBattleState * battleState)
{
	//nothing
}

void BattleStackMoved::applyGs(CGameState *gs)
{
	applyBattle(gs->getBattle(battleID));
}

void BattleStackMoved::applyBattle(IBattleState * battleState)
{
	battleState->moveUnit(stack, tilesToMove.back());
}

void BattleStackAttacked::applyGs(CGameState *gs)
{
	applyBattle(gs->getBattle(battleID));
}

void BattleStackAttacked::applyBattle(IBattleState * battleState)
{
	battleState->setUnitState(newState.id, newState.data, newState.healthDelta);
}

void BattleAttack::applyGs(CGameState *gs)
{
	CStack * attacker = gs->getBattle(battleID)->getStack(stackAttacking);
	assert(attacker);

	attackerChanges.applyGs(gs);

	for(BattleStackAttacked & stackAttacked : bsa)
		stackAttacked.applyGs(gs);

	attacker->removeBonusesRecursive(Bonus::UntilAttack);

	if(!this->counter())
		attacker->removeBonusesRecursive(Bonus::UntilOwnAttack);
}

void StartAction::applyGs(CGameState *gs)
{
	CStack *st = gs->getBattle(battleID)->getStack(ba.stackNumber);

	if(ba.actionType == EActionType::END_TACTIC_PHASE)
	{
		gs->getBattle(battleID)->tacticDistance = 0;
		return;
	}

	if(gs->getBattle(battleID)->tacticDistance)
	{
		// moves in tactics phase do not affect creature status
		// (tactics stack queue is managed by client)
		return;
	}

	if (ba.isUnitAction())
	{
		assert(st); // stack must exists for all non-hero actions

		switch(ba.actionType)
		{
			case EActionType::DEFEND:
				st->waiting = false;
				st->defending = true;
				st->defendingAnim = true;
				break;
			case EActionType::WAIT:
				st->defendingAnim = false;
				st->waiting = true;
				st->waitedThisTurn = true;
				break;
			case EActionType::HERO_SPELL: //no change in current stack state
				break;
			default: //any active stack action - attack, catapult, heal, spell...
				st->waiting = false;
				st->defendingAnim = false;
				st->movedThisRound = true;
				st->castSpellThisTurn = ba.actionType == EActionType::MONSTER_SPELL;
				break;
		}
	}
	else
	{
		if(ba.actionType == EActionType::HERO_SPELL)
			gs->getBattle(battleID)->getSide(ba.side).usedSpellsHistory.push_back(ba.spell);
	}
}

void BattleSpellCast::applyGs(CGameState *gs)
{
	if(castByHero && side != BattleSide::NONE)
		gs->getBattle(battleID)->getSide(side).castSpellsCount++;
}

void SetStackEffect::applyGs(CGameState *gs)
{
	applyBattle(gs->getBattle(battleID));
}

void SetStackEffect::applyBattle(IBattleState * battleState)
{
	for(const auto & stackData : toRemove)
		battleState->removeUnitBonus(stackData.first, stackData.second);

	for(const auto & stackData : toUpdate)
		battleState->updateUnitBonus(stackData.first, stackData.second);

	for(const auto & stackData : toAdd)
		battleState->addUnitBonus(stackData.first, stackData.second);
}


void StacksInjured::applyGs(CGameState *gs)
{
	applyBattle(gs->getBattle(battleID));
}

void StacksInjured::applyBattle(IBattleState * battleState)
{
	for(BattleStackAttacked stackAttacked : stacks)
		stackAttacked.applyBattle(battleState);
}

void BattleUnitsChanged::applyGs(CGameState *gs)
{
	applyBattle(gs->getBattle(battleID));
}

void BattleUnitsChanged::applyBattle(IBattleState * battleState)
{
	for(auto & elem : changedStacks)
	{
		switch(elem.operation)
		{
		case BattleChanges::EOperation::RESET_STATE:
			battleState->setUnitState(elem.id, elem.data, elem.healthDelta);
			break;
		case BattleChanges::EOperation::REMOVE:
			battleState->removeUnit(elem.id);
			break;
		case BattleChanges::EOperation::ADD:
			battleState->addUnit(elem.id, elem.data);
			break;
		case BattleChanges::EOperation::UPDATE:
			battleState->updateUnit(elem.id, elem.data);
			break;
		default:
			logNetwork->error("Unknown unit operation %d", static_cast<int>(elem.operation));
			break;
		}
	}
}

void BattleObstaclesChanged::applyGs(CGameState *gs)
{
	applyBattle(gs->getBattle(battleID));
}

void BattleObstaclesChanged::applyBattle(IBattleState * battleState)
{
	for(const auto & change : changes)
	{
		switch(change.operation)
		{
		case BattleChanges::EOperation::REMOVE:
			battleState->removeObstacle(change.id);
			break;
		case BattleChanges::EOperation::ADD:
			battleState->addObstacle(change);
			break;
		case BattleChanges::EOperation::UPDATE:
			battleState->updateObstacle(change);
			break;
		default:
			logNetwork->error("Unknown obstacle operation %d", static_cast<int>(change.operation));
			break;
		}
	}
}

CatapultAttack::CatapultAttack() = default;

CatapultAttack::~CatapultAttack() = default;

void CatapultAttack::applyGs(CGameState *gs)
{
	applyBattle(gs->getBattle(battleID));
}

void CatapultAttack::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitCatapultAttack(*this);
}

void CatapultAttack::applyBattle(IBattleState * battleState)
{
	const auto * town = battleState->getDefendedTown();
	if(!town)
		return;

	if(town->fortificationsLevel().wallsHealth == 0)
		return;

	for(const auto & part : attackedParts)
	{
		auto newWallState = SiegeInfo::applyDamage(battleState->getWallState(part.attackedPart), part.damageDealt);
		battleState->setWallState(part.attackedPart, newWallState);
	}
}

void BattleSetStackProperty::applyGs(CGameState *gs)
{
	CStack * stack = gs->getBattle(battleID)->getStack(stackID, false);
	switch(which)
	{
		case CASTS:
		{
			if(absolute)
				logNetwork->error("Can not change casts in absolute mode");
			else
				stack->casts.use(-val);
			break;
		}
		case ENCHANTER_COUNTER:
		{
			auto & counter = gs->getBattle(battleID)->getSide(gs->getBattle(battleID)->whatSide(stack->unitOwner())).enchanterCounter;
			if(absolute)
				counter = val;
			else
				counter += val;
			vstd::amax(counter, 0);
			break;
		}
		case UNBIND:
		{
			stack->removeBonusesRecursive(Selector::type()(BonusType::BIND_EFFECT));
			break;
		}
		case CLONED:
		{
			stack->cloned = true;
			break;
		}
		case HAS_CLONE:
		{
			stack->cloneID = val;
			break;
		}
	}
}

void PlayerCheated::applyGs(CGameState *gs)
{
	if(!player.isValidPlayer())
		return;

	gs->getPlayerState(player)->enteredLosingCheatCode = losingCheatCode;
	gs->getPlayerState(player)->enteredWinningCheatCode = winningCheatCode;
	gs->getPlayerState(player)->cheated = true;
}

void PlayerStartsTurn::applyGs(CGameState *gs)
{
	//assert(gs->actingPlayers.count(player) == 0);//Legal - may happen after loading of deserialized map state
	gs->actingPlayers.insert(player);
}

void PlayerEndsTurn::applyGs(CGameState *gs)
{
	assert(gs->actingPlayers.count(player) == 1);
	gs->actingPlayers.erase(player);
}

void DaysWithoutTown::applyGs(CGameState *gs)
{
	auto & playerState = gs->players[player];
	playerState.daysWithoutCastle = daysWithoutCastle;
}

void TurnTimeUpdate::applyGs(CGameState *gs)
{
	auto & playerState = gs->players[player];
	playerState.turnTimer = turnTimer;
}

void EntitiesChanged::applyGs(CGameState *gs)
{
	for(const auto & change : changes)
		gs->updateEntity(change.metatype, change.entityIndex, change.data);
}

void SetRewardableConfiguration::applyGs(CGameState *gs)
{
	auto * objectPtr = gs->getObjInstance(objectID);

	if (!buildingID.hasValue())
	{
		auto * rewardablePtr = dynamic_cast<CRewardableObject *>(objectPtr);
		assert(rewardablePtr);
		rewardablePtr->configuration = configuration;
		rewardablePtr->initializeGuards();
	}
	else
	{
		auto * townPtr = dynamic_cast<CGTownInstance*>(objectPtr);
		TownBuildingInstance * buildingPtr = nullptr;

		for (auto & building : townPtr->rewardableBuildings)
			if (building.second->getBuildingType() == buildingID)
				buildingPtr = building.second;

		auto * rewardablePtr = dynamic_cast<TownRewardableBuildingInstance *>(buildingPtr);
		assert(rewardablePtr);
		rewardablePtr->configuration = configuration;
	}
}

const CArtifactInstance * ArtSlotInfo::getArt() const
{
	if(locked)
		return nullptr;
	return artifact;
}

VCMI_LIB_NAMESPACE_END

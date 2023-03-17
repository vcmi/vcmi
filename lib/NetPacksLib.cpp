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
#include "NetPacks.h"
#include "NetPackVisitor.h"
#include "CGeneralTextHandler.h"
#include "mapObjects/CObjectClassesHandler.h"
#include "CArtHandler.h"
#include "CHeroHandler.h"
#include "mapObjects/CObjectHandler.h"
#include "CModHandler.h"
#include "VCMI_Lib.h"
#include "mapping/CMap.h"
#include "spells/CSpellHandler.h"
#include "CCreatureHandler.h"
#include "CGameState.h"
#include "CStack.h"
#include "battle/BattleInfo.h"
#include "CTownHandler.h"
#include "mapping/CMapInfo.h"
#include "StartInfo.h"
#include "CPlayerState.h"
#include "TerrainHandler.h"
#include "mapping/CCampaignHandler.h"
#include "GameSettings.h"


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

void YourTurn::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitYourTurn(*this);
}

void EntitiesChanged::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitEntitiesChanged(*this);
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

void SetAvailableHeroes::visitTyped(ICPackVisitor & visitor)
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

void UpdateMapEvents::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitUpdateMapEvents(*this);
}

void UpdateCastleEvents::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitUpdateCastleEvents(*this);
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

void EraseArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitEraseArtifact(*this);
}

void MoveArtifact::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitMoveArtifact(*this);
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

void PrepareHeroLevelUp::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitPrepareHeroLevelUp(*this);
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

void RazeStructure::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitRazeStructure(*this);
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

void AssembleArtifacts::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitAssembleArtifacts(*this);
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

void MakeCustomAction::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitMakeCustomAction(*this);
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

void SaveGameClient::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitSaveGameClient(*this);
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

void LobbyEndGame::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyEndGame(*this);
}

void LobbyStartGame::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbyStartGame(*this);
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

void LobbySetTurnTime::visitTyped(ICPackVisitor & visitor)
{
	visitor.visitLobbySetTurnTime(*this);
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

void SetResources::applyGs(CGameState * gs) const
{
	assert(player < PlayerColor::PLAYER_LIMIT);
	if(abs)
		gs->getPlayerState(player)->resources = res;
	else
		gs->getPlayerState(player)->resources += res;

	//just ensure that player resources are not negative
	//server is responsible to check if player can afford deal
	//but events on server side are allowed to take more than player have
	gs->getPlayerState(player)->resources.positive();
}

void SetPrimSkill::applyGs(CGameState * gs) const
{
	CGHeroInstance * hero = gs->getHero(id);
	assert(hero);
	hero->setPrimarySkill(which, val, abs);
}

void SetSecSkill::applyGs(CGameState * gs) const
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
			commander->specialSKills.insert (additionalInfo);
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
			break;
	}
}

void AddQuest::applyGs(CGameState * gs) const
{
	assert (vstd::contains(gs->players, player));
	auto * vec = &gs->players[player].quests;
	if (!vstd::contains(*vec, quest))
		vec->push_back (quest);
	else
		logNetwork->warn("Warning! Attempt to add duplicated quest");
}

void UpdateArtHandlerLists::applyGs(CGameState * gs) const
{
	VLC->arth->minors = minors;
	VLC->arth->majors = majors;
	VLC->arth->treasures = treasures;
	VLC->arth->relics = relics;
}

void UpdateMapEvents::applyGs(CGameState * gs) const
{
	gs->map->events = events;
}

void UpdateCastleEvents::applyGs(CGameState * gs) const
{
	auto * t = gs->getTown(town);
	t->events = events;
}

void ChangeFormation::applyGs(CGameState * gs) const
{
	gs->getHero(hid)->setFormation(formation);
}

void HeroVisitCastle::applyGs(CGameState * gs) const
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

void SetMana::applyGs(CGameState * gs) const
{
	CGHeroInstance * hero = gs->getHero(hid);

	assert(hero);

	if(absolute)
		hero->mana = val;
	else
		hero->mana += val;

	vstd::amax(hero->mana, 0); //not less than 0
}

void SetMovePoints::applyGs(CGameState * gs) const
{
	CGHeroInstance *hero = gs->getHero(hid);

	assert(hero);

	if(absolute)
		hero->movement = val;
	else
		hero->movement += val;

	vstd::amax(hero->movement, 0); //not less than 0
}

void FoWChange::applyGs(CGameState *gs)
{
	TeamState * team = gs->getPlayerTeam(player);
	auto fogOfWarMap = team->fogOfWarMap;
	for(const int3 & t : tiles)
		(*fogOfWarMap)[t.z][t.x][t.y] = mode;
	if (mode == 0) //do not hide too much
	{
		std::unordered_set<int3, ShashInt3> tilesRevealed;
		for (auto & elem : gs->map->objects)
		{
			const CGObjectInstance *o = elem;
			if (o)
			{
				switch(o->ID)
				{
				case Obj::HERO:
				case Obj::MINE:
				case Obj::TOWN:
				case Obj::ABANDONED_MINE:
					if(vstd::contains(team->players, o->tempOwner)) //check owned observators
						gs->getTilesInRange(tilesRevealed, o->getSightCenter(), o->getSightRadius(), o->tempOwner, 1);
					break;
				}
			}
		}
		for(const int3 & t : tilesRevealed) //probably not the most optimal solution ever
			(*fogOfWarMap)[t.z][t.x][t.y] = 1;
	}
}

void SetAvailableHeroes::applyGs(CGameState *gs)
{
	PlayerState *p = gs->getPlayerState(player);
	p->availableHeroes.clear();

	for (int i = 0; i < GameConstants::AVAILABLE_HEROES_PER_PLAYER; i++)
	{
		CGHeroInstance *h = (hid[i]>=0 ?  gs->hpool.heroesPool[hid[i]].get() : nullptr);
		if(h && army[i])
			h->setToArmy(army[i]);
		p->availableHeroes.emplace_back(h);
	}
}

void GiveBonus::applyGs(CGameState *gs)
{
	CBonusSystemNode *cbsn = nullptr;
	switch(who)
	{
	case HERO:
		cbsn = gs->getHero(ObjectInstanceID(id));
		break;
	case PLAYER:
		cbsn = gs->getPlayerState(PlayerColor(id));
		break;
	case TOWN:
		cbsn = gs->getTown(ObjectInstanceID(id));
		break;
	}

	assert(cbsn);

	if(Bonus::OneWeek(&bonus))
		bonus.turnsRemain = 8 - gs->getDate(Date::DAY_OF_WEEK); // set correct number of days before adding bonus

	auto b = std::make_shared<Bonus>(bonus);
	cbsn->addNewBonus(b);

	std::string &descr = b->description;

	if(bdescr.message.empty() && (bonus.type == Bonus::LUCK || bonus.type == Bonus::MORALE))
	{
		if (bonus.source == Bonus::OBJECT)
		{
			descr = VLC->generaltexth->arraytxt[bonus.val > 0 ? 110 : 109]; //+/-%d Temporary until next battle"
		}
		else if(bonus.source == Bonus::TOWN_STRUCTURE)
		{
			descr = bonus.description;
			return;
		}
		else
		{
			bdescr.toString(descr);
		}
	}
	else
	{
		bdescr.toString(descr);
	}
	// Some of(?) versions of H3 use %s here instead of %d. Try to replace both of them
	boost::replace_first(descr, "%d", std::to_string(std::abs(bonus.val)));
	boost::replace_first(descr, "%s", std::to_string(std::abs(bonus.val)));
}

void ChangeObjPos::applyGs(CGameState *gs)
{
	CGObjectInstance *obj = gs->getObjInstance(objid);
	if(!obj)
	{
		logNetwork->error("Wrong ChangeObjPos: object %d doesn't exist!", objid.getNum());
		return;
	}
	gs->map->removeBlockVisTiles(obj);
	obj->pos = nPos;
	gs->map->addBlockVisTiles(obj);
}

void ChangeObjectVisitors::applyGs(CGameState * gs) const
{
	switch (mode) {
		case VISITOR_ADD:
			gs->getHero(hero)->visitedObjects.insert(object);
			gs->getPlayerState(gs->getHero(hero)->tempOwner)->visitedObjects.insert(object);
			break;
		case VISITOR_ADD_TEAM:
			{
				TeamState *ts = gs->getPlayerTeam(gs->getHero(hero)->tempOwner);
				for(const auto & color : ts->players)
				{
					gs->getPlayerState(color)->visitedObjects.insert(object);
				}
			}
			break;
		case VISITOR_CLEAR:
			for (CGHeroInstance * hero : gs->map->allHeroes)
			{
				if (hero)
				{
					hero->visitedObjects.erase(object); // remove visit info from all heroes, including those that are not present on map
				}
			}

			for(auto &elem : gs->players)
			{
				elem.second.visitedObjects.erase(object);
			}

			break;
		case VISITOR_REMOVE:
			gs->getHero(hero)->visitedObjects.erase(object);
			break;
	}
}

void PlayerEndsGame::applyGs(CGameState * gs) const
{
	PlayerState *p = gs->getPlayerState(player);
	if(victoryLossCheckResult.victory())
	{
		p->status = EPlayerStatus::WINNER;

		// TODO: Campaign-specific code might as well go somewhere else
		if(p->human && gs->scenarioOps->campState)
		{
			std::vector<CGHeroInstance *> crossoverHeroes;
			for (CGHeroInstance * hero : gs->map->heroesOnMap)
			{
				if (hero->tempOwner == player)
				{
					// keep all heroes from the winning player
					crossoverHeroes.push_back(hero);
				}
				else if (vstd::contains(gs->scenarioOps->campState->getCurrentScenario().keepHeroes, HeroTypeID(hero->subID)))
				{
					// keep hero whether lost or won (like Xeron in AB campaign)
					crossoverHeroes.push_back(hero);
				}
			}
			// keep lost heroes which are in heroes pool
			for (auto & heroPair : gs->hpool.heroesPool)
			{
				if (vstd::contains(gs->scenarioOps->campState->getCurrentScenario().keepHeroes, HeroTypeID(heroPair.first)))
				{
					crossoverHeroes.push_back(heroPair.second.get());
				}
			}

			gs->scenarioOps->campState->setCurrentMapAsConquered(crossoverHeroes);
		}
	}
	else
	{
		p->status = EPlayerStatus::LOSER;
	}
}

void PlayerReinitInterface::applyGs(CGameState *gs)
{
	if(!gs || !gs->scenarioOps)
		return;
	
	//TODO: what does mean if more that one player connected?
	if(playerConnectionId == PlayerSettings::PLAYER_AI)
	{
		for(const auto & player : players)
			gs->scenarioOps->getIthPlayersSettings(player).connectedPlayerIDs.clear();
	}
}

void RemoveBonus::applyGs(CGameState *gs)
{
	CBonusSystemNode * node = nullptr;
	if (who == HERO)
		node = gs->getHero(ObjectInstanceID(whoID));
	else
		node = gs->getPlayerState(PlayerColor(whoID));

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

	CGObjectInstance *obj = gs->getObjInstance(id);
	logGlobal->debug("removing object id=%d; address=%x; name=%s", id, (intptr_t)obj, obj->getObjectName());
	//unblock tiles
	gs->map->removeBlockVisTiles(obj);

	if(obj->ID == Obj::HERO) //remove beaten hero
	{
		auto * beatenHero = dynamic_cast<CGHeroInstance *>(obj);
		assert(beatenHero);
		PlayerState * p = gs->getPlayerState(beatenHero->tempOwner);
		gs->map->heroesOnMap -= beatenHero;
		p->heroes -= beatenHero;
		beatenHero->detachFrom(*beatenHero->whereShouldBeAttachedOnSiege(gs));
		beatenHero->tempOwner = PlayerColor::NEUTRAL; //no one owns beaten hero
		vstd::erase_if(beatenHero->artifactsInBackpack, [](const ArtSlotInfo& asi)
		{
			return asi.artifact->artType->getId() == ArtifactID::GRAIL;
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
		gs->hpool.heroesPool[beatenHero->subID] = beatenHero;

		if(!vstd::contains(gs->hpool.pavailable, beatenHero->subID))
			gs->hpool.pavailable[beatenHero->subID] = 0xff;

		gs->map->objects[id.getNum()] = nullptr;

		//If hero on Boat is removed, the Boat disappears
		if(beatenHero->boat)
		{
			gs->map->instanceNames.erase(beatenHero->boat->instanceName);
			gs->map->objects[beatenHero->boat->id.getNum()].dellNull();
			beatenHero->boat = nullptr;
		}
		return;
	}

	const auto * quest = dynamic_cast<const IQuestObject *>(obj);
	if (quest)
	{
		gs->map->quests[quest->quest->qid] = nullptr;
		for (auto &player : gs->players)
		{
			for (auto &q : player.second.quests)
			{
				if (q.obj == obj)
				{
					q.obj = nullptr;
				}
			}
		}
	}

	for (TriggeredEvent & event : gs->map->triggeredEvents)
	{
		auto patcher = [&](EventCondition cond) -> EventExpression::Variant
		{
			if (cond.object == obj)
			{
				if (cond.condition == EventCondition::DESTROY || cond.condition == EventCondition::DESTROY_0)
				{
					cond.condition = EventCondition::CONST_VALUE;
					cond.value = 1; // destroyed object, from now on always fulfilled
				}
				else if (cond.condition == EventCondition::CONTROL || cond.condition == EventCondition::HAVE_0)
				{
					cond.condition = EventCondition::CONST_VALUE;
					cond.value = 0; // destroyed object, from now on can not be fulfilled
				}
			}
			return cond;
		};
		event.trigger = event.trigger.morph(patcher);
	}
	gs->map->instanceNames.erase(obj->instanceName);
	gs->map->objects[id.getNum()].dellNull();
	gs->map->calculateGuardingGreaturePositions();
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

	h->movement = movePoints;

	if((result == SUCCESS || result == BLOCKING_VISIT || result == EMBARK || result == DISEMBARK) && start != end)
	{
		auto dir = getDir(start,end);
		if(dir > 0  &&  dir <= 8)
			h->moveDir = dir;
		//else don`t change move direction - hero might have traversed the subterranean gate, direction should be kept
	}

	if(result == EMBARK) //hero enters boat at destination tile
	{
		const TerrainTile &tt = gs->map->getTile(h->convertToVisitablePos(end));
		assert(tt.visitableObjects.size() >= 1  &&  tt.visitableObjects.back()->ID == Obj::BOAT); //the only visitable object at destination is Boat
		auto * boat = dynamic_cast<CGBoat *>(tt.visitableObjects.back());
		assert(boat);

		gs->map->removeBlockVisTiles(boat); //hero blockvis mask will be used, we don't need to duplicate it with boat
		h->boat = boat;
		boat->hero = h;
	}
	else if(result == DISEMBARK) //hero leaves boat to destination tile
	{
		auto * b = const_cast<CGBoat *>(h->boat);
		b->direction = h->moveDir;
		b->pos = start;
		b->hero = nullptr;
		gs->map->addBlockVisTiles(b);
		h->boat = nullptr;
	}

	if(start!=end && (result == SUCCESS || result == TELEPORTATION || result == EMBARK || result == DISEMBARK))
	{
		gs->map->removeBlockVisTiles(h);
		h->pos = end;
		if(auto * b = const_cast<CGBoat *>(h->boat))
			b->pos = end;
		gs->map->addBlockVisTiles(h);
	}

	auto fogOfWarMap = gs->getPlayerTeam(h->getOwner())->fogOfWarMap;
	for(const int3 & t : fowRevealed)
		(*fogOfWarMap)[t.z][t.x][t.y] = 1;
}

void NewStructures::applyGs(CGameState *gs)
{
	CGTownInstance *t = gs->getTown(tid);

	for(const auto & id : bid)
	{
		assert(t->town->buildings.at(id) != nullptr);
		t->builtBuildings.insert(id);
		t->updateAppearance();
		auto currentBuilding = t->town->buildings.at(id);

		if(currentBuilding->overrideBids.empty())
			continue;

		for(const auto & overrideBid : currentBuilding->overrideBids)
		{
			t->overriddenBuildings.insert(overrideBid);
			t->deleteTownBonus(overrideBid);
		}
	}
	t->builded = builded;
	t->recreateBuildingsBonuses();
}

void RazeStructures::applyGs(CGameState *gs)
{
	CGTownInstance *t = gs->getTown(tid);
	for(const auto & id : bid)
	{
		t->builtBuildings.erase(id);

		t->updateAppearance();
	}
	t->destroyed = destroyed; //yeaha
	t->recreateBuildingsBonuses();
}

void SetAvailableCreatures::applyGs(CGameState * gs) const
{
	auto * dw = dynamic_cast<CGDwelling *>(gs->getObjInstance(tid));
	assert(dw);
	dw->creatures = creatures;
}

void SetHeroesInTown::applyGs(CGameState * gs) const
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
		gs->map->addBlockVisTiles(v);
	}
	if(g)
	{
		gs->map->removeBlockVisTiles(g);
	}
}

void HeroRecruited::applyGs(CGameState * gs) const
{
	assert(vstd::contains(gs->hpool.heroesPool, hid));
	CGHeroInstance *h = gs->hpool.heroesPool[hid];
	CGTownInstance *t = gs->getTown(tid);
	PlayerState *p = gs->getPlayerState(player);

	assert(!h->boat);

	h->setOwner(player);
	h->pos = tile;
	bool fresh = !h->isInitialized();
	if(fresh)
	{ // this is a fresh hero who hasn't appeared yet
		h->movement = h->maxMovePoints(true);
	}

	gs->hpool.heroesPool.erase(hid);
	if(h->id == ObjectInstanceID())
	{
		h->id = ObjectInstanceID(static_cast<si32>(gs->map->objects.size()));
		gs->map->objects.emplace_back(h);
	}
	else
		gs->map->objects[h->id.getNum()] = h;

	gs->map->heroesOnMap.emplace_back(h);
	p->heroes.emplace_back(h);
	h->attachTo(*p);
	if(fresh)
	{
		h->initObj(gs->getRandomGenerator());
	}
	gs->map->addBlockVisTiles(h);

	if(t)
	{
		t->setVisitingHero(h);
	}
}

void GiveHero::applyGs(CGameState * gs) const
{
	CGHeroInstance *h = gs->getHero(id);

	//bonus system
	h->detachFrom(gs->globalEffects);
	h->attachTo(*gs->getPlayerState(player));

	auto oldVisitablePos = h->visitablePos();
	gs->map->removeBlockVisTiles(h,true);
	h->appearance = VLC->objtypeh->getHandlerFor(Obj::HERO, h->type->heroClass->getIndex())->getTemplates().front();

	h->setOwner(player);
	h->movement =  h->maxMovePoints(true);
	h->pos = h->convertFromVisitablePos(oldVisitablePos);
	gs->map->heroesOnMap.emplace_back(h);
	gs->getPlayerState(h->getOwner())->heroes.emplace_back(h);

	gs->map->addBlockVisTiles(h);
	h->inTownGarrison = false;
}

void NewObject::applyGs(CGameState *gs)
{
	TerrainId terrainType = ETerrainId::NONE;

	if(ID == Obj::BOAT && !gs->isInTheMap(pos)) //special handling for bug #3060 - pos outside map but visitablePos is not
	{
		CGObjectInstance testObject = CGObjectInstance();
		testObject.pos = pos;
		testObject.appearance = VLC->objtypeh->getHandlerFor(ID, subID)->getTemplates(ETerrainId::WATER).front();

		const int3 previousXAxisTile = int3(pos.x - 1, pos.y, pos.z);
		assert(gs->isInTheMap(previousXAxisTile) && (testObject.visitablePos() == previousXAxisTile));
		MAYBE_UNUSED(previousXAxisTile);
	}
	else
	{
		const TerrainTile & t = gs->map->getTile(pos);
		terrainType = t.terType->getId();
	}

	CGObjectInstance *o = nullptr;
	switch(ID)
	{
	case Obj::BOAT:
		o = new CGBoat();
		terrainType = ETerrainId::WATER; //TODO: either boat should only spawn on water, or all water objects should be handled this way
		break;
	case Obj::MONSTER: //probably more options will be needed
		o = new CGCreature();
		{
			//CStackInstance hlp;
			auto * cre = dynamic_cast<CGCreature *>(o);
			//cre->slots[0] = hlp;
			assert(cre);
			cre->notGrowingTeam = cre->neverFlees = false;
			cre->character = 2;
			cre->gainedArtifact = ArtifactID::NONE;
			cre->identifier = -1;
			cre->addToSlot(SlotID(0), new CStackInstance(CreatureID(subID), -1)); //add placeholder stack
		}
		break;
	default:
		o = new CGObjectInstance();
		break;
	}
	o->ID = ID;
	o->subID = subID;
	o->pos = pos;
	o->appearance = VLC->objtypeh->getHandlerFor(o->ID, o->subID)->getTemplates(terrainType).front();
	id = o->id = ObjectInstanceID(static_cast<si32>(gs->map->objects.size()));

	gs->map->objects.emplace_back(o);
	gs->map->addBlockVisTiles(o);
	o->initObj(gs->getRandomGenerator());
	gs->map->calculateGuardingGreaturePositions();

	logGlobal->debug("Added object id=%d; address=%x; name=%s", id, (intptr_t)o, o->getObjectName());
}

void NewArtifact::applyGs(CGameState *gs)
{
	assert(!vstd::contains(gs->map->artInstances, art));
	gs->map->addNewArtifactInstance(art);

	assert(!art->getParentNodes().size());
	art->setType(art->artType);
	if(auto * cart = dynamic_cast<CCombinedArtifactInstance *>(art.get()))
		cart->createConstituents();
}

const CStackInstance * StackLocation::getStack()
{
	if(!army->hasStackAtSlot(slot))
	{
		logNetwork->warn("%s don't have a stack at slot %d", army->nodeName(), slot.getNum());
		return nullptr;
	}
	return &army->getStack(slot);
}

struct ObjectRetriever : boost::static_visitor<const CArmedInstance *>
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
template <typename T>
struct GetBase : boost::static_visitor<T*>
{
	template <typename TArg>
	T * operator()(TArg &arg) const
	{
		return arg;
	}
};


void ArtifactLocation::removeArtifact()
{
	CArtifactInstance *a = getArt();
	assert(a);
	a->removeFrom(*this);
}

const CArmedInstance * ArtifactLocation::relatedObj() const
{
	return boost::apply_visitor(ObjectRetriever(), artHolder);
}

PlayerColor ArtifactLocation::owningPlayer() const
{
	const auto * obj = relatedObj();
	return obj ? obj->tempOwner : PlayerColor::NEUTRAL;
}

CArtifactSet *ArtifactLocation::getHolderArtSet()
{
	return boost::apply_visitor(GetBase<CArtifactSet>(), artHolder);
}

CBonusSystemNode *ArtifactLocation::getHolderNode()
{
	return boost::apply_visitor(GetBase<CBonusSystemNode>(), artHolder);
}

const CArtifactInstance *ArtifactLocation::getArt() const
{
	const auto * s = getSlot();
	if(s)
		return s->getArt();
	else
		return nullptr;
}

const CArtifactSet * ArtifactLocation::getHolderArtSet() const
{
	auto * t = const_cast<ArtifactLocation *>(this);
	return t->getHolderArtSet();
}

const CBonusSystemNode * ArtifactLocation::getHolderNode() const
{
	auto * t = const_cast<ArtifactLocation *>(this);
	return t->getHolderNode();
}

CArtifactInstance *ArtifactLocation::getArt()
{
	const ArtifactLocation *t = this;
	return const_cast<CArtifactInstance*>(t->getArt());
}

const ArtSlotInfo *ArtifactLocation::getSlot() const
{
	return getHolderArtSet()->getSlot(slot);
}

void ChangeStackCount::applyGs(CGameState * gs)
{
	auto * srcObj = gs->getArmyInstance(army);
	if(!srcObj)
		logNetwork->error("[CRITICAL] ChangeStackCount: invalid army object %d, possible game state corruption.", army.getNum());

	if(absoluteValue)
		srcObj->setStackCount(slot, count);
	else
		srcObj->changeStackCount(slot, count);
}

void SetStackType::applyGs(CGameState * gs)
{
	auto * srcObj = gs->getArmyInstance(army);
	if(!srcObj)
		logNetwork->error("[CRITICAL] SetStackType: invalid army object %d, possible game state corruption.", army.getNum());

	srcObj->setStackType(slot, type);
}

void EraseStack::applyGs(CGameState * gs)
{
	auto * srcObj = gs->getArmyInstance(army);
	if(!srcObj)
		logNetwork->error("[CRITICAL] EraseStack: invalid army object %d, possible game state corruption.", army.getNum());

	srcObj->eraseStack(slot);
}

void SwapStacks::applyGs(CGameState * gs)
{
	auto * srcObj = gs->getArmyInstance(srcArmy);
	if(!srcObj)
		logNetwork->error("[CRITICAL] SwapStacks: invalid army object %d, possible game state corruption.", srcArmy.getNum());

	auto * dstObj = gs->getArmyInstance(dstArmy);
	if(!dstObj)
		logNetwork->error("[CRITICAL] SwapStacks: invalid army object %d, possible game state corruption.", dstArmy.getNum());

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
		logNetwork->error("[CRITICAL] InsertNewStack: invalid army object %d, possible game state corruption.", army.getNum());
}

void RebalanceStacks::applyGs(CGameState * gs)
{
	auto * srcObj = gs->getArmyInstance(srcArmy);
	if(!srcObj)
		logNetwork->error("[CRITICAL] RebalanceStacks: invalid army object %d, possible game state corruption.", srcArmy.getNum());

	auto * dstObj = gs->getArmyInstance(dstArmy);
	if(!dstObj)
		logNetwork->error("[CRITICAL] RebalanceStacks: invalid army object %d, possible game state corruption.", dstArmy.getNum());

	StackLocation src(srcObj, srcSlot);
	StackLocation dst(dstObj, dstSlot);

	const CCreature * srcType = src.army->getCreature(src.slot);
	TQuantity srcCount = src.army->getStackCount(src.slot);
	bool stackExp = VLC->settings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE);

	if(srcCount == count) //moving whole stack
	{
		if(const CCreature *c = dst.army->getCreature(dst.slot)) //stack at dest -> merge
		{
			assert(c == srcType);
			MAYBE_UNUSED(c);
			auto alHere = ArtifactLocation (src.getStack(), ArtifactPosition::CREATURE_SLOT);
			auto alDest = ArtifactLocation (dst.getStack(), ArtifactPosition::CREATURE_SLOT);
			auto * artHere = alHere.getArt();
			auto * artDest = alDest.getArt();
			if (artHere)
			{
				if (alDest.getArt())
				{
					auto * hero = dynamic_cast<CGHeroInstance *>(src.army.get());
					if (hero)
					{
						artDest->move (alDest, ArtifactLocation (hero, alDest.getArt()->firstBackpackSlot (hero)));
					}
					//else - artifact cna be lost :/
					else
					{
						logNetwork->warn("Artifact is present at destination slot!");
					}
					artHere->move (alHere, alDest);
					//TODO: choose from dialog
				}
				else //just move to the other slot before stack gets erased
				{
					artHere->move (alHere, alDest);
				}
			}
			if (stackExp)
			{
				ui64 totalExp = srcCount * src.army->getStackExperience(src.slot) + dst.army->getStackCount(dst.slot) * dst.army->getStackExperience(dst.slot);
				src.army->eraseStack(src.slot);
				dst.army->changeStackCount(dst.slot, count);
				dst.army->setStackExp(dst.slot, totalExp /(dst.army->getStackCount(dst.slot))); //mean
			}
			else
			{
				src.army->eraseStack(src.slot);
				dst.army->changeStackCount(dst.slot, count);
			}
		}
		else //move stack to an empty slot, no exp change needed
		{
			CStackInstance *stackDetached = src.army->detachStack(src.slot);
			dst.army->putStack(dst.slot, stackDetached);
		}
	}
	else
	{
		if(const CCreature *c = dst.army->getCreature(dst.slot)) //stack at dest -> rebalance
		{
			assert(c == srcType);
			MAYBE_UNUSED(c);
			if (stackExp)
			{
				ui64 totalExp = srcCount * src.army->getStackExperience(src.slot) + dst.army->getStackCount(dst.slot) * dst.army->getStackExperience(dst.slot);
				src.army->changeStackCount(src.slot, -count);
				dst.army->changeStackCount(dst.slot, count);
				dst.army->setStackExp(dst.slot, totalExp /(src.army->getStackCount(src.slot) + dst.army->getStackCount(dst.slot))); //mean
			}
			else
			{
				src.army->changeStackCount(src.slot, -count);
				dst.army->changeStackCount(dst.slot, count);
			}
		}
		else //split stack to an empty slot
		{
			src.army->changeStackCount(src.slot, -count);
			dst.army->addToSlot(dst.slot, srcType->idNumber, count, false);
			if (stackExp)
				dst.army->setStackExp(dst.slot, src.army->getStackExperience(src.slot));
		}
	}

	CBonusSystemNode::treeHasChanged();
}

void BulkRebalanceStacks::applyGs(CGameState * gs)
{
	for(auto & move : moves)
		move.applyGs(gs);
}

void BulkSmartRebalanceStacks::applyGs(CGameState * gs)
{
	for(auto & move : moves)
		move.applyGs(gs);

	for(auto & change : changes)
		change.applyGs(gs);
}

void PutArtifact::applyGs(CGameState *gs)
{
	assert(art->canBePutAt(al));
	art->putAt(al);
	//al.hero->putArtifact(al.slot, art);
}

void EraseArtifact::applyGs(CGameState *gs)
{
	const auto * slot = al.getSlot();
	if(slot->locked)
	{
		logGlobal->debug("Erasing locked artifact: %s", slot->artifact->artType->getNameTranslated());
		DisassembledArtifact dis;
		dis.al.artHolder = al.artHolder;
		auto * aset = al.getHolderArtSet();
		#ifndef NDEBUG
		bool found = false;
		#endif
		for(auto& p : aset->artifactsWorn)
		{
			auto art = p.second.artifact;
			if(art->canBeDisassembled() && art->isPart(slot->artifact))
			{
				dis.al.slot = aset->getArtPos(art);
				#ifndef NDEBUG
				found = true;
				#endif
				break;
			}
		}
		assert(found && "Failed to determine the assembly this locked artifact belongs to");
		logGlobal->debug("Found the corresponding assembly: %s", dis.al.getSlot()->artifact->artType->getNameTranslated());
		dis.applyGs(gs);
	}
	else
	{
		logGlobal->debug("Erasing artifact %s", slot->artifact->artType->getNameTranslated());
	}
	al.removeArtifact();
}

void MoveArtifact::applyGs(CGameState * gs)
{
	CArtifactInstance * art = src.getArt();
	if(!ArtifactUtils::isSlotBackpack(dst.slot))
		assert(!dst.getArt());

	art->move(src, dst);
}

void BulkMoveArtifacts::applyGs(CGameState * gs)
{
	enum class EBulkArtsOp
	{
		BULK_MOVE,
		BULK_REMOVE,
		BULK_PUT
	};

	auto bulkArtsOperation = [this](std::vector<LinkedSlots> & artsPack, 
		CArtifactSet * artSet, EBulkArtsOp operation) -> void
	{
		int numBackpackArtifactsMoved = 0;
		for(auto & slot : artsPack)
		{
			// When an object gets removed from the backpack, the backpack shrinks
			// so all the following indices will be affected. Thus, we need to update
			// the subsequent artifact slots to account for that
			auto srcPos = slot.srcPos;
			if(ArtifactUtils::isSlotBackpack(srcPos) && (operation != EBulkArtsOp::BULK_PUT))
			{
				srcPos = ArtifactPosition(srcPos.num - numBackpackArtifactsMoved);
			}
			const auto * slotInfo = artSet->getSlot(srcPos);
			assert(slotInfo);
			auto * art = const_cast<CArtifactInstance *>(slotInfo->getArt());
			assert(art);
			switch(operation)
			{
			case EBulkArtsOp::BULK_MOVE:
				const_cast<CArtifactInstance*>(art)->move(
					ArtifactLocation(srcArtHolder, srcPos), ArtifactLocation(dstArtHolder, slot.dstPos));
				break;
			case EBulkArtsOp::BULK_REMOVE:
				art->removeFrom(ArtifactLocation(dstArtHolder, srcPos));
				break;
			case EBulkArtsOp::BULK_PUT:
				art->putAt(ArtifactLocation(srcArtHolder, slot.dstPos));
				break;
			default:
				break;
			}

			if(srcPos >= GameConstants::BACKPACK_START)
			{
				numBackpackArtifactsMoved++;
			}
		}
	};
	
	if(swap)
	{
		// Swap
		auto * leftSet = getSrcHolderArtSet();
		auto * rightSet = getDstHolderArtSet();
		CArtifactFittingSet artFittingSet(leftSet->bearerType());

		artFittingSet.artifactsWorn = rightSet->artifactsWorn;
		artFittingSet.artifactsInBackpack = rightSet->artifactsInBackpack;

		bulkArtsOperation(artsPack1, rightSet, EBulkArtsOp::BULK_REMOVE);
		bulkArtsOperation(artsPack0, leftSet, EBulkArtsOp::BULK_MOVE);
		bulkArtsOperation(artsPack1, &artFittingSet, EBulkArtsOp::BULK_PUT);
	}
	else
	{
		bulkArtsOperation(artsPack0, getSrcHolderArtSet(), EBulkArtsOp::BULK_MOVE);
	}
}

void AssembledArtifact::applyGs(CGameState *gs)
{
	CArtifactSet * artSet = al.getHolderArtSet();
	const CArtifactInstance *transformedArt = al.getArt();
	assert(transformedArt);
	bool combineEquipped = !ArtifactUtils::isSlotBackpack(al.slot);
	assert(vstd::contains_if(transformedArt->assemblyPossibilities(artSet, combineEquipped), [=](const CArtifact * art)->bool
		{
			return art->getId() == builtArt->getId();
		}));
	MAYBE_UNUSED(transformedArt);

	auto * combinedArt = new CCombinedArtifactInstance(builtArt);
	gs->map->addNewArtifactInstance(combinedArt);
	// Retrieve all constituents
	for(const CArtifact * constituent : *builtArt->constituents)
	{
		ArtifactPosition pos = combineEquipped ? artSet->getArtPos(constituent->getId(), true, false) :
			artSet->getArtBackpackPos(constituent->getId());
		assert(pos >= 0);
		CArtifactInstance * constituentInstance = artSet->getArt(pos);

		//move constituent from hero to be part of new, combined artifact
		constituentInstance->removeFrom(ArtifactLocation(al.artHolder, pos));
		combinedArt->addAsConstituent(constituentInstance, pos);
		if(combineEquipped)
		{
			if(!vstd::contains(combinedArt->artType->possibleSlots[artSet->bearerType()], al.slot)
				&& vstd::contains(combinedArt->artType->possibleSlots[artSet->bearerType()], pos))
				al.slot = pos;
		}
		else
		{
			al.slot = std::min(al.slot, pos);
		}
	}

	//put new combined artifacts
	combinedArt->putAt(al);
}

void DisassembledArtifact::applyGs(CGameState *gs)
{
	auto * disassembled = dynamic_cast<CCombinedArtifactInstance *>(al.getArt());
	assert(disassembled);

	std::vector<CCombinedArtifactInstance::ConstituentInfo> constituents = disassembled->constituentsInfo;
	disassembled->removeFrom(al);
	for(CCombinedArtifactInstance::ConstituentInfo &ci : constituents)
	{
		ArtifactLocation constituentLoc = al;
		constituentLoc.slot = (ci.slot >= 0 ? ci.slot : al.slot); //-1 is slot of main constituent -> it'll replace combined artifact in its pos
		disassembled->detachFrom(*ci.art);
		ci.art->putAt(constituentLoc);
	}

	gs->map->eraseArtifactInstance(disassembled);
}

void HeroVisit::applyGs(CGameState *gs)
{
}

void SetAvailableArtifacts::applyGs(CGameState * gs) const
{
	if(id >= 0)
	{
		if(auto * bm = dynamic_cast<CGBlackMarket *>(gs->map->objects[id].get()))
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
		CGTownInstance::merchantArtifacts = arts;
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

	for(const NewTurn::Hero & h : heroes) //give mana/movement point
	{
		CGHeroInstance *hero = gs->getHero(h.id);
		if(!hero)
		{
			// retreated or surrendered hero who has not been reset yet
			for(auto& hp : gs->hpool.heroesPool)
			{
				if(hp.second->id == h.id)
				{
					hero = hp.second;
					break;
				}
			}
		}
		if(!hero)
		{
			logGlobal->error("Hero %d not found in NewTurn::applyGs", h.id.getNum());
			continue;
		}
		hero->movement = h.move;
		hero->mana = h.mana;
	}

	for(const auto & re : res)
	{
		assert(re.first < PlayerColor::PLAYER_LIMIT);
		gs->getPlayerState(re.first)->resources = re.second;
	}

	for(const auto & creatureSet : cres) //set available creatures in towns
		creatureSet.second.applyGs(gs);

	for(CGTownInstance* t : gs->map->towns)
		t->builded = 0;

	if(gs->getDate(Date::DAY_OF_WEEK) == 1)
		gs->updateRumor();

	//count days without town for all players, regardless of their turn order
	for (auto &p : gs->players)
	{
		PlayerState & playerState = p.second;
		if (playerState.status == EPlayerStatus::INGAME)
		{
			if (playerState.towns.empty())
			{
				if (playerState.daysWithoutCastle)
					++(*playerState.daysWithoutCastle);
				else
					playerState.daysWithoutCastle = boost::make_optional(0);
			}
			else
			{
				playerState.daysWithoutCastle = boost::none;
			}
		}
	}
}

void SetObjectProperty::applyGs(CGameState * gs) const
{
	CGObjectInstance *obj = gs->getObjInstance(id);
	if(!obj)
	{
		logNetwork->error("Wrong object ID - property cannot be set!");
		return;
	}

	auto * cai = dynamic_cast<CArmedInstance *>(obj);
	if(what == ObjProperty::OWNER && cai)
	{
		if(obj->ID == Obj::TOWN)
		{
			auto * t = dynamic_cast<CGTownInstance *>(obj);
			assert(t);
			if(t->tempOwner < PlayerColor::PLAYER_LIMIT)
				gs->getPlayerState(t->tempOwner)->towns -= t;
			if(val < PlayerColor::PLAYER_LIMIT_I)
			{
				PlayerState * p = gs->getPlayerState(PlayerColor(val));
				p->towns.emplace_back(t);

				//reset counter before NewTurn to avoid no town message if game loaded at turn when one already captured
				if(p->daysWithoutCastle)
					p->daysWithoutCastle = boost::none;
			}
		}

		CBonusSystemNode & nodeToMove = cai->whatShouldBeAttached();
		nodeToMove.detachFrom(cai->whereShouldBeAttached(gs));
		obj->setProperty(what,val);
		nodeToMove.attachTo(cai->whereShouldBeAttached(gs));
	}
	else //not an armed instance
	{
		obj->setProperty(what,val);
	}
}

void PrepareHeroLevelUp::applyGs(CGameState * gs)
{
	auto * hero = gs->getHero(heroId);
	assert(hero);

	auto proposedSkills = hero->getLevelUpProposedSecondarySkills();

	if(skills.size() == 1 || hero->tempOwner == PlayerColor::NEUTRAL) //choose skill automatically
	{
		skills.push_back(*RandomGeneratorUtil::nextItem(proposedSkills, hero->skillsInfo.rand));
	}
	else
	{
		skills = proposedSkills;
	}
}

void HeroLevelUp::applyGs(CGameState * gs) const
{
	auto * hero = gs->getHero(heroId);
	assert(hero);
	hero->levelUp(skills);
}

void CommanderLevelUp::applyGs(CGameState * gs) const
{
	auto * hero = gs->getHero(heroId);
	assert(hero);
	auto commander = hero->commander;
	assert(commander);
	commander->levelUp();
}

void BattleStart::applyGs(CGameState * gs) const
{
	gs->curB = info;
	gs->curB->localInit();
}

void BattleNextRound::applyGs(CGameState * gs) const
{
	gs->curB->nextRound(round);
}

void BattleSetActiveStack::applyGs(CGameState * gs) const
{
	gs->curB->nextTurn(stack);
}

void BattleTriggerEffect::applyGs(CGameState * gs) const
{
	CStack * st = gs->curB->getStack(stackID);
	assert(st);
	switch(effect)
	{
	case Bonus::HP_REGENERATION:
	{
		int64_t toHeal = val;
		st->heal(toHeal, EHealLevel::HEAL, EHealPower::PERMANENT);
		break;
	}
	case Bonus::MANA_DRAIN:
	{
		CGHeroInstance * h = gs->getHero(ObjectInstanceID(additionalInfo));
		st->drainedMana = true;
		h->mana -= val;
		vstd::amax(h->mana, 0);
		break;
	}
	case Bonus::POISON:
	{
		auto b = st->getBonusLocalFirst(Selector::source(Bonus::SPELL_EFFECT, SpellID::POISON)
				.And(Selector::type()(Bonus::STACK_HEALTH)));
		if (b)
			b->val = val;
		break;
	}
	case Bonus::ENCHANTER:
		break;
	case Bonus::FEAR:
		st->fear = true;
		break;
	default:
		logNetwork->error("Unrecognized trigger effect type %d", effect);
	}
}

void BattleUpdateGateState::applyGs(CGameState * gs) const
{
	if(gs->curB)
		gs->curB->si.gateState = state;
}

void BattleResult::applyGs(CGameState *gs)
{
	for (auto & elem : gs->curB->stacks)
		delete elem;


	for(int i = 0; i < 2; ++i)
	{
		if(auto * h = gs->curB->battleGetFightingHero(i))
		{
			h->removeBonusesRecursive(Bonus::OneBattle); 	//remove any "until next battle" bonuses
			if (h->commander && h->commander->alive)
			{
				for (auto art : h->commander->artifactsWorn) //increment bonuses for commander artifacts
				{
					art.second.artifact->artType->levelUpArtifact (art.second.artifact);
				}
			}
		}
	}

	if(VLC->settings()->getBoolean(EGameSettings::MODULE_STACK_EXPERIENCE))
	{
		for(int i = 0; i < 2; i++)
			if(exp[i])
				gs->curB->battleGetArmyObject(i)->giveStackExp(exp[i]);

		CBonusSystemNode::treeHasChanged();
	}

	for(int i = 0; i < 2; i++)
		gs->curB->battleGetArmyObject(i)->battle = nullptr;

	gs->curB.dellNull();
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
	applyBattle(gs->curB);
}

void BattleStackMoved::applyBattle(IBattleState * battleState)
{
	battleState->moveUnit(stack, tilesToMove.back());
}

void BattleStackAttacked::applyGs(CGameState * gs)
{
	applyBattle(gs->curB);
}

void BattleStackAttacked::applyBattle(IBattleState * battleState)
{
	battleState->setUnitState(newState.id, newState.data, newState.healthDelta);
}

void BattleAttack::applyGs(CGameState * gs)
{
	CStack * attacker = gs->curB->getStack(stackAttacking);
	assert(attacker);

	attackerChanges.applyGs(gs);

	for(BattleStackAttacked & stackAttacked : bsa)
		stackAttacked.applyGs(gs);

	attacker->removeBonusesRecursive(Bonus::UntilAttack);
}

void StartAction::applyGs(CGameState *gs)
{
	CStack *st = gs->curB->getStack(ba.stackNumber);

	if(ba.actionType == EActionType::END_TACTIC_PHASE)
	{
		gs->curB->tacticDistance = 0;
		return;
	}

	if(gs->curB->tacticDistance)
	{
		// moves in tactics phase do not affect creature status
		// (tactics stack queue is managed by client)
		return;
	}

	if(ba.actionType != EActionType::HERO_SPELL) //don't check for stack if it's custom action by hero
	{
		assert(st);
	}
	else
	{
		gs->curB->sides[ba.side].usedSpellsHistory.emplace_back(ba.actionSubtype);
	}

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
		break;
	}
}

void BattleSpellCast::applyGs(CGameState * gs) const
{
	assert(gs->curB);

	if(castByHero)
	{
		if(side < 2)
		{
			gs->curB->sides[side].castSpellsCount++;
		}
	}
}

void SetStackEffect::applyGs(CGameState *gs)
{
	applyBattle(gs->curB);
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
	applyBattle(gs->curB);
}

void StacksInjured::applyBattle(IBattleState * battleState)
{
	for(BattleStackAttacked stackAttacked : stacks)
		stackAttacked.applyBattle(battleState);
}

void BattleUnitsChanged::applyGs(CGameState *gs)
{
	applyBattle(gs->curB);
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

void BattleObstaclesChanged::applyGs(CGameState * gs)
{
	if(gs->curB)
		applyBattle(gs->curB);
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
		case BattleChanges::EOperation::ACTIVATE_AND_UPDATE:
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

void CatapultAttack::applyGs(CGameState * gs)
{
	if(gs->curB)
		applyBattle(gs->curB);
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

	if(town->fortLevel() == CGTownInstance::NONE)
		return;

	for(const auto & part : attackedParts)
	{
		auto newWallState = SiegeInfo::applyDamage(battleState->getWallState(part.attackedPart), part.damageDealt);
		battleState->setWallState(part.attackedPart, newWallState);
	}
}

void BattleSetStackProperty::applyGs(CGameState * gs) const
{
	CStack * stack = gs->curB->getStack(stackID);
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
			auto & counter = gs->curB->sides[gs->curB->whatSide(stack->owner)].enchanterCounter;
			if(absolute)
				counter = val;
			else
				counter += val;
			vstd::amax(counter, 0);
			break;
		}
		case UNBIND:
		{
			stack->removeBonusesRecursive(Selector::type()(Bonus::BIND_EFFECT));
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

void PlayerCheated::applyGs(CGameState * gs) const
{
	if(!player.isValidPlayer())
		return;

	gs->getPlayerState(player)->enteredLosingCheatCode = losingCheatCode;
	gs->getPlayerState(player)->enteredWinningCheatCode = winningCheatCode;
}

void YourTurn::applyGs(CGameState * gs) const
{
	gs->currentPlayer = player;

	auto & playerState = gs->players[player];
	playerState.daysWithoutCastle = daysWithoutCastle;
}

Component::Component(const CStackBasicDescriptor & stack)
	: id(EComponentType::CREATURE)
	, subtype(stack.type->idNumber)
	, val(stack.count)
{
}

void EntitiesChanged::applyGs(CGameState * gs)
{
	for(const auto & change : changes)
		gs->updateEntity(change.metatype, change.entityIndex, change.data);
}

const CArtifactInstance * ArtSlotInfo::getArt() const
{
	if(locked)
	{
		logNetwork->warn("ArtifactLocation::getArt: This location is locked!");
		return nullptr;
	}
	return artifact;
}

CArtifactSet * BulkMoveArtifacts::getSrcHolderArtSet()
{
	return boost::apply_visitor(GetBase<CArtifactSet>(), srcArtHolder);
}

CArtifactSet * BulkMoveArtifacts::getDstHolderArtSet()
{
	return boost::apply_visitor(GetBase<CArtifactSet>(), dstArtHolder);
}

VCMI_LIB_NAMESPACE_END

/*
 * NetPacksServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ServerNetPackVisitors.h"

#include "CGameHandler.h"
#include "battles/BattleProcessor.h"
#include "processors/HeroPoolProcessor.h"
#include "processors/PlayerMessageProcessor.h"
#include "processors/TurnOrderProcessor.h"
#include "queries/QueriesProcessor.h"
#include "queries/MapQueries.h"

#include "../lib/CPlayerState.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/battle/IBattleState.h"
#include "../lib/battle/Unit.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/spells/ISpellMechanics.h"

void ApplyGhNetPackVisitor::visitSaveGame(SaveGame & pack)
{
	gh.save(pack.fname);
	logGlobal->info("Game has been saved as %s", pack.fname);
	result = true;
}

void ApplyGhNetPackVisitor::visitGamePause(GamePause & pack)
{
	auto turnQuery = std::make_shared<TimerPauseQuery>(&gh, pack.player);
	turnQuery->queryID = QueryID::CLIENT;
	gh.queries->addQuery(turnQuery);
	result = true;
}

void ApplyGhNetPackVisitor::visitEndTurn(EndTurn & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);
	gh.throwIfPlayerNotActive(connection, &pack);
	result = gh.turnOrder->onPlayerEndsTurn(pack.player);
}

void ApplyGhNetPackVisitor::visitDismissHero(DismissHero & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.hid);
	gh.throwIfPlayerNotActive(connection, &pack);
	result = gh.removeObject(gh.gameInfo().getObj(pack.hid), pack.player);
}

void ApplyGhNetPackVisitor::visitMoveHero(MoveHero & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.hid);
	gh.throwIfPlayerNotActive(connection, &pack);

	for (auto const & dest : pack.path)
	{
		if (!gh.moveHero(pack.hid, dest, EMovementMode::STANDARD, pack.transit, pack.player))
		{
			result = false;
			return;
		}

		// player got some query he has to reply to first for example, from triggered event
		// ignore remaining path (if any), but handle this as success - since at least part of path was legal & was applied
		auto query = gh.queries->topQuery(pack.player);
		if (query && query->blocksPack(&pack))
		{
			result = true;
			return;
		}
	}

	result = true;
}

void ApplyGhNetPackVisitor::visitCastleTeleportHero(CastleTeleportHero & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.hid);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.teleportHero(pack.hid, pack.dest, pack.source, pack.player);
}

void ApplyGhNetPackVisitor::visitArrangeStacks(ArrangeStacks & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.arrangeStacks(pack.id1, pack.id2, pack.what, pack.p1, pack.p2, pack.val, pack.player);
}

void ApplyGhNetPackVisitor::visitBulkMoveArmy(BulkMoveArmy & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.srcArmy);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.bulkMoveArmy(pack.srcArmy, pack.destArmy, pack.srcSlot);
}

void ApplyGhNetPackVisitor::visitBulkSplitStack(BulkSplitStack & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.bulkSplitStack(pack.src, pack.srcOwner, pack.amount);
}

void ApplyGhNetPackVisitor::visitBulkMergeStacks(BulkMergeStacks & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.bulkMergeStacks(pack.src, pack.srcOwner);
}

void ApplyGhNetPackVisitor::visitBulkSplitAndRebalanceStack(BulkSplitAndRebalanceStack & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.bulkSplitAndRebalanceStack(pack.src, pack.srcOwner);
}

void ApplyGhNetPackVisitor::visitDisbandCreature(DisbandCreature & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.id);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.disbandCreature(pack.id, pack.pos);
}

void ApplyGhNetPackVisitor::visitBuildStructure(BuildStructure & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.tid);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.buildStructure(pack.tid, pack.bid);
}

void ApplyGhNetPackVisitor::visitSpellResearch(SpellResearch & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.tid);
	gh.throwIfPlayerNotActive(connection, &pack);
	
	result = gh.spellResearch(pack.tid, pack.spellAtSlot, pack.accepted);
}

void ApplyGhNetPackVisitor::visitVisitTownBuilding(VisitTownBuilding & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.tid);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.visitTownBuilding(pack.tid, pack.bid);
}

void ApplyGhNetPackVisitor::visitRecruitCreatures(RecruitCreatures & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);
	gh.throwIfPlayerNotActive(connection, &pack);
	// ownership checks are inside recruitCreatures
	result = gh.recruitCreatures(pack.tid, pack.dst, pack.crid, pack.amount, pack.level, pack.player);
}

void ApplyGhNetPackVisitor::visitUpgradeCreature(UpgradeCreature & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.id);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.upgradeCreature(pack.id, pack.pos, pack.cid);
}

void ApplyGhNetPackVisitor::visitGarrisonHeroSwap(GarrisonHeroSwap & pack)
{
	const CGTownInstance * town = gh.gameInfo().getTown(pack.tid);
	if(!gh.isPlayerOwns(connection, &pack, pack.tid) && !(town->getGarrisonHero() && gh.isPlayerOwns(connection, &pack, town->getGarrisonHero()->id)))
		gh.throwNotAllowedAction(connection); //neither town nor garrisoned hero (if present) is ours
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.garrisonSwap(pack.tid);
}

void ApplyGhNetPackVisitor::visitExchangeArtifacts(ExchangeArtifacts & pack)
{
	if(gh.gameInfo().getHero(pack.src.artHolder))
		gh.throwIfWrongPlayer(connection, &pack, gh.gameState().getOwner(pack.src.artHolder)); //second hero can be ally
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.moveArtifact(pack.player, pack.src, pack.dst);
}

void ApplyGhNetPackVisitor::visitBulkExchangeArtifacts(BulkExchangeArtifacts & pack)
{
	if(gh.gameState().getMarket(pack.srcHero) == nullptr)
		gh.throwIfWrongOwner(connection, &pack, pack.srcHero);
	if(pack.swap)
		gh.throwIfWrongOwner(connection, &pack, pack.dstHero);

	gh.throwIfPlayerNotActive(connection, &pack);
	result = gh.bulkMoveArtifacts(pack.player, pack.srcHero, pack.dstHero, pack.swap, pack.equipped, pack.backpack);
}

void ApplyGhNetPackVisitor::visitManageBackpackArtifacts(ManageBackpackArtifacts & pack)
{
	gh.throwIfPlayerNotActive(connection, &pack);

	if(gh.gameInfo().getPlayerRelations(pack.player, gh.gameState().getOwner(pack.artHolder)) != PlayerRelations::ENEMIES)
		result = gh.manageBackpackArtifacts(pack.player, pack.artHolder, pack.cmd);
}

void ApplyGhNetPackVisitor::visitManageEquippedArtifacts(ManageEquippedArtifacts & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.artHolder);
	if(pack.saveCostume)
		result = gh.saveArtifactsCostume(pack.player, pack.artHolder, pack.costumeIdx);
	else
		result = gh.switchArtifactsCostume(pack.player, pack.artHolder, pack.costumeIdx);
}

void ApplyGhNetPackVisitor::visitAssembleArtifacts(AssembleArtifacts & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.heroID);
	result = gh.assembleArtifacts(pack.heroID, pack.artifactSlot, pack.assemble, pack.assembleTo);
}

void ApplyGhNetPackVisitor::visitEraseArtifactByClient(EraseArtifactByClient & pack)
{
	gh.throwIfWrongPlayer(connection, &pack, gh.gameState().getOwner(pack.al.artHolder));
	gh.throwIfPlayerNotActive(connection, &pack);
	result = gh.eraseArtifactByClient(pack.al);
}

void ApplyGhNetPackVisitor::visitBuyArtifact(BuyArtifact & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.hid);
	gh.throwIfPlayerNotActive(connection, &pack);
	result = gh.buyArtifact(pack.hid, pack.aid);
}

void ApplyGhNetPackVisitor::visitTradeOnMarketplace(TradeOnMarketplace & pack)
{
	const CGObjectInstance * object = gh.gameInfo().getObj(pack.marketId);
	const CGHeroInstance * hero = gh.gameInfo().getHero(pack.heroId);
	const auto * market = gh.gameState().getMarket(pack.marketId);

	gh.throwIfWrongPlayer(connection, &pack);
	gh.throwIfPlayerNotActive(connection, &pack);

	if(!object)
		gh.throwAndComplain(connection, "Invalid market object");

	if(!market)
		gh.throwAndComplain(connection, "market is not-a-market! :/");

	bool heroCanBeInvalid = false;

	if (pack.mode == EMarketMode::RESOURCE_RESOURCE || pack.mode == EMarketMode::RESOURCE_PLAYER)
	{
		// For resource exchange we must use our own market or visit neutral market
		if (object->getOwner().isValidPlayer())
		{
			gh.throwIfWrongOwner(connection, &pack, pack.marketId);
			heroCanBeInvalid = true;
		}
	}

	if (pack.mode == EMarketMode::CREATURE_UNDEAD)
	{
		// For skeleton transformer, if hero is null then object must be owned
		if (!hero)
		{
			gh.throwIfWrongOwner(connection, &pack, pack.marketId);
			heroCanBeInvalid = true;
		}
	}

	if (!heroCanBeInvalid)
	{
		gh.throwIfWrongOwner(connection, &pack, pack.heroId);

		if (!hero)
			gh.throwAndComplain(connection, "Can not trade - no hero!");

		// TODO: check that object is actually being visited (e.g. Query exists)
		if (!object->visitableAt(hero->visitablePos()))
			gh.throwAndComplain(connection, "Can not trade - object not visited!");

		if (object->getOwner().isValidPlayer() && gh.gameInfo().getPlayerRelations(object->getOwner(), hero->getOwner()) == PlayerRelations::ENEMIES)
			gh.throwAndComplain(connection, "Can not trade - market not owned!");
	}

	result = true;

	switch(pack.mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.tradeResources(market, pack.val[i], pack.player, pack.r1[i].as<GameResID>(), pack.r2[i].as<GameResID>());
		break;
	case EMarketMode::RESOURCE_PLAYER:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.sendResources(pack.val[i], pack.player, pack.r1[i].as<GameResID>(), pack.r2[i].as<PlayerColor>());
		break;
	case EMarketMode::CREATURE_RESOURCE:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.sellCreatures(pack.val[i], market, hero, pack.r1[i].as<SlotID>(), pack.r2[i].as<GameResID>());
		break;
	case EMarketMode::RESOURCE_ARTIFACT:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.buyArtifact(market, hero, pack.r1[i].as<GameResID>(), pack.r2[i].as<ArtifactID>());
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.sellArtifact(market, hero, pack.r1[i].as<ArtifactInstanceID>(), pack.r2[i].as<GameResID>());
		break;
	case EMarketMode::CREATURE_UNDEAD:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.transformInUndead(market, hero, pack.r1[i].as<SlotID>());
		break;
	case EMarketMode::RESOURCE_SKILL:
		for(int i = 0; i < pack.r2.size(); ++i)
			result &= gh.buySecSkill(market, hero, pack.r2[i].as<SecondarySkill>());
		break;
	case EMarketMode::CREATURE_EXP:
	{
		std::vector<SlotID> slotIDs;
		std::vector<ui32> count(pack.val.begin(), pack.val.end());

		for(auto const & slot : pack.r1)
			slotIDs.push_back(slot.as<SlotID>());

		result = gh.sacrificeCreatures(market, hero, slotIDs, count);
		return;
	}
	case EMarketMode::ARTIFACT_EXP:
	{
		std::vector<ArtifactInstanceID> positions;
		for(auto const & artInstId : pack.r1)
			positions.push_back(artInstId.as<ArtifactInstanceID>());

		result = gh.sacrificeArtifact(market, hero, positions);
		return;
	}
	default:
		gh.throwAndComplain(connection, "Unknown exchange pack.mode!");
	}
}

void ApplyGhNetPackVisitor::visitSetFormation(SetFormation & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.hid);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.setFormation(pack.hid, pack.formation);
}

void ApplyGhNetPackVisitor::visitHireHero(HireHero & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.heroPool->hireHero(pack.tid, pack.hid, pack.player, pack.nhid);
}

void ApplyGhNetPackVisitor::visitBuildBoat(BuildBoat & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);
	gh.throwIfPlayerNotActive(connection, &pack);

	if(gh.gameInfo().getPlayerRelations(gh.gameState().getOwner(pack.objid), pack.player) == PlayerRelations::ENEMIES)
		gh.throwAndComplain(connection, "Can't build boat at enemy shipyard");

	result = gh.buildBoat(pack.objid, pack.player);
}

void ApplyGhNetPackVisitor::visitQueryReply(QueryReply & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);

	if(pack.qid == QueryID(-1))
		gh.throwAndComplain(connection, "Cannot answer the query with pack.id -1!");

	result = gh.queryReply(pack.qid, pack.reply, pack.player);
}

void ApplyGhNetPackVisitor::visitSaveLocalState(SaveLocalState & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);
	*gh.gameState().getPlayerState(pack.player)->playerLocalSettings = pack.data;
	result = true;
}

void ApplyGhNetPackVisitor::visitMakeAction(MakeAction & pack)
{
	gh.throwIfWrongPlayer(connection, &pack);
	// allowed even if it is not our turn - will be filtered by battle sides

	result = gh.battles->makePlayerBattleAction(pack.battleID, pack.player, pack.ba);
}

void ApplyGhNetPackVisitor::visitDigWithHero(DigWithHero & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.id);
	gh.throwIfPlayerNotActive(connection, &pack);

	result = gh.dig(gh.gameInfo().getHero(pack.id));
}

void ApplyGhNetPackVisitor::visitCastAdvSpell(CastAdvSpell & pack)
{
	gh.throwIfWrongOwner(connection, &pack, pack.hid);
	gh.throwIfPlayerNotActive(connection, &pack);

	if (!pack.sid.hasValue())
		gh.throwNotAllowedAction(connection);

	const CGHeroInstance * h = gh.gameInfo().getHero(pack.hid);
	if(!h)
		gh.throwNotAllowedAction(connection);

	AdventureSpellCastParameters p;
	p.caster = h;
	p.pos = pack.pos;

	const CSpell * s = pack.sid.toSpell();
	result = s->adventureCast(gh.spellEnv.get(), p);
}

void ApplyGhNetPackVisitor::visitPlayerMessage(PlayerMessage & pack)
{
	if(!pack.player.isSpectator()) // TODO: clearly not a great way to verify permissions
		gh.throwIfWrongPlayer(connection, &pack, pack.player);
	
	gh.playerMessages->playerMessage(pack.player, pack.text, pack.currObj);
	result = true;
}

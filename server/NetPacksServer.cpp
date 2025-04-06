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

#include "../lib/IGameCallback.h"
#include "../lib/CPlayerState.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/CGHeroInstance.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/battle/IBattleState.h"
#include "../lib/battle/BattleAction.h"
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
	gh.throwIfWrongPlayer(&pack);
	gh.throwIfPlayerNotActive(&pack);
	result = gh.turnOrder->onPlayerEndsTurn(pack.player);
}

void ApplyGhNetPackVisitor::visitDismissHero(DismissHero & pack)
{
	gh.throwIfWrongOwner(&pack, pack.hid);
	gh.throwIfPlayerNotActive(&pack);
	result = gh.removeObject(gh.getObj(pack.hid), pack.player);
}

void ApplyGhNetPackVisitor::visitMoveHero(MoveHero & pack)
{
	gh.throwIfWrongOwner(&pack, pack.hid);
	gh.throwIfPlayerNotActive(&pack);

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
	gh.throwIfWrongOwner(&pack, pack.hid);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.teleportHero(pack.hid, pack.dest, pack.source, pack.player);
}

void ApplyGhNetPackVisitor::visitArrangeStacks(ArrangeStacks & pack)
{
	gh.throwIfWrongPlayer(&pack);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.arrangeStacks(pack.id1, pack.id2, pack.what, pack.p1, pack.p2, pack.val, pack.player);
}

void ApplyGhNetPackVisitor::visitBulkMoveArmy(BulkMoveArmy & pack)
{
	gh.throwIfWrongOwner(&pack, pack.srcArmy);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.bulkMoveArmy(pack.srcArmy, pack.destArmy, pack.srcSlot);
}

void ApplyGhNetPackVisitor::visitBulkSplitStack(BulkSplitStack & pack)
{
	gh.throwIfWrongPlayer(&pack);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.bulkSplitStack(pack.src, pack.srcOwner, pack.amount);
}

void ApplyGhNetPackVisitor::visitBulkMergeStacks(BulkMergeStacks & pack)
{
	gh.throwIfWrongPlayer(&pack);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.bulkMergeStacks(pack.src, pack.srcOwner);
}

void ApplyGhNetPackVisitor::visitBulkSmartSplitStack(BulkSmartSplitStack & pack)
{
	gh.throwIfWrongPlayer(&pack);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.bulkSmartSplitStack(pack.src, pack.srcOwner);
}

void ApplyGhNetPackVisitor::visitDisbandCreature(DisbandCreature & pack)
{
	gh.throwIfWrongOwner(&pack, pack.id);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.disbandCreature(pack.id, pack.pos);
}

void ApplyGhNetPackVisitor::visitBuildStructure(BuildStructure & pack)
{
	gh.throwIfWrongOwner(&pack, pack.tid);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.buildStructure(pack.tid, pack.bid);
}

void ApplyGhNetPackVisitor::visitSpellResearch(SpellResearch & pack)
{
	gh.throwIfWrongOwner(&pack, pack.tid);
	gh.throwIfPlayerNotActive(&pack);
	
	result = gh.spellResearch(pack.tid, pack.spellAtSlot, pack.accepted);
}

void ApplyGhNetPackVisitor::visitVisitTownBuilding(VisitTownBuilding & pack)
{
	gh.throwIfWrongOwner(&pack, pack.tid);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.visitTownBuilding(pack.tid, pack.bid);
}

void ApplyGhNetPackVisitor::visitRecruitCreatures(RecruitCreatures & pack)
{
	gh.throwIfWrongPlayer(&pack);
	gh.throwIfPlayerNotActive(&pack);
	// ownership checks are inside recruitCreatures
	result = gh.recruitCreatures(pack.tid, pack.dst, pack.crid, pack.amount, pack.level, pack.player);
}

void ApplyGhNetPackVisitor::visitUpgradeCreature(UpgradeCreature & pack)
{
	gh.throwIfWrongOwner(&pack, pack.id);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.upgradeCreature(pack.id, pack.pos, pack.cid);
}

void ApplyGhNetPackVisitor::visitGarrisonHeroSwap(GarrisonHeroSwap & pack)
{
	const CGTownInstance * town = gh.getTown(pack.tid);
	if(!gh.isPlayerOwns(&pack, pack.tid) && !(town->garrisonHero && gh.isPlayerOwns(&pack, town->garrisonHero->id)))
		gh.throwNotAllowedAction(&pack); //neither town nor garrisoned hero (if present) is ours
	gh.throwIfPlayerNotActive(&pack);

	result = gh.garrisonSwap(pack.tid);
}

void ApplyGhNetPackVisitor::visitExchangeArtifacts(ExchangeArtifacts & pack)
{
	if(gh.getHero(pack.src.artHolder))
		gh.throwIfWrongPlayer(&pack, gh.getOwner(pack.src.artHolder)); //second hero can be ally
	gh.throwIfPlayerNotActive(&pack);

	result = gh.moveArtifact(pack.player, pack.src, pack.dst);
}

void ApplyGhNetPackVisitor::visitBulkExchangeArtifacts(BulkExchangeArtifacts & pack)
{
	if(gh.getMarket(pack.srcHero) == nullptr)
		gh.throwIfWrongOwner(&pack, pack.srcHero);
	if(pack.swap)
		gh.throwIfWrongOwner(&pack, pack.dstHero);

	gh.throwIfPlayerNotActive(&pack);
	result = gh.bulkMoveArtifacts(pack.player, pack.srcHero, pack.dstHero, pack.swap, pack.equipped, pack.backpack);
}

void ApplyGhNetPackVisitor::visitManageBackpackArtifacts(ManageBackpackArtifacts & pack)
{
	gh.throwIfPlayerNotActive(&pack);

	if(gh.getPlayerRelations(pack.player, gh.getOwner(pack.artHolder)) != PlayerRelations::ENEMIES)
		result = gh.manageBackpackArtifacts(pack.player, pack.artHolder, pack.cmd);
}

void ApplyGhNetPackVisitor::visitManageEquippedArtifacts(ManageEquippedArtifacts & pack)
{
	gh.throwIfWrongOwner(&pack, pack.artHolder);
	if(pack.saveCostume)
		result = gh.saveArtifactsCostume(pack.player, pack.artHolder, pack.costumeIdx);
	else
		result = gh.switchArtifactsCostume(pack.player, pack.artHolder, pack.costumeIdx);
}

void ApplyGhNetPackVisitor::visitAssembleArtifacts(AssembleArtifacts & pack)
{
	gh.throwIfWrongOwner(&pack, pack.heroID);
	// gh.throwIfPlayerNotActive(&pack); // Might happen when player captures artifacts in battle?
	result = gh.assembleArtifacts(pack.heroID, pack.artifactSlot, pack.assemble, pack.assembleTo);
}

void ApplyGhNetPackVisitor::visitEraseArtifactByClient(EraseArtifactByClient & pack)
{
	gh.throwIfWrongPlayer(&pack, gh.getOwner(pack.al.artHolder));
	gh.throwIfPlayerNotActive(&pack);
	result = gh.eraseArtifactByClient(pack.al);
}

void ApplyGhNetPackVisitor::visitBuyArtifact(BuyArtifact & pack)
{
	gh.throwIfWrongOwner(&pack, pack.hid);
	gh.throwIfPlayerNotActive(&pack);
	result = gh.buyArtifact(pack.hid, pack.aid);
}

void ApplyGhNetPackVisitor::visitTradeOnMarketplace(TradeOnMarketplace & pack)
{
	const CGObjectInstance * object = gh.getObj(pack.marketId);
	const CGHeroInstance * hero = gh.getHero(pack.heroId);
	const auto * market = gh.getMarket(pack.marketId);

	gh.throwIfWrongPlayer(&pack);
	gh.throwIfPlayerNotActive(&pack);

	if(!object)
		gh.throwAndComplain(&pack, "Invalid market object");

	if(!market)
		gh.throwAndComplain(&pack, "market is not-a-market! :/");

	bool heroCanBeInvalid = false;

	if (pack.mode == EMarketMode::RESOURCE_RESOURCE || pack.mode == EMarketMode::RESOURCE_PLAYER)
	{
		// For resource exchange we must use our own market or visit neutral market
		if (object->getOwner().isValidPlayer())
		{
			gh.throwIfWrongOwner(&pack, pack.marketId);
			heroCanBeInvalid = true;
		}
	}

	if (pack.mode == EMarketMode::CREATURE_UNDEAD)
	{
		// For skeleton transformer, if hero is null then object must be owned
		if (!hero)
		{
			gh.throwIfWrongOwner(&pack, pack.marketId);
			heroCanBeInvalid = true;
		}
	}

	if (!heroCanBeInvalid)
	{
		gh.throwIfWrongOwner(&pack, pack.heroId);

		if (!hero)
			gh.throwAndComplain(&pack, "Can not trade - no hero!");

		// TODO: check that object is actually being visited (e.g. Query exists)
		if (!object->visitableAt(hero->visitablePos()))
			gh.throwAndComplain(&pack, "Can not trade - object not visited!");

		if (object->getOwner().isValidPlayer() && gh.getPlayerRelations(object->getOwner(), hero->getOwner()) == PlayerRelations::ENEMIES)
			gh.throwAndComplain(&pack, "Can not trade - market not owned!");
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
		gh.throwAndComplain(&pack, "Unknown exchange pack.mode!");
	}
}

void ApplyGhNetPackVisitor::visitSetFormation(SetFormation & pack)
{
	gh.throwIfWrongOwner(&pack, pack.hid);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.setFormation(pack.hid, pack.formation);
}

void ApplyGhNetPackVisitor::visitHireHero(HireHero & pack)
{
	gh.throwIfWrongPlayer(&pack);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.heroPool->hireHero(pack.tid, pack.hid, pack.player, pack.nhid);
}

void ApplyGhNetPackVisitor::visitBuildBoat(BuildBoat & pack)
{
	gh.throwIfWrongPlayer(&pack);
	gh.throwIfPlayerNotActive(&pack);

	if(gh.getPlayerRelations(gh.getOwner(pack.objid), pack.player) == PlayerRelations::ENEMIES)
		gh.throwAndComplain(&pack, "Can't build boat at enemy shipyard");

	result = gh.buildBoat(pack.objid, pack.player);
}

void ApplyGhNetPackVisitor::visitQueryReply(QueryReply & pack)
{
	gh.throwIfWrongPlayer(&pack);

	auto playerToConnection = gh.connections.find(pack.player);
	if(playerToConnection == gh.connections.end())
		gh.throwAndComplain(&pack, "No such pack.player!");
	if(!vstd::contains(playerToConnection->second, pack.c))
		gh.throwAndComplain(&pack, "Message came from wrong connection!");
	if(pack.qid == QueryID(-1))
		gh.throwAndComplain(&pack, "Cannot answer the query with pack.id -1!");

	result = gh.queryReply(pack.qid, pack.reply, pack.player);
}

void ApplyGhNetPackVisitor::visitSaveLocalState(SaveLocalState & pack)
{
	gh.throwIfWrongPlayer(&pack);
	*gh.gameState()->getPlayerState(pack.player)->playerLocalSettings = pack.data;
	result = true;
}

void ApplyGhNetPackVisitor::visitMakeAction(MakeAction & pack)
{
	gh.throwIfWrongPlayer(&pack);
	// allowed even if it is not our turn - will be filtered by battle sides

	result = gh.battles->makePlayerBattleAction(pack.battleID, pack.player, pack.ba);
}

void ApplyGhNetPackVisitor::visitDigWithHero(DigWithHero & pack)
{
	gh.throwIfWrongOwner(&pack, pack.id);
	gh.throwIfPlayerNotActive(&pack);

	result = gh.dig(gh.getHero(pack.id));
}

void ApplyGhNetPackVisitor::visitCastAdvSpell(CastAdvSpell & pack)
{
	gh.throwIfWrongOwner(&pack, pack.hid);
	gh.throwIfPlayerNotActive(&pack);

	if (!pack.sid.hasValue())
		gh.throwNotAllowedAction(&pack);

	const CGHeroInstance * h = gh.getHero(pack.hid);
	if(!h)
		gh.throwNotAllowedAction(&pack);

	AdventureSpellCastParameters p;
	p.caster = h;
	p.pos = pack.pos;

	const CSpell * s = pack.sid.toSpell();
	result = s->adventureCast(gh.spellEnv, p);
}

void ApplyGhNetPackVisitor::visitPlayerMessage(PlayerMessage & pack)
{
	if(!pack.player.isSpectator()) // TODO: clearly not a great way to verify permissions
		gh.throwIfWrongPlayer(&pack, pack.player);
	
	gh.playerMessages->playerMessage(pack.player, pack.text, pack.currObj);
	result = true;
}

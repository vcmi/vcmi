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
#include "../lib/IGameCallback.h"
#include "../lib/mapping/CMap.h"
#include "../lib/CGameState.h"
#include "../lib/battle/BattleInfo.h"
#include "../lib/battle/BattleAction.h"
#include "../lib/battle/Unit.h"
#include "../lib/serializer/Connection.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/spells/ISpellMechanics.h"
#include "../lib/serializer/Cast.h"

#include "../lib/filesystem/FileInfo.h"
#include "../lib/VCMIDirs.h"
#include "rusty_bridge/lib.h"

void ApplyGhNetPackVisitor::visitSaveGame(SaveGame & pack)
{
	const std::string stem = FileInfo::GetPathStem(pack.fname).to_string();

	gh.save(pack.fname);
	logGlobal->info("Game has been saved as %s", pack.fname);
	result = true;

	const std::string vcgmPath = VCMIDirs::get().userDataPath().string() + '/' + stem + ".vcgm1";
	const std::string vsgmPath = VCMIDirs::get().userDataPath().string() + '/' + stem + ".vsgm1";
	auto result = save_files_onchain(vcgmPath, vsgmPath);
}

void ApplyGhNetPackVisitor::visitEndTurn(EndTurn & pack)
{
	PlayerColor currentPlayer = gs.currentPlayer;
	if(pack.player != currentPlayer)
	{
		if(gh.getPlayerStatus(pack.player) == EPlayerStatus::INGAME)
			gh.throwAndComplain(&pack, "pack.player attempted to end turn for another pack.player!");

		logGlobal->debug("pack.player attempted to end turn after game over. Ignoring this request.");

		result = true;
		return;
	}

	gh.throwOnWrongPlayer(&pack, pack.player);
	if(gh.queries.topQuery(pack.player))
		gh.throwAndComplain(&pack, "Cannot end turn before resolving queries!");

	gh.states.setFlag(gs.currentPlayer, &PlayerStatus::makingTurn, false);
	result = true;
}

void ApplyGhNetPackVisitor::visitDismissHero(DismissHero & pack)
{
	gh.throwOnWrongOwner(&pack, pack.hid);
	result = gh.removeObject(gh.getObj(pack.hid));
}

void ApplyGhNetPackVisitor::visitMoveHero(MoveHero & pack)
{
	gh.throwOnWrongOwner(&pack, pack.hid);
	result = gh.moveHero(pack.hid, pack.dest, 0, pack.transit, gh.getPlayerAt(pack.c));
}

void ApplyGhNetPackVisitor::visitCastleTeleportHero(CastleTeleportHero & pack)
{
	gh.throwOnWrongOwner(&pack, pack.hid);

	result = gh.teleportHero(pack.hid, pack.dest, pack.source, gh.getPlayerAt(pack.c));
}

void ApplyGhNetPackVisitor::visitArrangeStacks(ArrangeStacks & pack)
{
	//checks for owning in the gh func
	result = gh.arrangeStacks(pack.id1, pack.id2, pack.what, pack.p1, pack.p2, pack.val, gh.getPlayerAt(pack.c));
}

void ApplyGhNetPackVisitor::visitBulkMoveArmy(BulkMoveArmy & pack)
{
	result = gh.bulkMoveArmy(pack.srcArmy, pack.destArmy, pack.srcSlot);
}

void ApplyGhNetPackVisitor::visitBulkSplitStack(BulkSplitStack & pack)
{
	result = gh.bulkSplitStack(pack.src, pack.srcOwner, pack.amount);
}

void ApplyGhNetPackVisitor::visitBulkMergeStacks(BulkMergeStacks & pack)
{
	result = gh.bulkMergeStacks(pack.src, pack.srcOwner);
}

void ApplyGhNetPackVisitor::visitBulkSmartSplitStack(BulkSmartSplitStack & pack)
{
	result = gh.bulkSmartSplitStack(pack.src, pack.srcOwner);
}

void ApplyGhNetPackVisitor::visitDisbandCreature(DisbandCreature & pack)
{
	gh.throwOnWrongOwner(&pack, pack.id);
	result = gh.disbandCreature(pack.id, pack.pos);
}

void ApplyGhNetPackVisitor::visitBuildStructure(BuildStructure & pack)
{
	gh.throwOnWrongOwner(&pack, pack.tid);
	result = gh.buildStructure(pack.tid, pack.bid);
}

void ApplyGhNetPackVisitor::visitRecruitCreatures(RecruitCreatures & pack)
{
	result = gh.recruitCreatures(pack.tid, pack.dst, pack.crid, pack.amount, pack.level);
}

void ApplyGhNetPackVisitor::visitUpgradeCreature(UpgradeCreature & pack)
{
	gh.throwOnWrongOwner(&pack, pack.id);
	result = gh.upgradeCreature(pack.id, pack.pos, pack.cid);
}

void ApplyGhNetPackVisitor::visitGarrisonHeroSwap(GarrisonHeroSwap & pack)
{
	const CGTownInstance * town = gh.getTown(pack.tid);
	if(!gh.isPlayerOwns(&pack, pack.tid) && !(town->garrisonHero && gh.isPlayerOwns(&pack, town->garrisonHero->id)))
		gh.throwNotAllowedAction(&pack); //neither town nor garrisoned hero (if present) is ours
	result = gh.garrisonSwap(pack.tid);
}

void ApplyGhNetPackVisitor::visitExchangeArtifacts(ExchangeArtifacts & pack)
{
	gh.throwOnWrongPlayer(&pack, pack.src.owningPlayer()); //second hero can be ally
	result = gh.moveArtifact(pack.src, pack.dst);
}

void ApplyGhNetPackVisitor::visitBulkExchangeArtifacts(BulkExchangeArtifacts & pack)
{
	const CGHeroInstance * pSrcHero = gh.getHero(pack.srcHero);
	gh.throwOnWrongPlayer(&pack, pSrcHero->getOwner());
	result = gh.bulkMoveArtifacts(pack.srcHero, pack.dstHero, pack.swap);
}

void ApplyGhNetPackVisitor::visitAssembleArtifacts(AssembleArtifacts & pack)
{
	gh.throwOnWrongOwner(&pack, pack.heroID);
	result = gh.assembleArtifacts(pack.heroID, pack.artifactSlot, pack.assemble, pack.assembleTo);
}

void ApplyGhNetPackVisitor::visitBuyArtifact(BuyArtifact & pack)
{
	gh.throwOnWrongOwner(&pack, pack.hid);
	result = gh.buyArtifact(pack.hid, pack.aid);
}

void ApplyGhNetPackVisitor::visitTradeOnMarketplace(TradeOnMarketplace & pack)
{
	const CGObjectInstance * market = gh.getObj(pack.marketId);
	if(!market)
		gh.throwAndComplain(&pack, "Invalid market object");
	const CGHeroInstance * hero = gh.getHero(pack.heroId);

	//market must be owned or visited
	const IMarket * m = IMarket::castFrom(market);

	if(!m)
		gh.throwAndComplain(&pack, "market is not-a-market! :/");

	PlayerColor player = market->tempOwner;

	if(player >= PlayerColor::PLAYER_LIMIT)
		player = gh.getTile(market->visitablePos())->visitableObjects.back()->tempOwner;

	if(player >= PlayerColor::PLAYER_LIMIT)
		gh.throwAndComplain(&pack, "No player can use this market!");

	bool allyTownSkillTrade = (pack.mode == EMarketMode::RESOURCE_SKILL && gh.getPlayerRelations(player, hero->tempOwner) == PlayerRelations::ALLIES);

	if(hero && (!(player == hero->tempOwner || allyTownSkillTrade)
		|| hero->visitablePos() != market->visitablePos()))
		gh.throwAndComplain(&pack, "This hero can't use this marketplace!");

	if(!allyTownSkillTrade)
		gh.throwOnWrongPlayer(&pack, player);

	result = true;

	switch(pack.mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.tradeResources(m, pack.val[i], player, pack.r1[i], pack.r2[i]);
		break;
	case EMarketMode::RESOURCE_PLAYER:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.sendResources(pack.val[i], player, static_cast<Res::ERes>(pack.r1[i]), PlayerColor(pack.r2[i]));
		break;
	case EMarketMode::CREATURE_RESOURCE:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.sellCreatures(pack.val[i], m, hero, SlotID(pack.r1[i]), static_cast<Res::ERes>(pack.r2[i]));
		break;
	case EMarketMode::RESOURCE_ARTIFACT:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.buyArtifact(m, hero, static_cast<Res::ERes>(pack.r1[i]), ArtifactID(pack.r2[i]));
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.sellArtifact(m, hero, ArtifactInstanceID(pack.r1[i]), static_cast<Res::ERes>(pack.r2[i]));
		break;
	case EMarketMode::CREATURE_UNDEAD:
		for(int i = 0; i < pack.r1.size(); ++i)
			result &= gh.transformInUndead(m, hero, SlotID(pack.r1[i]));
		break;
	case EMarketMode::RESOURCE_SKILL:
		for(int i = 0; i < pack.r2.size(); ++i)
			result &= gh.buySecSkill(m, hero, SecondarySkill(pack.r2[i]));
		break;
	case EMarketMode::CREATURE_EXP:
	{
		std::vector<SlotID> slotIDs(pack.r1.begin(), pack.r1.end());
		std::vector<ui32> count(pack.val.begin(), pack.val.end());
		result = gh.sacrificeCreatures(m, hero, slotIDs, count);
		return;
	}
	case EMarketMode::ARTIFACT_EXP:
	{
		std::vector<ArtifactPosition> positions(pack.r1.begin(), pack.r1.end());
		result = gh.sacrificeArtifact(m, hero, positions);
		return;
	}
	default:
		gh.throwAndComplain(&pack, "Unknown exchange pack.mode!");
	}
}

void ApplyGhNetPackVisitor::visitSetFormation(SetFormation & pack)
{
	gh.throwOnWrongOwner(&pack, pack.hid);
	result = gh.setFormation(pack.hid, pack.formation);
}

void ApplyGhNetPackVisitor::visitHireHero(HireHero & pack)
{
	const CGObjectInstance * obj = gh.getObj(pack.tid);
	const CGTownInstance * town = dynamic_ptr_cast<CGTownInstance>(obj);
	if(town && PlayerRelations::ENEMIES == gh.getPlayerRelations(obj->tempOwner, gh.getPlayerAt(pack.c)))
		gh.throwAndComplain(&pack, "Can't buy hero in enemy town!");

	result = gh.hireHero(obj, pack.hid, pack.player);
}

void ApplyGhNetPackVisitor::visitBuildBoat(BuildBoat & pack)
{
	if(gh.getPlayerRelations(gh.getOwner(pack.objid), gh.getPlayerAt(pack.c)) == PlayerRelations::ENEMIES)
		gh.throwAndComplain(&pack, "Can't build boat at enemy shipyard");

	result = gh.buildBoat(pack.objid, gh.getPlayerAt(pack.c));
}

void ApplyGhNetPackVisitor::visitQueryReply(QueryReply & pack)
{
	auto playerToConnection = gh.connections.find(pack.player);
	if(playerToConnection == gh.connections.end())
		gh.throwAndComplain(&pack, "No such pack.player!");
	if(!vstd::contains(playerToConnection->second, pack.c))
		gh.throwAndComplain(&pack, "Message came from wrong connection!");
	if(pack.qid == QueryID(-1))
		gh.throwAndComplain(&pack, "Cannot answer the query with pack.id -1!");

	assert(vstd::contains(gh.states.players, pack.player));

	result = gh.queryReply(pack.qid, pack.reply, pack.player);
}

void ApplyGhNetPackVisitor::visitMakeAction(MakeAction & pack)
{
	const BattleInfo * b = gs.curB;
	if(!b)
		gh.throwNotAllowedAction(&pack);

	if(b->tacticDistance)
	{
		if(pack.ba.actionType != EActionType::WALK && pack.ba.actionType != EActionType::END_TACTIC_PHASE
			&& pack.ba.actionType != EActionType::RETREAT && pack.ba.actionType != EActionType::SURRENDER)
			gh.throwNotAllowedAction(&pack);
		if(!vstd::contains(gh.connections[b->sides[b->tacticsSide].color], pack.c))
			gh.throwNotAllowedAction(&pack);
	}
	else
	{
		auto active = b->battleActiveUnit();
		if(!active)
			gh.throwNotAllowedAction(&pack);
		auto unitOwner = b->battleGetOwner(active);
		if(!vstd::contains(gh.connections[unitOwner], pack.c))
			gh.throwNotAllowedAction(&pack);
	}

	result = gh.makeBattleAction(pack.ba);
}

void ApplyGhNetPackVisitor::visitMakeCustomAction(MakeCustomAction & pack)
{
	const BattleInfo * b = gs.curB;
	if(!b)
		gh.throwNotAllowedAction(&pack);
	if(b->tacticDistance)
		gh.throwNotAllowedAction(&pack);
	auto active = b->battleActiveUnit();
	if(!active)
		gh.throwNotAllowedAction(&pack);
	auto unitOwner = b->battleGetOwner(active);
	if(!vstd::contains(gh.connections[unitOwner], pack.c))
		gh.throwNotAllowedAction(&pack);
	if(pack.ba.actionType != EActionType::HERO_SPELL)
		gh.throwNotAllowedAction(&pack);

	result = gh.makeCustomAction(pack.ba);
}

void ApplyGhNetPackVisitor::visitDigWithHero(DigWithHero & pack)
{
	gh.throwOnWrongOwner(&pack, pack.id);
	result = gh.dig(gh.getHero(pack.id));
}

void ApplyGhNetPackVisitor::visitCastAdvSpell(CastAdvSpell & pack)
{
	gh.throwOnWrongOwner(&pack, pack.hid);

	const CSpell * s = pack.sid.toSpell();
	if(!s)
		gh.throwNotAllowedAction(&pack);
	const CGHeroInstance * h = gh.getHero(pack.hid);
	if(!h)
		gh.throwNotAllowedAction(&pack);

	AdventureSpellCastParameters p;
	p.caster = h;
	p.pos = pack.pos;

	result = s->adventureCast(gh.spellEnv, p);
}

void ApplyGhNetPackVisitor::visitPlayerMessage(PlayerMessage & pack)
{
	if(!pack.player.isSpectator()) // TODO: clearly not a great way to verify permissions
		gh.throwOnWrongPlayer(&pack, pack.player);

	gh.playerMessage(pack.player, pack.text, pack.currObj);
	result = true;
}

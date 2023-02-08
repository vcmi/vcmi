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
#include "../lib/NetPacks.h"

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

bool CPackForServer::isPlayerOwns(CGameHandler * gh, ObjectInstanceID id)
{
	return gh->getPlayerAt(c) == gh->getOwner(id);
}

void CPackForServer::throwNotAllowedAction()
{
	if(c)
	{
		SystemMessage temp_message("You are not allowed to perform this action!");
		c->sendPack(&temp_message);
	}
	logNetwork->error("Player is not allowed to perform this action!");
	throw ExceptionNotAllowedAction();
}

void CPackForServer::wrongPlayerMessage(CGameHandler * gh, PlayerColor expectedplayer)
{
	std::ostringstream oss;
	oss << "You were identified as player " << gh->getPlayerAt(c) << " while expecting " << expectedplayer;
	logNetwork->error(oss.str());
	if(c)
	{
		SystemMessage temp_message(oss.str());
		c->sendPack(&temp_message);
	}
}

void CPackForServer::throwOnWrongOwner(CGameHandler * gh, ObjectInstanceID id)
{
	if(!isPlayerOwns(gh, id))
	{
		wrongPlayerMessage(gh, gh->getOwner(id));
		throwNotAllowedAction();
	}
}

void CPackForServer::throwOnWrongPlayer(CGameHandler * gh, PlayerColor player)
{
	if(!gh->hasPlayerAt(player, c) && player != gh->getPlayerAt(c))
	{
		wrongPlayerMessage(gh, player);
		throwNotAllowedAction();
	}
}

void CPackForServer::throwAndComplain(CGameHandler * gh, std::string txt)
{
	gh->complain(txt);
	throwNotAllowedAction();
}


CGameState * CPackForServer::GS(CGameHandler * gh)
{
	return gh->gs;
}

bool SaveGame::applyGh(CGameHandler * gh)
{
	gh->save(fname);
	logGlobal->info("Game has been saved as %s", fname);
	return true;
}

bool EndTurn::applyGh(CGameHandler * gh)
{
	PlayerColor currentPlayer = GS(gh)->currentPlayer;
	if(player != currentPlayer)
	{
		if(gh->getPlayerStatus(player) == EPlayerStatus::INGAME)
			throwAndComplain(gh, "Player attempted to end turn for another player!");

		logGlobal->debug("Player attempted to end turn after game over. Ignoring this request.");

		return true;
	}

	throwOnWrongPlayer(gh, player);
	if(gh->queries.topQuery(player))
		throwAndComplain(gh, "Cannot end turn before resolving queries!");

	gh->states.setFlag(GS(gh)->currentPlayer, &PlayerStatus::makingTurn, false);
	return true;
}

bool DismissHero::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, hid);
	return gh->removeObject(gh->getObj(hid));
}

bool MoveHero::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, hid);
	return gh->moveHero(hid, dest, 0, transit, gh->getPlayerAt(c));
}

bool CastleTeleportHero::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, hid);

	return gh->teleportHero(hid, dest, source, gh->getPlayerAt(c));
}

bool ArrangeStacks::applyGh(CGameHandler * gh)
{
	//checks for owning in the gh func
	return gh->arrangeStacks(id1, id2, what, p1, p2, val, gh->getPlayerAt(c));
}

bool BulkMoveArmy::applyGh(CGameHandler * gh)
{
	return gh->bulkMoveArmy(srcArmy, destArmy, srcSlot);
}

bool BulkSplitStack::applyGh(CGameHandler * gh)
{
	return gh->bulkSplitStack(src, srcOwner, amount);
}

bool BulkMergeStacks::applyGh(CGameHandler* gh)
{
	return gh->bulkMergeStacks(src, srcOwner);
}

bool BulkSmartSplitStack::applyGh(CGameHandler * gh)
{
	return gh->bulkSmartSplitStack(src, srcOwner);
}

bool DisbandCreature::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, id);
	return gh->disbandCreature(id, pos);
}

bool BuildStructure::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, tid);
	return gh->buildStructure(tid, bid);
}

bool RecruitCreatures::applyGh(CGameHandler * gh)
{
	return gh->recruitCreatures(tid, dst, crid, amount, level);
}

bool UpgradeCreature::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, id);
	return gh->upgradeCreature(id, pos, cid);
}

bool GarrisonHeroSwap::applyGh(CGameHandler * gh)
{
	const CGTownInstance * town = gh->getTown(tid);
	if(!isPlayerOwns(gh, tid) && !(town->garrisonHero && isPlayerOwns(gh, town->garrisonHero->id)))
		throwNotAllowedAction(); //neither town nor garrisoned hero (if present) is ours
	return gh->garrisonSwap(tid);
}

bool ExchangeArtifacts::applyGh(CGameHandler * gh)
{
	throwOnWrongPlayer(gh, src.owningPlayer()); //second hero can be ally
	return gh->moveArtifact(src, dst);
}

bool BulkExchangeArtifacts::applyGh(CGameHandler * gh)
{
	const CGHeroInstance * pSrcHero = gh->getHero(srcHero);
	throwOnWrongPlayer(gh, pSrcHero->getOwner());
	return gh->bulkMoveArtifacts(srcHero, dstHero, swap);
}

bool AssembleArtifacts::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, heroID);
	return gh->assembleArtifacts(heroID, artifactSlot, assemble, assembleTo);
}

bool BuyArtifact::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, hid);
	return gh->buyArtifact(hid, aid);
}

bool TradeOnMarketplace::applyGh(CGameHandler * gh)
{
	const CGObjectInstance * market = gh->getObj(marketId);
	if(!market)
		throwAndComplain(gh, "Invalid market object");
	const CGHeroInstance * hero = gh->getHero(heroId);

	//market must be owned or visited
	const IMarket * m = IMarket::castFrom(market);

	if(!m)
		throwAndComplain(gh, "market is not-a-market! :/");

	PlayerColor player = market->tempOwner;

	if(player >= PlayerColor::PLAYER_LIMIT)
		player = gh->getTile(market->visitablePos())->visitableObjects.back()->tempOwner;

	if(player >= PlayerColor::PLAYER_LIMIT)
		throwAndComplain(gh, "No player can use this market!");

	bool allyTownSkillTrade = (mode == EMarketMode::RESOURCE_SKILL && gh->getPlayerRelations(player, hero->tempOwner) == PlayerRelations::ALLIES);

	if(hero && (!(player == hero->tempOwner || allyTownSkillTrade)
		|| hero->visitablePos() != market->visitablePos()))
		throwAndComplain(gh, "This hero can't use this marketplace!");

	if(!allyTownSkillTrade)
		throwOnWrongPlayer(gh, player);

	bool result = true;

	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		for(int i = 0; i < r1.size(); ++i)
			result &= gh->tradeResources(m, val[i], player, r1[i], r2[i]);
		break;
	case EMarketMode::RESOURCE_PLAYER:
		for(int i = 0; i < r1.size(); ++i)
			result &= gh->sendResources(val[i], player, static_cast<Res::ERes>(r1[i]), PlayerColor(r2[i]));
		break;
	case EMarketMode::CREATURE_RESOURCE:
		for(int i = 0; i < r1.size(); ++i)
			result &= gh->sellCreatures(val[i], m, hero, SlotID(r1[i]), static_cast<Res::ERes>(r2[i]));
		break;
	case EMarketMode::RESOURCE_ARTIFACT:
		for(int i = 0; i < r1.size(); ++i)
			result &= gh->buyArtifact(m, hero, static_cast<Res::ERes>(r1[i]), ArtifactID(r2[i]));
		break;
	case EMarketMode::ARTIFACT_RESOURCE:
		for(int i = 0; i < r1.size(); ++i)
			result &= gh->sellArtifact(m, hero, ArtifactInstanceID(r1[i]), static_cast<Res::ERes>(r2[i]));
		break;
	case EMarketMode::CREATURE_UNDEAD:
		for(unsigned int i : r1)
			result &= gh->transformInUndead(m, hero, SlotID(i));
		break;
	case EMarketMode::RESOURCE_SKILL:
		for(unsigned int i : r2)
			result &= gh->buySecSkill(m, hero, SecondarySkill(i));
		break;
	case EMarketMode::CREATURE_EXP:
	{
		std::vector<SlotID> slotIDs(r1.begin(), r1.end());
		std::vector<ui32> count(val.begin(), val.end());
		return gh->sacrificeCreatures(m, hero, slotIDs, count);
	}
	case EMarketMode::ARTIFACT_EXP:
	{
		std::vector<ArtifactPosition> positions(r1.begin(), r1.end());
		return gh->sacrificeArtifact(m, hero, positions);
	}
	default:
		throwAndComplain(gh, "Unknown exchange mode!");
	}

	return result;
}

bool SetFormation::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, hid);
	return gh->setFormation(hid, formation);
}

bool HireHero::applyGh(CGameHandler * gh)
{
	const CGObjectInstance * obj = gh->getObj(tid);
	const auto * town = dynamic_ptr_cast<CGTownInstance>(obj);
	if(town && PlayerRelations::ENEMIES == gh->getPlayerRelations(obj->tempOwner, gh->getPlayerAt(c)))
		throwAndComplain(gh, "Can't buy hero in enemy town!");

	return gh->hireHero(obj, hid, player);
}

bool BuildBoat::applyGh(CGameHandler * gh)
{
	if(gh->getPlayerRelations(gh->getOwner(objid), gh->getPlayerAt(c)) == PlayerRelations::ENEMIES)
		throwAndComplain(gh, "Can't build boat at enemy shipyard");

	return gh->buildBoat(objid);
}

bool QueryReply::applyGh(CGameHandler * gh)
{
	auto playerToConnection = gh->connections.find(player);
	if(playerToConnection == gh->connections.end())
		throwAndComplain(gh, "No such player!");
	if(!vstd::contains(playerToConnection->second, c))
		throwAndComplain(gh, "Message came from wrong connection!");
	if(qid == QueryID(-1))
		throwAndComplain(gh, "Cannot answer the query with id -1!");

	assert(vstd::contains(gh->states.players, player));
	return gh->queryReply(qid, reply, player);
}

bool MakeAction::applyGh(CGameHandler * gh)
{
	const BattleInfo * b = GS(gh)->curB;
	if(!b)
		throwNotAllowedAction();

	if(b->tacticDistance)
	{
		if(ba.actionType != EActionType::WALK && ba.actionType != EActionType::END_TACTIC_PHASE
			&& ba.actionType != EActionType::RETREAT && ba.actionType != EActionType::SURRENDER)
			throwNotAllowedAction();
		if(!vstd::contains(gh->connections[b->sides[b->tacticsSide].color], c))
			throwNotAllowedAction();
	}
	else
	{
		auto active = b->battleActiveUnit();
		if(!active)
			throwNotAllowedAction();
		auto unitOwner = b->battleGetOwner(active);
		if(!vstd::contains(gh->connections[unitOwner], c))
			throwNotAllowedAction();
	}
	return gh->makeBattleAction(ba);
}

bool MakeCustomAction::applyGh(CGameHandler * gh)
{
	const BattleInfo * b = GS(gh)->curB;
	if(!b)
		throwNotAllowedAction();
	if(b->tacticDistance)
		throwNotAllowedAction();
	auto active = b->battleActiveUnit();
	if(!active)
		throwNotAllowedAction();
	auto unitOwner = b->battleGetOwner(active);
	if(!vstd::contains(gh->connections[unitOwner], c))
		throwNotAllowedAction();
	if(ba.actionType != EActionType::HERO_SPELL)
		throwNotAllowedAction();
	return gh->makeCustomAction(ba);
}

bool DigWithHero::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, id);
	return gh->dig(gh->getHero(id));
}

bool CastAdvSpell::applyGh(CGameHandler * gh)
{
	throwOnWrongOwner(gh, hid);

	const CSpell * s = sid.toSpell();
	if(!s)
		throwNotAllowedAction();
	const CGHeroInstance * h = gh->getHero(hid);
	if(!h)
		throwNotAllowedAction();

	AdventureSpellCastParameters p;
	p.caster = h;
	p.pos = pos;

	return s->adventureCast(gh->spellEnv, p);
}

bool PlayerMessage::applyGh(CGameHandler * gh)
{
	if(!player.isSpectator()) // TODO: clearly not a great way to verify permissions
		throwOnWrongPlayer(gh, player);
	
	gh->playerMessage(player, text, currObj);
	return true;
}

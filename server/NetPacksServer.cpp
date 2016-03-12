#include "StdInc.h"
#include "../lib/NetPacks.h"

#include "CGameHandler.h"
#include "../lib/IGameCallback.h"
#include "../lib/mapping/CMap.h"
#include "../lib/CGameState.h"
#include "../lib/BattleState.h"
#include "../lib/BattleAction.h"


#define PLAYER_OWNS(id) (gh->getPlayerAt(c)==gh->getOwner(id))
#define ERROR_AND_RETURN												\
	do { if(c) {														\
			SystemMessage temp_message("You are not allowed to perform this action!"); \
			boost::unique_lock<boost::mutex> lock(*c->wmx);				\
			*c << &temp_message;										\
		}																\
		logNetwork->errorStream()<<"Player is not allowed to perform this action!";		\
		return false;} while(0)

#define WRONG_PLAYER_MSG(expectedplayer) do {std::ostringstream oss;\
			oss << "You were identified as player " << gh->getPlayerAt(c) << " while expecting " << expectedplayer;\
			logNetwork->errorStream() << oss.str(); \
			if(c) { SystemMessage temp_message(oss.str()); boost::unique_lock<boost::mutex> lock(*c->wmx); *c << &temp_message; } } while(0)

#define ERROR_IF_NOT_OWNS(id)	do{if(!PLAYER_OWNS(id)){WRONG_PLAYER_MSG(gh->getOwner(id)); ERROR_AND_RETURN; }}while(0)
#define ERROR_IF_NOT(player)	do{if(player != gh->getPlayerAt(c)){WRONG_PLAYER_MSG(player); ERROR_AND_RETURN; }}while(0)
#define COMPLAIN_AND_RETURN(txt)	{ gh->complain(txt); ERROR_AND_RETURN; }

/*
 * NetPacksServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


CGameState* CPackForServer::GS(CGameHandler *gh)
{
	return gh->gs;
}

bool SaveGame::applyGh( CGameHandler *gh )
{
	gh->save(fname);
	logGlobal->infoStream() << "Game has been saved as " + fname;
	return true;
}

bool CommitPackage::applyGh( CGameHandler *gh )
{
	gh->sendAndApply(packToCommit);
	return true;
}

bool CloseServer::applyGh( CGameHandler *gh )
{
	gh->close();
	return true;
}

bool EndTurn::applyGh( CGameHandler *gh )
{
	PlayerColor player = GS(gh)->currentPlayer;
	ERROR_IF_NOT(player);
	if(gh->queries.topQuery(player))
		COMPLAIN_AND_RETURN("Cannot end turn before resolving queries!");

	gh->states.setFlag(GS(gh)->currentPlayer,&PlayerStatus::makingTurn,false);
	return true;
}

bool DismissHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	return gh->removeObject(gh->getObj(hid));
}

bool MoveHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	return gh->moveHero(hid,dest,0,transit,gh->getPlayerAt(c));
}

bool CastleTeleportHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);

	return gh->teleportHero(hid,dest,source,gh->getPlayerAt(c));
}

bool ArrangeStacks::applyGh( CGameHandler *gh )
{
	//checks for owning in the gh func
	return gh->arrangeStacks(id1,id2,what,p1,p2,val,gh->getPlayerAt(c));
}

bool DisbandCreature::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(id);
	return gh->disbandCreature(id,pos);
}

bool BuildStructure::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(tid);
	return gh->buildStructure(tid,bid);
}

bool RecruitCreatures::applyGh( CGameHandler *gh )
{
	return gh->recruitCreatures(tid,dst,crid,amount,level);
}

bool UpgradeCreature::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(id);
	return gh->upgradeCreature(id,pos,cid);
}

bool GarrisonHeroSwap::applyGh( CGameHandler *gh )
{
	const CGTownInstance * town = gh->getTown(tid);
	if (!PLAYER_OWNS(tid) && !( town->garrisonHero && PLAYER_OWNS(town->garrisonHero->id) ) )
		ERROR_AND_RETURN;//neither town nor garrisoned hero (if present) is ours
	return gh->garrisonSwap(tid);
}

bool ExchangeArtifacts::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT(src.owningPlayer());//second hero can be ally
	return gh->moveArtifact(src, dst);
}

bool AssembleArtifacts::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(heroID);
	return gh->assembleArtifacts(heroID, artifactSlot, assemble, assembleTo);
}

bool BuyArtifact::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	return gh->buyArtifact(hid,aid);
}

bool TradeOnMarketplace::applyGh( CGameHandler *gh )
{
	//market must be owned or visited
	const IMarket *m = IMarket::castFrom(market);

	if(!m)
		COMPLAIN_AND_RETURN("market is not-a-market! :/");

	PlayerColor player = market->tempOwner;

	if(player >= PlayerColor::PLAYER_LIMIT)
		player = gh->getTile(market->visitablePos())->visitableObjects.back()->tempOwner;

	if(player >= PlayerColor::PLAYER_LIMIT)
		COMPLAIN_AND_RETURN("No player can use this market!");

	if(hero && (player != hero->tempOwner || hero->visitablePos() != market->visitablePos()))
		COMPLAIN_AND_RETURN("This hero can't use this marketplace!");

	ERROR_IF_NOT(player);

	switch(mode)
	{
	case EMarketMode::RESOURCE_RESOURCE:
		return gh->tradeResources(m, val, player, r1, r2);
	case EMarketMode::RESOURCE_PLAYER:
		return gh->sendResources(val, player, static_cast<Res::ERes>(r1), PlayerColor(r2));
	case EMarketMode::CREATURE_RESOURCE:
		if(!hero)
			COMPLAIN_AND_RETURN("Only hero can sell creatures!");
		return gh->sellCreatures(val, m, hero, SlotID(r1), static_cast<Res::ERes>(r2));
	case EMarketMode::RESOURCE_ARTIFACT:
		if(!hero)
			COMPLAIN_AND_RETURN("Only hero can buy artifacts!");
		return gh->buyArtifact(m, hero, static_cast<Res::ERes>(r1), ArtifactID(r2));
	case EMarketMode::ARTIFACT_RESOURCE:
		if(!hero)
			COMPLAIN_AND_RETURN("Only hero can sell artifacts!");
		return gh->sellArtifact(m, hero, ArtifactInstanceID(r1), static_cast<Res::ERes>(r2));
	case EMarketMode::CREATURE_UNDEAD:
		return gh->transformInUndead(m, hero, SlotID(r1));
	case EMarketMode::RESOURCE_SKILL:
		return gh->buySecSkill(m, hero, SecondarySkill(r2));
	case EMarketMode::CREATURE_EXP:
		return gh->sacrificeCreatures(m, hero, SlotID(r1), val);
	case EMarketMode::ARTIFACT_EXP:
		return gh->sacrificeArtifact(m, hero, ArtifactPosition(r1));
	default:
		COMPLAIN_AND_RETURN("Unknown exchange mode!");
	}
}

bool SetFormation::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	return gh->setFormation(hid,formation);
}

bool HireHero::applyGh( CGameHandler *gh )
{
	const CGObjectInstance *obj = gh->getObj(tid);
	const CGTownInstance *town = dynamic_ptr_cast<CGTownInstance>(obj);
	if(town && PlayerRelations::ENEMIES == gh->getPlayerRelations(obj->tempOwner, gh->getPlayerAt(c)))
		COMPLAIN_AND_RETURN("Can't buy hero in enemy town!");

	return gh->hireHero(obj, hid,player);
}

bool BuildBoat::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(objid);
	return gh->buildBoat(objid);
}

bool QueryReply::applyGh( CGameHandler *gh )
{
	auto playerToConnection = gh->connections.find(player);
	if(playerToConnection == gh->connections.end())
		COMPLAIN_AND_RETURN("No such player!");
	if(playerToConnection->second != c)
		COMPLAIN_AND_RETURN("Message came from wrong connection!");
	if(qid == QueryID(-1))
		COMPLAIN_AND_RETURN("Cannot answer the query with id -1!");

	assert(vstd::contains(gh->states.players, player));
	return gh->queryReply(qid, answer, player);
}

bool MakeAction::applyGh( CGameHandler *gh )
{
	const BattleInfo *b = GS(gh)->curB;
	if(!b) ERROR_AND_RETURN;

	if(b->tacticDistance)
	{
		if(ba.actionType != Battle::WALK  &&  ba.actionType != Battle::END_TACTIC_PHASE
			&& ba.actionType != Battle::RETREAT && ba.actionType != Battle::SURRENDER)
			ERROR_AND_RETURN;
		if(gh->connections[b->sides[b->tacticsSide].color] != c)
			ERROR_AND_RETURN;
	}
	else if(gh->connections[b->battleGetStackByID(b->activeStack)->owner] != c)
		ERROR_AND_RETURN;

	return gh->makeBattleAction(ba);
}

bool MakeCustomAction::applyGh( CGameHandler *gh )
{
	const BattleInfo *b = GS(gh)->curB;
	if(!b) ERROR_AND_RETURN;
	if(b->tacticDistance) ERROR_AND_RETURN;
	const CStack *active = GS(gh)->curB->battleGetStackByID(GS(gh)->curB->activeStack);
	if(!active) ERROR_AND_RETURN;
	if(gh->connections[active->owner] != c) ERROR_AND_RETURN;
	if(ba.actionType != Battle::HERO_SPELL) ERROR_AND_RETURN;
	return gh->makeCustomAction(ba);
}

bool DigWithHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(id);
	return gh->dig(gh->getHero(id));
}

bool CastAdvSpell::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	return gh->castSpell(gh->getHero(hid), sid, pos);
}

bool PlayerMessage::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT(player);
	if(gh->getPlayerAt(c) != player) ERROR_AND_RETURN;
	gh->playerMessage(player,text, currObj);
	return true;
}

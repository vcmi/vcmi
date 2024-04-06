/*
 * MapQueries.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MapQueries.h"

#include "QueriesProcessor.h"
#include "../CGameHandler.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/networkPacks/PacksForServer.h"
#include "../../lib/serializer/Cast.h"

TimerPauseQuery::TimerPauseQuery(CGameHandler * owner, PlayerColor player):
	CQuery(owner)
{
	addPlayer(player);
}

bool TimerPauseQuery::blocksPack(const CPack *pack) const
{
	return blockAllButReply(pack);
}

void TimerPauseQuery::onAdding(PlayerColor color)
{
	gh->turnTimerHandler.setTimerEnabled(color, false);
}

void TimerPauseQuery::onRemoval(PlayerColor color)
{
	gh->turnTimerHandler.setTimerEnabled(color, true);
}

bool TimerPauseQuery::endsByPlayerAnswer() const
{
	return true;
}

CObjectVisitQuery::CObjectVisitQuery(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero, int3 Tile):
	CQuery(owner), visitedObject(Obj), visitingHero(Hero), tile(Tile), removeObjectAfterVisit(false)
{
	addPlayer(Hero->tempOwner);
}

bool CObjectVisitQuery::blocksPack(const CPack *pack) const
{
	//During the visit itself ALL actions are blocked.
	//(However, the visit may trigger a query above that'll pass some.)
	return true;
}

void CObjectVisitQuery::onRemoval(PlayerColor color)
{
	gh->objectVisitEnded(*this);

	//TODO or should it be destructor?
	//Can object visit affect 2 players and what would be desired behavior?
	if(removeObjectAfterVisit)
		gh->removeObject(visitedObject, color);
}

void CObjectVisitQuery::onExposure(QueryPtr topQuery)
{
	//Object may have been removed and deleted.
	if(gh->isValidObject(visitedObject))
		topQuery->notifyObjectAboutRemoval(*this);

	owner->popIfTop(*this);
}

void CGarrisonDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	objectVisit.visitedObject->garrisonDialogClosed(objectVisit.visitingHero);
}

CGarrisonDialogQuery::CGarrisonDialogQuery(CGameHandler * owner, const CArmedInstance * up, const CArmedInstance * down):
	CDialogQuery(owner)
{
	exchangingArmies[0] = up;
	exchangingArmies[1] = down;

	addPlayer(up->tempOwner);
	addPlayer(down->tempOwner);
}

bool CGarrisonDialogQuery::blocksPack(const CPack * pack) const
{
	std::set<ObjectInstanceID> ourIds;
	ourIds.insert(this->exchangingArmies[0]->id);
	ourIds.insert(this->exchangingArmies[1]->id);

	if(auto stacks = dynamic_ptr_cast<ArrangeStacks>(pack))
		return !vstd::contains(ourIds, stacks->id1) || !vstd::contains(ourIds, stacks->id2);

	if(auto stacks = dynamic_ptr_cast<BulkSplitStack>(pack))
		return !vstd::contains(ourIds, stacks->srcOwner);

	if(auto stacks = dynamic_ptr_cast<BulkMergeStacks>(pack))
		return !vstd::contains(ourIds, stacks->srcOwner);

	if(auto stacks = dynamic_ptr_cast<BulkSmartSplitStack>(pack))
		return !vstd::contains(ourIds, stacks->srcOwner);

	if(auto stacks = dynamic_ptr_cast<BulkMoveArmy>(pack))
		return !vstd::contains(ourIds, stacks->srcArmy) || !vstd::contains(ourIds, stacks->destArmy);

	if(auto arts = dynamic_ptr_cast<ExchangeArtifacts>(pack))
	{
		if(auto id1 = arts->src.artHolder)
			if(!vstd::contains(ourIds, id1))
				return true;

		if(auto id2 = arts->dst.artHolder)
			if(!vstd::contains(ourIds, id2))
				return true;
		return false;
	}
	if(auto dismiss = dynamic_ptr_cast<DisbandCreature>(pack))
		return !vstd::contains(ourIds, dismiss->id);

	if(auto arts = dynamic_ptr_cast<BulkExchangeArtifacts>(pack))
		return !vstd::contains(ourIds, arts->srcHero) || !vstd::contains(ourIds, arts->dstHero);

	if(auto arts = dynamic_ptr_cast<ManageBackpackArtifacts>(pack))
		return !vstd::contains(ourIds, arts->artHolder);

	if(auto art = dynamic_ptr_cast<EraseArtifactByClient>(pack))
	{
		if(auto id = art->al.artHolder)
			return !vstd::contains(ourIds, id);
	}

	if(auto dismiss = dynamic_ptr_cast<AssembleArtifacts>(pack))
		return !vstd::contains(ourIds, dismiss->heroID);

	if(auto upgrade = dynamic_ptr_cast<UpgradeCreature>(pack))
		return !vstd::contains(ourIds, upgrade->id);

	if(auto formation = dynamic_ptr_cast<SetFormation>(pack))
		return !vstd::contains(ourIds, formation->hid);

	return CDialogQuery::blocksPack(pack);
}

void CBlockingDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	assert(answer);
	objectVisit.visitedObject->blockingDialogAnswered(objectVisit.visitingHero, *answer);
}

CBlockingDialogQuery::CBlockingDialogQuery(CGameHandler * owner, const BlockingDialog & bd):
	CDialogQuery(owner)
{
	this->bd = bd;
	addPlayer(bd.player);
}

OpenWindowQuery::OpenWindowQuery(CGameHandler * owner, const CGHeroInstance *hero, EOpenWindowMode mode):
	CDialogQuery(owner),
	mode(mode)
{
	addPlayer(hero->getOwner());
}

void OpenWindowQuery::onExposure(QueryPtr topQuery)
{
	//do nothing - wait for reply
}

bool OpenWindowQuery::blocksPack(const CPack *pack) const
{
	if (mode == EOpenWindowMode::RECRUITMENT_FIRST || mode == EOpenWindowMode::RECRUITMENT_ALL)
	{
		if(dynamic_ptr_cast<RecruitCreatures>(pack) != nullptr)
			return false;

		// If hero has no free slots, he might get some stacks merged automatically
		if(dynamic_ptr_cast<ArrangeStacks>(pack) != nullptr)
			return false;
	}

	if (mode == EOpenWindowMode::TAVERN_WINDOW)
	{
		if(dynamic_ptr_cast<HireHero>(pack) != nullptr)
			return false;
	}

	if (mode == EOpenWindowMode::UNIVERSITY_WINDOW)
	{
		if(dynamic_ptr_cast<TradeOnMarketplace>(pack) != nullptr)
			return false;
	}

	if (mode == EOpenWindowMode::MARKET_WINDOW)
	{
		if(dynamic_ptr_cast<ExchangeArtifacts>(pack) != nullptr)
			return false;

		if(dynamic_ptr_cast<BulkExchangeArtifacts>(pack) != nullptr)
			return false;

		if(dynamic_ptr_cast<ManageBackpackArtifacts>(pack) != nullptr)
			return false;

		if(dynamic_ptr_cast<AssembleArtifacts>(pack))
			return false;

		if(dynamic_ptr_cast<EraseArtifactByClient>(pack))
			return false;

		if(dynamic_ptr_cast<TradeOnMarketplace>(pack) != nullptr)
			return false;
	}

	return CDialogQuery::blocksPack(pack);
}

void CTeleportDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	// do not change to dynamic_ptr_cast - SIGSEGV!
	auto obj = dynamic_cast<const CGTeleport*>(objectVisit.visitedObject);
	if(obj)
		obj->teleportDialogAnswered(objectVisit.visitingHero, *answer, td.exits);
	else
		logGlobal->error("Invalid instance in teleport query");
}

CTeleportDialogQuery::CTeleportDialogQuery(CGameHandler * owner, const TeleportDialog & td):
	CDialogQuery(owner)
{
	this->td = td;
	addPlayer(gh->getHero(td.hero)->getOwner());
}

CHeroLevelUpDialogQuery::CHeroLevelUpDialogQuery(CGameHandler * owner, const HeroLevelUp & Hlu, const CGHeroInstance * Hero):
	CDialogQuery(owner), hero(Hero)
{
	hlu = Hlu;
	addPlayer(hero->tempOwner);
}

void CHeroLevelUpDialogQuery::onRemoval(PlayerColor color)
{
	assert(answer);
	logGlobal->trace("Completing hero level-up query. %s gains skill %d", hero->getObjectName(), answer.value());
	gh->levelUpHero(hero, hlu.skills[*answer]);
}

void CHeroLevelUpDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	objectVisit.visitedObject->heroLevelUpDone(objectVisit.visitingHero);
}

CCommanderLevelUpDialogQuery::CCommanderLevelUpDialogQuery(CGameHandler * owner, const CommanderLevelUp & Clu, const CGHeroInstance * Hero):
	CDialogQuery(owner), hero(Hero)
{
	clu = Clu;
	addPlayer(hero->tempOwner);
}

void CCommanderLevelUpDialogQuery::onRemoval(PlayerColor color)
{
	assert(answer);
	logGlobal->trace("Completing commander level-up query. Commander of hero %s gains skill %s", hero->getObjectName(), answer.value());
	gh->levelUpCommander(hero->commander, clu.skills[*answer]);
}

void CCommanderLevelUpDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	objectVisit.visitedObject->heroLevelUpDone(objectVisit.visitingHero);
}

CHeroMovementQuery::CHeroMovementQuery(CGameHandler * owner, const TryMoveHero & Tmh, const CGHeroInstance * Hero, bool VisitDestAfterVictory):
	CQuery(owner), tmh(Tmh), visitDestAfterVictory(VisitDestAfterVictory), hero(Hero)
{
	players.push_back(hero->tempOwner);
}

void CHeroMovementQuery::onExposure(QueryPtr topQuery)
{
	assert(players.size() == 1);

	if(visitDestAfterVictory && hero->tempOwner == players[0]) //hero still alive, so he won with the guard
		//TODO what if there were H4-like escape? we should also check pos
	{
		logGlobal->trace("Hero %s after victory over guard finishes visit to %s", hero->getNameTranslated(), tmh.end.toString());
		//finish movement
		visitDestAfterVictory = false;
		gh->visitObjectOnTile(*gh->getTile(hero->convertToVisitablePos(tmh.end)), hero);
	}

	owner->popIfTop(*this);
}

void CHeroMovementQuery::onRemoval(PlayerColor color)
{
	PlayerBlocked pb;
	pb.player = color;
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_ENDED;
	gh->sendAndApply(&pb);
}

void CHeroMovementQuery::onAdding(PlayerColor color)
{
	PlayerBlocked pb;
	pb.player = color;
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_STARTED;
	gh->sendAndApply(&pb);
}

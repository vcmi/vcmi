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
#include "../TurnTimerHandler.h"
#include "../../lib/callback/IGameInfoCallback.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/networkPacks/PacksForServer.h"

TimerPauseQuery::TimerPauseQuery(CGameHandler * owner, PlayerColor player):
	CQuery(owner)
{
	addPlayer(player);
}

bool TimerPauseQuery::blocksPack(const CPackForServer *pack) const
{
	if(dynamic_cast<const SaveGame*>(pack) != nullptr)
		return false;

	return blockAllButReply(pack);
}

void TimerPauseQuery::onAdding(PlayerColor color)
{
	gh->turnTimerHandler->setTimerEnabled(color, false);
}

void TimerPauseQuery::onRemoval(PlayerColor color)
{
	gh->turnTimerHandler->setTimerEnabled(color, true);
}

bool TimerPauseQuery::endsByPlayerAnswer() const
{
	return true;
}

void CGarrisonDialogQuery::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const
{
	visitedObject->garrisonDialogClosed(*gh, visitingHero);
}

CGarrisonDialogQuery::CGarrisonDialogQuery(CGameHandler * owner, const CArmedInstance * up, const CArmedInstance * down):
	CDialogQuery(owner)
{
	exchangingArmies[0] = up;
	exchangingArmies[1] = down;

	addPlayer(up->tempOwner);
	addPlayer(down->tempOwner);
}

bool CGarrisonDialogQuery::blocksPack(const CPackForServer * pack) const
{
	std::set<ObjectInstanceID> ourIds;
	ourIds.insert(this->exchangingArmies[0]->id);
	ourIds.insert(this->exchangingArmies[1]->id);

	if(auto stacks = dynamic_cast<const ArrangeStacks*>(pack))
		return !vstd::contains(ourIds, stacks->id1) || !vstd::contains(ourIds, stacks->id2);

	if(auto stacks = dynamic_cast<const BulkSplitStack*>(pack))
		return !vstd::contains(ourIds, stacks->srcOwner);

	if(auto stacks = dynamic_cast<const BulkMergeStacks*>(pack))
		return !vstd::contains(ourIds, stacks->srcOwner);

	if(auto stacks = dynamic_cast<const BulkSplitAndRebalanceStack*>(pack))
		return !vstd::contains(ourIds, stacks->srcOwner);

	if(auto stacks = dynamic_cast<const BulkMoveArmy*>(pack))
		return !vstd::contains(ourIds, stacks->srcArmy) || !vstd::contains(ourIds, stacks->destArmy);

	if(auto arts = dynamic_cast<const ExchangeArtifacts*>(pack))
	{
		auto id1 = arts->src.artHolder;
		if(id1.hasValue() && !vstd::contains(ourIds, id1))
			return true;

		auto id2 = arts->dst.artHolder;
		if(id2.hasValue() && !vstd::contains(ourIds, id2))
			return true;

		return false;
	}
	if(auto dismiss = dynamic_cast<const DisbandCreature*>(pack))
		return !vstd::contains(ourIds, dismiss->id);

	if(auto arts = dynamic_cast<const BulkExchangeArtifacts*>(pack))
		return !vstd::contains(ourIds, arts->srcHero) || !vstd::contains(ourIds, arts->dstHero);

	if(auto arts = dynamic_cast<const ManageBackpackArtifacts*>(pack))
		return !vstd::contains(ourIds, arts->artHolder);

	if(auto art = dynamic_cast<const EraseArtifactByClient*>(pack))
	{
		auto id = art->al.artHolder;
		if(id.hasValue())
			return !vstd::contains(ourIds, id);
	}

	if(auto dismiss = dynamic_cast<const AssembleArtifacts*>(pack))
		return !vstd::contains(ourIds, dismiss->heroID);

	if(auto upgrade = dynamic_cast<const UpgradeCreature*>(pack))
		return !vstd::contains(ourIds, upgrade->id);

	if(auto formation = dynamic_cast<const SetFormation*>(pack))
		return !vstd::contains(ourIds, formation->hid);

	return CDialogQuery::blocksPack(pack);
}

void CBlockingDialogQuery::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const
{
	assert(answer);
	caller->blockingDialogAnswered(*gh, visitingHero, *answer);
}

CBlockingDialogQuery::CBlockingDialogQuery(CGameHandler * owner, const IObjectInterface * caller, const BlockingDialog & bd):
	CDialogQuery(owner),
	caller(caller)
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

bool OpenWindowQuery::blocksPack(const CPackForServer *pack) const
{
	if (mode == EOpenWindowMode::RECRUITMENT_FIRST || mode == EOpenWindowMode::RECRUITMENT_ALL)
	{
		if(dynamic_cast<const RecruitCreatures*>(pack) != nullptr)
			return false;

		// If hero has no free slots, he might get some stacks merged automatically
		if(dynamic_cast<const ArrangeStacks*>(pack) != nullptr)
			return false;
	}

	if (mode == EOpenWindowMode::TAVERN_WINDOW)
	{
		if(dynamic_cast<const HireHero*>(pack) != nullptr)
			return false;
	}

	if (mode == EOpenWindowMode::UNIVERSITY_WINDOW)
	{
		if(dynamic_cast<const TradeOnMarketplace*>(pack) != nullptr)
			return false;
	}

	if (mode == EOpenWindowMode::MARKET_WINDOW)
	{
		if(dynamic_cast<const ExchangeArtifacts*>(pack) != nullptr)
			return false;

		if(dynamic_cast<const BulkExchangeArtifacts*>(pack) != nullptr)
			return false;

		if(dynamic_cast<const ManageBackpackArtifacts*>(pack) != nullptr)
			return false;

		if(dynamic_cast<const AssembleArtifacts*>(pack))
			return false;

		if(dynamic_cast<const EraseArtifactByClient*>(pack))
			return false;

		if(dynamic_cast<const TradeOnMarketplace*>(pack) != nullptr)
			return false;
	}

	return CDialogQuery::blocksPack(pack);
}

void CTeleportDialogQuery::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const
{
	auto obj = dynamic_cast<const CGTeleport*>(visitedObject);
	if(obj)
		obj->teleportDialogAnswered(*gh, visitingHero, *answer, td.exits);
	else
		logGlobal->error("Invalid instance in teleport query");
}

CTeleportDialogQuery::CTeleportDialogQuery(CGameHandler * owner, const TeleportDialog & td):
	CDialogQuery(owner)
{
	this->td = td;
	addPlayer(gh->gameInfo().getHero(td.hero)->getOwner());
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

void CHeroLevelUpDialogQuery::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const
{
	visitedObject->heroLevelUpDone(*gh, visitingHero);
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
	gh->levelUpCommander(hero->getCommander(), clu.skills[*answer]);
}

void CCommanderLevelUpDialogQuery::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const
{
	visitedObject->heroLevelUpDone(*gh, visitingHero);
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
	{
		logGlobal->trace("Hero %s after victory over guard finishes visit to %s", hero->getNameTranslated(), tmh.end.toString());
		//finish movement
		visitDestAfterVictory = false;
		gh->visitObjectOnTile(*gh->gameInfo().getTile(hero->convertToVisitablePos(tmh.end)), hero);
	}

	owner->popIfTop(*this);
}

void CHeroMovementQuery::onRemoval(PlayerColor color)
{
	PlayerBlocked pb;
	pb.player = color;
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_ENDED;
	gh->sendAndApply(pb);
}

void CHeroMovementQuery::onAdding(PlayerColor color)
{
	PlayerBlocked pb;
	pb.player = color;
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_STARTED;
	gh->sendAndApply(pb);
}

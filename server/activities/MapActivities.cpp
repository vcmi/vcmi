/*
 * MapActivities.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "MapActivities.h"
#include "../../lib/networkPacks/PacksForClient.h"

#include "ActivityProcessor.h"
#include "../CGameHandler.h"
#include "../TurnTimerHandler.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/callback/IGameInfoCallback.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/MiscObjects.h"
#include "../../lib/networkPacks/PacksForServer.h"

TimerPauseActivity::TimerPauseActivity(CGameHandler * owner, PlayerColor player):
	Activity(owner, TYPE)
{
	addPlayer(player);
}

bool TimerPauseActivity::blocksPack(const CPackForServer * pack) const
{
	if(dynamic_cast<const SaveGame *>(pack) != nullptr)
		return false;

	return blockAllButReply(pack);
}

void TimerPauseActivity::onExposure(ActivityPtr topActivity)
{
	// do not self-pop: this activity ends on player reply, which ActivityProcessor resolves
	// once the activity is exposed, or on explicit removal by the timer handler
}

void TimerPauseActivity::onAdding(PlayerColor color)
{
	gh->turnTimerHandler->setTimerEnabled(color, false);
}

void TimerPauseActivity::onRemoval(PlayerColor color)
{
	gh->turnTimerHandler->setTimerEnabled(color, true);
}

bool TimerPauseActivity::endsByPlayerAnswer() const
{
	return true;
}

void GarrisonDialogActivity::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const
{
	visitedObject->garrisonDialogClosed(*gh, visitingHero);
}

GarrisonDialogActivity::GarrisonDialogActivity(CGameHandler * owner, const CArmedInstance * up, const CArmedInstance * down):
	DialogActivity(owner, TYPE)
{
	exchangingArmies[0] = up;
	exchangingArmies[1] = down;

	if(up->tempOwner.isValidPlayer())
		addPlayer(up->tempOwner);
	if(down->tempOwner.isValidPlayer())
		addPlayer(down->tempOwner);
}

bool GarrisonDialogActivity::blocksPack(const CPackForServer * pack) const
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

	if(auto tactics = dynamic_cast<const SetTactics*>(pack))
		return !vstd::contains(ourIds, tactics->hid);

	return DialogActivity::blocksPack(pack);
}

void BlockingDialogActivity::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const
{
	assert(answer);

	visitedObject->blockingDialogAnswered(*gh, visitingHero, *answer);
}

BlockingDialogActivity::BlockingDialogActivity(CGameHandler * owner, const BlockingDialog & bd):
	DialogActivity(owner, TYPE)
{
	this->bd = bd;
	addPlayer(bd.player);
}

OpenWindowActivity::OpenWindowActivity(CGameHandler * owner, const CGHeroInstance * hero, EOpenWindowMode mode)
	: DialogActivity(owner, TYPE), mode(mode)
{
	addPlayer(hero->getOwner());
}

void OpenWindowActivity::onExposure(ActivityPtr topActivity)
{
	//do nothing - wait for reply
}

bool OpenWindowActivity::blocksPack(const CPackForServer * pack) const
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

	return DialogActivity::blocksPack(pack);
}

void TeleportDialogActivity::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const
{
	auto obj = dynamic_cast<const CGTeleport*>(visitedObject);
	if(obj)
		obj->teleportDialogAnswered(*gh, visitingHero, *answer, td.exits);
	else
		logGlobal->error("Invalid instance in teleport activity");
}

TeleportDialogActivity::TeleportDialogActivity(CGameHandler * owner, const TeleportDialog & dialog) :
	DialogActivity(owner, TYPE)
{
	td = dialog;
	addPlayer(gh->gameInfo().getHero(dialog.hero)->getOwner());
}

LevelUpRoutine::LevelUpRoutine(CGameHandler * owner, const CGHeroInstance * hero)
	: Activity(owner, TYPE), hero(hero->id)
{
	addPlayer(hero->tempOwner);
}

bool LevelUpRoutine::blocksPack(const CPackForServer * pack) const
{
	return blockAllButReply(pack);
}

StepResult LevelUpRoutine::advance()
{
	const auto * levellingHero = gh->gameInfo().getHero(hero);
	if(!levellingHero)
		return StepResult::Done;

	const auto * commander = levellingHero->getCommander();
	const bool heroLevels = levellingHero->gainsLevel();
	const bool commanderLevels = commander && commander->gainsLevel();

	if(!heroLevels && !commanderLevels)
		return StepResult::Done;

	// Hero levels first, in the order in which the game applies them
	if(heroLevels)
	{
		owner->addActivity(std::make_shared<HeroLevelUpPrompt>(gh, levellingHero, gh->rollHeroLevelUp(levellingHero)));
		return StepResult::Continue;
	}

	owner->addActivity(std::make_shared<CommanderLevelUpPrompt>(gh, levellingHero, gh->rollCommanderLevelUp(commander)));
	return StepResult::Continue;
}

void LevelUpRoutine::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const
{
	visitedObject->heroLevelUpDone(*gh, visitingHero, continuationTag);
}

HeroLevelUpPrompt::HeroLevelUpPrompt(CGameHandler * owner, const CGHeroInstance * hero, const HeroLevelUp & rolled)
	: DialogActivity(owner, TYPE), hero(hero->id), levelUp(rolled)
{
	addPlayer(hero->tempOwner);
}

void HeroLevelUpPrompt::onAdded(PlayerColor color)
{
	levelUp.questionID = askQuestion();
	gh->sendAndApply(levelUp);
}

void HeroLevelUpPrompt::onRemoval(PlayerColor color)
{
	// The client keeps its window open until the question it was given is reported resolved,
	// and expects that before the next one of the chain arrives.
	gh->sendQuestionResolved(getActiveQuestionID());

	const auto * levellingHero = gh->gameInfo().getHero(hero);
	if(!levellingHero || levelUp.skills.empty())
		return;

	if(answer && *answer < levelUp.skills.size())
	{
		logGlobal->trace("%s gains skill %d", levellingHero->getNameTextID(), *answer);
		gh->applyHeroLevelUp(levellingHero, levelUp.skills.at(*answer));
	}
	else
	{
		logGlobal->warn("Invalid secondary skill %d chosen for %s - granting none",
			answer ? static_cast<int>(*answer) : -1, levellingHero->getNameTextID());
	}
}

CommanderLevelUpPrompt::CommanderLevelUpPrompt(CGameHandler * owner, const CGHeroInstance * hero, const CommanderLevelUp & rolled)
	: DialogActivity(owner, TYPE), hero(hero->id), levelUp(rolled)
{
	addPlayer(hero->tempOwner);
}

void CommanderLevelUpPrompt::onAdded(PlayerColor color)
{
	levelUp.questionID = askQuestion();
	gh->sendAndApply(levelUp);
}

void CommanderLevelUpPrompt::onRemoval(PlayerColor color)
{
	gh->sendQuestionResolved(getActiveQuestionID());

	const auto * levellingHero = gh->gameInfo().getHero(hero);
	if(!levellingHero || !levellingHero->getCommander() || levelUp.skills.empty())
		return;

	if(answer && *answer < levelUp.skills.size())
	{
		logGlobal->trace("Commander of %s gains skill %d", levellingHero->getNameTextID(), *answer);
		gh->applyCommanderLevelUp(levellingHero->getCommander(), levelUp.skills.at(*answer));
	}
	else
	{
		logGlobal->warn("Invalid commander skill %d chosen for %s - granting none",
			answer ? static_cast<int>(*answer) : -1, levellingHero->getNameTextID());
	}
}

HeroMovementActivity::HeroMovementActivity(CGameHandler * owner, const TryMoveHero & Tmh, const CGHeroInstance * Hero, bool VisitDestAfterVictory):
	Activity(owner, TYPE), tmh(Tmh), visitDestAfterVictory(VisitDestAfterVictory), hero(Hero->id)
{
	players.push_back(Hero->tempOwner);
}

void HeroMovementActivity::onExposure(ActivityPtr topActivity)
{
	assert(players.size() == 1);

	const auto * movingHero = gh->gameInfo().getHero(hero);

	// A hero that lost the guard battle is no longer on the map, and one that changed
	// owner is no longer ours, so there is no visit to finish.
	if(visitDestAfterVictory && movingHero && movingHero->tempOwner == players[0])
	{
		logGlobal->trace("Hero %s after victory over guard finishes visit to %s", movingHero->getNameTextID(), tmh.end.toString());
		//finish movement
		visitDestAfterVictory = false;
		gh->visitObjectOnTile(*gh->gameInfo().getTile(movingHero->convertToVisitablePos(tmh.end)), movingHero);
	}

	owner->popIfTop(*this);
}

void HeroMovementActivity::onRemoval(PlayerColor color)
{
	PlayerBlocked pb;
	pb.player = color;
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_ENDED;
	gh->sendAndApply(pb);
}

void HeroMovementActivity::onAdding(PlayerColor color)
{
	PlayerBlocked pb;
	pb.player = color;
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_STARTED;
	gh->sendAndApply(pb);
}

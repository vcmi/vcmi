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

void TimerPauseActivity::onAdded()
{
	gh->turnTimerHandler->setTimerEnabled(getPlayers().front(), false);
}

void TimerPauseActivity::onRemoval()
{
	gh->turnTimerHandler->setTimerEnabled(getPlayers().front(), true);
}

bool TimerPauseActivity::endsByPlayerAnswer() const
{
	return true;
}

void GarrisonDialogActivity::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const
{
	visitedObject->garrisonDialogClosed(*gh, visitingHero, visitState);
}

GarrisonDialogActivity::GarrisonDialogActivity(CGameHandler * owner, PlayerColor player, const CArmedInstance * up, const CArmedInstance * down):
	DialogActivity(owner, TYPE)
{
	exchangingArmies[0] = up;
	exchangingArmies[1] = down;
	addPlayer(player);
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

void BlockingDialogActivity::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const
{
	assert(answer);

	visitedObject->blockingDialogAnswered(*gh, visitingHero, *answer, visitState);
}

BlockingDialogActivity::BlockingDialogActivity(CGameHandler * owner, const BlockingDialog & bd):
	DialogActivity(owner, TYPE)
{
	this->bd = bd;
	addPlayer(bd.player);
}

bool BlockingDialogActivity::acceptsAnswer(int32_t answer) const
{
	// A selection answers with the 1-based index of a component, anything else with yes or no
	const int32_t maxAnswer = bd.selection() ? static_cast<int32_t>(bd.components.size()) : 1;
	return answer >= 0 && answer <= maxAnswer;
}

OpenWindowActivity::OpenWindowActivity(CGameHandler * owner, const CGHeroInstance * hero, EOpenWindowMode mode)
	: DialogActivity(owner, TYPE), mode(mode)
{
	addPlayer(hero->getOwner());
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

void TeleportDialogActivity::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const
{
	auto obj = dynamic_cast<const CGTeleport*>(visitedObject);
	if(!obj)
		throw std::runtime_error("Teleport dialog answered to an object that is not a teleport");

	obj->teleportDialogAnswered(*gh, visitingHero, *answer, td.exits);
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
		throw std::runtime_error("Hero disappeared during level-up");

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

void LevelUpRoutine::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const
{
	visitedObject->experienceApplied(*gh, visitingHero, visitState);
}

HeroLevelUpPrompt::HeroLevelUpPrompt(CGameHandler * owner, const CGHeroInstance * hero, const HeroLevelUp & rolled)
	: DialogActivity(owner, TYPE), hero(hero->id), levelUp(rolled)
{
	addPlayer(hero->tempOwner);
}

bool HeroLevelUpPrompt::acceptsAnswer(int32_t answer) const
{
	// With no skill to learn the window is closed with 0
	return answer >= 0 && (levelUp.skills.empty() ? answer == 0 : answer < static_cast<int32_t>(levelUp.skills.size()));
}

void HeroLevelUpPrompt::onAdded()
{
	levelUp.questionID = askQuestion();
	gh->sendAndApply(levelUp);
}

void HeroLevelUpPrompt::onRemoval()
{
	// The client keeps its window open until the question it was given is reported resolved,
	// and expects that before the next one of the chain arrives.
	gh->sendQuestionResolved(getActiveQuestionID());

	const auto * levellingHero = gh->gameInfo().getHero(hero);
	if(!levellingHero)
		throw std::runtime_error("Hero disappeared during level-up");

	// A hero who knows every skill he can learn is offered none
	if(levelUp.skills.empty())
		return;

	logGlobal->trace("%s gains skill %d", levellingHero->getNameTextID(), *answer);
	gh->applyHeroLevelUp(levellingHero, levelUp.skills.at(*answer));
}

CommanderLevelUpPrompt::CommanderLevelUpPrompt(CGameHandler * owner, const CGHeroInstance * hero, const CommanderLevelUp & rolled)
	: DialogActivity(owner, TYPE), hero(hero->id), levelUp(rolled)
{
	addPlayer(hero->tempOwner);
}

bool CommanderLevelUpPrompt::acceptsAnswer(int32_t answer) const
{
	// With no skill to learn the window is closed with 0
	return answer >= 0 && (levelUp.skills.empty() ? answer == 0 : answer < static_cast<int32_t>(levelUp.skills.size()));
}

void CommanderLevelUpPrompt::onAdded()
{
	levelUp.questionID = askQuestion();
	gh->sendAndApply(levelUp);
}

void CommanderLevelUpPrompt::onRemoval()
{
	gh->sendQuestionResolved(getActiveQuestionID());

	const auto * levellingHero = gh->gameInfo().getHero(hero);
	if(!levellingHero || !levellingHero->getCommander())
		throw std::runtime_error("Hero or commander disappeared during level-up");

	// A commander with every skill maxed out is offered none
	if(levelUp.skills.empty())
		return;

	logGlobal->trace("Commander of %s gains skill %d", levellingHero->getNameTextID(), *answer);
	gh->applyCommanderLevelUp(levellingHero->getCommander(), levelUp.skills.at(*answer));
}

HeroMovementActivity::HeroMovementActivity(CGameHandler * owner, const TryMoveHero & Tmh, const CGHeroInstance * Hero, bool VisitDestAfterVictory):
	Activity(owner, TYPE), tmh(Tmh), visitDestAfterVictory(VisitDestAfterVictory), hero(Hero->id)
{
	addPlayer(Hero->tempOwner);
}

void HeroMovementActivity::onChildCompleted(const ActivityPtr & child)
{
	const auto * movingHero = gh->gameInfo().getHero(hero);

	// A hero that lost the guard battle is no longer on the map, and one that changed
	// owner is no longer ours, so there is no visit to finish.
	if(visitDestAfterVictory && movingHero && movingHero->tempOwner == getPlayers().front())
	{
		logGlobal->trace("Hero %s after victory over guard finishes visit to %s", movingHero->getNameTextID(), tmh.end.toString());
		//finish movement
		visitDestAfterVictory = false;
		gh->visitObjectOnTile(*gh->gameInfo().getTile(movingHero->convertToVisitablePos(tmh.end)), movingHero);
	}

	finish();
}

void HeroMovementActivity::onRemoval()
{
	PlayerBlocked pb;
	pb.player = getPlayers().front();
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_ENDED;
	gh->sendAndApply(pb);
}

void HeroMovementActivity::onAdded()
{
	PlayerBlocked pb;
	pb.player = getPlayers().front();
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_STARTED;
	gh->sendAndApply(pb);
}

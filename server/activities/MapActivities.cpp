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

	if(dynamic_cast<const AdvInterfaceReady *>(pack) != nullptr)
		return false;

	return blockAllButReply(pack);
}

void TimerPauseActivity::onExposure(ActivityPtr topActivity)
{
	// do nothing - don't self-pop. This activity ends either when the player replies
	// (ActivityProcessor pops answered activities once they are exposed) or when the
	// timer/handler removes it explicitly.
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

void GarrisonDialogActivity::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const
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

void BlockingDialogActivity::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const
{
	assert(answer);
	caller->blockingDialogAnswered(*gh, visitingHero, continuationTag, *answer);
}

BlockingDialogActivity::BlockingDialogActivity(CGameHandler * owner, const IObjectInterface * caller, const BlockingDialog & bd):
	DialogActivity(owner, TYPE),
	caller(caller)
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

void TeleportDialogActivity::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const
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

LevelUpActivity::LevelUpActivity(CGameHandler * owner, const CGHeroInstance * hero)
	: Activity(owner, TYPE), hero(hero->id)
{
	addPlayer(hero->tempOwner);
}

bool LevelUpActivity::endsByPlayerAnswer() const
{
	return true;
}

bool LevelUpActivity::blocksPack(const CPackForServer * pack) const
{
	return blockAllButReply(pack);
}

PromptResult LevelUpActivity::askNextQuestion()
{
	if(phase == Phase::Hero)
	{
		auto result = askHeroLevelUp();
		if(result != PromptResult::Finished)
			return result;

		phase = Phase::Commander;
	}

	if(phase == Phase::Commander)
	{
		auto result = askCommanderLevelUp();
		if(result != PromptResult::Finished)
			return result;

		phase = Phase::Finished;
	}

	return PromptResult::Finished;
}

PromptResult LevelUpActivity::askHeroLevelUp()
{
	const auto * levellingHero = gh->gameInfo().getHero(hero);

	if(!levellingHero || !levellingHero->gainsLevel())
		return PromptResult::Finished;

	// The dialog is only sent once the player's interface can show it. Until then the
	// level is not applied either, so gainsLevel() stays true and we try again later.
	if(!gh->uiReadyForDialogs.contains(players.front()))
		return PromptResult::NotReady;

	auto levelUp = gh->rollHeroLevelUp(levellingHero);
	offeredHeroSkills = levelUp.skills;

	askedQuestionID = askQuestion();
	levelUp.questionID = askedQuestionID;
	gh->sendAndApply(levelUp);

	return PromptResult::Asked;
}

PromptResult LevelUpActivity::askCommanderLevelUp()
{
	const auto * levellingHero = gh->gameInfo().getHero(hero);

	if(!levellingHero || !levellingHero->getCommander() || !levellingHero->getCommander()->gainsLevel())
		return PromptResult::Finished;

	if(!gh->uiReadyForDialogs.contains(players.front()))
		return PromptResult::NotReady;

	auto levelUp = gh->rollCommanderLevelUp(levellingHero->getCommander());
	if(!levelUp)
		return PromptResult::Finished;

	offeredCommanderSkills = levelUp->skills;

	askedQuestionID = askQuestion();
	levelUp->questionID = askedQuestionID;
	gh->sendAndApply(*levelUp);

	return PromptResult::Asked;
}

void LevelUpActivity::applyAnswer(std::optional<int32_t> answer)
{
	// Release the dialog the player just answered before the next question is sent.
	// The client keeps an activity-backed dialog open until it is told that question is
	// resolved, and relies on being told before the following one arrives.
	if(askedQuestionID.hasValue())
	{
		gh->sendQuestionResolved(askedQuestionID);
		askedQuestionID = QuestionID::NONE;
	}

	const auto * levellingHero = gh->gameInfo().getHero(hero);
	if(!levellingHero)
		return;

	if(phase == Phase::Hero)
	{
		if(offeredHeroSkills.empty())
		{
			logGlobal->trace("%s gains no secondary skill", levellingHero->getNameTextID());
		}
		else if(answer && *answer >= 0 && *answer < static_cast<int32_t>(offeredHeroSkills.size()))
		{
			logGlobal->trace("%s gains skill %d", levellingHero->getNameTextID(), *answer);
			gh->applyHeroLevelUp(levellingHero, offeredHeroSkills.at(*answer));
		}
		else
		{
			logGlobal->warn("Invalid secondary skill %d chosen for %s - granting none",
				answer.value_or(-1), levellingHero->getNameTextID());
		}

		offeredHeroSkills.clear();
		return;
	}

	if(offeredCommanderSkills.empty())
	{
		logGlobal->trace("Commander of %s gains no skill", levellingHero->getNameTextID());
	}
	else if(answer && *answer >= 0 && *answer < static_cast<int32_t>(offeredCommanderSkills.size()))
	{
		logGlobal->trace("Commander of %s gains skill %d", levellingHero->getNameTextID(), *answer);
		gh->applyCommanderLevelUp(levellingHero->getCommander(), offeredCommanderSkills.at(*answer));
	}
	else
	{
		logGlobal->warn("Invalid commander skill %d chosen for %s - granting none",
			answer.value_or(-1), levellingHero->getNameTextID());
	}

	offeredCommanderSkills.clear();
}

void LevelUpActivity::onRemoval(PlayerColor color)
{
	// Normally every question has already been released as it was answered. One may
	// still be outstanding if the activity was removed without being answered, and the
	// client would otherwise be left holding that dialog open.
	if(askedQuestionID.hasValue())
		gh->sendQuestionResolved(askedQuestionID);
}

void LevelUpActivity::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const
{
	visitedObject->heroLevelUpDone(*gh, visitingHero, continuationTag);
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

	// A hero that lost the guard battle is no longer on the map, and one that
	// changed hands is no longer ours - either way there is no visit to finish.
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

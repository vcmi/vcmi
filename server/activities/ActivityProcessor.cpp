/*
 * ActivityProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ActivityProcessor.h"

#include "../CGameHandler.h"
#include "Activity.h"
#include "VisitActivities.h"

ActivityProcessor::ActivityProcessor(CGameHandler & gameHandler)
	: gameHandler(gameHandler)
{
}

void ActivityProcessor::popActivity(PlayerColor player, ActivityPtr activity)
{
	LOG_TRACE_PARAMS(logGlobal, "player='%s', activity='%s'", player % activity->toString());
	if(topActivity(player) != activity)
		throw std::runtime_error("Removing activity that is not on top of player's stack: " + activity->toString());

	auto & stack = activities.at(player.getNum());
	stack.pop_back();
	auto nextActivity = topActivity(player);

	rememberCompleted(player, activity->getActiveQuestionID());
	markStackChanged(player);

	// A multi-player activity is done once it has left every stack, e.g. a battle result is
	// applied once and not once per side
	if(countActivity(activity.get()) == 0)
		activity->onRemoval();

	// Only if removal did not add a new activity on top
	if(nextActivity && nextActivity == topActivity(player))
		nextActivity->onChildCompleted(activity);

	// Resolving answered activities and checking victory conditions happen in settle(),
	// once the stacks have stopped changing.
}

void ActivityProcessor::addActivity(ActivityPtr activity)
{
	if(!activity || activity->players.empty())
		throw std::runtime_error("Adding an activity that affects no player");

	{
		MutationScope mutation(*this);

		for(auto player : activity->players)
			addActivity(player, activity);

		activity->onAdded();
	}
	settleIfOutermost();
}

void ActivityProcessor::addActivity(PlayerColor player, ActivityPtr activity)
{
	LOG_TRACE_PARAMS(logGlobal, "player='%d', activity='%s'", player.getNum() % activity->toString());

	auto & stack = activities.at(player.getNum());
	if(vstd::contains(stack, activity))
		throw std::runtime_error("Adding an activity that is already on player's stack: " + activity->toString());

	stack.push_back(activity);
	markStackChanged(player);
}

ActivityPtr ActivityProcessor::topActivity(PlayerColor player)
{
	if(!player.isValidPlayer())
		throw std::runtime_error("Requesting activities of invalid player " + player.toString());

	return vstd::backOrNull(activities[player]);
}

void ActivityProcessor::finishActivity(Activity & activity)
{
	activity.finished = true;
	settleIfOutermost();
}

MapObjectVisitActivity * ActivityProcessor::findVisit(ObjectInstanceID object) const
{
	auto it = activeVisits.find(object);
	return it == activeVisits.end() ? nullptr : it->second;
}

void ActivityProcessor::registerVisit(MapObjectVisitActivity * visit)
{
	assert(visit);
	// A second visit of one object would make the first unreachable, and the object would
	// then be told about the wrong one
	if(!activeVisits.try_emplace(visit->visitedObject, visit).second)
		throw std::runtime_error("Object " + std::to_string(visit->visitedObject.getNum()) + " is already being visited");
}

void ActivityProcessor::unregisterVisit(MapObjectVisitActivity * visit)
{
	auto it = activeVisits.find(visit->visitedObject);
	if(it == activeVisits.end() || it->second != visit)
		throw std::runtime_error("Visit of object " + std::to_string(visit->visitedObject.getNum()) + " was not registered");

	activeVisits.erase(it);
}

ActivityPtr ActivityProcessor::getActivity(QuestionID questionID)
{
	for(auto & playerActivities : activities)
		for(auto & activity : playerActivities)
			if(activity->getActiveQuestionID() == questionID)
				return activity;
	return nullptr;
}

int ActivityProcessor::countActivity(const Activity * activity) const
{
	if(!activity)
		return 0;

	int result = 0;
	for(const auto & playerActivities : activities)
		for(const auto & currentActivity : playerActivities)
			if(currentActivity.get() == activity)
				++result;

	return result;
}

ActivityPtr ActivityProcessor::getActivity(QuestionID questionID, PlayerColor player)
{
	// The player comes from a client's reply
	if(!player.isValidPlayer())
		return nullptr;

	for(const auto & activity : activities.at(player.getNum()))
		if(activity->getActiveQuestionID() == questionID)
			return activity;

	return nullptr;
}

void ActivityProcessor::rememberCompleted(PlayerColor player, QuestionID questionID)
{
	// Activities that never asked anything, e.g. routines, can not be replied to
	if(!questionID.hasValue())
		return;

	auto & completed = recentlyCompleted.at(player.getNum());
	completed.push_back(questionID);

	while(completed.size() > RECENTLY_COMPLETED_LIMIT)
		completed.pop_front();
}

bool ActivityProcessor::wasRecentlyCompleted(PlayerColor player, QuestionID questionID) const
{
	return vstd::contains(recentlyCompleted.at(player.getNum()), questionID);
}

void ActivityProcessor::markStackChanged(PlayerColor player)
{
	stackChanged.at(player.getNum()) = true;
}

ActivityProcessor::MutationScope::MutationScope(ActivityProcessor & owner)
	: owner(owner)
{
	owner.mutationDepth++;
}

ActivityProcessor::MutationScope::~MutationScope()
{
	owner.mutationDepth--;
}

void ActivityProcessor::settleIfOutermost()
{
	// Called after the scope is closed and not from its destructor, so that an exception
	// thrown while settling reaches the caller
	if(mutationDepth == 0)
		settle();
}

bool ActivityProcessor::advanceRoutines()
{
	bool changedAnything = false;

	for(size_t idx = 0; idx < activities.size(); ++idx)
	{
		const PlayerColor player(static_cast<int32_t>(idx));

		for(int step = 0; step < MAX_ROUTINE_STEPS; ++step)
		{
			auto top = topActivity(player);
			if(!top)
				break;

			auto * routine = top->asRoutine();
			if(!routine)
				break;

			const size_t depthBefore = activities.at(idx).size();
			const StepResult result = routine->advance();
			changedAnything = true;

			// The step pushed a child activity: the routine is suspended until it is done.
			if(activities.at(idx).size() != depthBefore || topActivity(player) != top)
				break;

			if(result == StepResult::Done)
			{
				if(top->players.size() != 1)
					throw std::runtime_error("Routine affects more than one player: " + top->toString());

				popActivity(player, top);
				break;
			}

			if(step + 1 == MAX_ROUTINE_STEPS)
				throw std::runtime_error("Routine did not finish after " + std::to_string(MAX_ROUTINE_STEPS) + " steps: " + top->toString());
		}
	}

	return changedAnything;
}

bool ActivityProcessor::resolveAnsweredActivities()
{
	bool changedAnything = false;

	for(size_t idx = 0; idx < activities.size(); ++idx)
	{
		const PlayerColor player(static_cast<int32_t>(idx));

		while(auto top = topActivity(player))
		{
			if(!top->isFinished())
				break;

			popActivity(player, top);
			changedAnything = true;
		}
	}

	return changedAnything;
}

bool ActivityProcessor::runVictoryChecks()
{
	// Taken before the checks, so that the flags then reflect only what the checks change
	const auto changedStacks = std::exchange(stackChanged, {});

	// Only players whose stack has just emptied: a check may end the game outright, e.g. on
	// a battle-only map, so an idle player must not be checked on every unrelated change
	for(size_t idx = 0; idx < activities.size(); ++idx)
	{
		if(!changedStacks.at(idx) || !activities.at(idx).empty())
			continue;

		gameHandler.checkVictoryLossConditionsForPlayer(PlayerColor(static_cast<int32_t>(idx)));
	}

	return std::ranges::any_of(stackChanged, [](bool value){ return value; });
}

void ActivityProcessor::settle()
{
	if(settling)
		return; // the running settle() loop will pick up the change

	settling = true;

	// Must be cleared even if an activity hook throws, otherwise no deferred work
	// would ever run again
	struct SettlingReset
	{
		bool & flag;
		~SettlingReset() { flag = false; }
	} settlingReset{settling};

	for(int round = 0; round < MAX_SETTLE_ROUNDS; ++round)
	{
		// A reply may have arrived for an activity that was not on top at that moment
		if(resolveAnsweredActivities())
			continue;

		// A routine exposed by that removal may have more work before its player is idle
		if(advanceRoutines())
			continue;

		if(runVictoryChecks())
			continue;

		return;
	}

	throw std::runtime_error("Activity stacks did not settle after " + std::to_string(MAX_SETTLE_ROUNDS) + " rounds! Activities:\n" + describeStacks());
}

ReplyOutcome ActivityProcessor::submitReply(QuestionID questionID, PlayerColor player, std::optional<int32_t> reply)
{
	auto activity = getActivity(questionID, player);

	if(!activity)
	{
		if(wasRecentlyCompleted(player, questionID))
			return ReplyOutcome::IgnoredAlreadyCompleted;

		if(getActivity(questionID))
			return ReplyOutcome::RejectedWrongPlayer;

		return ReplyOutcome::RejectedUnknownActivity;
	}

	if(!activity->endsByPlayerAnswer())
		return ReplyOutcome::RejectedNotAnswerable;

	if(!activity->acceptsAnswerFrom(player))
		return ReplyOutcome::RejectedWrongPlayer;

	if(activity->isAnswered())
		return ReplyOutcome::IgnoredAlreadyAnswered;

	// Only an activity that the player can cancel, e.g. town selection, may be answered
	// without a value
	if(!reply.has_value() && !activity->acceptsAnswerWithoutValue())
		return ReplyOutcome::RejectedMissingAnswer;

	if(reply.has_value() && !activity->acceptsAnswer(*reply))
		return ReplyOutcome::RejectedInvalidAnswer;

	activity->setReply(reply);
	activity->answeredBy = player;

	// The activity is resolved for every player that it affects, once it is at the top of
	// each of their stacks
	settleIfOutermost();
	return ReplyOutcome::Accepted;
}

std::string ActivityProcessor::describeStacks() const
{
	std::string result;

	for(size_t idx = 0; idx < activities.size(); ++idx)
	{
		const auto & stack = activities.at(idx);
		if(stack.empty())
			continue;

		result += boost::str(boost::format("  player %d, %d activit%s (top last):\n")
			% idx
			% stack.size()
			% (stack.size() == 1 ? "y" : "ies"));

		for(const auto & activity : stack)
			result += "    " + activity->toString() + "\n";
	}

	if(result.empty())
		return "  (no player has any activities)\n";

	return result;
}

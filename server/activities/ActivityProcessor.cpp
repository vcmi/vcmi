/*
 * Activity.cpp, part of VCMI engine
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

ActivityProcessor::ActivityProcessor(CGameHandler & gameHandler)
	: gameHandler(gameHandler)
{
}

void ActivityProcessor::popActivity(PlayerColor player, ActivityPtr activity)
{
	LOG_TRACE_PARAMS(logGlobal, "player='%s', activity='%s'", player % activity);
	if(topActivity(player) != activity)
	{
		logGlobal->trace("Cannot remove, not a top!");
		return;
	}

	const auto idx = static_cast<size_t>(player.getNum());
	assert(activity);

	auto & stack = activities.at(idx);
	stack.pop_back();
	auto nextActivity = topActivity(player);

	rememberCompleted(player, activity->getActiveQuestionID());
	markStackChanged(player);

	activity->onRemoval(player);

	//Exposure on activity below happens only if removal didn't trigger any new activity
	if(nextActivity && nextActivity == topActivity(player))
	{
		// A routine receives onChildCompleted() and is then stepped by settle(),
		// so it must not also receive the generic exposure hook.
		if(auto * routine = nextActivity->asRoutine())
			routine->onChildCompleted(activity);
		else
			nextActivity->onExposure(activity);
	}

	// Resolving answered activities and checking victory conditions happen in settle(),
	// once the stacks have stopped changing.
}

void ActivityProcessor::popActivity(const Activity &activity)
{
	LOG_TRACE_PARAMS(logGlobal, "activity='%s'", activity);
	MutationScope mutation(*this);

	assert(activity.players.size());
	for(auto player : activity.players)
	{
		auto top = topActivity(player);
		if(top.get() == &activity)
			popActivity(top);
		else
		{
			if(logGlobal->isTraceEnabled())
			{
				const auto idx = static_cast<size_t>(player.getNum());

				logGlobal->trace("Cannot remove activity %s", activity.toString());
				logGlobal->trace("Activities found:");
				for(const auto & q : activities.at(idx))
				{
					logGlobal->trace(q->toString());
				}
			}
		}
	}
}

void ActivityProcessor::popActivity(ActivityPtr activity)
{
	MutationScope mutation(*this);

	for(auto player : activity->players)
		popActivity(player, activity);
}

void ActivityProcessor::addActivity(ActivityPtr activity)
{
	MutationScope mutation(*this);

	for(auto player : activity->players)
		addActivity(player, activity);
}

void ActivityProcessor::addActivityWhenIdle(ActivityPtr activity)
{
	MutationScope mutation(*this);

	assert(activity);
	for(auto player : activity->players)
	{
		if(player.isValidPlayer())
			waiting.at(player.getNum()).push_back(activity);
	}
}

void ActivityProcessor::addActivity(PlayerColor player, ActivityPtr activity)
{
	LOG_TRACE_PARAMS(logGlobal, "player='%d', activity='%s'", player.getNum() % activity);

	const auto idx = static_cast<size_t>(player.getNum());
	assert(activity);
	auto & stack = activities.at(idx);
	// Prevent adding the same activity twice in a row
	if(!stack.empty() && stack.back() == activity)
		return;
	activity->onAdding(player);
	activities.at(idx).push_back(activity);
	markStackChanged(player);
	activity->onAdded(player);
}

ActivityPtr ActivityProcessor::topActivity(PlayerColor player)
{
	assert(player.isValidPlayer());
	if(!player.isValidPlayer())
		return nullptr;

	return vstd::backOrNull(activities[player]);
}

void ActivityProcessor::popIfTop(ActivityPtr activity)
{
	LOG_TRACE_PARAMS(logGlobal, "activity='%d'", activity);
	if(!activity)
	{
		logGlobal->error("The activity is nullptr! Ignoring.");
		return;
	}

	popIfTop(*activity);
}

void ActivityProcessor::popIfTop(const Activity & activity)
{
	MutationScope mutation(*this);

	for(PlayerColor color : activity.players)
		if(topActivity(color).get() == &activity)
			popActivity(color, topActivity(color));
}

ActivityProcessor::AllActivitiesViewConst ActivityProcessor::allActivities() const
{
	return AllActivitiesViewConst(activities);
}

ActivityProcessor::AllActivitiesView ActivityProcessor::allActivities()
{
	return AllActivitiesView(activities);
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
	for(const auto & currentActivity : allActivities())
	{
		if(currentActivity.get() == activity)
			++result;
	}
	return result;
}

ActivityPtr ActivityProcessor::getActivity(QuestionID questionID, PlayerColor player)
{
	if(!player.isValidPlayer())
		return nullptr;

	for(const auto & activity : activities.at(player.getNum()))
		if(activity->getActiveQuestionID() == questionID)
			return activity;

	return nullptr;
}

void ActivityProcessor::rememberCompleted(PlayerColor player, QuestionID questionID)
{
	if(!player.isValidPlayer() || !questionID.hasValue())
		return;

	auto & completed = recentlyCompleted.at(player.getNum());
	completed.push_back(questionID);

	while(completed.size() > RECENTLY_COMPLETED_LIMIT)
		completed.pop_front();
}

bool ActivityProcessor::wasRecentlyCompleted(PlayerColor player, QuestionID questionID) const
{
	if(!player.isValidPlayer())
		return false;

	return vstd::contains(recentlyCompleted.at(player.getNum()), questionID);
}

void ActivityProcessor::markStackChanged(PlayerColor player)
{
	if(player.isValidPlayer())
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

	if(owner.mutationDepth == 0)
		owner.settle();
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
				// Routines affect a single player, so only one stack is unwound
				assert(top->players.size() == 1);
				popActivity(player, top);
				break;
			}

			if(step + 1 == MAX_ROUTINE_STEPS)
				logGlobal->error("Routine did not finish after %d steps: %s", MAX_ROUTINE_STEPS, top->toString());
		}
	}

	return changedAnything;
}

bool ActivityProcessor::advanceInteractions()
{
	bool changedAnything = false;

	for(size_t idx = 0; idx < activities.size(); ++idx)
	{
		const PlayerColor player(static_cast<int32_t>(idx));

		auto top = topActivity(player);
		if(!top)
			continue;

		auto * interaction = top->asInteraction();
		if(!interaction || top->isAnswered())
			continue;

		if(top->hasOutstandingQuestion())
			continue;

		switch(interaction->askNextQuestion())
		{
			case PromptResult::Asked:
				changedAnything = true;
				break;

			case PromptResult::Finished:
				// Interaction affects a single player, so only one stack is unwound
				assert(top->players.size() == 1);
				popActivity(player, top);
				changedAnything = true;
				break;

			case PromptResult::NotReady:
				break;
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
			if(!top->isAnswered())
				break;

			if(!top->endsByPlayerAnswer())
				break; // should not happen - submitReply refuses to answer such activities

			popActivity(player, top);
			changedAnything = true;
		}
	}

	return changedAnything;
}

void ActivityProcessor::discardQueuedWork(PlayerColor player)
{
	if(player.isValidPlayer())
		waiting.at(player.getNum()).clear();
}

bool ActivityProcessor::promoteWaitingActivities()
{
	bool startedAnything = false;

	for(size_t idx = 0; idx < activities.size(); ++idx)
	{
		if(!activities.at(idx).empty())
			continue; // fast path, everyPlayerIsIdle below is the actual condition

		auto & queue = waiting.at(idx);
		if(queue.empty())
			continue;

		auto activity = queue.front();
		queue.pop_front();

		// An activity shared by several players is queued for each of them, and started
		// only once all of them are idle
		const bool everyPlayerIsIdle = std::ranges::all_of(activity->players, [this](PlayerColor player)
		{
			return player.isValidPlayer() && activities.at(player.getNum()).empty();
		});

		if(!everyPlayerIsIdle)
		{
			queue.push_front(activity);
			continue;
		}

		for(auto player : activity->players)
			vstd::erase_if_present(waiting.at(player.getNum()), activity);

		addActivity(activity);
		startedAnything = true;
	}

	return startedAnything;
}

bool ActivityProcessor::runVictoryChecks()
{
	// Cleared first so that the flags reflect only what the checks themselves change
	stackChanged = {};

	// checkVictoryLossConditionsForPlayer() ignores players that still have activities,
	// so it can be called for every idle player
	for(size_t idx = 0; idx < activities.size(); ++idx)
	{
		const PlayerColor player(static_cast<int32_t>(idx));

		if(!activities.at(idx).empty())
			continue;

		gameHandler.checkVictoryLossConditionsForPlayer(player);
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

		// An interaction may still have questions to ask
		if(advanceInteractions())
			continue;

		// Queued work starts only once the player has nothing left to do
		if(promoteWaitingActivities())
			continue;

		if(runVictoryChecks())
			continue;

		return;
	}

	logGlobal->error("Activity stacks did not settle after %d rounds! Activities:\n%s", MAX_SETTLE_ROUNDS, describeStacks());
}

ReplyOutcome ActivityProcessor::submitReply(QuestionID questionID, PlayerColor player, std::optional<int32_t> reply)
{
	MutationScope mutation(*this);

	auto activity = getActivity(questionID, player);

	if(!activity)
	{
		// The activity may have been removed while the reply was in flight, which is a
		// normal race and not an error
		if(wasRecentlyCompleted(player, questionID))
			return ReplyOutcome::IgnoredAlreadyCompleted;

		if(getActivity(questionID))
			return ReplyOutcome::RejectedWrongPlayer;

		return ReplyOutcome::RejectedUnknownActivity;
	}

	if(!vstd::contains(activity->players, player))
		return ReplyOutcome::RejectedWrongPlayer;

	if(!activity->endsByPlayerAnswer())
		return ReplyOutcome::RejectedNotAnswerable;

	if(activity->isAnswered())
		return ReplyOutcome::IgnoredAlreadyAnswered;

	// Only an activity that the player can cancel, e.g. town selection, may be answered
	// without a value
	if(!reply.has_value() && !activity->acceptsAnswerWithoutValue())
		return ReplyOutcome::RejectedMissingAnswer;

	if(auto * interaction = activity->asInteraction())
	{
		// An interaction is not finished by an answer since it may have more to ask, so
		// remember the question to recognize a repeated answer as stale instead of taking
		// it for an answer to the next question
		rememberCompleted(player, activity->getActiveQuestionID());
		activity->activeQuestionID = QuestionID::NONE;
		interaction->applyAnswer(reply);
		return ReplyOutcome::Accepted;
	}

	activity->setReply(reply);
	activity->answeredBy = player;

	// The activity is resolved for every player that it affects, once it is at the top of
	// each of their stacks. Done by settle() when this scope closes.
	return ReplyOutcome::Accepted;
}

void ActivityProcessor::retryDeferredWork(PlayerColor player)
{
	// settle() runs when the outermost scope closes and retries every step for every player
	MutationScope mutation(*this);
}

std::string ActivityProcessor::describeStacks() const
{
	std::string result;

	for(size_t idx = 0; idx < activities.size(); ++idx)
	{
		const auto & stack = activities.at(idx);
		if(stack.empty())
			continue;

		result += boost::str(boost::format("  player %d, %d quer%s (top last):\n")
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

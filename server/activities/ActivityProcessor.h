/*
 * ActivityProcessor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"
#include "constants/EntityIdentifiers.h"
#include "activities/Activity.h"

class CGameHandler;
class Activity;
using ActivityPtr = std::shared_ptr<Activity>;

/// Result of submitting a player's reply. Only the Rejected* outcomes indicate a problem,
/// the Ignored* ones are races that happen in normal play and are not reported to the player.
enum class ReplyOutcome : uint8_t
{
	Accepted, ///< Reply was recorded, activity is resolved once it is on top of every affected stack
	IgnoredAlreadyCompleted, ///< Activity was removed by another event while the reply was in flight
	IgnoredAlreadyAnswered, ///< Duplicate or retried reply
	RejectedUnknownActivity, ///< No such activity and none recently completed, client is out of sync
	RejectedWrongPlayer, ///< Activity does not affect this player
	RejectedNotAnswerable, ///< Activity can not be ended by a player reply
	RejectedMissingAnswer, ///< Reply has no answer, but the activity requires one
};

class ActivityProcessor
{
public:
	explicit ActivityProcessor(CGameHandler & gameHandler);

	using ActivityStack = std::vector<ActivityPtr>;
	using ActivitiesPerPlayer = std::array<ActivityStack, PlayerColor::PLAYER_LIMIT_I>;

private:
	void addActivity(PlayerColor player, ActivityPtr activity);
	void popActivity(PlayerColor player, ActivityPtr activity);

	ActivitiesPerPlayer activities;
	CGameHandler & gameHandler;

	/// Activities that wait for their player to become idle. Work unrelated to what the
	/// player is currently doing must not be pushed on top of it, since the player would
	/// then answer it before the question that was asked first.
	std::array<std::deque<ActivityPtr>, PlayerColor::PLAYER_LIMIT_I> waiting;

	/// Questions of activities that recently left a player's stack, so that submitReply can
	/// distinguish a lost race from an invalid question id.
	std::array<std::deque<QuestionID>, PlayerColor::PLAYER_LIMIT_I> recentlyCompleted;
	static constexpr size_t RECENTLY_COMPLETED_LIMIT = 64;

	/// Number of stack mutations in progress. Activity hooks add or remove further
	/// activities, so one player action nests several levels deep. Deferred work runs
	/// only once the outermost mutation finishes.
	int mutationDepth = 0;

	/// Set while settle() runs, so that mutations caused by it are picked up by its own
	/// loop instead of recursing into it.
	bool settling = false;

	/// Players whose stack changed. Cleared and re-read around the victory checks, the only
	/// deferred work that does not report back directly.
	std::array<bool, PlayerColor::PLAYER_LIMIT_I> stackChanged = {};

	/// Limit of settle() rounds, to catch two pieces of deferred work that trigger each
	/// other endlessly. Every round that continues has made progress, so this bounds total
	/// work and must stay far above a legitimate case such as a turn start that queues a
	/// visit for every town and every building in them.
	static constexpr int MAX_SETTLE_ROUNDS = 100000;

	/// Steps that a single routine may take in one go before it is assumed to be stuck.
	/// A routine takes one step per unit of work, e.g. per building visited in a town.
	static constexpr int MAX_ROUTINE_STEPS = 1000;

	void rememberCompleted(PlayerColor player, QuestionID questionID);
	bool wasRecentlyCompleted(PlayerColor player, QuestionID questionID) const;
	void markStackChanged(PlayerColor player);

	/// Steps every routine at the top of a player's stack until it finishes or suspends
	/// itself by pushing a child. Returns true if anything changed.
	bool advanceRoutines();

	/// Asks the next question of every interaction at the top of a player's stack and
	/// removes those with nothing left to ask. Returns true if anything changed.
	bool advanceInteractions();

	/// Pops every already answered activity at the top of a player's stack. Returns true
	/// if anything was removed.
	bool resolveAnsweredActivities();

	/// Starts the next waiting activity of every player that has become idle.
	/// Returns true if it started anything.
	bool promoteWaitingActivities();

	/// Runs victory/loss checks for players that just became idle. Returns true if a stack changed.
	bool runVictoryChecks();

	/// Runs everything that must not happen while the stacks are still changing: resolving
	/// answered activities, stepping routines, starting waiting activities and victory/loss
	/// checks. Loops until nothing changes, so callers always observe a settled state.
	void settle();

	/// RAII bracket around a public mutation. The outermost one settles on exit.
	class MutationScope : boost::noncopyable
	{
	public:
		explicit MutationScope(ActivityProcessor & owner);
		~MutationScope();

	private:
		ActivityProcessor & owner;
	};

	template<typename StorageT>
	class AllActivitiesViewT
	{
	public:
		explicit AllActivitiesViewT(StorageT & s) : storage(&s) {}

		struct iterator
		{
			StorageT * storage = nullptr;
			size_t outer = 0;
			size_t inner = 0;

			void advance()
			{
				while(outer < storage->size())
				{
					auto & v = (*storage)[outer];
					if(inner < v.size())
						return;

					outer++;
					inner = 0;
				}
			}

			decltype(auto) operator*() const
			{
				return (*storage)[outer][inner]; // ActivityPtr& or const ActivityPtr&
			}

			iterator & operator++()
			{
				inner++;
				advance();
				return *this;
			}

			bool operator==(const iterator & other) const
			{
				return storage == other.storage && outer == other.outer && inner == other.inner;
			}

			bool operator!=(const iterator & other) const
			{
				return !(*this == other);
			}
		};

		iterator begin() const
		{
			iterator it{ storage, 0, 0 };
			it.advance();
			return it;
		}

		iterator end() const
		{
			return iterator{ storage, storage->size(), 0 };
		}

	private:
		StorageT * storage;
	};

public:
	using AllActivitiesView = AllActivitiesViewT<ActivitiesPerPlayer>;
	using AllActivitiesViewConst = AllActivitiesViewT<const ActivitiesPerPlayer>;

	void addActivity(ActivityPtr activity);

	/// Adds an activity once its players have nothing else to do. Use for work unrelated to
	/// the activity that the player is currently dealing with, so that it queues up behind
	/// it instead of interrupting it.
	void addActivityWhenIdle(ActivityPtr activity);

	/// Drops everything still queued for a player. Used on turn end, so that work queued
	/// during that turn can not surface in a later one.
	void discardQueuedWork(PlayerColor player);

	void popActivity(const Activity &activity);
	void popActivity(ActivityPtr activity);
	void popIfTop(const Activity &activity); //removes this activity if it is at the top (otherwise, do nothing)
	void popIfTop(ActivityPtr activity); //removes this activity if it is at the top (otherwise, do nothing)

	ActivityPtr topActivity(PlayerColor player);
	ActivityPtr getActivity(QuestionID questionID);

	/// Looks the activity up on this player's stack only. Use instead of getActivity() when
	/// handling player input: question ids are not unique across players (QuestionID::CLIENT
	/// is shared by every pause activity), so a global lookup can return another player's
	/// activity.
	ActivityPtr getActivity(QuestionID questionID, PlayerColor player);

	/// Records a player's reply to an activity. The activity does not have to be at the top of
	/// the stack: the server may have pushed something else between sending the question and
	/// receiving the answer, and rejecting the reply would leave both sides waiting forever.
	ReplyOutcome submitReply(QuestionID questionID, PlayerColor player, std::optional<int32_t> reply);

	/// Re-runs deferred work for every player, after something outside the activity system
	/// changed whether it can go on, e.g. a player's interface became able to show a dialog.
	void retryDeferredWork();

	/// Multi-line dump of every player's stack, for diagnosing a stuck player.
	std::string describeStacks() const;

	AllActivitiesView allActivities();
	AllActivitiesViewConst allActivities() const;
	/// On how many players' stacks this activity sits.
	int countActivity(const Activity * activity) const;

	template<typename T, typename ActivityPtrT>
	using ActivityAsResult = std::conditional_t<
		std::is_const_v<std::remove_pointer_t<decltype(std::declval<const ActivityPtrT &>().get())>>,
		const T *,
		T *>;

	template<typename T, typename ActivityPtrT>
	ActivityAsResult<T, ActivityPtrT> activityAs(const ActivityPtrT & activity)
	{
		using ResultT = std::remove_pointer_t<ActivityAsResult<T, ActivityPtrT>>;

		if(!activity)
			return nullptr;

		if(activity->getType() != T::TYPE)
			return nullptr;

		assert(dynamic_cast<ResultT*>(activity.get()) != nullptr);
		return static_cast<ResultT*>(activity.get());
	}

	/// The single activity of the given type anywhere on a player's stack, or nullptr. Some
	/// types are limited to one per player, e.g. a player can only be in one battle. Logs an
	/// error instead of picking one if there are several.
	template<typename T>
	T * findSoleActivity(PlayerColor player)
	{
		if(!player.isValidPlayer())
			return nullptr;

		T * result = nullptr;

		for(const auto & activity : activities.at(player.getNum()))
		{
			auto * typed = activityAs<T>(activity);
			if(!typed)
				continue;

			if(result != nullptr)
			{
				logGlobal->error("Player %s has more than one activity of type '%s'!\nActivities:\n%s",
					player.toString(), ::toString(T::TYPE), describeStacks());
				assert(false);
				break;
			}

			result = typed;
		}

		return result;
	}

	template<typename T, typename Predicate>
	T * findActivity(Predicate predicate) const
	{
		for(const auto & playerActivities : activities)
		{
			for(auto it = playerActivities.rbegin(); it != playerActivities.rend(); ++it)
			{
				auto * activity = dynamic_cast<T *>(it->get());
				if(activity && predicate(*activity))
					return activity;
			}
		}

		return nullptr;
	}

};

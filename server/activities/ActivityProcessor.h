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

/// Result of offering a player's reply to the activity system.
/// Only Rejected* outcomes indicate a problem; the Ignored* ones are benign races
/// that are expected in normal play and must not be reported to the player.
enum class ReplyOutcome : uint8_t
{
	/// Reply was recorded. The activity may or may not have been resolved yet:
	/// it is resolved once it reaches the top of every affected player's stack.
	Accepted,

	/// The activity completed before this reply arrived - typically because it was
	/// removed by some other event while the reply was in flight. Benign.
	IgnoredAlreadyCompleted,

	/// This activity has already been answered (duplicate or retried reply). Benign.
	IgnoredAlreadyAnswered,

	/// No such activity, and none recently completed - the client is out of sync
	/// or the reply is malformed.
	RejectedUnknownActivity,

	/// The activity exists but this player is not one of the players it affects.
	RejectedWrongPlayer,

	/// The activity exists but is not the kind that a player reply can end.
	RejectedNotAnswerable,

	/// The reply carried no answer, for a activity that needs one.
	RejectedMissingAnswer,
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

	/// Activities that are waiting for their player to become idle. Work that is not
	/// part of whatever the player is currently doing must not push itself on top of
	/// it - the player would be answering it instead of what they were asked first.
	std::array<std::deque<ActivityPtr>, PlayerColor::PLAYER_LIMIT_I> waiting;

	/// IDs of activities that recently left a player's stack. Lets submitReply tell a
	/// harmless "your reply lost the race" apart from a genuinely bogus activity ID.
	std::array<std::deque<QuestionID>, PlayerColor::PLAYER_LIMIT_I> recentlyCompleted;
	static constexpr size_t RECENTLY_COMPLETED_LIMIT = 64;

	/// Number of activity stack mutations currently in progress. Activity hooks routinely
	/// add or remove further activities, so a single player action can nest several
	/// levels deep; deferred work runs only once the outermost one finishes.
	int mutationDepth = 0;

	/// Set while settle() is running, so that mutations it causes do not recurse
	/// back into it - its own loop picks them up instead.
	bool settling = false;

	/// Players whose stack changed. Cleared and re-read around the victory checks,
	/// which are the one piece of deferred work that does not report back directly.
	std::array<bool, PlayerColor::PLAYER_LIMIT_I> stackChanged = {};

	/// Absolute ceiling on settle() rounds, to catch two pieces of deferred work that
	/// keep triggering each other forever. Every round that continues has made
	/// progress, so this bounds total work rather than futile spinning: it must stay
	/// far above anything legitimate, such as a turn start queuing a visit for every
	/// town and every building in them. Hitting it means a bug, not a busy turn.
	static constexpr int MAX_SETTLE_ROUNDS = 100000;

	/// Steps a single routine may take in one go before the processor assumes it is
	/// stuck. Generous: a routine legitimately takes one step per unit of work, such
	/// as one per building visited in a town.
	static constexpr int MAX_ROUTINE_STEPS = 1000;

	void rememberCompleted(PlayerColor player, QuestionID questionID);
	bool wasRecentlyCompleted(PlayerColor player, QuestionID questionID) const;
	void markStackChanged(PlayerColor player);

	/// Steps every routine that is at the top of a player's stack, until it either
	/// finishes or suspends itself by pushing a child. Returns true if it changed
	/// anything. Routines affect one player, and are removed from that one stack.
	bool advanceRoutines();

	/// Puts the next question of every interaction at the top of a player's stack,
	/// and removes those that have nothing left to ask. Returns true if it changed
	/// anything.
	bool advanceInteractions();

	/// Pops every activity at the top of a player's stack that has already been
	/// answered. Returns true if anything was removed.
	bool resolveAnsweredActivities();

	/// Starts the next waiting activity of every player that has become idle.
	/// Returns true if it started anything.
	bool promoteWaitingActivities();


	/// Runs victory/loss checks for players that just became idle. Returns true if
	/// that changed a stack.
	bool runVictoryChecks();

	/// Runs everything that must not happen while the stacks are still moving:
	/// resolving answered activities, stepping routines, starting waiting activities and
	/// victory/loss checks.
	/// Loops until no further change, so callers always observe a settled state.
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

	/// Adds a activity once its players have nothing else to do. Use this for work that
	/// is not caused by the activity the player is currently dealing with, so that it
	/// queues up behind it instead of interrupting it.
	void addActivityWhenIdle(ActivityPtr activity);

	/// Drops everything still queued for a player. Used when their turn ends, so that
	/// work queued for it cannot surface during a later one.
	void discardQueuedWork(PlayerColor player);

	void popActivity(const Activity &activity);
	void popActivity(ActivityPtr activity);
	void popIfTop(const Activity &activity); //removes this activity if it is at the top (otherwise, do nothing)
	void popIfTop(ActivityPtr activity); //removes this activity if it is at the top (otherwise, do nothing)

	ActivityPtr topActivity(PlayerColor player);
	ActivityPtr getActivity(QuestionID questionID);

	/// Looks the activity up on this player's stack only. Prefer this over getActivity()
	/// when handling player input: activity IDs are not guaranteed to be unique across
	/// players (QuestionID::CLIENT is shared by every pause activity), so a global lookup
	/// can return another player's activity.
	ActivityPtr getActivity(QuestionID questionID, PlayerColor player);

	/// Records a player's reply to a activity. The activity does not have to be at the top
	/// of the stack - the server may well have pushed something else between sending
	/// the prompt and receiving the answer, and rejecting the reply for that reason
	/// would leave both sides waiting for each other forever.
	ReplyOutcome submitReply(QuestionID questionID, PlayerColor player, std::optional<int32_t> reply);

	/// Re-runs deferred work for a player. Needed when something outside the activity
	/// system changes whether it can go on - such as the player's interface becoming
	/// ready to be shown a dialog.
	void retryDeferredWork(PlayerColor player);

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

	/// The one activity of the given type anywhere on a player's stack, or nullptr.
	/// Some kinds of activity are limited to one per player - a player can only be in
	/// one battle - and callers rely on that. Complains if the invariant is broken
	/// rather than silently picking one.
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

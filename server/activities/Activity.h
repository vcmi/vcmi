/*
 * Activity.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ActivityID.h"
#include <boost/container/small_vector.hpp>

struct CPackForServer;
class CGObjectInstance;
class CGHeroInstance;

class MapObjectVisitActivity;
class ActivityProcessor;
class Activity;
class CGameHandler;

using ActivityPtr = std::shared_ptr<Activity>;



enum class ActivityType : uint8_t
{
	BlockingDialog,
	GarrisonDialog,
	TeleportDialog,
	HeroLevelUpDialog,
	OpenWindow,
	MapObjectVisit,
	TownBuildingVisit,
	TurnStartVisit,
	Battle,
	BattleDialog,
	HeroMovement,
	TimerPause,
	Generic,
	LuaScript,
	Unknown
};

/// Outcome of a single step of a routine.
enum class StepResult : uint8_t
{
	/// More steps remain. If the step pushed a child activity the routine is suspended
	/// until that child completes; otherwise it is stepped again straight away.
	Continue,

	/// Nothing left to do - the processor removes the routine.
	Done
};

/// Implemented by activities that are multi-step server-side work rather than
/// questions to a player: visiting an object, visiting the buildings of a town,
/// moving a hero. The processor drives the routine by calling advance() until it
/// reports Done, suspending it whenever a step pushes a child activity.
///
/// Everything a routine needs in order to resume must live in its own members, so
/// that the position within the activity is explicit state rather than something
/// reconstructed from the shape of the stack. Members must be restricted to plain
/// data and object IDs - never pointers into the game state, which do not survive
/// a suspension: the object or hero may be gone by the time the routine resumes.
class IRoutine
{
public:
	virtual ~IRoutine() = default;

	/// Perform one step. Called only while this routine is at the top of the stack.
	virtual StepResult advance() = 0;

	/// A child activity pushed by an earlier step has finished. Called before the next
	/// advance(), so that its result can be recorded.
	virtual void onChildCompleted(const ActivityPtr & child) {}
};

/// Outcome of offering a player the next question of an interaction.
enum class PromptResult : uint8_t
{
	/// A question was put to the player. The interaction now waits for the answer.
	Asked,

	/// There is more to ask, but not right now - typically the player's interface
	/// is not ready to show a dialog yet. The processor will try again later.
	NotReady,

	/// Everything has been asked and answered; the processor removes the interaction.
	Finished
};

/// Implemented by activities that put one or more questions to a player. A plain dialog
/// asks once; a hero gaining several levels at once asks once per level, without
/// leaving the stack in between - so the player is never momentarily free to act, and
/// whatever is waiting underneath is told only once, at the end.
///
/// Each question carries its own id, separate from the activity's, so that an answer to
/// a question that has already been superseded can be told apart from an answer to
/// the current one.
class IInteraction
{
public:
	virtual ~IInteraction() = default;

	/// Put the next question to the player, if there is one and they can receive it.
	virtual PromptResult askNextQuestion() = 0;

	/// Apply an answer to the question that was last asked.
	virtual void applyAnswer(std::optional<int32_t> answer) = 0;
};

// Any kind of prolonged interaction that may need to do something special once it is over.
// It does not necessarily require player action - it can also be used internally within the server.
// Examples:
// - all kinds of blocking dialog windows
// - battle
// - object visit
// - hero movement
// An activity can cause another, forming a stack of them for each player.
// Eg: hero movement -> object visit -> dialog.
class Activity : boost::noncopyable
{
public:
	boost::container::small_vector<PlayerColor, PlayerColor::PLAYER_LIMIT_I> players; //players that are affected (often "blocked") by activity
	ActivityID activityID;

	ActivityType getType() const
	{
		return type;
	}

	/// Player whose reply this activity is resolved by, once one has been accepted.
	/// Set by ActivityProcessor::submitReply, never by the activity itself. A activity may
	/// be answered long before it reaches the top of the stack: the processor stores
	/// the reply here and resolves the activity once it is actually exposed.
	const std::optional<PlayerColor> & getAnsweredBy() const
	{
		return answeredBy;
	}

	bool isAnswered() const
	{
		return answeredBy.has_value();
	}

	/// activity can block attempting actions by player. Eg. he can't move hero during the battle.
	virtual bool blocksPack(const CPackForServer *pack) const;

	/// activity is removed after player gives answer (like dialogs)
	virtual bool endsByPlayerAnswer() const;

	/// Whether an answer carrying no value is meaningful. True only where the player
	/// is offered a way out, such as cancelling a town selection.
	virtual bool acceptsAnswerWithoutValue() const;

	/// called just before activity is pushed on stack
	virtual void onAdding(PlayerColor color);

	/// called right after activity is pushed on stack
	virtual void onAdded(PlayerColor color);

	/// called after activity is removed from stack
	virtual void onRemoval(PlayerColor color);

	/// called when activity immediately above is removed and this is exposed (becomes top)
	virtual void onExposure(ActivityPtr topActivity);

	/// called when this activity is being removed and must report its result to currently visited object
	virtual void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const;

	virtual void setReply(std::optional<int32_t> reply);
	virtual std::string toString() const;

	/// Non-null for activities that the processor should drive step by step.
	virtual IRoutine * asRoutine() { return nullptr; }

	/// Non-null for activities that put questions to a player.
	virtual IInteraction * asInteraction() { return nullptr; }

	/// The question the player was last asked and has not yet answered, if any. This
	/// is what an answer must name - never the activity, which the player never sees.
	QuestionID getActiveQuestionID() const
	{
		return activeQuestionID;
	}

	/// Whether the player has been asked something and has not answered yet.
	bool hasOutstandingQuestion() const
	{
		return activeQuestionID.hasValue();
	}

	/// Allocates the next question and records it as the outstanding one. The caller
	/// puts the returned id into the pack it sends to the player.
	QuestionID askQuestion();

	/// Records a question the player will answer by a well-known id rather than one
	/// allocated here. Used for unpausing, which the client reports with a reserved
	/// id it was never given.
	void expectAnswerTo(QuestionID reserved);

	virtual ~Activity();
protected:
	explicit Activity(CGameHandler * gh, ActivityType type);

	ActivityProcessor * owner;
	CGameHandler * gh;
	void addPlayer(PlayerColor color);
	bool blockAllButReply(const CPackForServer * pack) const;

private:
	friend class ActivityProcessor;

	ActivityType type = ActivityType::Unknown;
	std::optional<PlayerColor> answeredBy;

protected:
	/// Set whenever a question is put to the player, cleared once it is answered.
	QuestionID activeQuestionID = QuestionID::NONE;
};

/// Human-readable name of a activity type, for logs and player-facing complaints.
std::string toString(ActivityType type);

std::ostream &operator<<(std::ostream &out, const Activity &activity);
std::ostream &operator<<(std::ostream &out, ActivityPtr activity);

class DialogActivity : public Activity
{
public:
	explicit DialogActivity(CGameHandler * owner, ActivityType type);
	bool endsByPlayerAnswer() const override;
	bool blocksPack(const CPackForServer *pack) const override;
	void setReply(std::optional<int32_t> reply) override;
protected:
	std::optional<ui32> answer;
};

class CallbackActivity : public Activity
{
public:
	CallbackActivity(CGameHandler * gh, PlayerColor color, const std::function<void(std::optional<int32_t>)> & callback);

	bool blocksPack(const CPackForServer * pack) const override;
	bool endsByPlayerAnswer() const override;
	bool acceptsAnswerWithoutValue() const override;
	void onExposure(ActivityPtr topActivity) override;
	void setReply(std::optional<int32_t> reply) override;
	void onRemoval(PlayerColor color) override;
private:
	std::function<void(std::optional<int32_t>)> callback;
	std::optional<int32_t> reply;
};

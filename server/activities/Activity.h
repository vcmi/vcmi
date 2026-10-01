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

#include "../../lib/constants/EntityIdentifiers.h"
#include <boost/container/small_vector.hpp>

struct CPackForServer;
class JsonNode;
class CGObjectInstance;
class IObjectInterface;
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
	HeroLevelUp,
	HeroLevelUpDialog,
	CommanderLevelUpDialog,
	OpenWindow,
	MapObjectVisit,
	TownBuildingVisit,
	TurnStart,
	Battle,
	BattleDialog,
	HeroMovement,
	TimerPause,
	TownSelection,
	ScriptDialog,
	LuaScript,
	Unknown
};

/// Outcome of a single step of a routine.
enum class StepResult : uint8_t
{
	Continue, ///< More steps remain. Suspended if the step pushed a child activity, stepped again otherwise
	Done ///< Nothing left to do, the processor removes the routine
};

/// Implemented by activities that are multi-step server-side work instead of a single
/// question to a player: object visit, town building visit, a chain of level-ups. The
/// processor calls advance() until it returns Done, suspending the routine whenever a
/// step pushes a child activity.
///
/// State needed to resume must be kept in members of the routine, restricted to plain data
/// and object IDs: a pointer into the game state may dangle by the time the routine resumes.
class IRoutine
{
public:
	virtual ~IRoutine() = default;

	/// Perform one step. Called only while this routine is at the top of the stack.
	virtual StepResult advance() = 0;
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
	using PlayerList = boost::container::small_vector<PlayerColor, PlayerColor::PLAYER_LIMIT_I>;

	uint32_t traceNumber = 0; ///< sequence number for logs and stack dumps only, never sent anywhere

	ActivityType getType() const
	{
		return type;
	}

	/// Players that are affected (often "blocked") by activity
	const PlayerList & getPlayers() const
	{
		return players;
	}

	/// An activity may be answered before it reaches the top of the stack, in which case the
	/// reply is stored and applied once the activity is exposed.
	bool isAnswered() const
	{
		return answeredBy.has_value();
	}

	/// Answered, or done on its own. The processor removes it from each stack where it is on top.
	bool isFinished() const
	{
		return finished || isAnswered();
	}

	/// Marks the activity as done without a player's answer
	void finish();

	/// activity can block attempting actions by player. Eg. he can't move hero during the battle.
	virtual bool blocksPack(const CPackForServer *pack) const;

	/// activity is removed after player gives answer (like dialogs)
	virtual bool endsByPlayerAnswer() const;

	/// Whether an answer without a value is valid. True only if the player can cancel,
	/// e.g. town selection.
	virtual bool acceptsAnswerWithoutValue() const;

	/// called once the activity is pushed on the stacks of all its players
	virtual void onAdded();

	/// called once the activity is removed from the stacks of all its players
	virtual void onRemoval();

	/// called when the activity immediately above is removed and this one becomes top. A routine
	/// is stepped again afterwards.
	virtual void onChildCompleted(const ActivityPtr & child);

	/// called when this activity is being removed and must report its result to currently visited object
	virtual void notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const;

	virtual void setReply(std::optional<int32_t> reply);
	virtual std::string toString() const;

	/// Non-null for activities that the processor should drive step by step.
	virtual IRoutine * asRoutine() { return nullptr; }

	/// The question that the player was last asked and has not answered yet, if any.
	/// Answers from the player identify this id, never the activity.
	QuestionID getActiveQuestionID() const
	{
		return activeQuestionID;
	}

	/// Whether a question has been asked and not answered yet.
	bool hasOutstandingQuestion() const
	{
		return activeQuestionID.hasValue();
	}

	/// Allocates the next question and records it as the outstanding one. Caller puts the
	/// returned id into the pack that it sends to the player.
	QuestionID askQuestion();

	/// Records a question that the player will answer with a reserved id instead of an
	/// allocated one. Used for unpausing, which the client reports with a fixed id.
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

	std::string typeName() const;

	PlayerList players;
	ActivityType type = ActivityType::Unknown;
	std::optional<PlayerColor> answeredBy;
	bool finished = false;

protected:
	QuestionID activeQuestionID = QuestionID::NONE; ///< set when a question is asked, cleared when it is answered
};

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


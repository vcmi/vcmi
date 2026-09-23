/*
 * CQuery.h, part of VCMI engine
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
class CGObjectInstance;
class CGHeroInstance;

class CObjectVisitQuery;
class QueriesProcessor;
class CQuery;
class CGameHandler;

using QueryPtr = std::shared_ptr<CQuery>;

enum class QueryType : uint8_t
{
	BlockingDialog,
	GarrisonDialog,
	TeleportDialog,
	HeroLevelUpDialog,
	CommanderLevelUpDialog,
	OpenWindow,
	MapObjectVisit,
	TownBuildingVisit,
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
	/// More steps remain. If the step pushed a child query the routine is suspended
	/// until that child completes; otherwise it is stepped again straight away.
	Continue,

	/// Nothing left to do - the processor removes the routine.
	Done
};

/// Implemented by queries that are multi-step server-side activities rather than
/// questions to a player: visiting an object, visiting the buildings of a town,
/// moving a hero. The processor drives the routine by calling advance() until it
/// reports Done, suspending it whenever a step pushes a child query.
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

	/// A child query pushed by an earlier step has finished. Called before the next
	/// advance(), so that its result can be recorded.
	virtual void onChildCompleted(const QueryPtr & child) {}
};

// This class represents any kind of prolonged interaction that may need to do something special after it is over.
// It does not necessarily has to be "query" requiring player action, it can be also used internally within server.
// Examples:
// - all kinds of blocking dialog windows
// - battle
// - object visit
// - hero movement
// Queries can cause another queries, forming a stack of queries for each player. Eg: hero movement -> object visit -> dialog.
class CQuery : boost::noncopyable
{
public:
	boost::container::small_vector<PlayerColor, PlayerColor::PLAYER_LIMIT_I> players; //players that are affected (often "blocked") by query
	QueryID queryID;

	QueryType getType() const
	{
		return type;
	}

	/// Player whose reply this query is resolved by, once one has been accepted.
	/// Set by QueriesProcessor::submitReply, never by the query itself. A query may
	/// be answered long before it reaches the top of the stack: the processor stores
	/// the reply here and resolves the query once it is actually exposed.
	const std::optional<PlayerColor> & getAnsweredBy() const
	{
		return answeredBy;
	}

	bool isAnswered() const
	{
		return answeredBy.has_value();
	}

	/// query can block attempting actions by player. Eg. he can't move hero during the battle.
	virtual bool blocksPack(const CPackForServer *pack) const;

	/// query is removed after player gives answer (like dialogs)
	virtual bool endsByPlayerAnswer() const;

	/// called just before query is pushed on stack
	virtual void onAdding(PlayerColor color);

	/// called right after query is pushed on stack
	virtual void onAdded(PlayerColor color);

	/// called after query is removed from stack
	virtual void onRemoval(PlayerColor color);

	/// called when query immediately above is removed and this is exposed (becomes top)
	virtual void onExposure(QueryPtr topQuery);

	/// called when this query is being removed and must report its result to currently visited object
	virtual void notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const;

	virtual void setReply(std::optional<int32_t> reply);
	virtual std::string toString() const;

	/// Non-null for queries that the processor should drive step by step.
	virtual IRoutine * asRoutine() { return nullptr; }

	virtual ~CQuery();
protected:
	explicit CQuery(CGameHandler * gh, QueryType type);

	QueriesProcessor * owner;
	CGameHandler * gh;
	void addPlayer(PlayerColor color);
	bool blockAllButReply(const CPackForServer * pack) const;

private:
	friend class QueriesProcessor;

	QueryType type = QueryType::Unknown;
	std::optional<PlayerColor> answeredBy;
};

/// Human-readable name of a query type, for logs and player-facing complaints.
std::string toString(QueryType type);

std::ostream &operator<<(std::ostream &out, const CQuery &query);
std::ostream &operator<<(std::ostream &out, QueryPtr query);

class CDialogQuery : public CQuery
{
public:
	explicit CDialogQuery(CGameHandler * owner, QueryType type);
	bool endsByPlayerAnswer() const override;
	bool blocksPack(const CPackForServer *pack) const override;
	void setReply(std::optional<int32_t> reply) override;
protected:
	std::optional<ui32> answer;
};

class CGenericQuery : public CQuery
{
public:
	CGenericQuery(CGameHandler * gh, PlayerColor color, const std::function<void(std::optional<int32_t>)> & callback);

	bool blocksPack(const CPackForServer * pack) const override;
	bool endsByPlayerAnswer() const override;
	void onExposure(QueryPtr topQuery) override;
	void setReply(std::optional<int32_t> reply) override;
	void onRemoval(PlayerColor color) override;
private:
	std::function<void(std::optional<int32_t>)> callback;
	std::optional<int32_t> reply;
};

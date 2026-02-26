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

VCMI_LIB_NAMESPACE_BEGIN

struct CPackForServer;
class CGObjectInstance;
class CGHeroInstance;

VCMI_LIB_NAMESPACE_END

class CObjectVisitQuery;
class QueriesProcessor;
class CQuery;
class CGameHandler;

using QueryPtr = std::shared_ptr<CQuery>;

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
	std::vector<PlayerColor> players; //players that are affected (often "blocked") by query
	QueryID queryID;

	CQuery(CGameHandler * gh);

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

	virtual ~CQuery();
protected:
	QueriesProcessor * owner;
	CGameHandler * gh;
	void addPlayer(PlayerColor color);
	bool blockAllButReply(const CPackForServer * pack) const;
};

std::ostream &operator<<(std::ostream &out, const CQuery &query);
std::ostream &operator<<(std::ostream &out, QueryPtr query);

class CDialogQuery : public CQuery
{
public:
	explicit CDialogQuery(CGameHandler * owner);
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

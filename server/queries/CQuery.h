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

#include "../../lib/GameConstants.h"
#include "../../lib/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CPack;

VCMI_LIB_NAMESPACE_END

class CObjectVisitQuery;
class QueriesProcessor;
class CQuery;

using QueryPtr = std::shared_ptr<CQuery>;

// This class represents any kind of prolonged interaction that may need to do something special after it is over.
// It does not necessarily has to be "query" requiring player action, it can be also used internally within server.
// Examples:
// - all kinds of blocking dialog windows
// - battle
// - object visit
// - hero movement
// Queries can cause another queries, forming a stack of queries for each player. Eg: hero movement -> object visit -> dialog.
class CQuery
{
public:
	std::vector<PlayerColor> players; //players that are affected (often "blocked") by query
	QueryID queryID;

	CQuery(QueriesProcessor * Owner);


	virtual bool blocksPack(const CPack *pack) const; //query can block attempting actions by player. Eg. he can't move hero during the battle.

	virtual bool endsByPlayerAnswer() const; //query is removed after player gives answer (like dialogs)
	virtual void onAdding(PlayerColor color); //called just before query is pushed on stack
	virtual void onAdded(PlayerColor color); //called right after query is pushed on stack
	virtual void onRemoval(PlayerColor color); //called after query is removed from stack
	virtual void onExposure(QueryPtr topQuery);//called when query immediately above is removed and this is exposed (becomes top)
	virtual std::string toString() const;

	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const;

	virtual void setReply(const JsonNode & reply);

	virtual ~CQuery();
protected:
	QueriesProcessor * owner;
	void addPlayer(PlayerColor color);
	bool blockAllButReply(const CPack * pack) const;
};

std::ostream &operator<<(std::ostream &out, const CQuery &query);
std::ostream &operator<<(std::ostream &out, QueryPtr query);

class CGhQuery : public CQuery
{
public:
	CGhQuery(CGameHandler * owner);
protected:
	CGameHandler * gh;
};

class CDialogQuery : public CGhQuery
{
public:
	CDialogQuery(CGameHandler * owner);
	virtual bool endsByPlayerAnswer() const override;
	virtual bool blocksPack(const CPack *pack) const override;
	void setReply(const JsonNode & reply) override;
protected:
	std::optional<ui32> answer;
};

class CGenericQuery : public CQuery
{
public:
	CGenericQuery(QueriesProcessor * Owner, PlayerColor color, std::function<void(const JsonNode &)> Callback);

	bool blocksPack(const CPack * pack) const override;
	bool endsByPlayerAnswer() const override;
	void onExposure(QueryPtr topQuery) override;
	void setReply(const JsonNode & reply) override;
	void onRemoval(PlayerColor color) override;
private:
	std::function<void(const JsonNode &)> callback;
	JsonNode reply;
};

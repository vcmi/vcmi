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
#include "../lib/GameConstants.h"
#include "../lib/int3.h"
#include "../lib/NetPacks.h"
#include "JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CGHeroInstance;
class CArmedInstance;

VCMI_LIB_NAMESPACE_END

class CGameHandler;
class CObjectVisitQuery;
class CQuery;
class Queries;

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

	CQuery(Queries * Owner);


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
	Queries * owner;
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

//Created when hero visits object.
//Removed when query above is resolved (or immediately after visit if no queries were created)
class CObjectVisitQuery : public CGhQuery
{
public:
	const CGObjectInstance *visitedObject;
	const CGHeroInstance *visitingHero;
	int3 tile; //may be different than hero pos -> eg. visit via teleport
	bool removeObjectAfterVisit;

	CObjectVisitQuery(CGameHandler * owner, const CGObjectInstance *Obj, const CGHeroInstance *Hero, int3 Tile);

	virtual bool blocksPack(const CPack *pack) const override;
	virtual void onRemoval(PlayerColor color) override;
	virtual void onExposure(QueryPtr topQuery) override;
};

class CBattleQuery : public CGhQuery
{
public:
	std::array<const CArmedInstance *,2> belligerents;
	std::array<int, 2> initialHeroMana;

	const BattleInfo *bi;
	std::optional<BattleResult> result;

	CBattleQuery(CGameHandler * owner);
	CBattleQuery(CGameHandler * owner, const BattleInfo * Bi); //TODO
	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;
	virtual bool blocksPack(const CPack *pack) const override;
	virtual void onRemoval(PlayerColor color) override;
};

//Created when hero attempts move and something happens
//(not necessarily position change, could be just an object interaction).
class CHeroMovementQuery : public CGhQuery
{
public:
	TryMoveHero tmh;
	bool visitDestAfterVictory; //if hero moved to guarded tile and it should be visited once guard is defeated
	const CGHeroInstance *hero;

	virtual void onExposure(QueryPtr topQuery) override;

	CHeroMovementQuery(CGameHandler * owner, const TryMoveHero & Tmh, const CGHeroInstance * Hero, bool VisitDestAfterVictory = false);
	virtual void onAdding(PlayerColor color) override;
	virtual void onRemoval(PlayerColor color) override;
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

class CGarrisonDialogQuery : public CDialogQuery //used also for hero exchange dialogs
{
public:
	std::array<const CArmedInstance *,2> exchangingArmies;

	CGarrisonDialogQuery(CGameHandler * owner, const CArmedInstance *up, const CArmedInstance *down);
	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;
	virtual bool blocksPack(const CPack *pack) const override;
};

class CBattleDialogQuery : public CDialogQuery
{
public:
	CBattleDialogQuery(CGameHandler * owner, const BattleInfo * Bi);

	const BattleInfo * bi;

	virtual void onRemoval(PlayerColor color) override;
};

//yes/no and component selection dialogs
class CBlockingDialogQuery : public CDialogQuery
{
public:
	BlockingDialog bd; //copy of pack... debug purposes

	CBlockingDialogQuery(CGameHandler * owner, const BlockingDialog &bd);

	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;
};

class CTeleportDialogQuery : public CDialogQuery
{
public:
	TeleportDialog td; //copy of pack... debug purposes

	CTeleportDialogQuery(CGameHandler * owner, const TeleportDialog &td);

	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;
};

class CHeroLevelUpDialogQuery : public CDialogQuery
{
public:
	CHeroLevelUpDialogQuery(CGameHandler * owner, const HeroLevelUp &Hlu, const CGHeroInstance * Hero);

	virtual void onRemoval(PlayerColor color) override;
	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;

	HeroLevelUp hlu;
	const CGHeroInstance * hero;
};

class CCommanderLevelUpDialogQuery : public CDialogQuery
{
public:
	CCommanderLevelUpDialogQuery(CGameHandler * owner, const CommanderLevelUp &Clu, const CGHeroInstance * Hero);

	virtual void onRemoval(PlayerColor color) override;
	virtual void notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const override;

	CommanderLevelUp clu;
	const CGHeroInstance * hero;
};

class CGenericQuery : public CQuery
{
public:
	CGenericQuery(Queries * Owner, PlayerColor color, std::function<void(const JsonNode &)> Callback);

	bool blocksPack(const CPack * pack) const override;
	bool endsByPlayerAnswer() const override;
	void onExposure(QueryPtr topQuery) override;
	void setReply(const JsonNode & reply) override;
	void onRemoval(PlayerColor color) override;
private:
	std::function<void(const JsonNode &)> callback;
	JsonNode reply;
};

class Queries
{
private:
	void addQuery(PlayerColor player, QueryPtr query);
	void popQuery(PlayerColor player, QueryPtr query);

	std::map<PlayerColor, std::vector<QueryPtr>> queries; //player => stack of queries

public:
	static boost::mutex mx;

	void addQuery(QueryPtr query);
	void popQuery(const CQuery &query);
	void popQuery(QueryPtr query);
	void popIfTop(const CQuery &query); //removes this query if it is at the top (otherwise, do nothing)
	void popIfTop(QueryPtr query); //removes this query if it is at the top (otherwise, do nothing)

	QueryPtr topQuery(PlayerColor player);

	std::vector<std::shared_ptr<const CQuery>> allQueries() const;
	std::vector<QueryPtr> allQueries();
	QueryPtr getQuery(QueryID queryID);
	//void removeQuery
};

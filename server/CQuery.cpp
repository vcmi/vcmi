/*
 * CQuery.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CQuery.h"
#include "CGameHandler.h"
#include "../lib/battle/BattleInfo.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/serializer/Cast.h"

boost::mutex Queries::mx;

template <typename Container>
std::string formatContainer(const Container & c, std::string delimeter = ", ", std::string opener = "(", std::string closer=")")
{
	std::string ret = opener;
	auto itr = std::begin(c);
	if(itr != std::end(c))
	{
		ret += boost::lexical_cast<std::string>(*itr);
		while(++itr != std::end(c))
		{
			ret += delimeter;
			ret += boost::lexical_cast<std::string>(*itr);
		}
	}
	ret += closer;
	return ret;
}

std::ostream & operator<<(std::ostream & out, const CQuery & query)
{
	return out << query.toString();
}

std::ostream & operator<<(std::ostream & out, QueryPtr query)
{
	return out << "[" << query.get() << "] " << query->toString();
}

CQuery::CQuery(Queries * Owner):
	owner(Owner)
{
	boost::unique_lock<boost::mutex> l(Queries::mx);

	static QueryID QID = QueryID(0);

	queryID = ++QID;
	logGlobal->trace("Created a new query with id %d", queryID);
}


CQuery::~CQuery()
{
	logGlobal->trace("Destructed the query with id %d", queryID);
}

void CQuery::addPlayer(PlayerColor color)
{
	if(color.isValidPlayer())
		players.push_back(color);
}

std::string CQuery::toString() const
{
	const auto size = players.size();
	const std::string plural = size > 1 ? "s" : "";
	std::string names;

	for(size_t i = 0; i < size; i++)
	{
		names += boost::to_upper_copy<std::string>(players[i].getStr());

		if(i < size - 2)
			names += ", ";
		else if(size > 1 && i == size - 2)
			names += " and ";
	}
	std::string ret = boost::str(boost::format("A query of type '%s' and qid = %d affecting player%s %s")
		% typeid(*this).name()
		% queryID 
		% plural
		% names
	);
	return ret;
}

bool CQuery::endsByPlayerAnswer() const
{
	return false;
}

void CQuery::onRemoval(PlayerColor color)
{

}

bool CQuery::blocksPack(const CPack * pack) const
{
	return false;
}

void CQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{

}

void CQuery::onExposure(QueryPtr topQuery)
{
	logGlobal->trace("Exposed query with id %d", queryID);
	owner->popQuery(*this);
}

void CQuery::onAdding(PlayerColor color)
{

}

void CQuery::onAdded(PlayerColor color)
{

}

void CQuery::setReply(const JsonNode & reply)
{

}

bool CQuery::blockAllButReply(const CPack * pack) const
{
	//We accept only query replies from correct player
	if(auto reply = dynamic_ptr_cast<QueryReply>(pack))
		return !vstd::contains(players, reply->player);

	return true;
}

CGhQuery::CGhQuery(CGameHandler * owner):
	CQuery(&owner->queries), gh(owner)
{

}

CObjectVisitQuery::CObjectVisitQuery(CGameHandler * owner, const CGObjectInstance * Obj, const CGHeroInstance * Hero, int3 Tile):
	CGhQuery(owner), visitedObject(Obj), visitingHero(Hero), tile(Tile), removeObjectAfterVisit(false)
{
	addPlayer(Hero->tempOwner);
}

bool CObjectVisitQuery::blocksPack(const CPack *pack) const
{
	//During the visit itself ALL actions are blocked.
	//(However, the visit may trigger a query above that'll pass some.)
	return true;
}

void CObjectVisitQuery::onRemoval(PlayerColor color)
{
	gh->objectVisitEnded(*this);

	//TODO or should it be destructor?
	//Can object visit affect 2 players and what would be desired behavior?
	if(removeObjectAfterVisit)
		gh->removeObject(visitedObject);
}

void CObjectVisitQuery::onExposure(QueryPtr topQuery)
{
	//Object may have been removed and deleted.
	if(gh->isValidObject(visitedObject))
		topQuery->notifyObjectAboutRemoval(*this);

	owner->popQuery(*this);
}

void Queries::popQuery(PlayerColor player, QueryPtr query)
{
	//LOG_TRACE_PARAMS(logGlobal, "player='%s', query='%s'", player % query);
	if(topQuery(player) != query)
	{
		logGlobal->trace("Cannot remove, not a top!");
		return;
	}

	queries[player] -= query;
	auto nextQuery = topQuery(player);

	query->onRemoval(player);

	//Exposure on query below happens only if removal didn't trigger any new query
	if(nextQuery && nextQuery == topQuery(player))
		nextQuery->onExposure(query);
}

void Queries::popQuery(const CQuery &query)
{
	//LOG_TRACE_PARAMS(logGlobal, "query='%s'", query);

	assert(query.players.size());
	for(auto player : query.players)
	{
		auto top = topQuery(player);
		if(top.get() == &query)
			popQuery(top);
		else
			logGlobal->trace("Cannot remove query %s", query.toString());
	}
}

void Queries::popQuery(QueryPtr query)
{
	for(auto player : query->players)
		popQuery(player, query);
}

void Queries::addQuery(QueryPtr query)
{
	for(auto player : query->players)
		addQuery(player, query);

	for(auto player : query->players)
		query->onAdded(player);
}

void Queries::addQuery(PlayerColor player, QueryPtr query)
{
	//LOG_TRACE_PARAMS(logGlobal, "player='%d', query='%s'", player.getNum() % query);
	query->onAdding(player);
	queries[player].push_back(query);
}

QueryPtr Queries::topQuery(PlayerColor player)
{
	return vstd::backOrNull(queries[player]);
}

void Queries::popIfTop(QueryPtr query)
{
	//LOG_TRACE_PARAMS(logGlobal, "query='%d'", query);
	if(!query)
		logGlobal->error("The query is nullptr! Ignoring.");

	popIfTop(*query);
}

void Queries::popIfTop(const CQuery & query)
{
	for(PlayerColor color : query.players)
		if(topQuery(color).get() == &query)
			popQuery(color, topQuery(color));
}

std::vector<std::shared_ptr<const CQuery>> Queries::allQueries() const
{
	std::vector<std::shared_ptr<const CQuery>> ret;
	for(auto & playerQueries : queries)
		for(auto & query : playerQueries.second)
			ret.push_back(query);

	return ret;
}

std::vector<QueryPtr> Queries::allQueries()
{
	//TODO code duplication with const function :(
	std::vector<QueryPtr> ret;
	for(auto & playerQueries : queries)
		for(auto & query : playerQueries.second)
			ret.push_back(query);

	return ret;
}

QueryPtr Queries::getQuery(QueryID queryID)
{
	for(auto & playerQueries : queries)
		for(auto & query : playerQueries.second)
			if(query->queryID == queryID)
				return query;
	return nullptr;
}

void CBattleQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	assert(result);
	objectVisit.visitedObject->battleFinished(objectVisit.visitingHero, *result);
}

CBattleQuery::CBattleQuery(CGameHandler * owner, const BattleInfo * Bi):
	CGhQuery(owner)
{
	belligerents[0] = Bi->sides[0].armyObject;
	belligerents[1] = Bi->sides[1].armyObject;

	bi = Bi;

	for(auto & side : bi->sides)
		addPlayer(side.color);
}

CBattleQuery::CBattleQuery(CGameHandler * owner):
	CGhQuery(owner), bi(nullptr)
{
	belligerents[0] = belligerents[1] = nullptr;
}

bool CBattleQuery::blocksPack(const CPack * pack) const
{
	const char * name = typeid(*pack).name();
	return strcmp(name, typeid(MakeAction).name()) && strcmp(name, typeid(MakeCustomAction).name());
}

void CBattleQuery::onRemoval(PlayerColor color)
{
	gh->battleAfterLevelUp(*result);
}

void CGarrisonDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	objectVisit.visitedObject->garrisonDialogClosed(objectVisit.visitingHero);
}

CGarrisonDialogQuery::CGarrisonDialogQuery(CGameHandler * owner, const CArmedInstance * up, const CArmedInstance * down):
	CDialogQuery(owner)
{
	exchangingArmies[0] = up;
	exchangingArmies[1] = down;

	addPlayer(up->tempOwner);
	addPlayer(down->tempOwner);
}

bool CGarrisonDialogQuery::blocksPack(const CPack * pack) const
{
	std::set<ObjectInstanceID> ourIds;
	ourIds.insert(this->exchangingArmies[0]->id);
	ourIds.insert(this->exchangingArmies[1]->id);

	if(auto stacks = dynamic_ptr_cast<ArrangeStacks>(pack))
		return !vstd::contains(ourIds, stacks->id1) || !vstd::contains(ourIds, stacks->id2);

	if(auto stacks = dynamic_ptr_cast<BulkSplitStack>(pack))
		return !vstd::contains(ourIds, stacks->srcOwner);

	if(auto stacks = dynamic_ptr_cast<BulkMergeStacks>(pack))
		return !vstd::contains(ourIds, stacks->srcOwner);

	if(auto stacks = dynamic_ptr_cast<BulkSmartSplitStack>(pack))
		return !vstd::contains(ourIds, stacks->srcOwner);

	if(auto stacks = dynamic_ptr_cast<BulkMoveArmy>(pack))
		return !vstd::contains(ourIds, stacks->srcArmy) || !vstd::contains(ourIds, stacks->destArmy);

	if(auto arts = dynamic_ptr_cast<ExchangeArtifacts>(pack))
	{
		if(auto id1 = boost::apply_visitor(GetEngagedHeroIds(), arts->src.artHolder))
			if(!vstd::contains(ourIds, *id1))
				return true;

		if(auto id2 = boost::apply_visitor(GetEngagedHeroIds(), arts->dst.artHolder))
			if(!vstd::contains(ourIds, *id2))
				return true;
		return false;
	}
	if(auto dismiss = dynamic_ptr_cast<DisbandCreature>(pack))
		return !vstd::contains(ourIds, dismiss->id);
	
	if(auto arts = dynamic_ptr_cast<BulkExchangeArtifacts>(pack))
		return !vstd::contains(ourIds, arts->srcHero) || !vstd::contains(ourIds, arts->dstHero);

	if(auto dismiss = dynamic_ptr_cast<AssembleArtifacts>(pack))
		return !vstd::contains(ourIds, dismiss->heroID);

	if(auto upgrade = dynamic_ptr_cast<UpgradeCreature>(pack))
		return !vstd::contains(ourIds, upgrade->id);

	return CDialogQuery::blocksPack(pack);
}

void CBlockingDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	assert(answer);
	objectVisit.visitedObject->blockingDialogAnswered(objectVisit.visitingHero, *answer);
}

CBlockingDialogQuery::CBlockingDialogQuery(CGameHandler * owner, const BlockingDialog & bd):
	CDialogQuery(owner)
{
	this->bd = bd;
	addPlayer(bd.player);
}

void CTeleportDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	// do not change to dynamic_ptr_cast - SIGSEGV!
	auto obj = dynamic_cast<const CGTeleport*>(objectVisit.visitedObject);
	if(obj)
		obj->teleportDialogAnswered(objectVisit.visitingHero, *answer, td.exits);
	else
		logGlobal->error("Invalid instance in teleport query");
}

CTeleportDialogQuery::CTeleportDialogQuery(CGameHandler * owner, const TeleportDialog & td):
	CDialogQuery(owner)
{
	this->td = td;
	addPlayer(td.player);
}

CHeroLevelUpDialogQuery::CHeroLevelUpDialogQuery(CGameHandler * owner, const HeroLevelUp & Hlu, const CGHeroInstance * Hero):
	CDialogQuery(owner), hero(Hero)
{
	hlu = Hlu;
	addPlayer(hero->tempOwner);
}

void CHeroLevelUpDialogQuery::onRemoval(PlayerColor color)
{
	assert(answer);
	logGlobal->trace("Completing hero level-up query. %s gains skill %d", hero->getObjectName(), answer.get());
	gh->levelUpHero(hero, hlu.skills[*answer]);
}

void CHeroLevelUpDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	objectVisit.visitedObject->heroLevelUpDone(objectVisit.visitingHero);
}

CCommanderLevelUpDialogQuery::CCommanderLevelUpDialogQuery(CGameHandler * owner, const CommanderLevelUp & Clu, const CGHeroInstance * Hero):
	CDialogQuery(owner), hero(Hero)
{
	clu = Clu;
	addPlayer(hero->tempOwner);
}

void CCommanderLevelUpDialogQuery::onRemoval(PlayerColor color)
{
	assert(answer);
	logGlobal->trace("Completing commander level-up query. Commander of hero %s gains skill %s", hero->getObjectName(), answer.get());
	gh->levelUpCommander(hero->commander, clu.skills[*answer]);
}

void CCommanderLevelUpDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	objectVisit.visitedObject->heroLevelUpDone(objectVisit.visitingHero);
}

CDialogQuery::CDialogQuery(CGameHandler * owner):
	CGhQuery(owner)
{

}

bool CDialogQuery::endsByPlayerAnswer() const
{
	return true;
}

bool CDialogQuery::blocksPack(const CPack * pack) const
{
	return blockAllButReply(pack);
}

void CDialogQuery::setReply(const JsonNode & reply)
{
	if(reply.getType() == JsonNode::JsonType::DATA_INTEGER)
		answer = reply.Integer();
}

CHeroMovementQuery::CHeroMovementQuery(CGameHandler * owner, const TryMoveHero & Tmh, const CGHeroInstance * Hero, bool VisitDestAfterVictory):
	CGhQuery(owner), tmh(Tmh), visitDestAfterVictory(VisitDestAfterVictory), hero(Hero)
{
	players.push_back(hero->tempOwner);
}

void CHeroMovementQuery::onExposure(QueryPtr topQuery)
{
	assert(players.size() == 1);

	if(visitDestAfterVictory && hero->tempOwner == players[0]) //hero still alive, so he won with the guard
		//TODO what if there were H4-like escape? we should also check pos
	{
		logGlobal->trace("Hero %s after victory over guard finishes visit to %s", hero->name, tmh.end.toString());
		//finish movement
		visitDestAfterVictory = false;
		gh->visitObjectOnTile(*gh->getTile(tmh.end - hero->getVisitableOffset()), hero);
	}

	owner->popIfTop(*this);
}

void CHeroMovementQuery::onRemoval(PlayerColor color)
{
	PlayerBlocked pb;
	pb.player = color;
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_ENDED;
	gh->sendAndApply(&pb);
}

void CHeroMovementQuery::onAdding(PlayerColor color)
{
	PlayerBlocked pb;
	pb.player = color;
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_STARTED;
	gh->sendAndApply(&pb);
}

CGenericQuery::CGenericQuery(Queries * Owner, PlayerColor color, std::function<void(const JsonNode &)> Callback):
	CQuery(Owner), callback(Callback)
{
	addPlayer(color);
}

bool CGenericQuery::blocksPack(const CPack * pack) const
{
	return blockAllButReply(pack);
}

bool CGenericQuery::endsByPlayerAnswer() const
{
	return true;
}

void CGenericQuery::onExposure(QueryPtr topQuery)
{
	//do nothing
}

void CGenericQuery::setReply(const JsonNode & reply)
{
	callback(reply);
}

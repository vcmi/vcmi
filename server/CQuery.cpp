#include "StdInc.h"
#include "CQuery.h"
#include "CGameHandler.h"
#include "../lib/BattleState.h"
#include "../lib/mapObjects/MiscObjects.h"

boost::mutex Queries::mx;

template <typename Container>
std::string formatContainer(const Container &c, std::string delimeter=", ", std::string opener="(", std::string closer=")")
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

std::ostream & operator<<(std::ostream &out, const CQuery &query)
{
	return out << query.toString();
}

std::ostream & operator<<(std::ostream &out, QueryPtr query)
{
	return out << "[" << query.get() << "] " << query->toString();
}

CQuery::CQuery(void)
{
	boost::unique_lock<boost::mutex> l(Queries::mx);

	static QueryID QID = QueryID(0);

	queryID = ++QID;
	logGlobal->traceStream() << "Created a new query with id " << queryID;
}


CQuery::~CQuery(void)
{
	logGlobal->traceStream() << "Destructed the query with id " << queryID;
}

void CQuery::addPlayer(PlayerColor color)
{
	if(color.isValidPlayer())
	{
		players.push_back(color);
	}
}

std::string CQuery::toString() const
{
	std::string ret = boost::str(boost::format("A query of type %s and qid=%d affecting players %s") % typeid(*this).name() % queryID % formatContainer(players));
	return ret;
}

bool CQuery::endsByPlayerAnswer() const
{
	return false;
}

void CQuery::onRemoval(CGameHandler *gh, PlayerColor color)
{
}

bool CQuery::blocksPack(const CPack *pack) const
{
	return false;
}

void CQuery::notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const
{
}

void CQuery::onExposure(CGameHandler *gh, QueryPtr topQuery)
{
	gh->queries.popQuery(*this);
}

void CQuery::onAdding(CGameHandler *gh, PlayerColor color)
{

}

CObjectVisitQuery::CObjectVisitQuery(const CGObjectInstance *Obj, const CGHeroInstance *Hero, int3 Tile)
	: visitedObject(Obj), visitingHero(Hero), tile(Tile), removeObjectAfterVisit(false)
{
	addPlayer(Hero->tempOwner);
}

bool CObjectVisitQuery::blocksPack(const CPack *pack) const
{
	//During the visit itself ALL actions are blocked.
	//(However, the visit may trigger a query above that'll pass some.)
	return true;
}

void CObjectVisitQuery::onRemoval(CGameHandler *gh, PlayerColor color)
{
	gh->objectVisitEnded(*this);

	//TODO or should it be destructor?
	//Can object visit affect 2 players and what would be desired behavior?
	if(removeObjectAfterVisit)
		gh->removeObject(visitedObject);
}

void CObjectVisitQuery::onExposure(CGameHandler *gh, QueryPtr topQuery)
{
	//Object may have been removed and deleted.
	if(gh->isValidObject(visitedObject))
		topQuery->notifyObjectAboutRemoval(*this);

	gh->queries.popQuery(*this);
}

void Queries::popQuery(PlayerColor player, QueryPtr query)
{
	LOG_TRACE_PARAMS(logGlobal, "player='%s', query='%s'", player % query);
	if(topQuery(player) != query)
	{
		logGlobal->traceStream() << "Cannot remove, not a top!";
		return;
	}

	queries[player] -= query;
	auto nextQuery = topQuery(player);

	query->onRemoval(gh, player);

	//Exposure on query below happens only if removal didn't trigger any new query
	if(nextQuery && nextQuery == topQuery(player))
	{
		nextQuery->onExposure(gh, query);
	}
}

void Queries::popQuery(const CQuery &query)
{
	LOG_TRACE_PARAMS(logGlobal, "query='%s'", query);

	assert(query.players.size());
	for(auto player : query.players)
	{
		auto top = topQuery(player);
		if(top.get() == &query)
			popQuery(top);
		else
			logGlobal->traceStream() << "Cannot remove query " << query;
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
}

void Queries::addQuery(PlayerColor player, QueryPtr query)
{
	LOG_TRACE_PARAMS(logGlobal, "player='%s', query='%s'", player % query);
	query->onAdding(gh, player);
	queries[player].push_back(query);
}

QueryPtr Queries::topQuery(PlayerColor player)
{
	return vstd::backOrNull(queries[player]);
}

void Queries::popIfTop(QueryPtr query)
{
	LOG_TRACE_PARAMS(logGlobal, "query='%d'", query);
	if(!query)
		logGlobal->errorStream() << "The query is nullptr! Ignoring.";

	popIfTop(*query);
}

void Queries::popIfTop(const CQuery &query)
{
	for(PlayerColor color : query.players)
		if(topQuery(color).get() == &query)
			popQuery(color, topQuery(color));
}

std::vector<std::shared_ptr<const CQuery>> Queries::allQueries() const
{
	std::vector<std::shared_ptr<const CQuery>> ret;
	for(auto &playerQueries : queries)
		for(auto &query : playerQueries.second)
			ret.push_back(query);

	return ret;
}

std::vector<std::shared_ptr<CQuery>> Queries::allQueries()
{
	//TODO code duplication with const function :(
	std::vector<std::shared_ptr<CQuery>> ret;
	for(auto &playerQueries : queries)
		for(auto &query : playerQueries.second)
		ret.push_back(query);

	return ret;
}

void CBattleQuery::notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const
{
	assert(result);
	objectVisit.visitedObject->battleFinished(objectVisit.visitingHero, *result);
}

CBattleQuery::CBattleQuery(const BattleInfo *Bi)
{
	belligerents[0] = Bi->sides[0].armyObject;
	belligerents[1] = Bi->sides[1].armyObject;

	bi = Bi;

	for(auto &side : bi->sides)
		addPlayer(side.color);
}

CBattleQuery::CBattleQuery()
{

}

bool CBattleQuery::blocksPack(const CPack *pack) const
{
	const char * name = typeid(*pack).name();
	return strcmp(name, typeid(MakeAction).name()) && strcmp(name, typeid(MakeCustomAction).name());
}

void CBattleQuery::onRemoval(CGameHandler *gh, PlayerColor color)
{
	gh->battleAfterLevelUp(*result);
}

void CGarrisonDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const
{
	objectVisit.visitedObject->garrisonDialogClosed(objectVisit.visitingHero);
}

CGarrisonDialogQuery::CGarrisonDialogQuery(const CArmedInstance *up, const CArmedInstance *down)
{
	exchangingArmies[0] = up;
	exchangingArmies[1] = down;

	addPlayer(up->tempOwner);
	addPlayer(down->tempOwner);
}

bool CGarrisonDialogQuery::blocksPack(const CPack *pack) const
{
	std::set<ObjectInstanceID> ourIds;
	ourIds.insert(this->exchangingArmies[0]->id);
	ourIds.insert(this->exchangingArmies[1]->id);

	if (auto stacks = dynamic_ptr_cast<ArrangeStacks>(pack))
	{
		return !vstd::contains(ourIds, stacks->id1) || !vstd::contains(ourIds, stacks->id2);
	}

	if (auto arts = dynamic_ptr_cast<ExchangeArtifacts>(pack))
	{
		if(auto id1 = boost::apply_visitor(GetEngagedHeroIds(), arts->src.artHolder))
			if(!vstd::contains(ourIds, *id1))
				return true;

		if(auto id2 = boost::apply_visitor(GetEngagedHeroIds(), arts->dst.artHolder))
			if(!vstd::contains(ourIds, *id2))
				return true;
		return false;
	}
	if (auto dismiss = dynamic_ptr_cast<DisbandCreature>(pack))
	{
		return !vstd::contains(ourIds, dismiss->id);
	}

	if (auto dismiss = dynamic_ptr_cast<AssembleArtifacts>(pack))
	{
		return !vstd::contains(ourIds, dismiss->heroID);
	}

	if(auto upgrade = dynamic_ptr_cast<UpgradeCreature>(pack))
	{
		return !vstd::contains(ourIds, upgrade->id);
	}
	return CDialogQuery::blocksPack(pack);
}

void CBlockingDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const
{
	assert(answer);
	objectVisit.visitedObject->blockingDialogAnswered(objectVisit.visitingHero, *answer);
}

CBlockingDialogQuery::CBlockingDialogQuery(const BlockingDialog &bd)
{
	this->bd = bd;
	addPlayer(bd.player);
}

void CTeleportDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const
{
	// do not change to dynamic_ptr_cast - SIGSEGV!
	auto obj = dynamic_cast<const CGTeleport*>(objectVisit.visitedObject);
	obj->teleportDialogAnswered(objectVisit.visitingHero, *answer, td.exits);
}

CTeleportDialogQuery::CTeleportDialogQuery(const TeleportDialog &td)
{
	this->td = td;
	addPlayer(td.hero->tempOwner);
}

CHeroLevelUpDialogQuery::CHeroLevelUpDialogQuery(const HeroLevelUp &Hlu)
{
	hlu = Hlu;
	addPlayer(hlu.hero->tempOwner);
}

void CHeroLevelUpDialogQuery::onRemoval(CGameHandler *gh, PlayerColor color)
{
	assert(answer);
	logGlobal->traceStream() << "Completing hero level-up query. " << hlu.hero->getObjectName() << " gains skill " << *answer;
	gh->levelUpHero(hlu.hero, hlu.skills[*answer]);
}

void CHeroLevelUpDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const
{
	objectVisit.visitedObject->heroLevelUpDone(objectVisit.visitingHero);
}

CCommanderLevelUpDialogQuery::CCommanderLevelUpDialogQuery(const CommanderLevelUp &Clu)
{
	clu = Clu;
	addPlayer(clu.hero->tempOwner);
}

void CCommanderLevelUpDialogQuery::onRemoval(CGameHandler *gh, PlayerColor color)
{
	assert(answer);
	logGlobal->traceStream() << "Completing commander level-up query. Commander of hero " << clu.hero->getObjectName() << " gains skill " << *answer;
	gh->levelUpCommander(clu.hero->commander, clu.skills[*answer]);
}

void CCommanderLevelUpDialogQuery::notifyObjectAboutRemoval(const CObjectVisitQuery &objectVisit) const
{
	objectVisit.visitedObject->heroLevelUpDone(objectVisit.visitingHero);
}

bool CDialogQuery::endsByPlayerAnswer() const
{
	return true;
}

bool CDialogQuery::blocksPack(const CPack *pack) const
{
	//We accept only query replies from correct player
	if(auto reply = dynamic_ptr_cast<QueryReply>(pack))
	{
		return !vstd::contains(players, reply->player);
	}

	return true;
}

CHeroMovementQuery::CHeroMovementQuery(const TryMoveHero &Tmh, const CGHeroInstance *Hero, bool VisitDestAfterVictory)
	: tmh(Tmh), visitDestAfterVictory(VisitDestAfterVictory), hero(Hero)
{
	players.push_back(hero->tempOwner);
}

void CHeroMovementQuery::onExposure(CGameHandler *gh, QueryPtr topQuery)
{
	assert(players.size() == 1);

	if(visitDestAfterVictory && hero->tempOwner == players[0]) //hero still alive, so he won with the guard
		//TODO what if there were H4-like escape? we should also check pos
	{
		logGlobal->traceStream() << "Hero " << hero->name << " after victory over guard finishes visit to " << tmh.end;
		//finish movement
		visitDestAfterVictory = false;
		gh->visitObjectOnTile(*gh->getTile(CGHeroInstance::convertPosition(tmh.end, false)), hero);
	}

	gh->queries.popIfTop(*this);
}

void CHeroMovementQuery::onRemoval(CGameHandler *gh, PlayerColor color)
{
	PlayerBlocked pb;
	pb.player = color;
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_ENDED;
	gh->sendAndApply(&pb);
}

void CHeroMovementQuery::onAdding(CGameHandler *gh, PlayerColor color)
{
	PlayerBlocked pb;
	pb.player = color;
	pb.reason = PlayerBlocked::ONGOING_MOVEMENT;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_STARTED;
	gh->sendAndApply(&pb);
}

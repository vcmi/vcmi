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

#include "QueriesProcessor.h"

#include "../CGameHandler.h"

#include "../../lib/networkPacks/PacksForServer.h"

std::ostream & operator<<(std::ostream & out, const CQuery & query)
{
	return out << query.toString();
}

std::ostream & operator<<(std::ostream & out, QueryPtr query)
{
	return out << "[" << query.get() << "] " << query->toString();
}

CQuery::CQuery(CGameHandler * gameHandler)
	: owner(gameHandler->queries.get())
	, gh(gameHandler)
{
	queryID = ++gameHandler->QID;
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
		names += boost::to_upper_copy<std::string>(players[i].toString());

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

bool CQuery::blocksPack(const CPackForServer * pack) const
{
	return false;
}

void CQuery::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const
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

void CQuery::setReply(std::optional<int32_t> reply)
{

}

bool CQuery::blockAllButReply(const CPackForServer * pack) const
{
	//We accept only query replies from correct player
	if(auto reply = dynamic_cast<const QueryReply*>(pack))
		return !vstd::contains(players, reply->player);

	return true;
}

CDialogQuery::CDialogQuery(CGameHandler * owner):
	CQuery(owner)
{

}

bool CDialogQuery::endsByPlayerAnswer() const
{
	return true;
}

bool CDialogQuery::blocksPack(const CPackForServer * pack) const
{
	return blockAllButReply(pack);
}

void CDialogQuery::setReply(std::optional<int32_t> reply)
{
	if(reply.has_value())
		answer = *reply;
}

CGenericQuery::CGenericQuery(CGameHandler * gh, PlayerColor color, const std::function<void(std::optional<int32_t>)> & callback):
	CQuery(gh), callback(callback)
{
	addPlayer(color);
}

bool CGenericQuery::blocksPack(const CPackForServer * pack) const
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

void CGenericQuery::setReply(std::optional<int32_t> receivedReply)
{
	reply = receivedReply;
}

void CGenericQuery::onRemoval(PlayerColor color)
{
	callback(reply);
}

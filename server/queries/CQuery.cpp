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

#include "../../lib/serializer/Cast.h"
#include "../../lib/NetPacks.h"

template <typename Container>
std::string formatContainer(const Container & c, std::string delimeter = ", ", std::string opener = "(", std::string closer=")")
{
	std::string ret = opener;
	auto itr = std::begin(c);
	if(itr != std::end(c))
	{
		ret += std::to_string(*itr);
		while(++itr != std::end(c))
		{
			ret += delimeter;
			ret += std::to_string(*itr);
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

CQuery::CQuery(QueriesProcessor * Owner):
	owner(Owner)
{
	boost::unique_lock<boost::mutex> l(QueriesProcessor::mx);

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
	CQuery(owner->queries.get()), gh(owner)
{

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

CGenericQuery::CGenericQuery(QueriesProcessor * Owner, PlayerColor color, std::function<void(const JsonNode &)> Callback):
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
	this->reply = reply;
}

void CGenericQuery::onRemoval(PlayerColor color)
{
	callback(reply);
}

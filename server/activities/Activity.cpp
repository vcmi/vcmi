/*
 * Activity.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "Activity.h"

#include "ActivityProcessor.h"

#include "../CGameHandler.h"

#include "../../lib/networkPacks/PacksForServer.h"

std::string Activity::typeName() const
{
	switch(type)
	{
		case ActivityType::BlockingDialog:         return "BlockingDialog";
		case ActivityType::GarrisonDialog:         return "GarrisonDialog";
		case ActivityType::TeleportDialog:         return "TeleportDialog";
		case ActivityType::HeroLevelUp:            return "HeroLevelUp";
		case ActivityType::HeroLevelUpDialog:      return "HeroLevelUpDialog";
		case ActivityType::CommanderLevelUpDialog: return "CommanderLevelUpDialog";
		case ActivityType::OpenWindow:             return "OpenWindow";
		case ActivityType::MapObjectVisit:         return "MapObjectVisit";
		case ActivityType::TownBuildingVisit:      return "TownBuildingVisit";
		case ActivityType::TurnStart:              return "TurnStart";
		case ActivityType::Battle:                 return "Battle";
		case ActivityType::BattleDialog:           return "BattleDialog";
		case ActivityType::HeroMovement:           return "HeroMovement";
		case ActivityType::TimerPause:             return "TimerPause";
		case ActivityType::TownSelection:          return "TownSelection";
		case ActivityType::ScriptDialog:           return "ScriptDialog";
		case ActivityType::LuaScript:              return "LuaScript";
		default:                                return "Unknown";
	}
}

Activity::Activity(CGameHandler * gameHandler, ActivityType type)
	: owner(gameHandler->activities.get())
	, gh(gameHandler)
	, type(type)
{
	traceNumber = ++gameHandler->activityTraceCounter;
	logGlobal->trace("Created a new activity #%d", traceNumber);
}

Activity::~Activity()
{
	logGlobal->trace("Destructed activity #%d", traceNumber);
}

void Activity::addPlayer(PlayerColor color)
{
	if(!color.isValidPlayer())
		throw std::runtime_error("Activity can not affect invalid player " + color.toString());

	// prevent duplicates
	if(vstd::contains(players, color))
		return;

	players.push_back(color);
}

std::string Activity::toString() const
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
	std::string ret = boost::str(boost::format("Activity #%d of type '%s' affecting player%s %s")
		% traceNumber
		% typeName()
		% plural
		% names
	);

	if(activeQuestionID.hasValue())
		ret += boost::str(boost::format(" [awaiting an answer to question %d]") % activeQuestionID);

	if(answeredBy)
		ret += boost::str(boost::format(" [answered by %s, awaiting exposure]") % answeredBy->toString());
	else if(finished)
		ret += " [finished, awaiting exposure]";

	return ret;
}

bool Activity::endsByPlayerAnswer() const
{
	return false;
}

QuestionID Activity::askQuestion()
{
	activeQuestionID = ++gh->questionCounter;
	return activeQuestionID;
}

void Activity::expectAnswerTo(QuestionID reserved)
{
	activeQuestionID = reserved;
}

bool Activity::acceptsAnswerWithoutValue() const
{
	return false;
}

bool Activity::acceptsAnswer(int32_t answer) const
{
	return true;
}

bool Activity::acceptsAnswerFrom(PlayerColor player) const
{
	return true;
}

void Activity::onRemoval()
{
}

void Activity::finish()
{
	owner->finishActivity(*this);
}

bool Activity::blocksPack(const CPackForServer * pack) const
{
	return false;
}

void Activity::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const
{

}

void Activity::onChildCompleted(const ActivityPtr & child)
{
}

void Activity::onAdded()
{
}

void Activity::setReply(std::optional<int32_t> reply)
{

}

bool Activity::blockAllButReply(const CPackForServer * pack) const
{
	//We accept only activity replies from correct player
	if(auto reply = dynamic_cast<const QuestionAnswer*>(pack))
		return !vstd::contains(players, reply->player);

	return true;
}

DialogActivity::DialogActivity(CGameHandler * owner, ActivityType type):
	Activity(owner, type)
{

}

bool DialogActivity::endsByPlayerAnswer() const
{
	return true;
}

bool DialogActivity::blocksPack(const CPackForServer * pack) const
{
	return blockAllButReply(pack);
}

void DialogActivity::setReply(std::optional<int32_t> reply)
{
	if(reply.has_value())
		answer = *reply;
}

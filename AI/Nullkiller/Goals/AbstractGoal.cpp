/*
* AbstractGoal.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "AbstractGoal.h"
#include "../VCAI.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"
#include "../../../lib/StringConstants.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

TSubgoal Goals::sptr(const AbstractGoal & tmp)
{
	TSubgoal ptr;
	ptr.reset(tmp.clone());
	return ptr;
}

std::string AbstractGoal::name() const //TODO: virtualize
{
	std::string desc;
	switch(goalType)
	{
	case INVALID:
		return "INVALID";
	case WIN:
		return "WIN";
	case CONQUER:
		return "CONQUER";
	case BUILD:
		return "BUILD";
	case EXPLORE:
		desc = "EXPLORE";
		break;
	case GATHER_ARMY:
		desc = "GATHER ARMY";
		break;
	case BUY_ARMY:
		return "BUY ARMY";
		break;
	case BOOST_HERO:
		desc = "BOOST_HERO (unsupported)";
		break;
	case RECRUIT_HERO:
		return "RECRUIT HERO";
	case BUILD_STRUCTURE:
		return "BUILD STRUCTURE";
	case COLLECT_RES:
		desc = "COLLECT RESOURCE " + GameConstants::RESOURCE_NAMES[resID] + " (" + boost::lexical_cast<std::string>(value) + ")";
		break;
	case TRADE:
	{
		auto obj = cb->getObjInstance(ObjectInstanceID(objid));
		if (obj)
			desc = (boost::format("TRADE %d of %s at %s") % value % GameConstants::RESOURCE_NAMES[resID] % obj->getObjectName()).str();
	}
	break;
	case GATHER_TROOPS:
		desc = "GATHER TROOPS";
		break;
	case VISIT_OBJ:
	{
		auto obj = cb->getObjInstance(ObjectInstanceID(objid));
		if(obj)
			desc = "VISIT OBJ " + obj->getObjectName();
	}
	break;
	case FIND_OBJ:
		desc = "FIND OBJ " + boost::lexical_cast<std::string>(objid);
		break;
	case VISIT_HERO:
	{
		auto obj = cb->getObjInstance(ObjectInstanceID(objid));
		if(obj)
			desc = "VISIT HERO " + obj->getObjectName();
	}
	break;
	case GET_ART_TYPE:
		desc = "GET ARTIFACT OF TYPE " + VLC->arth->artifacts[aid]->Name();
		break;
	case VISIT_TILE:
		desc = "VISIT TILE " + tile.toString();
		break;
	case CLEAR_WAY_TO:
		desc = "CLEAR WAY TO " + tile.toString();
		break;
	case DIG_AT_TILE:
		desc = "DIG AT TILE " + tile.toString();
		break;
	default:
		return boost::lexical_cast<std::string>(goalType);
	}
	if(hero.get(true)) //FIXME: used to crash when we lost hero and failed goal
		desc += " (" + hero->name + ")";
	return desc;
}

bool AbstractGoal::operator==(const AbstractGoal & g) const
{
	return false;
}

//TODO: find out why the following are not generated automatically on MVS?
bool TSubgoal::operator==(const TSubgoal & rhs) const
{
	return *get() == *rhs.get(); //comparison for Goals is overloaded, so they don't need to be identical to match
}

bool AbstractGoal::invalid() const
{
	return goalType == EGoals::INVALID;
}

void AbstractGoal::accept(VCAI * ai)
{
	ai->tryRealize(*this);
}

EvaluationContext::EvaluationContext()
	: movementCost(0.0),
	manaCost(0),
	danger(0),
	closestWayRatio(1),
	armyLoss(0),
	heroStrength(0),
	movementCostByRole(),
	skillReward(0),
	goldReward(0),
	goldCost(0),
	armyReward(0),
	armyLossPersentage(0),
	heroRole(HeroRole::SCOUT),
	turn(0),
	strategicalValue(0)
{
}
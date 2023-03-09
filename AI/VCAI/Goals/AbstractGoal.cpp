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
#include "../ResourceManager.h"
#include "../BuildingManager.h"
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
		desc = "COLLECT RESOURCE " + GameConstants::RESOURCE_NAMES[resID] + " (" + std::to_string(value) + ")";
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
		desc = "FIND OBJ " + std::to_string(objid);
		break;
	case VISIT_HERO:
	{
		auto obj = cb->getObjInstance(ObjectInstanceID(objid));
		if(obj)
			desc = "VISIT HERO " + obj->getObjectName();
	}
	break;
	case GET_ART_TYPE:
		desc = "GET ARTIFACT OF TYPE " + VLC->artifacts()->getByIndex(aid)->getNameTranslated();
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
		return std::to_string(goalType);
	}
	if(hero.get(true)) //FIXME: used to crash when we lost hero and failed goal
		desc += " (" + hero->getNameTranslated() + ")";
	return desc;
}

bool AbstractGoal::operator==(const AbstractGoal & g) const
{
	return false;
}

bool AbstractGoal::operator<(AbstractGoal & g) //for std::unique
{
	//TODO: make sure it gets goals consistent with == operator
	if (goalType < g.goalType)
		return true;
	if (goalType > g.goalType)
		return false;
	if (hero < g.hero)
		return true;
	if (hero > g.hero)
		return false;
	if (tile < g.tile)
		return true;
	if (g.tile < tile)
		return false;
	if (objid < g.objid)
		return true;
	if (objid > g.objid)
		return false;
	if (town < g.town)
		return true;
	if (town > g.town)
		return false;
	if (value < g.value)
		return true;
	if (value > g.value)
		return false;
	if (priority < g.priority)
		return true;
	if (priority > g.priority)
		return false;
	if (resID < g.resID)
		return true;
	if (resID > g.resID)
		return false;
	if (bid < g.bid)
		return true;
	if (bid > g.bid)
		return false;
	if (aid < g.aid)
		return true;
	if (aid > g.aid)
		return false;
	return false;
}

//TODO: find out why the following are not generated automatically on MVS?
bool TSubgoal::operator==(const TSubgoal & rhs) const
{
	return *get() == *rhs.get(); //comparison for Goals is overloaded, so they don't need to be identical to match
}

bool TSubgoal::operator<(const TSubgoal & rhs) const
{
	return get() < rhs.get(); //compae by value
}

bool AbstractGoal::invalid() const
{
	return goalType == EGoals::INVALID;
}

void AbstractGoal::accept(VCAI * ai)
{
	ai->tryRealize(*this);
}

float AbstractGoal::accept(FuzzyHelper * f)
{
	return f->evaluate(*this);
}

/*
* VisitHero.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "VisitHero.h"
#include "Explore.h"
#include "Invalid.h"
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../ResourceManager.h"
#include "../BuildingManager.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions


extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

using namespace Goals;

bool VisitHero::operator==(const VisitHero & other) const
{
	return other.hero.h == hero.h && other.objid == objid;
}

std::string VisitHero::completeMessage() const
{
	return "hero " + hero.get()->getNameTranslated() + " visited hero " + boost::lexical_cast<std::string>(objid);
}

TSubgoal VisitHero::whatToDoToAchieve()
{
	const CGObjectInstance * obj = cb->getObj(ObjectInstanceID(objid));
	if(!obj)
		return sptr(Explore());
	int3 pos = obj->visitablePos();

	if(hero && ai->isAccessibleForHero(pos, hero, true) && isSafeToVisit(hero, pos)) //enemy heroes can get reinforcements
	{
		if(hero->visitablePos() == pos)
			logAi->error("Hero %s tries to visit himself.", hero.name);
		else
		{
			//can't use VISIT_TILE here as tile appears blocked by target hero
			//FIXME: elementar goal should not be abstract
			return sptr(VisitHero(objid).sethero(hero).settile(pos).setisElementar(true));
		}
	}
	return sptr(Invalid());
}

bool VisitHero::fulfillsMe(TSubgoal goal)
{
	//TODO: VisitObj shoudl not be used for heroes, but...
	if(goal->goalType == VISIT_TILE)
	{
		auto obj = cb->getObj(ObjectInstanceID(objid));
		if (!obj)
		{
			logAi->error("Hero %s: VisitHero::fulfillsMe at %s: object %d not found", hero.name, goal->tile.toString(), objid);
			return false;
		}
		return obj->visitablePos() == goal->tile;
	}
	return false;
}

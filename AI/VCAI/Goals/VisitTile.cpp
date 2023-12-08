/*
* VisitTile.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Goals.h"
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../ResourceManager.h"
#include "../BuildingManager.h"
#include "../../../lib/constants/StringConstants.h"

using namespace Goals;

bool VisitTile::operator==(const VisitTile & other) const
{
	return other.hero.h == hero.h && other.tile == tile;
}

std::string VisitTile::completeMessage() const
{
	return "Hero " + hero.get()->getNameTranslated() + " visited tile " + tile.toString();
}

TSubgoal VisitTile::whatToDoToAchieve()
{
	auto ret = fh->chooseSolution(getAllPossibleSubgoals());

	if(ret->hero)
	{
		if(isSafeToVisit(ret->hero, tile) && ai->isAccessibleForHero(tile, ret->hero))
		{
			ret->setisElementar(true);
			return ret;
		}
		else
		{
			return sptr(GatherArmy((int)(fh->evaluateDanger(tile, *ret->hero) * SAFE_ATTACK_CONSTANT))
						.sethero(ret->hero).setisAbstract(true));
		}
	}
	return ret;
}

TGoalVec VisitTile::getAllPossibleSubgoals()
{
	assert(cb->isInTheMap(tile));

	TGoalVec ret;
	if(!cb->isVisible(tile))
		ret.push_back(sptr(Explore())); //what sense does it make?
	else
	{
		std::vector<const CGHeroInstance *> heroes;
		if(hero)
			heroes.push_back(hero.h); //use assigned hero if any
		else
			heroes = cb->getHeroesInfo(); //use most convenient hero

		for(auto h : heroes)
		{
			if(ai->isAccessibleForHero(tile, h))
				ret.push_back(sptr(VisitTile(tile).sethero(h)));
		}
		if(ai->canRecruitAnyHero())
			ret.push_back(sptr(RecruitHero()));
	}
	if(ret.empty())
	{
		auto obj = vstd::frontOrNull(cb->getVisitableObjs(tile));
		if(obj && obj->ID == Obj::HERO && obj->tempOwner == ai->playerID) //our own hero stands on that tile
		{
			if(hero.get(true) && hero->id == obj->id) //if it's assigned hero, visit tile. If it's different hero, we can't visit tile now
				ret.push_back(sptr(VisitTile(tile).sethero(dynamic_cast<const CGHeroInstance *>(obj)).setisElementar(true)));
			else
				throw cannotFulfillGoalException("Tile is already occupied by another hero "); //FIXME: we should give up this tile earlier
		}
		else
			ret.push_back(sptr(ClearWayTo(tile)));
	}

	//important - at least one sub-goal must handle case which is impossible to fulfill (unreachable tile)
	return ret;
}

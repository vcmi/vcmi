/*
 * TownRelatedSpellUtils.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TownRelatedSpellUtils.h"

#include "../../CPlayerState.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapObjects/CGTownInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace adventure
{
std::vector<const CGTownInstance *> getPlayerTeamTowns(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, bool skipOccupiedTowns)
{
	std::vector<const CGTownInstance *> result;

	const TeamState * team = env->getCb()->getPlayerTeam(parameters.caster->getCasterOwner());
	for(const auto & color : team->players)
	{
		for(const auto * town : env->getCb()->getPlayerState(color)->getTowns())
		{
			if(!skipOccupiedTowns || town->getVisitingHero() == nullptr)
				result.push_back(town);
		}
	}

	return result;
}

const CGTownInstance * findNearestTown(const AdventureSpellCastParameters & parameters, const std::vector<const CGTownInstance *> & pool)
{
	if(pool.empty() || !parameters.caster->getHeroCaster())
		return nullptr;

	auto nearest = pool.cbegin();
	si32 distance = (*nearest)->visitablePos().dist2dSQ(parameters.caster->getHeroCaster()->visitablePos());

	for(auto iter = nearest + 1; iter != pool.cend(); ++iter)
	{
		si32 currentDistance = (*iter)->visitablePos().dist2dSQ(parameters.caster->getHeroCaster()->visitablePos());

		if(currentDistance < distance)
		{
			nearest = iter;
			distance = currentDistance;
		}
	}

	return *nearest;
}
}
}

VCMI_LIB_NAMESPACE_END

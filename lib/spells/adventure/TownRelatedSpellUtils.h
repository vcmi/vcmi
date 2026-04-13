/*
 * TownRelatedSpellUtils.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AdventureSpellEffect.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGTownInstance;

namespace spells
{
namespace adventure
{
std::vector<const CGTownInstance *> getPlayerTeamTowns(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, bool skipOccupiedTowns);
const CGTownInstance * findNearestTown(const AdventureSpellCastParameters & parameters, const std::vector<const CGTownInstance *> & pool);
}
}

VCMI_LIB_NAMESPACE_END

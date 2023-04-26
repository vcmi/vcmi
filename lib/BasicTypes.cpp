/*
 * BasicTypes.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "VCMI_Lib.h"
#include "GameConstants.h"
#include "HeroBonus.h"

#include <vcmi/Entity.h>
#include <vcmi/Faction.h>
#include <vcmi/FactionService.h>

VCMI_LIB_NAMESPACE_BEGIN

bool INativeTerrainProvider::isNativeTerrain(TerrainId terrain) const
{
	auto native = getNativeTerrain();
	return native == terrain || native == ETerrainId::ANY_TERRAIN;
}

TerrainId IFactionMember::getNativeTerrain() const
{
	constexpr auto any = TerrainId(ETerrainId::ANY_TERRAIN);
	const std::string cachingStringNoTerrainPenalty = "type_NO_TERRAIN_PENALTY_sANY";
	static const auto selectorNoTerrainPenalty = Selector::typeSubtype(Bonus::NO_TERRAIN_PENALTY, any);

	//this code is used in the CreatureTerrainLimiter::limit to setup battle bonuses
	//and in the CGHeroInstance::getNativeTerrain() to setup movement bonuses or/and penalties.
	return getBonusBearer()->hasBonus(selectorNoTerrainPenalty, cachingStringNoTerrainPenalty)
		? any : VLC->factions()->getById(getFaction())->getNativeTerrain();
}

VCMI_LIB_NAMESPACE_END
/*
 * AdventureSpellEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "AdventureSpellEffect.h"

#include "../../json/JsonNode.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../callback/IGameInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

AdventureSpellRangedEffect::AdventureSpellRangedEffect(const JsonNode & config)
	: rangeX(config["rangeX"].Integer())
	, rangeY(config["rangeY"].Integer())
	, ignoreFow(config["ignoreFow"].Bool())
{
}

bool AdventureSpellRangedEffect::isTargetInRange(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	if(!cb->isInTheMap(pos))
		return false;

	if(caster->getHeroCaster())
	{
		int3 center = caster->getHeroCaster()->getSightCenter();

		int3 diff = pos - center;
		return diff.x >= -rangeX && diff.x <= rangeX && diff.y >= -rangeY && diff.y <= rangeY;
	}

	if(!ignoreFow && !cb->isVisibleFor(pos, caster->getCasterOwner()))
		return false;

	return true;
}

VCMI_LIB_NAMESPACE_END

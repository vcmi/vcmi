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

AdventureSpellRangedEffect::AdventureSpellRangedEffect(const JsonNode & config)
	: rangeX(config["rangeX"].Integer())
	, rangeY(config["rangeY"].Integer())
	, ignoreFow(config["ignoreFow"].Bool())
{
}

int AdventureSpellRangedEffect::getRangeX() const
{
	return rangeX;
}

int AdventureSpellRangedEffect::getRangeY() const
{
	return rangeY;
}

bool AdventureSpellRangedEffect::ignoresFogOfWar() const
{
	return ignoreFow;
}

bool AdventureSpellRangedEffect::isTargetInRange(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	if(caster->getHeroCaster())
		return isTargetInRangeFrom(cb, caster, caster->getHeroCaster()->getSightCenter(), pos);

	return isTargetValidForRangeCheck(cb, caster, pos);
}

bool AdventureSpellRangedEffect::isTargetValidForRangeCheck(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	if(!cb->isInTheMap(pos))
		return false;

	if(!caster->getHeroCaster() && !ignoreFow && !cb->isVisibleFor(pos, caster->getCasterOwner()))
		return false;

	return true;
}

bool AdventureSpellRangedEffect::isTargetInRangeFrom(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & source, const int3 & pos) const
{
	if(!cb->isInTheMap(source) || !isTargetValidForRangeCheck(cb, caster, pos))
		return false;

	int3 diff = pos - source;
	return diff.x >= -rangeX && diff.x <= rangeX && diff.y >= -rangeY && diff.y <= rangeY;
}

bool AdventureSpellRangedEffect::isValidTargetFrom(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & source, const int3 & pos) const
{
	return isTargetInRangeFrom(cb, caster, source, pos);
}

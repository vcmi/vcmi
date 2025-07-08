/*
 * ViewWorldMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ViewWorldMechanics.h"

#include "../CSpellHandler.h"

#include "../../CPlayerState.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapping/CMap.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

ViewMechanics::ViewMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult ViewMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	ShowWorldViewEx pack;

	pack.player = parameters.caster->getCasterOwner();

	const auto spellLevel = parameters.caster->getSpellSchoolLevel(owner);

	const auto & fowMap = env->getCb()->getPlayerTeam(parameters.caster->getCasterOwner())->fogOfWarMap;

	for(const auto & obj : env->getMap()->getObjects())
	{
		//deleted object remain as empty pointer
		if(obj && filterObject(obj, spellLevel))
		{
			ObjectPosInfo posInfo(obj);

			if(fowMap[posInfo.pos.z][posInfo.pos.x][posInfo.pos.y] == 0)
				pack.objectPositions.push_back(posInfo);
		}
	}
	pack.showTerrain = showTerrain(spellLevel);

	env->apply(pack);

	return ESpellCastResult::OK;
}

///ViewAirMechanics
ViewAirMechanics::ViewAirMechanics(const CSpell * s)
	: ViewMechanics(s)
{
}

bool ViewAirMechanics::filterObject(const CGObjectInstance * obj, const int32_t spellLevel) const
{
	return (obj->ID == Obj::ARTIFACT) || (spellLevel > 1 && obj->ID == Obj::HERO) || (spellLevel > 2 && obj->ID == Obj::TOWN);
}

bool ViewAirMechanics::showTerrain(const int32_t spellLevel) const
{
	return false;
}

///ViewEarthMechanics
ViewEarthMechanics::ViewEarthMechanics(const CSpell * s)
	: ViewMechanics(s)
{
}

bool ViewEarthMechanics::filterObject(const CGObjectInstance * obj, const int32_t spellLevel) const
{
	return (obj->ID == Obj::RESOURCE) || (spellLevel > 1 && obj->ID == Obj::MINE);
}

bool ViewEarthMechanics::showTerrain(const int32_t spellLevel) const
{
	return spellLevel > 2;
}

VCMI_LIB_NAMESPACE_END

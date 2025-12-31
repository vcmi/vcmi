/*
 * ViewWorldEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "ViewWorldEffect.h"

#include "../CSpellHandler.h"

#include "../../CPlayerState.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapping/CMap.h"
#include "../../modding/IdentifierStorage.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

ViewWorldEffect::ViewWorldEffect(const CSpell * s, const JsonNode & config)
{
	showTerrain = config["showTerrain"].Bool();

	for(const auto & objectNode : config["objects"].Struct())
	{
		if(objectNode.second.Bool())
		{
			LIBRARY->identifiers()->requestIdentifier(objectNode.second.getModScope(), "object", objectNode.first, [this](si32 index)
			{
				filteredObjects.push_back(MapObjectID(index));
			});
		}
	}
}

ESpellCastResult ViewWorldEffect::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	ShowWorldViewEx pack;

	pack.player = parameters.caster->getCasterOwner();
	pack.showTerrain = showTerrain;

	const auto & fowMap = env->getCb()->getPlayerTeam(parameters.caster->getCasterOwner())->fogOfWarMap;

	for(const auto & obj : env->getMap()->getObjects())
	{
		//deleted object remain as empty pointer
		if(obj && vstd::contains(filteredObjects, obj->ID))
		{
			ObjectPosInfo posInfo(obj);

			if(fowMap[posInfo.pos.z][posInfo.pos.x][posInfo.pos.y] == 0)
				pack.objectPositions.push_back(posInfo);
		}
	}

	env->apply(pack);

	return ESpellCastResult::OK;
}

VCMI_LIB_NAMESPACE_END

/*
 * RemoveObjectEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RemoveObjectEffect.h"

#include "../CSpellHandler.h"

#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapping/CMap.h"
#include "../../networkPacks/PacksForClient.h"
#include "../../modding/IdentifierStorage.h"

VCMI_LIB_NAMESPACE_BEGIN

RemoveObjectEffect::RemoveObjectEffect(const CSpell * s, const JsonNode & config)
	: AdventureSpellRangedEffect(config)
	, owner(s)
	, failMessage(MetaString::createFromTextID("core.genrltxt.337")) //%s tried to scuttle the boat, but failed
	, cursor(config["cursor"].String())
{
	for(const auto & objectNode : config["objects"].Struct())
	{
		if(objectNode.second.Bool())
		{
			LIBRARY->identifiers()->requestIdentifier(objectNode.second.getModScope(), "object", objectNode.first, [this](si32 index)
			{
				removedObjects.push_back(MapObjectID(index));
			});
		}
	}
}

std::string RemoveObjectEffect::getCursorForTarget(const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	return cursor;
}

bool RemoveObjectEffect::canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	if (!isTargetInRange(cb, caster, pos))
		return false;

	const TerrainTile * t = cb->getTileUnchecked(pos);
	if(!t || t->visitableObjects.empty())
		return false;

	const CGObjectInstance * topObject = cb->getObj(t->visitableObjects.back());

	return vstd::contains(removedObjects, topObject->ID);
}

ESpellCastResult RemoveObjectEffect::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const auto schoolLevel = parameters.caster->getSpellSchoolLevel(owner);
	//check if spell works at all
	if(env->getRNG()->nextInt(0, 99) >= owner->getLevelPower(schoolLevel)) //power is % chance of success
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text = failMessage;
		parameters.caster->getCasterName(iw.text);
		env->apply(iw);
		return ESpellCastResult::OK;
	}

	const TerrainTile & t = env->getMap()->getTile(parameters.pos);

	RemoveObject ro;
	ro.initiator = parameters.caster->getCasterOwner();
	ro.objectID = t.visitableObjects.back();
	env->apply(ro);
	return ESpellCastResult::OK;
}

VCMI_LIB_NAMESPACE_END

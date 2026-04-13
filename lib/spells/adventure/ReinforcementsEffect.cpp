/*
 * ReinforcementsEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ReinforcementsEffect.h"

#include "AdventureSpellMechanics.h"
#include "TownRelatedSpellUtils.h"

#include "../CSpellHandler.h"

#include "../../CPlayerState.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../mapping/CMap.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

ReinforcementsEffect::ReinforcementsEffect(const CSpell * s, const JsonNode & config)
	: owner(s)
	, allowTownSelection(config["allowTownSelection"].Bool())
{
}

ESpellCastResult ReinforcementsEffect::beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const AdventureSpellMechanics & mechanics) const
{
	std::vector<const CGTownInstance *> towns = spells::adventure::getPlayerTeamTowns(env, parameters, false);

	if(!parameters.caster->getHeroCaster())
	{
		env->complain("Not a hero caster!");
		return ESpellCastResult::ERROR;
	}

	if(towns.empty())
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 124);
		env->apply(iw);
		return ESpellCastResult::CANCEL;
	}

	if(!parameters.pos.isValid() && allowTownSelection)
	{
		std::vector<ObjectInstanceID> offeredTownIDs;
		offeredTownIDs.reserve(towns.size());

		for(const auto * town : towns)
			offeredTownIDs.push_back(town->id);

		auto queryCallback = [&mechanics, env, parameters, offeredTownIDs](std::optional<int32_t> reply) -> void
		{
			if(reply.has_value())
			{
				ObjectInstanceID townId(*reply);
				if(!vstd::contains(offeredTownIDs, townId))
				{
					env->complain("Invalid town selected in reinforcement dialog");
					return;
				}

				const CGObjectInstance * object = env->getCb()->getObj(townId, true);
				if(object == nullptr)
				{
					env->complain("Invalid object instance selected");
					return;
				}

				if(!dynamic_cast<const CGTownInstance *>(object))
				{
					env->complain("Object instance is not town");
					return;
				}

				AdventureSpellCastParameters nextCast;
				nextCast.caster = parameters.caster;
				nextCast.pos = object->visitablePos();
				mechanics.performCast(env, nextCast);
			}
		};

		MapObjectSelectDialog request;
		request.player = parameters.caster->getCasterOwner();
		request.title.appendTextID("vcmi.spells.reinforcements.selectTown.title");
		request.description.appendTextID("vcmi.spells.reinforcements.selectTown.description");
		request.icon = Component(ComponentType::SPELL, owner->id);

		request.objects = offeredTownIDs;

		env->genericQuery(&request, request.player, queryCallback);
		return ESpellCastResult::PENDING;
	}

	return ESpellCastResult::OK;
}

ESpellCastResult ReinforcementsEffect::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const CGTownInstance * destination = nullptr;
	std::vector<const CGTownInstance *> towns = spells::adventure::getPlayerTeamTowns(env, parameters, false);

	if(!parameters.caster->getHeroCaster())
	{
		env->complain("Not a hero caster!");
		return ESpellCastResult::ERROR;
	}

	if(!allowTownSelection)
	{
		destination = spells::adventure::findNearestTown(parameters, towns);
	}
	else if(env->getMap()->isInTheMap(parameters.pos))
	{
		auto selectedTown = std::find_if(towns.begin(), towns.end(), [&parameters](const CGTownInstance * town)
		{
			return town->visitablePos() == parameters.pos;
		});

		if(selectedTown != towns.end())
			destination = *selectedTown;
	}

	if(destination == nullptr)
	{
		env->complain("Failed to find destination town for reinforcements");
		return ESpellCastResult::ERROR;
	}

	env->showGarrisonDialog(destination->id, ObjectInstanceID(parameters.caster->getCasterUnitId()), true);
	return ESpellCastResult::OK;
}

VCMI_LIB_NAMESPACE_END

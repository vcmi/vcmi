/*
 * TownRelatedAdventureSpellEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TownRelatedAdventureSpellEffect.h"

#include "AdventureSpellMechanics.h"

#include "../CSpell.h"

#include "../../CPlayerState.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace adventure
{
std::vector<const CGTownInstance *> TownRelatedAdventureSpellEffect::getPlayerTeamTowns(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
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

const CGTownInstance * TownRelatedAdventureSpellEffect::findNearestTown(const AdventureSpellCastParameters & parameters, const std::vector<const CGTownInstance *> & pool) const
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

TownRelatedAdventureSpellEffect::TownRelatedAdventureSpellEffect(const CSpell * owner_, bool allowTownSelection_, bool skipOccupiedTowns_)
	: owner(owner_)
	, allowTownSelection(allowTownSelection_)
	, skipOccupiedTowns(skipOccupiedTowns_)
{
}

bool TownRelatedAdventureSpellEffect::shouldOfferTownInDialog(const CGTownInstance * town) const
{
	return town != nullptr;
}

ESpellCastResult TownRelatedAdventureSpellEffect::beginCastExtraChecks(SpellCastEnvironment *, const AdventureSpellCastParameters &, const std::vector<const CGTownInstance *> &) const
{
	return ESpellCastResult::OK;
}

ESpellCastResult TownRelatedAdventureSpellEffect::onNoTownToSelect(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	InfoWindow iw;
	iw.player = parameters.caster->getCasterOwner();
	iw.text.appendLocalString(EMetaText::GENERAL_TXT, 124);
	env->apply(iw);
	return ESpellCastResult::CANCEL;
}

ESpellCastResult TownRelatedAdventureSpellEffect::beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const AdventureSpellMechanics & mechanics) const
{
	std::vector<const CGTownInstance *> towns = getPlayerTeamTowns(env, parameters);

	if(!parameters.caster->getHeroCaster())
	{
		env->complain("Not a hero caster!");
		return ESpellCastResult::ERROR;
	}

	auto extraChecksResult = beginCastExtraChecks(env, parameters, towns);
	if(extraChecksResult != ESpellCastResult::OK)
		return extraChecksResult;

	if(towns.empty())
		return onNoTownToSelect(env, parameters);

	if(!parameters.pos.isValid() && allowTownSelection)
	{
		std::vector<ObjectInstanceID> offeredTownIDs;
		offeredTownIDs.reserve(towns.size());

		for(const auto * town : towns)
		{
			if(shouldOfferTownInDialog(town))
				offeredTownIDs.push_back(town->id);
		}

		if(offeredTownIDs.empty())
			return onNoTownToSelect(env, parameters);

		auto queryCallback = [&mechanics, env, parameters, offeredTownIDs](std::optional<int32_t> reply) -> void
		{
			if(reply.has_value())
			{
				ObjectInstanceID townId(*reply);
				if(!vstd::contains(offeredTownIDs, townId))
				{
					env->complain("Invalid town selected in dialog");
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
		configureDialogTitleAndDescription(request.title, request.description);
		request.icon = Component(ComponentType::SPELL, owner->id);
		request.objects = offeredTownIDs;

		env->genericQuery(&request, request.player, queryCallback);
		return ESpellCastResult::PENDING;
	}

	return ESpellCastResult::OK;
}
}
}

VCMI_LIB_NAMESPACE_END

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

#include "../CSpellHandler.h"

#include "../../CPlayerState.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../mapping/CMap.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

ReinforcementsEffect::ReinforcementsEffect(const CSpell * s, const JsonNode & config)
	: TownRelatedAdventureSpellEffect(s, config["allowTownSelection"].Bool(), false)
	, casterInTownTextID(config["casterInTown"].String().starts_with("@") ? config["casterInTown"].String().substr(1) : s->getAdventureEffectTextID("reinforcements", "casterInTown"))
	, selectTownTitleTextID(config["selectTownTitle"].String().starts_with("@") ? config["selectTownTitle"].String().substr(1) : s->getAdventureEffectTextID("reinforcements", "selectTownTitle"))
	, selectTownDescriptionTextID(config["selectTownDescription"].String().starts_with("@") ? config["selectTownDescription"].String().substr(1) : s->getAdventureEffectTextID("reinforcements", "selectTownDescription"))
	, garrisonTitleTextID(config["garrisonTitle"].String().starts_with("@") ? config["garrisonTitle"].String().substr(1) : s->getAdventureEffectTextID("reinforcements", "garrisonTitle"))
{
}

void ReinforcementsEffect::configureDialogTitleAndDescription(MetaString & title, MetaString & description) const
{
	title.appendTextID(selectTownTitleTextID);
	description.appendTextID(selectTownDescriptionTextID);
}

ESpellCastResult ReinforcementsEffect::beginCastExtraChecks(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector<const CGTownInstance *> &) const
{
	const auto * casterHero = parameters.caster->getHeroCaster();
	if(casterHero->getVisitedTown() != nullptr)
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.appendTextID(casterInTownTextID);
		env->apply(iw);
		return ESpellCastResult::CANCEL;
	}

	return ESpellCastResult::OK;
}

ESpellCastResult ReinforcementsEffect::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const CGTownInstance * destination = nullptr;
	std::vector<const CGTownInstance *> towns = getPlayerTeamTowns(env, parameters);

	const auto * casterHero = parameters.caster->getHeroCaster();
	if(!casterHero)
	{
		env->complain("Not a hero caster!");
		return ESpellCastResult::ERROR;
	}

	if(casterHero->getVisitedTown() != nullptr)
		return ESpellCastResult::CANCEL;

	if(!allowTownSelection)
	{
		destination = findNearestTown(parameters, towns);
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

	const auto * sourceArmy = destination->getUpperArmy();
	MetaString garrisonTitle;
	garrisonTitle.appendTextID(garrisonTitleTextID);
	env->showGarrisonDialog(sourceArmy->id, ObjectInstanceID(parameters.caster->getCasterUnitId()), true, garrisonTitle);

	return ESpellCastResult::OK;
}

VCMI_LIB_NAMESPACE_END

/*
 * TownPortalEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TownPortalEffect.h"

#include "TownRelatedAdventureSpellEffect.h"

#include "../../CPlayerState.h"
#include "../../IGameSettings.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../mapping/CMap.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

TownPortalEffect::TownPortalEffect(const CSpell * s, const JsonNode & config)
	: TownRelatedAdventureSpellEffect(s, config["allowTownSelection"].Bool(), config["skipOccupiedTowns"].Bool())
	, movementPointsRequired(config["movementPointsRequired"].Integer())
	, movementPointsTaken(config["movementPointsTaken"].Integer())
{
}

bool TownPortalEffect::shouldOfferTownInDialog(const CGTownInstance * town) const
{
	return town->getVisitingHero() == nullptr;
}

void TownPortalEffect::configureDialogTitleAndDescription(MetaString & title, MetaString & description) const
{
	title.appendLocalString(EMetaText::JK_TXT, 40);
	description.appendLocalString(EMetaText::JK_TXT, 41);
}

ESpellCastResult TownPortalEffect::beginCastExtraChecks(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector<const CGTownInstance *> &) const
{
	if(static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()) < movementPointsTaken)
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 125);
		env->apply(iw);
		return ESpellCastResult::CANCEL;
	}

	return ESpellCastResult::OK;
}

ESpellCastResult TownPortalEffect::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const CGTownInstance * destination = nullptr;

	if(!parameters.caster->getHeroCaster())
	{
		env->complain("Not a hero caster!");
		return ESpellCastResult::ERROR;
	}

	if(!allowTownSelection)
	{
		std::vector<const CGTownInstance *> pool = getPlayerTeamTowns(env, parameters);
		destination = findNearestTown(parameters, pool);

		if(nullptr == destination)
			return ESpellCastResult::ERROR;

		if(static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()) < movementPointsRequired)
			return ESpellCastResult::ERROR;

		if(destination->getVisitingHero())
		{
			InfoWindow iw;
			iw.player = parameters.caster->getCasterOwner();
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 123);
			env->apply(iw);
			return ESpellCastResult::CANCEL;
		}
	}
	else if(env->getMap()->isInTheMap(parameters.pos))
	{
		const TerrainTile & tile = env->getMap()->getTile(parameters.pos);

		ObjectInstanceID topObjID = tile.topVisitableObj(false);
		const CGObjectInstance * topObj = env->getMap()->getObject(topObjID);

		if(!topObj)
		{
			env->complain("Destination tile is not visitable" + parameters.pos.toString());
			return ESpellCastResult::ERROR;
		}
		else if(topObj->ID == Obj::HERO)
		{
			env->complain("Can't teleport to occupied town at " + parameters.pos.toString());
			return ESpellCastResult::ERROR;
		}
		else if(topObj->ID != Obj::TOWN)
		{
			env->complain("No town at destination tile " + parameters.pos.toString());
			return ESpellCastResult::ERROR;
		}

		destination = dynamic_cast<const CGTownInstance *>(topObj);

		if(nullptr == destination)
		{
			env->complain("[Internal error] invalid town object at " + parameters.pos.toString());
			return ESpellCastResult::ERROR;
		}

		const auto relations = env->getCb()->getPlayerRelations(destination->tempOwner, parameters.caster->getCasterOwner());

		if(relations == PlayerRelations::ENEMIES)
		{
			env->complain("Can't teleport to enemy!");
			return ESpellCastResult::ERROR;
		}

		if(static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()) < movementPointsRequired)
		{
			env->complain("This hero has not enough movement points!");
			return ESpellCastResult::ERROR;
		}

		if(destination->getVisitingHero())
		{
			env->complain("[Internal error] Can't teleport to occupied town");
			return ESpellCastResult::ERROR;
		}
	}
	else
	{
		env->complain("Invalid destination tile");
		return ESpellCastResult::ERROR;
	}

	const TerrainTile & from = env->getMap()->getTile(parameters.caster->getHeroCaster()->visitablePos());
	const TerrainTile & dest = env->getMap()->getTile(destination->visitablePos());

	if(!dest.entrableTerrain(&from))
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 135);
		env->apply(iw);
		return ESpellCastResult::ERROR;
	}

	return ESpellCastResult::OK;
}

void TownPortalEffect::endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const CGTownInstance * destination = nullptr;

	if(!allowTownSelection)
	{
		std::vector<const CGTownInstance *> pool = getPlayerTeamTowns(env, parameters);
		destination = findNearestTown(parameters, pool);
	}
	else
	{
		const TerrainTile & tile = env->getMap()->getTile(parameters.pos);
		ObjectInstanceID topObjID = tile.topVisitableObj(false);
		const CGObjectInstance * topObj = env->getMap()->getObject(topObjID);

		destination = dynamic_cast<const CGTownInstance *>(topObj);
	}

	if(env->moveHero(ObjectInstanceID(parameters.caster->getCasterUnitId()), parameters.caster->getHeroCaster()->convertFromVisitablePos(destination->visitablePos()), EMovementMode::TOWN_PORTAL))
	{
		SetMovePoints smp;
		smp.hid = ObjectInstanceID(parameters.caster->getCasterUnitId());
		if(movementPointsTaken < static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()))
			smp.val = parameters.caster->getHeroCaster()->movementPointsRemaining() - movementPointsTaken;
		else
			smp.val = 0;
		env->apply(smp);
	}
}

VCMI_LIB_NAMESPACE_END

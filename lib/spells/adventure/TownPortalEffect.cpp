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

#include "AdventureSpellMechanics.h"

#include "../CSpellHandler.h"

#include "../../CPlayerState.h"
#include "../../IGameSettings.h"
#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapObjects/CGTownInstance.h"
#include "../../mapping/CMap.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

TownPortalEffect::TownPortalEffect(const CSpell * s, const JsonNode & config)
	: owner(s)
	, movementPointsRequired(config["movementPointsRequired"].Integer())
	, movementPointsTaken(config["movementPointsTaken"].Integer())
	, allowTownSelection(config["allowTownSelection"].Bool())
	, skipOccupiedTowns(config["skipOccupiedTowns"].Bool())
{
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
		std::vector<const CGTownInstance *> pool = getPossibleTowns(env, parameters);
		destination = findNearestTown(env, parameters, pool);

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

	if(parameters.caster->getSpellSchoolLevel(owner) < 2)
	{
		std::vector<const CGTownInstance *> pool = getPossibleTowns(env, parameters);
		destination = findNearestTown(env, parameters, pool);
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

ESpellCastResult TownPortalEffect::beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const AdventureSpellMechanics & mechanics) const
{
	std::vector<const CGTownInstance *> towns = getPossibleTowns(env, parameters);

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

	if(static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()) < movementPointsTaken)
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 125);
		env->apply(iw);
		return ESpellCastResult::CANCEL;
	}

	if(!parameters.pos.isValid() && parameters.caster->getSpellSchoolLevel(owner) >= 2)
	{
		auto queryCallback = [&mechanics, env, parameters](std::optional<int32_t> reply) -> void
		{
			if(reply.has_value())
			{
				ObjectInstanceID townId(*reply);

				const CGObjectInstance * o = env->getCb()->getObj(townId, true);
				if(o == nullptr)
				{
					env->complain("Invalid object instance selected");
					return;
				}

				if(!dynamic_cast<const CGTownInstance *>(o))
				{
					env->complain("Object instance is not town");
					return;
				}

				AdventureSpellCastParameters p;
				p.caster = parameters.caster;
				p.pos = o->visitablePos();
				mechanics.performCast(env, p);
			}
		};

		MapObjectSelectDialog request;

		for(const auto * t : towns)
		{
			if(t->getVisitingHero() == nullptr) //empty town
				request.objects.push_back(t->id);
		}

		if(request.objects.empty())
		{
			InfoWindow iw;
			iw.player = parameters.caster->getCasterOwner();
			iw.text.appendLocalString(EMetaText::GENERAL_TXT, 124);
			env->apply(iw);
			return ESpellCastResult::CANCEL;
		}

		request.player = parameters.caster->getCasterOwner();
		request.title.appendLocalString(EMetaText::JK_TXT, 40);
		request.description.appendLocalString(EMetaText::JK_TXT, 41);
		request.icon = Component(ComponentType::SPELL, owner->id);

		env->genericQuery(&request, request.player, queryCallback);

		return ESpellCastResult::PENDING;
	}

	return ESpellCastResult::OK;
}

const CGTownInstance * TownPortalEffect::findNearestTown(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector <const CGTownInstance *> & pool) const
{
	if(pool.empty())
		return nullptr;

	if(!parameters.caster->getHeroCaster())
		return nullptr;

	auto nearest = pool.cbegin(); //nearest town's iterator
	si32 dist = (*nearest)->visitablePos().dist2dSQ(parameters.caster->getHeroCaster()->visitablePos());

	for(auto i = nearest + 1; i != pool.cend(); ++i)
	{
		si32 curDist = (*i)->visitablePos().dist2dSQ(parameters.caster->getHeroCaster()->visitablePos());

		if(curDist < dist)
		{
			nearest = i;
			dist = curDist;
		}
	}
	return *nearest;
}

std::vector<const CGTownInstance *> TownPortalEffect::getPossibleTowns(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	std::vector<const CGTownInstance *> ret;

	const TeamState * team = env->getCb()->getPlayerTeam(parameters.caster->getCasterOwner());

	for(const auto & color : team->players)
	{
		for(auto currTown : env->getCb()->getPlayerState(color)->getTowns())
		{
			if (!skipOccupiedTowns || currTown->getVisitingHero() == nullptr)
				ret.push_back(currTown);
		}
	}
	return ret;
}

VCMI_LIB_NAMESPACE_END

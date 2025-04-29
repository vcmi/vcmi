/*
 * AdventureSpellMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "AdventureSpellMechanics.h"

#include "CSpellHandler.h"
#include "Problem.h"

#include "../CGameInfoCallback.h"
#include "../CPlayerState.h"
#include "../IGameSettings.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapping/CMap.h"
#include "../networkPacks/PacksForClient.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

///AdventureSpellMechanics
AdventureSpellMechanics::AdventureSpellMechanics(const CSpell * s):
	IAdventureSpellMechanics(s)
{
}

bool AdventureSpellMechanics::canBeCast(spells::Problem & problem, const CGameInfoCallback * cb, const spells::Caster * caster) const
{
	if(!owner->isAdventure())
		return false;

	const auto * heroCaster = dynamic_cast<const CGHeroInstance *>(caster);

	if (heroCaster)
	{
		if(heroCaster->isGarrisoned())
			return false;

		const auto level = heroCaster->getSpellSchoolLevel(owner);
		const auto cost = owner->getCost(level);

		if(!heroCaster->canCastThisSpell(owner))
			return false;

		if(heroCaster->mana < cost)
			return false;
	}

	return canBeCastImpl(problem, cb, caster);
}

bool AdventureSpellMechanics::canBeCastAt(spells::Problem & problem, const CGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	return canBeCast(problem, cb, caster) && canBeCastAtImpl(problem, cb, caster, pos);
}

bool AdventureSpellMechanics::canBeCastImpl(spells::Problem & problem, const CGameInfoCallback * cb, const spells::Caster * caster) const
{
	return true;
}

bool AdventureSpellMechanics::canBeCastAtImpl(spells::Problem & problem, const CGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	return true;
}

bool AdventureSpellMechanics::adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	spells::detail::ProblemImpl problem;

	if (!canBeCastAt(problem, env->getCb(), parameters.caster, parameters.pos))
		return false;

	ESpellCastResult result = beginCast(env, parameters);

	if(result == ESpellCastResult::OK)
		performCast(env, parameters);

	return result != ESpellCastResult::ERROR;
}

ESpellCastResult AdventureSpellMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	if(owner->hasEffects())
	{
		//todo: cumulative effects support
		const auto schoolLevel = parameters.caster->getSpellSchoolLevel(owner);

		std::vector<Bonus> bonuses;

		owner->getEffects(bonuses, schoolLevel, false, parameters.caster->getEnchantPower(owner));

		for(const Bonus & b : bonuses)
		{
			GiveBonus gb;
			gb.id = ObjectInstanceID(parameters.caster->getCasterUnitId());
			gb.bonus = b;
			env->apply(gb);
		}

		return ESpellCastResult::OK;
	}
	else
	{
		//There is no generic algorithm of adventure cast
		env->complain("Unimplemented adventure spell");
		return ESpellCastResult::ERROR;
	}
}

ESpellCastResult AdventureSpellMechanics::beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	return ESpellCastResult::OK;
}

void AdventureSpellMechanics::endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	// no-op, only for implementation in derived classes
}

void AdventureSpellMechanics::performCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const auto level = parameters.caster->getSpellSchoolLevel(owner);
	const auto cost = owner->getCost(level);

	AdvmapSpellCast asc;
	asc.casterID = ObjectInstanceID(parameters.caster->getCasterUnitId());
	asc.spellID = owner->id;
	env->apply(asc);

	ESpellCastResult result = applyAdventureEffects(env, parameters);

	switch(result)
	{
	case ESpellCastResult::OK:
		parameters.caster->spendMana(env, cost);
		endCast(env, parameters);
		break;
	default:
		break;
	}
}

///SummonBoatMechanics
SummonBoatMechanics::SummonBoatMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

bool SummonBoatMechanics::canBeCastImpl(spells::Problem & problem, const CGameInfoCallback * cb, const spells::Caster * caster) const
{
	if(!caster->getHeroCaster())
		return false;

	if(caster->getHeroCaster()->inBoat())
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.333");
		caster->getCasterName(message);
		problem.add(std::move(message));
		return false;
	}

	int3 summonPos = caster->getHeroCaster()->bestLocation();

	if(summonPos.x < 0)
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.334");
		caster->getCasterName(message);
		problem.add(std::move(message));
		return false;
	}

	return true;
}

ESpellCastResult SummonBoatMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const auto schoolLevel = parameters.caster->getSpellSchoolLevel(owner);

	//check if spell works at all
	if(env->getRNG()->nextInt(0, 99) >= owner->getLevelPower(schoolLevel)) //power is % chance of success
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 336); //%s tried to summon a boat, but failed.
		parameters.caster->getCasterName(iw.text);
		env->apply(iw);
		return ESpellCastResult::OK;
	}

	//try to find unoccupied boat to summon
	const CGBoat * nearest = nullptr;
	double dist = 0;
	for(const auto & b : env->getMap()->getObjects<CGBoat>())
	{
		if(b->getBoardedHero() || b->layer != EPathfindingLayer::SAIL)
			continue; //we're looking for unoccupied boat

		double nDist = b->visitablePos().dist2d(parameters.caster->getHeroCaster()->visitablePos());
		if(!nearest || nDist < dist) //it's first boat or closer than previous
		{
			nearest = b;
			dist = nDist;
		}
	}

	int3 summonPos = parameters.caster->getHeroCaster()->bestLocation();

	if(nullptr != nearest) //we found boat to summon
	{
		ChangeObjPos cop;
		cop.objid = nearest->id;
		cop.nPos = summonPos;
		cop.initiator = parameters.caster->getCasterOwner();
		env->apply(cop);
	}
	else if(schoolLevel < 2) //none or basic level -> cannot create boat :(
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 335); //There are no boats to summon.
		env->apply(iw);
		return ESpellCastResult::ERROR;
	}
	else //create boat
	{
		env->createBoat(summonPos, BoatId::NECROPOLIS, parameters.caster->getCasterOwner());
	}
	return ESpellCastResult::OK;
}

///ScuttleBoatMechanics
ScuttleBoatMechanics::ScuttleBoatMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

bool ScuttleBoatMechanics::canBeCastAtImpl(spells::Problem & problem, const CGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	if(!cb->isInTheMap(pos))
		return false;

	if (caster->getHeroCaster())
	{
		int3 casterPosition = caster->getHeroCaster()->getSightCenter();

		if(!isInScreenRange(casterPosition, pos))
			return false;
	}

	if(!cb->isVisible(pos, caster->getCasterOwner()))
		return false;

	const TerrainTile * t = cb->getTile(pos);
	if(!t || t->visitableObjects.empty())
		return false;

	const CGObjectInstance * topObject = cb->getObj(t->visitableObjects.back());
	if (topObject->ID != Obj::BOAT)
		return false;

	return true;
}

ESpellCastResult ScuttleBoatMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const auto schoolLevel = parameters.caster->getSpellSchoolLevel(owner);
	//check if spell works at all
	if(env->getRNG()->nextInt(0, 99) >= owner->getLevelPower(schoolLevel)) //power is % chance of success
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 337); //%s tried to scuttle the boat, but failed
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

///DimensionDoorMechanics
DimensionDoorMechanics::DimensionDoorMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

bool DimensionDoorMechanics::canBeCastImpl(spells::Problem & problem, const CGameInfoCallback * cb, const spells::Caster * caster) const
{
	if(!caster->getHeroCaster())
		return false;

	if(caster->getHeroCaster()->movementPointsRemaining() <= 0) //unlike town portal non-zero MP is enough
	{
		problem.add(MetaString::createFromTextID("core.genrltxt.125"));
		return false;
	}

	const auto schoolLevel = caster->getSpellSchoolLevel(owner);

	std::stringstream cachingStr;
	cachingStr << "source_" << vstd::to_underlying(BonusSource::SPELL_EFFECT) << "id_" << owner->id.num;

	int castsAlreadyPerformedThisTurn = caster->getHeroCaster()->getBonuses(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(owner->id)), Selector::all, cachingStr.str())->size();
	int castsLimit = owner->getLevelPower(schoolLevel);

	bool isTournamentRulesLimitEnabled = cb->getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_TOURNAMENT_RULES_LIMIT);
	if(isTournamentRulesLimitEnabled)
	{
		int3 mapSize = cb->getMapSize();

		bool meetsTournamentRulesTwoCastsRequirements =  mapSize.x * mapSize.y * mapSize.z >= GameConstants::TOURNAMENT_RULES_DD_MAP_TILES_THRESHOLD
			&& schoolLevel == MasteryLevel::EXPERT;

		castsLimit = meetsTournamentRulesTwoCastsRequirements ? 2 : 1;
	}

	if(castsAlreadyPerformedThisTurn >= castsLimit) //limit casts per turn
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.338");
		caster->getCasterName(message);
		problem.add(std::move(message));
		return false;
	}

	return true;
}

bool DimensionDoorMechanics::canBeCastAtImpl(spells::Problem & problem, const CGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	if(!cb->isInTheMap(pos))
		return false;

	if(cb->getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_ONLY_TO_UNCOVERED_TILES))
	{
		if(!cb->isVisible(pos, caster->getCasterOwner()))
			return false;
	}

	int3 casterPosition = caster->getHeroCaster()->getSightCenter();

	const TerrainTile * dest = cb->getTileUnchecked(pos);
	const TerrainTile * curr = cb->getTileUnchecked(casterPosition);

	if(!dest)
		return false;

	if(!curr)
		return false;

	if(!isInScreenRange(casterPosition, pos))
		return false;

	if(cb->getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_EXPOSES_TERRAIN_TYPE))
	{
		if(!dest->isClear(curr))
			return false;
	}
	else
	{
		if (dest->blocked())
			return false;
	}

	return true;
}

ESpellCastResult DimensionDoorMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const auto schoolLevel = parameters.caster->getSpellSchoolLevel(owner);
	const int baseCost = env->getCb()->getSettings().getInteger(EGameSettings::HEROES_MOVEMENT_COST_BASE);
	const int movementCost = baseCost * ((schoolLevel >= 3) ? 2 : 3);

	int3 casterPosition = parameters.caster->getHeroCaster()->getSightCenter();
	const TerrainTile * dest = env->getCb()->getTile(parameters.pos);
	const TerrainTile * curr = env->getCb()->getTile(casterPosition);

	if(!dest->isClear(curr))
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();

		// tile is either blocked or not possible to move (e.g. water <-> land)
		if(env->getCb()->getSettings().getBoolean(EGameSettings::DIMENSION_DOOR_FAILURE_SPENDS_POINTS))
		{
			// SOD: DD to such "wrong" terrain results in mana and move points spending, but fails to move hero
			iw.text = MetaString::createFromTextID("core.genrltxt.70"); // Dimension Door failed!
			env->apply(iw);
			// no return - resources will be spent
		}
		else
		{
			// HotA: game will show error message without taking mana or move points, even when DD into terra incognita
			iw.text = MetaString::createFromTextID("vcmi.dimensionDoor.seaToLandError");
			env->apply(iw);
			return ESpellCastResult::CANCEL;
		}
	}

	GiveBonus gb;
	gb.id = ObjectInstanceID(parameters.caster->getCasterUnitId());
	gb.bonus = Bonus(BonusDuration::ONE_DAY, BonusType::NONE, BonusSource::SPELL_EFFECT, 0, BonusSourceID(owner->id));
	env->apply(gb);

	SetMovePoints smp;
	smp.hid = ObjectInstanceID(parameters.caster->getCasterUnitId());
	if(movementCost < static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()))
		smp.val = parameters.caster->getHeroCaster()->movementPointsRemaining() - movementCost;
	else
		smp.val = 0;
	env->apply(smp);

	return ESpellCastResult::OK;
}

void DimensionDoorMechanics::endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	int3 casterPosition = parameters.caster->getHeroCaster()->getSightCenter();
	const TerrainTile * dest = env->getCb()->getTile(parameters.pos);
	const TerrainTile * curr = env->getCb()->getTile(casterPosition);

	if(dest->isClear(curr))
		env->moveHero(ObjectInstanceID(parameters.caster->getCasterUnitId()), parameters.caster->getHeroCaster()->convertFromVisitablePos(parameters.pos), EMovementMode::DIMENSION_DOOR);
}

///TownPortalMechanics
TownPortalMechanics::TownPortalMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult TownPortalMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const CGTownInstance * destination = nullptr;
	const int moveCost = movementCost(env, parameters);
	
	if(!parameters.caster->getHeroCaster())
	{
		env->complain("Not a hero caster!");
		return ESpellCastResult::ERROR;
	}

	if(parameters.caster->getSpellSchoolLevel(owner) < 2)
	{
		std::vector <const CGTownInstance*> pool = getPossibleTowns(env, parameters);
		destination = findNearestTown(env, parameters, pool);

		if(nullptr == destination)
			return ESpellCastResult::ERROR;

		if(static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()) < moveCost)
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

		destination = dynamic_cast<const CGTownInstance*>(topObj);

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

		if(static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()) < moveCost)
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

void TownPortalMechanics::endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const int moveCost = movementCost(env, parameters);
	const CGTownInstance * destination = nullptr;

	if(parameters.caster->getSpellSchoolLevel(owner) < 2)
	{
		std::vector <const CGTownInstance*> pool = getPossibleTowns(env, parameters);
		destination = findNearestTown(env, parameters, pool);
	}
	else
	{
		const TerrainTile & tile = env->getMap()->getTile(parameters.pos);
		ObjectInstanceID topObjID = tile.topVisitableObj(false);
		const CGObjectInstance * topObj = env->getMap()->getObject(topObjID);

		destination = dynamic_cast<const CGTownInstance*>(topObj);
	}

	if(env->moveHero(ObjectInstanceID(parameters.caster->getCasterUnitId()), parameters.caster->getHeroCaster()->convertFromVisitablePos(destination->visitablePos()), EMovementMode::TOWN_PORTAL))
	{
		SetMovePoints smp;
		smp.hid = ObjectInstanceID(parameters.caster->getCasterUnitId());
		smp.val = std::max<ui32>(0, parameters.caster->getHeroCaster()->movementPointsRemaining() - moveCost);
		env->apply(smp);
	}
}

ESpellCastResult TownPortalMechanics::beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	std::vector<const CGTownInstance *>	towns = getPossibleTowns(env, parameters);
	
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

	const int moveCost = movementCost(env, parameters);

	if(static_cast<int>(parameters.caster->getHeroCaster()->movementPointsRemaining()) < moveCost)
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 125);
		env->apply(iw);
		return ESpellCastResult::CANCEL;
	}

	if(!parameters.pos.isValid() && parameters.caster->getSpellSchoolLevel(owner) >= 2)
	{
		auto queryCallback = [this, env, parameters](std::optional<int32_t> reply) -> void
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
				performCast(env, p);
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

const CGTownInstance * TownPortalMechanics::findNearestTown(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector <const CGTownInstance *> & pool) const
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

std::vector <const CGTownInstance*> TownPortalMechanics::getPossibleTowns(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	std::vector <const CGTownInstance*> ret;

	const TeamState * team = env->getCb()->getPlayerTeam(parameters.caster->getCasterOwner());

	for(const auto & color : team->players)
	{
		for(auto currTown : env->getCb()->getPlayerState(color)->getTowns())
		{
			ret.push_back(currTown);
		}
	}
	return ret;
}

int32_t TownPortalMechanics::movementCost(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	if(parameters.caster != parameters.caster->getHeroCaster()) //if caster is not hero
		return 0;
	
	int baseMovementCost = env->getCb()->getSettings().getInteger(EGameSettings::HEROES_MOVEMENT_COST_BASE);
	return baseMovementCost * ((parameters.caster->getSpellSchoolLevel(owner) >= 3) ? 2 : 3);
}

///ViewMechanics
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
ViewAirMechanics::ViewAirMechanics(const CSpell * s):
	ViewMechanics(s)
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
ViewEarthMechanics::ViewEarthMechanics(const CSpell * s):
	ViewMechanics(s)
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

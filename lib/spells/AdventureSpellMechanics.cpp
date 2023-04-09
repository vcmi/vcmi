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

#include "../CRandomGenerator.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../NetPacks.h"
#include "../CGameInfoCallback.h"
#include "../mapping/CMap.h"
#include "../CPlayerState.h"

VCMI_LIB_NAMESPACE_BEGIN

///AdventureSpellMechanics
AdventureSpellMechanics::AdventureSpellMechanics(const CSpell * s):
	IAdventureSpellMechanics(s)
{
}

bool AdventureSpellMechanics::adventureCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	if(!owner->isAdventure())
	{
		env->complain("Attempt to cast non adventure spell in adventure mode");
		return false;
	}

	if(const CGHeroInstance * heroCaster = dynamic_cast<const CGHeroInstance *>(parameters.caster))
	{
		if(heroCaster->inTownGarrison)
		{
			env->complain("Attempt to cast an adventure spell in town garrison");
			return false;
		}

		const auto level = heroCaster->getSpellSchoolLevel(owner);
		const auto cost = owner->getCost(level);

		if(!heroCaster->canCastThisSpell(owner))
		{
			env->complain("Hero cannot cast this spell!");
			return false;
		}

		if(heroCaster->mana < cost)
		{
			env->complain("Hero doesn't have enough spell points to cast this spell!");
			return false;
		}
	}

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
			gb.id = parameters.caster->getCasterUnitId();
			gb.bonus = b;
			env->apply(&gb);
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

void AdventureSpellMechanics::performCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	AdvmapSpellCast asc;
	asc.casterID = ObjectInstanceID(parameters.caster->getCasterUnitId());
	asc.spellID = owner->id;
	env->apply(&asc);

	ESpellCastResult result = applyAdventureEffects(env, parameters);
	endCast(env, parameters, result);
}

void AdventureSpellMechanics::endCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const ESpellCastResult result) const
{
	const auto level = parameters.caster->getSpellSchoolLevel(owner);
	const auto cost = owner->getCost(level);

	switch(result)
	{
	case ESpellCastResult::OK:
		{
			SetMana sm;
			sm.hid = ObjectInstanceID(parameters.caster->getCasterUnitId());
			sm.absolute = false;
			sm.val = -cost;
			env->apply(&sm);
		}
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

ESpellCastResult SummonBoatMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	if(!parameters.caster->getHeroCaster())
	{
		env->complain("Not a hero caster!");
		return ESpellCastResult::ERROR;
	}
	
	if(parameters.caster->getHeroCaster()->boat)
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.addTxt(MetaString::GENERAL_TXT, 333);//%s is already in boat
		parameters.caster->getCasterName(iw.text);
		env->apply(&iw);
		return ESpellCastResult::CANCEL;
	}

	int3 summonPos = parameters.caster->getHeroCaster()->bestLocation();
	
	if(summonPos.x < 0)
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.addTxt(MetaString::GENERAL_TXT, 334);//There is no place to put the boat.
		env->apply(&iw);
		return ESpellCastResult::CANCEL;
	}

	const auto schoolLevel = parameters.caster->getSpellSchoolLevel(owner);

	//check if spell works at all
	if(env->getRNG()->getInt64Range(0, 99)() >= owner->getLevelPower(schoolLevel)) //power is % chance of success
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.addTxt(MetaString::GENERAL_TXT, 336); //%s tried to summon a boat, but failed.
		parameters.caster->getCasterName(iw.text);
		env->apply(&iw);
		return ESpellCastResult::OK;
	}

	//try to find unoccupied boat to summon
	const CGBoat * nearest = nullptr;
	double dist = 0;
	for(const CGObjectInstance * obj : env->getMap()->objects)
	{
		if(obj && obj->ID == Obj::BOAT)
		{
			const auto * b = dynamic_cast<const CGBoat *>(obj);
			if(b->hero)
				continue; //we're looking for unoccupied boat

			double nDist = b->pos.dist2d(parameters.caster->getHeroCaster()->visitablePos());
			if(!nearest || nDist < dist) //it's first boat or closer than previous
			{
				nearest = b;
				dist = nDist;
			}
		}
	}

	if(nullptr != nearest) //we found boat to summon
	{
		ChangeObjPos cop;
		cop.objid = nearest->id;
		cop.nPos = summonPos + int3(1,0,0);
		env->apply(&cop);
	}
	else if(schoolLevel < 2) //none or basic level -> cannot create boat :(
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.addTxt(MetaString::GENERAL_TXT, 335); //There are no boats to summon.
		env->apply(&iw);
	}
	else //create boat
	{
		NewObject no;
		no.ID = Obj::BOAT;
		no.subID = parameters.caster->getHeroCaster()->getBoatType();
		no.pos = summonPos + int3(1,0,0);
		env->apply(&no);
	}
	return ESpellCastResult::OK;
}

///ScuttleBoatMechanics
ScuttleBoatMechanics::ScuttleBoatMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult ScuttleBoatMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const auto schoolLevel = parameters.caster->getSpellSchoolLevel(owner);
	//check if spell works at all
	if(env->getRNG()->getInt64Range(0, 99)() >= owner->getLevelPower(schoolLevel)) //power is % chance of success
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.addTxt(MetaString::GENERAL_TXT, 337); //%s tried to scuttle the boat, but failed
		parameters.caster->getCasterName(iw.text);
		env->apply(&iw);
		return ESpellCastResult::OK;
	}

	if(!env->getMap()->isInTheMap(parameters.pos))
	{
		env->complain("Invalid dst tile for scuttle!");
		return ESpellCastResult::ERROR;
	}

	//TODO: test range, visibility
	const TerrainTile *t = &env->getMap()->getTile(parameters.pos);
	if(t->visitableObjects.empty() || t->visitableObjects.back()->ID != Obj::BOAT)
	{
		env->complain("There is no boat to scuttle!");
		return ESpellCastResult::ERROR;
	}

	RemoveObject ro;
	ro.id = t->visitableObjects.back()->id;
	env->apply(&ro);
	return ESpellCastResult::OK;
}

///DimensionDoorMechanics
DimensionDoorMechanics::DimensionDoorMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult DimensionDoorMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	if(!env->getMap()->isInTheMap(parameters.pos))
	{
		env->complain("Destination is out of map!");
		return ESpellCastResult::ERROR;
	}
	
	if(!parameters.caster->getHeroCaster())
	{
		env->complain("Not a hero caster!");
		return ESpellCastResult::ERROR;
	}

	const TerrainTile * dest = env->getCb()->getTile(parameters.pos);
	const TerrainTile * curr = env->getCb()->getTile(parameters.caster->getHeroCaster()->getSightCenter());

	if(nullptr == dest)
	{
		env->complain("Destination tile doesn't exist!");
		return ESpellCastResult::ERROR;
	}

	if(nullptr == curr)
	{
		env->complain("Source tile doesn't exist!");
		return ESpellCastResult::ERROR;
	}

	if(parameters.caster->getHeroCaster()->movement <= 0) //unlike town portal non-zero MP is enough
	{
		env->complain("Hero needs movement points to cast Dimension Door!");
		return ESpellCastResult::ERROR;
	}

	const auto schoolLevel = parameters.caster->getSpellSchoolLevel(owner);
	const int movementCost = GameConstants::BASE_MOVEMENT_COST * ((schoolLevel >= 3) ? 2 : 3);

	std::stringstream cachingStr;
	cachingStr << "source_" << Bonus::SPELL_EFFECT << "id_" << owner->id.num;

	if(parameters.caster->getHeroCaster()->getBonuses(Selector::source(Bonus::SPELL_EFFECT, owner->id), Selector::all, cachingStr.str())->size() >= owner->getLevelPower(schoolLevel)) //limit casts per turn
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.addTxt(MetaString::GENERAL_TXT, 338); //%s is not skilled enough to cast this spell again today.
		parameters.caster->getCasterName(iw.text);
		env->apply(&iw);
		return ESpellCastResult::CANCEL;
	}

	GiveBonus gb;
	gb.id = parameters.caster->getCasterUnitId();
	gb.bonus = Bonus(Bonus::ONE_DAY, Bonus::NONE, Bonus::SPELL_EFFECT, 0, owner->id);
	env->apply(&gb);

	if(!dest->isClear(curr)) //wrong dest tile
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.addTxt(MetaString::GENERAL_TXT, 70); //Dimension Door failed!
		env->apply(&iw);
	}
	else if(env->moveHero(ObjectInstanceID(parameters.caster->getCasterUnitId()), parameters.caster->getHeroCaster()->convertFromVisitablePos(parameters.pos), true))
	{
		SetMovePoints smp;
		smp.hid = ObjectInstanceID(parameters.caster->getCasterUnitId());
		if(movementCost < static_cast<int>(parameters.caster->getHeroCaster()->movement))
			smp.val = parameters.caster->getHeroCaster()->movement - movementCost;
		else
			smp.val = 0;
		env->apply(&smp);
	}
	return ESpellCastResult::OK;
}

///TownPortalMechanics
TownPortalMechanics::TownPortalMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult TownPortalMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const CGTownInstance * destination = nullptr;
	const int moveCost = movementCost(parameters);

	if(parameters.caster->getSpellSchoolLevel(owner) < 2)
	{
		std::vector <const CGTownInstance*> pool = getPossibleTowns(env, parameters);
		destination = findNearestTown(env, parameters, pool);

		if(nullptr == destination)
			return ESpellCastResult::ERROR;

		if(static_cast<int>(parameters.caster->movement) < moveCost)
			return ESpellCastResult::ERROR;

		if(destination->visitingHero)
		{
			InfoWindow iw;
			iw.player = parameters.caster->tempOwner;
			iw.text.addTxt(MetaString::GENERAL_TXT, 123);
			env->apply(&iw);
			return ESpellCastResult::CANCEL;
		}
    }
    else if(env->getMap()->isInTheMap(parameters.pos))
	{
		const TerrainTile & tile = env->getMap()->getTile(parameters.pos);

		auto * const topObj = tile.topVisitableObj(false);

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

		const auto relations = env->getCb()->getPlayerRelations(destination->tempOwner, parameters.caster->tempOwner);

		if(relations == PlayerRelations::ENEMIES)
		{
			env->complain("Can't teleport to enemy!");
			return ESpellCastResult::ERROR;
		}

		if(static_cast<int>(parameters.caster->movement) < moveCost)
		{
			env->complain("This hero has not enough movement points!");
			return ESpellCastResult::ERROR;
		}

		if(destination->visitingHero)
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

	if(env->moveHero(parameters.caster->id, parameters.caster->convertFromVisitablePos(destination->visitablePos()), true))
	{
		SetMovePoints smp;
		smp.hid = parameters.caster->id;
		smp.val = std::max<ui32>(0, parameters.caster->movement - moveCost);
		env->apply(&smp);
	}
	return ESpellCastResult::OK;
}

ESpellCastResult TownPortalMechanics::beginCast(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	std::vector<const CGTownInstance *>	towns = getPossibleTowns(env, parameters);

	if(towns.empty())
	{
		InfoWindow iw;
		iw.player = parameters.caster->tempOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 124);
		env->apply(&iw);
		return ESpellCastResult::CANCEL;
	}

	const int moveCost = movementCost(parameters);

	if(static_cast<int>(parameters.caster->movement) < moveCost)
	{
		InfoWindow iw;
		iw.player = parameters.caster->tempOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 125);
		env->apply(&iw);
		return ESpellCastResult::CANCEL;
	}

	if(!parameters.pos.valid() && parameters.caster->getSpellSchoolLevel(owner) >= 2)
	{
		auto queryCallback = [=](const JsonNode & reply) -> void
		{
			if(reply.getType() == JsonNode::JsonType::DATA_INTEGER)
			{
				ObjectInstanceID townId(static_cast<si32>(reply.Integer()));

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
			if(t->visitingHero == nullptr) //empty town
				request.objects.push_back(t->id);
		}

		if(request.objects.empty())
		{
			InfoWindow iw;
			iw.player = parameters.caster->tempOwner;
			iw.text.addTxt(MetaString::GENERAL_TXT, 124);
			env->apply(&iw);
			return ESpellCastResult::CANCEL;
		}

		request.player = parameters.caster->getOwner();
		request.title.addTxt(MetaString::JK_TXT, 40);
		request.description.addTxt(MetaString::JK_TXT, 41);
		request.icon.id = Component::EComponentType::SPELL;
		request.icon.subtype = owner->id.toEnum();

		env->genericQuery(&request, request.player, queryCallback);

		return ESpellCastResult::PENDING;
	}

	return ESpellCastResult::OK;
}

const CGTownInstance * TownPortalMechanics::findNearestTown(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector <const CGTownInstance *> & pool) const
{
	if(pool.empty())
		return nullptr;

	auto nearest = pool.cbegin(); //nearest town's iterator
	si32 dist = (*nearest)->pos.dist2dSQ(parameters.caster->pos);

	for(auto i = nearest + 1; i != pool.cend(); ++i)
	{
		si32 curDist = (*i)->pos.dist2dSQ(parameters.caster->pos);

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

	const TeamState * team = env->getCb()->getPlayerTeam(parameters.caster->getOwner());

	for(const auto & color : team->players)
	{
		for(auto currTown : env->getCb()->getPlayerState(color)->towns)
		{
			ret.push_back(currTown.get());
		}
	}
	return ret;
}

int32_t TownPortalMechanics::movementCost(const AdventureSpellCastParameters & parameters) const
{
	return GameConstants::BASE_MOVEMENT_COST * ((parameters.caster->getSpellSchoolLevel(owner) >= 3) ? 2 : 3);
}

///ViewMechanics
ViewMechanics::ViewMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult ViewMechanics::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	ShowWorldViewEx pack;

	pack.player = parameters.caster->getOwner();

	const auto spellLevel = parameters.caster->getSpellSchoolLevel(owner);

	const auto fowMap = env->getCb()->getPlayerTeam(parameters.caster->getOwner())->fogOfWarMap;

	for(const CGObjectInstance * obj : env->getMap()->objects)
	{
		//deleted object remain as empty pointer
		if(obj && filterObject(obj, spellLevel))
		{
			ObjectPosInfo posInfo(obj);

			if((*fowMap)[posInfo.pos.z][posInfo.pos.x][posInfo.pos.y] == 0)
				pack.objectPositions.push_back(posInfo);
		}
	}
	pack.showTerrain = showTerrain(spellLevel);

	env->apply(&pack);

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

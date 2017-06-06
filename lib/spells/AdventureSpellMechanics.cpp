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

#include "../CRandomGenerator.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../NetPacks.h"
#include "../CGameInfoCallback.h"
#include "../mapping/CMap.h"
#include "../CPlayerState.h"

///AdventureSpellMechanics
AdventureSpellMechanics::AdventureSpellMechanics(const CSpell * s):
	IAdventureSpellMechanics(s)
{
}

bool AdventureSpellMechanics::adventureCast(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const
{
	if(!owner->isAdventureSpell())
	{
		env->complain("Attempt to cast non adventure spell in adventure mode");
		return false;
	}

	const CGHeroInstance * caster = parameters.caster;

	if(caster->inTownGarrison)
	{
		env->complain("Attempt to cast an adventure spell in town garrison");
		return false;
	}

	const int cost = caster->getSpellCost(owner);

	if(!caster->canCastThisSpell(owner))
	{
		env->complain("Hero cannot cast this spell!");
		return false;
	}

	if(caster->mana < cost)
	{
		env->complain("Hero doesn't have enough spell points to cast this spell!");
		return false;
	}

	{
		AdvmapSpellCast asc;
		asc.caster = caster;
		asc.spellID = owner->id;
		env->sendAndApply(&asc);
	}

	switch(applyAdventureEffects(env, parameters))
	{
	case ESpellCastResult::OK:
		{
			SetMana sm;
			sm.hid = caster->id;
			sm.absolute = false;
			sm.val = -cost;
			env->sendAndApply(&sm);
			return true;
		}
		break;
	case ESpellCastResult::CANCEL:
		return true;
	}
	return false;
}

ESpellCastResult AdventureSpellMechanics::applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	if(owner->hasEffects())
	{
		//todo: cumulative effects support
		const int schoolLevel = parameters.caster->getSpellSchoolLevel(owner);

		std::vector<Bonus> bonuses;

		owner->getEffects(bonuses, schoolLevel, false, parameters.caster->getEnchantPower(owner));

		for(Bonus b : bonuses)
		{
			GiveBonus gb;
			gb.id = parameters.caster->id.getNum();
			gb.bonus = b;
			env->sendAndApply(&gb);
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

///SummonBoatMechanics
SummonBoatMechanics::SummonBoatMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult SummonBoatMechanics::applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	int3 summonPos = parameters.caster->bestLocation();
	if(summonPos.x < 0)
	{
		InfoWindow iw;
		iw.player = parameters.caster->tempOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 334);//There is no place to put the boat.
		env->sendAndApply(&iw);
		return ESpellCastResult::CANCEL;
	}

	const int schoolLevel = parameters.caster->getSpellSchoolLevel(owner);

	//check if spell works at all
	if(env->getRandomGenerator().nextInt(99) >= owner->getPower(schoolLevel)) //power is % chance of success
	{
		InfoWindow iw;
		iw.player = parameters.caster->tempOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 336); //%s tried to summon a boat, but failed.
		iw.text.addReplacement(parameters.caster->name);
		env->sendAndApply(&iw);
		return ESpellCastResult::OK;
	}

	//try to find unoccupied boat to summon
	const CGBoat * nearest = nullptr;
	double dist = 0;
	for(const CGObjectInstance * obj : env->getMap()->objects)
	{
		if(obj && obj->ID == Obj::BOAT)
		{
			const CGBoat *b = static_cast<const CGBoat*>(obj);
			if(b->hero)
				continue; //we're looking for unoccupied boat

			double nDist = b->pos.dist2d(parameters.caster->getPosition());
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
		cop.flags = 1;
		env->sendAndApply(&cop);
	}
	else if(schoolLevel < 2) //none or basic level -> cannot create boat :(
	{
		InfoWindow iw;
		iw.player = parameters.caster->tempOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 335); //There are no boats to summon.
		env->sendAndApply(&iw);
	}
	else //create boat
	{
		NewObject no;
		no.ID = Obj::BOAT;
		no.subID = parameters.caster->getBoatType();
		no.pos = summonPos + int3(1,0,0);
		env->sendAndApply(&no);
	}
	return ESpellCastResult::OK;
}

///ScuttleBoatMechanics
ScuttleBoatMechanics::ScuttleBoatMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult ScuttleBoatMechanics::applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const int schoolLevel = parameters.caster->getSpellSchoolLevel(owner);
	//check if spell works at all
	if(env->getRandomGenerator().nextInt(99) >= owner->getPower(schoolLevel)) //power is % chance of success
	{
		InfoWindow iw;
		iw.player = parameters.caster->tempOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 337); //%s tried to scuttle the boat, but failed
		iw.text.addReplacement(parameters.caster->name);
		env->sendAndApply(&iw);
		return ESpellCastResult::OK;
	}

	if(!env->getMap()->isInTheMap(parameters.pos))
	{
		env->complain("Invalid dst tile for scuttle!");
		return ESpellCastResult::ERROR;
	}

	//TODO: test range, visibility
	const TerrainTile *t = &env->getMap()->getTile(parameters.pos);
	if(!t->visitableObjects.size() || t->visitableObjects.back()->ID != Obj::BOAT)
	{
		env->complain("There is no boat to scuttle!");
		return ESpellCastResult::ERROR;
	}

	RemoveObject ro;
	ro.id = t->visitableObjects.back()->id;
	env->sendAndApply(&ro);
	return ESpellCastResult::OK;
}

///DimensionDoorMechanics
DimensionDoorMechanics::DimensionDoorMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult DimensionDoorMechanics::applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	if(!env->getMap()->isInTheMap(parameters.pos))
	{
		env->complain("Destination is out of map!");
		return ESpellCastResult::ERROR;
	}

	const TerrainTile * dest = env->getCb()->getTile(parameters.pos);
	const TerrainTile * curr = env->getCb()->getTile(parameters.caster->getSightCenter());

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

	if(parameters.caster->movement <= 0) //unlike town portal non-zero MP is enough
	{
		env->complain("Hero needs movement points to cast Dimension Door!");
		return ESpellCastResult::ERROR;
	}

	const int schoolLevel = parameters.caster->getSpellSchoolLevel(owner);
	const int movementCost = GameConstants::BASE_MOVEMENT_COST * ((schoolLevel >= 3) ? 2 : 3);

	std::stringstream cachingStr;
	cachingStr << "source_" << Bonus::SPELL_EFFECT << "id_" << owner->id.num;

	if(parameters.caster->getBonuses(Selector::source(Bonus::SPELL_EFFECT, owner->id), Selector::all, cachingStr.str())->size() >= owner->getPower(schoolLevel)) //limit casts per turn
	{
		InfoWindow iw;
		iw.player = parameters.caster->tempOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 338); //%s is not skilled enough to cast this spell again today.
		iw.text.addReplacement(parameters.caster->name);
		env->sendAndApply(&iw);
		return ESpellCastResult::CANCEL;
	}

	GiveBonus gb;
	gb.id = parameters.caster->id.getNum();
	gb.bonus = Bonus(Bonus::ONE_DAY, Bonus::NONE, Bonus::SPELL_EFFECT, 0, owner->id);
	env->sendAndApply(&gb);

	if(!dest->isClear(curr)) //wrong dest tile
	{
		InfoWindow iw;
		iw.player = parameters.caster->tempOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 70); //Dimension Door failed!
		env->sendAndApply(&iw);
	}
	else if(env->moveHero(parameters.caster->id, parameters.pos + parameters.caster->getVisitableOffset(), true))
	{
		SetMovePoints smp;
		smp.hid = parameters.caster->id;
		smp.val = std::max<ui32>(0, parameters.caster->movement - movementCost);
		env->sendAndApply(&smp);
	}
	return ESpellCastResult::OK;
}

///TownPortalMechanics
TownPortalMechanics::TownPortalMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult TownPortalMechanics::applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	const CGTownInstance * destination = nullptr;
	const int movementCost = GameConstants::BASE_MOVEMENT_COST * ((parameters.caster->getSpellSchoolLevel(owner) >= 3) ? 2 : 3);

    if(parameters.caster->getSpellSchoolLevel(owner) < 2)
    {
		std::vector <const CGTownInstance*> pool = getPossibleTowns(env, parameters);
		destination = findNearestTown(env, parameters, pool);

		if(nullptr == destination)
		{
			InfoWindow iw;
			iw.player = parameters.caster->tempOwner;
			iw.text.addTxt(MetaString::GENERAL_TXT, 124);
			env->sendAndApply(&iw);
			return ESpellCastResult::CANCEL;
		}

		if(parameters.caster->movement < movementCost)
		{
			InfoWindow iw;
			iw.player = parameters.caster->tempOwner;
			iw.text.addTxt(MetaString::GENERAL_TXT, 125);
			env->sendAndApply(&iw);
			return ESpellCastResult::CANCEL;
		}

		if (destination->visitingHero)
		{
			InfoWindow iw;
			iw.player = parameters.caster->tempOwner;
			iw.text.addTxt(MetaString::GENERAL_TXT, 123);
			env->sendAndApply(&iw);
			return ESpellCastResult::CANCEL;
		}
    }
    else if(env->getMap()->isInTheMap(parameters.pos))
	{
		const TerrainTile & tile = env->getMap()->getTile(parameters.pos);
		if(tile.visitableObjects.empty() || tile.visitableObjects.back()->ID != Obj::TOWN)
		{
			env->complain("No town at destination tile");
			return ESpellCastResult::ERROR;
		}

		destination = dynamic_cast<CGTownInstance*>(tile.visitableObjects.back());

		if(nullptr == destination)
		{
			env->complain("[Internal error] invalid town object");
			return ESpellCastResult::ERROR;
		}

		const auto relations = env->getCb()->getPlayerRelations(destination->tempOwner, parameters.caster->tempOwner);

		if(relations == PlayerRelations::ENEMIES)
		{
			env->complain("Can't teleport to enemy!");
			return ESpellCastResult::ERROR;
		}

		if(parameters.caster->movement < movementCost)
		{
			env->complain("This hero has not enough movement points!");
			return ESpellCastResult::ERROR;
		}

		if(destination->visitingHero)
		{
			env->complain("Can't teleport to occupied town!");
			return ESpellCastResult::ERROR;
		}
	}
	else
	{
		env->complain("Invalid destination tile");
		return ESpellCastResult::ERROR;
	}

	if(env->moveHero(parameters.caster->id, destination->visitablePos() + parameters.caster->getVisitableOffset(), 1))
	{
		SetMovePoints smp;
		smp.hid = parameters.caster->id;
		smp.val = std::max<ui32>(0, parameters.caster->movement - movementCost);
		env->sendAndApply(&smp);
	}
	return ESpellCastResult::OK;
}

const CGTownInstance * TownPortalMechanics::findNearestTown(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters, const std::vector <const CGTownInstance *> & pool) const
{
	if(pool.empty())
		return nullptr;

	auto nearest = pool.cbegin(); //nearest town's iterator
	si32 dist = (*nearest)->pos.dist2dSQ(parameters.caster->pos);

	for (auto i = nearest + 1; i != pool.cend(); ++i)
	{
		si32 curDist = (*i)->pos.dist2dSQ(parameters.caster->pos);

		if (curDist < dist)
		{
			nearest = i;
			dist = curDist;
		}
	}
	return *nearest;
}

std::vector <const CGTownInstance*> TownPortalMechanics::getPossibleTowns(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	std::vector <const CGTownInstance*> ret;

	const TeamState * team = env->getCb()->getPlayerTeam(parameters.caster->getOwner());

	for(const auto & color : team->players)
	{
		for(auto currTown : env->getCb()->getPlayer(color)->towns)
		{
			ret.push_back(currTown.get());
		}
	}
	return ret;
}

///ViewMechanics
ViewMechanics::ViewMechanics(const CSpell * s):
	AdventureSpellMechanics(s)
{
}

ESpellCastResult ViewMechanics::applyAdventureEffects(const SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	ShowWorldViewEx pack;

	pack.player = parameters.caster->getOwner();

	const int spellLevel = parameters.caster->getSpellSchoolLevel(owner);

	const auto & fowMap = env->getCb()->getPlayerTeam(parameters.caster->getOwner())->fogOfWarMap;

	for(const CGObjectInstance * obj : env->getMap()->objects)
	{
		//deleted object remain as empty pointer
		if(obj && filterObject(obj, spellLevel))
		{
			ObjectPosInfo posInfo(obj);

			if(fowMap[posInfo.pos.x][posInfo.pos.y][posInfo.pos.z] == 0)
				pack.objectPositions.push_back(posInfo);
		}
	}

	env->sendAndApply(&pack);

	return ESpellCastResult::OK;
}

///ViewAirMechanics
ViewAirMechanics::ViewAirMechanics(const CSpell * s):
	ViewMechanics(s)
{
}

bool ViewAirMechanics::filterObject(const CGObjectInstance * obj, const int spellLevel) const
{
	return (obj->ID == Obj::ARTIFACT) || (spellLevel > 1 && obj->ID == Obj::HERO) || (spellLevel > 2 && obj->ID == Obj::TOWN);
}

///ViewEarthMechanics
ViewEarthMechanics::ViewEarthMechanics(const CSpell * s):
	ViewMechanics(s)
{
}

bool ViewEarthMechanics::filterObject(const CGObjectInstance * obj, const int spellLevel) const
{
	return (obj->ID == Obj::RESOURCE) || (spellLevel > 1 && obj->ID == Obj::MINE);
}


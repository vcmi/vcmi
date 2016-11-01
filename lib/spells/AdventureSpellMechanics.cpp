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

ESpellCastResult AdventureSpellMechanics::applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const
{
	if(owner->hasEffects())
	{
		const int schoolLevel = parameters.caster->getSpellSchoolLevel(owner);

		std::vector<Bonus> bonuses;

		owner->getEffects(bonuses, schoolLevel);

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
ESpellCastResult SummonBoatMechanics::applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const
{
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
	int3 summonPos = parameters.caster->bestLocation();
	if(summonPos.x < 0)
	{
		env->complain("There is no water tile available!");
		return ESpellCastResult::ERROR;
	}

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
		cop.nPos = summonPos + int3(1,0,0);;
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
		no.pos = summonPos + int3(1,0,0);;
		env->sendAndApply(&no);
	}
	return ESpellCastResult::OK;
}

///ScuttleBoatMechanics
ESpellCastResult ScuttleBoatMechanics::applyAdventureEffects(const SpellCastEnvironment* env, AdventureSpellCastParameters& parameters) const
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
ESpellCastResult DimensionDoorMechanics::applyAdventureEffects(const SpellCastEnvironment* env, AdventureSpellCastParameters& parameters) const
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
ESpellCastResult TownPortalMechanics::applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters& parameters) const
{
	if (!env->getMap()->isInTheMap(parameters.pos))
	{
		env->complain("Destination tile not present!");
		return ESpellCastResult::ERROR;
	}

	TerrainTile tile = env->getMap()->getTile(parameters.pos);
	if (tile.visitableObjects.empty() || tile.visitableObjects.back()->ID != Obj::TOWN)
	{
		env->complain("Town not found for Town Portal!");
		return ESpellCastResult::ERROR;
	}

	CGTownInstance * town = static_cast<CGTownInstance*>(tile.visitableObjects.back());

	const auto relations = env->getCb()->getPlayerRelations(town->tempOwner, parameters.caster->tempOwner);

	if(relations == PlayerRelations::ENEMIES)
	{
		env->complain("Can't teleport to enemy!");
		return ESpellCastResult::ERROR;
	}

	if (town->visitingHero)
	{
		env->complain("Can't teleport to occupied town!");
		return ESpellCastResult::ERROR;
	}

	if (parameters.caster->getSpellSchoolLevel(owner) < 2)
	{
		si32 dist = town->pos.dist2dSQ(parameters.caster->pos);
		ObjectInstanceID nearest = town->id; //nearest town's ID
		for(const CGTownInstance * currTown : env->getCb()->getPlayer(parameters.caster->tempOwner)->towns)
		{
			si32 currDist = currTown->pos.dist2dSQ(parameters.caster->pos);
			if (currDist < dist)
			{
				nearest = currTown->id;
				dist = currDist;
			}
		}
		if (town->id != nearest)
		{
			env->complain("This hero can only teleport to nearest town!");
			return ESpellCastResult::ERROR;
		}

	}

	const int movementCost = GameConstants::BASE_MOVEMENT_COST * ((parameters.caster->getSpellSchoolLevel(owner) >= 3) ? 2 : 3);

	if(parameters.caster->movement < movementCost)
	{
		env->complain("This hero has not enough movement points!");
		return ESpellCastResult::ERROR;
	}

	if(env->moveHero(parameters.caster->id, town->visitablePos() + parameters.caster->getVisitableOffset() ,1))
	{
		SetMovePoints smp;
		smp.hid = parameters.caster->id;
		smp.val = std::max<ui32>(0, parameters.caster->movement - movementCost);
		env->sendAndApply(&smp);
	}
	return ESpellCastResult::OK;
}

ESpellCastResult ViewMechanics::applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const
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

bool ViewAirMechanics::filterObject(const CGObjectInstance * obj, const int spellLevel) const
{
	return (obj->ID == Obj::ARTIFACT) || (spellLevel>1 && obj->ID == Obj::HERO) || (spellLevel>2 && obj->ID == Obj::TOWN);
}

bool ViewEarthMechanics::filterObject(const CGObjectInstance * obj, const int spellLevel) const
{
	return (obj->ID == Obj::RESOURCE) || (spellLevel>1 && obj->ID == Obj::MINE);
}


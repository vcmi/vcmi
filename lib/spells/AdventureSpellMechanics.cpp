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
#include "../BattleState.h"
#include "../CGameState.h"
#include "../CGameInfoCallback.h"

///SummonBoatMechanics
bool SummonBoatMechanics::applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters & parameters) const
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
		return true;
	}

	//try to find unoccupied boat to summon
	const CGBoat * nearest = nullptr;
	double dist = 0;
	int3 summonPos = parameters.caster->bestLocation();
	if(summonPos.x < 0)
	{
		env->complain("There is no water tile available!");
		return false;
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
	return true;	
}

///ScuttleBoatMechanics
bool ScuttleBoatMechanics::applyAdventureEffects(const SpellCastEnvironment* env, AdventureSpellCastParameters& parameters) const
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
		return true;
	}
		
	if(!env->getMap()->isInTheMap(parameters.pos))
	{
		env->complain("Invalid dst tile for scuttle!");
		return false;
	}

	//TODO: test range, visibility
	const TerrainTile *t = &env->getMap()->getTile(parameters.pos);
	if(!t->visitableObjects.size() || t->visitableObjects.back()->ID != Obj::BOAT)
	{
		env->complain("There is no boat to scuttle!");
		return false;
	}
		

	RemoveObject ro;
	ro.id = t->visitableObjects.back()->id;
	env->sendAndApply(&ro);
	return true;	
}

///DimensionDoorMechanics
bool DimensionDoorMechanics::applyAdventureEffects(const SpellCastEnvironment* env, AdventureSpellCastParameters& parameters) const
{
	if(!env->getMap()->isInTheMap(parameters.pos))
	{
		env->complain("Destination is out of map!");
		return false;
	}	
	
	const TerrainTile * dest = env->getCb()->getTile(parameters.pos);
	const TerrainTile * curr = env->getCb()->getTile(parameters.caster->getSightCenter());

	if(nullptr == dest)
	{
		env->complain("Destination tile doesn't exist!");
		return false;
	}
	
	if(nullptr == curr)
	{
		env->complain("Source tile doesn't exist!");
		return false;
	}	
		
	if(parameters.caster->movement <= 0)
	{
		env->complain("Hero needs movement points to cast Dimension Door!");
		return false;
	}
	
	const int schoolLevel = parameters.caster->getSpellSchoolLevel(owner);
			
	if(parameters.caster->getBonusesCount(Bonus::SPELL_EFFECT, SpellID::DIMENSION_DOOR) >= owner->getPower(schoolLevel)) //limit casts per turn
	{
		InfoWindow iw;
		iw.player = parameters.caster->tempOwner;
		iw.text.addTxt(MetaString::GENERAL_TXT, 338); //%s is not skilled enough to cast this spell again today.
		iw.text.addReplacement(parameters.caster->name);
		env->sendAndApply(&iw);
		return true;
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
		smp.val = std::max<ui32>(0, parameters.caster->movement - 300);
		env->sendAndApply(&smp);
	}
	return true; 	
}

///TownPortalMechanics
bool TownPortalMechanics::applyAdventureEffects(const SpellCastEnvironment * env, AdventureSpellCastParameters& parameters) const
{
	if (!env->getMap()->isInTheMap(parameters.pos))
	{
		env->complain("Destination tile not present!");
		return false;
	}	

	TerrainTile tile = env->getMap()->getTile(parameters.pos);
	if (tile.visitableObjects.empty() || tile.visitableObjects.back()->ID != Obj::TOWN)
	{
		env->complain("Town not found for Town Portal!");	
		return false;
	}		

	CGTownInstance * town = static_cast<CGTownInstance*>(tile.visitableObjects.back());
	if (town->tempOwner != parameters.caster->tempOwner)
	{
		env->complain("Can't teleport to another player!");
		return false;
	}			
	
	if (town->visitingHero)
	{
		env->complain("Can't teleport to occupied town!");
		return false;
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
			return false;
		}
			
	}
	env->moveHero(parameters.caster->id, town->visitablePos() + parameters.caster->getVisitableOffset() ,1);	
	return true;
}


bool ViewAirMechanics::applyAdventureEffects(const SpellCastEnvironment* env, AdventureSpellCastParameters& parameters) const
{
	return true; //implemented on client side
}

bool ViewEarthMechanics::applyAdventureEffects(const SpellCastEnvironment* env, AdventureSpellCastParameters& parameters) const
{
	return true; //implemented on client side
}

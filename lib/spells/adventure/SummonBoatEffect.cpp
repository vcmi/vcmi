/*
 * SummonBoatEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "SummonBoatEffect.h"

#include "../CSpellHandler.h"

#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapObjects/MiscObjects.h"
#include "../../mapping/CMap.h"
#include "../../modding/IdentifierStorage.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

SummonBoatEffect::SummonBoatEffect(const CSpell * s, const JsonNode & config)
	: owner(s)
	, useExistingBoat(config["useExistingBoat"].Bool())
{
	if (!config["createdBoat"].isNull())
	{
		LIBRARY->identifiers()->requestIdentifier("core:boat", config["createdBoat"], [=](int32_t boatTypeID)
		{
			createdBoat = BoatId(boatTypeID);
		});
	}

}

bool SummonBoatEffect::canCreateNewBoat() const
{
	return createdBoat != BoatId::NONE;
}

int SummonBoatEffect::getSuccessChance(const spells::Caster * caster) const
{
	const auto schoolLevel = caster->getSpellSchoolLevel(owner);
	return owner->getLevelPower(schoolLevel);
}

bool SummonBoatEffect::canBeCastImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster) const
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

ESpellCastResult SummonBoatEffect::applyAdventureEffects(SpellCastEnvironment * env, const AdventureSpellCastParameters & parameters) const
{
	//check if spell works at all
	if(env->getRNG()->nextInt(0, 99) >= getSuccessChance(parameters.caster)) //power is % chance of success
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

	if (useExistingBoat)
	{
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
	else if(!canCreateNewBoat()) //none or basic level -> cannot create boat :(
	{
		InfoWindow iw;
		iw.player = parameters.caster->getCasterOwner();
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 335); //There are no boats to summon.
		env->apply(iw);
		return ESpellCastResult::ERROR;
	}
	else //create boat
	{
		env->createBoat(summonPos, createdBoat, parameters.caster->getCasterOwner());
	}
	return ESpellCastResult::OK;
}

VCMI_LIB_NAMESPACE_END

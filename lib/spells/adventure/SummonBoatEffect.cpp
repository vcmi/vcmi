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

#include "../CSpell.h"

#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapObjects/MiscObjects.h"
#include "../../mapping/CMap.h"
#include "../../modding/IdentifierStorage.h"
#include "../../networkPacks/PacksForClient.h"

SummonBoatEffect::SummonBoatEffect(const CSpell * s, const JsonNode & config)
	: owner(s)
	, useExistingBoat(config["useExistingBoat"].Bool())
{
	if (!config["createdBoat"].isNull())
	{
		LIBRARY->identifiers()->requestIdentifier("core:boat", config["createdBoat"], [this](int32_t boatTypeID)
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
		message.replaceTextID(caster->getCasterNameTextID());
		problem.add(std::move(message));
		return false;
	}

	int3 summonPos = caster->getHeroCaster()->bestLocation();

	if(summonPos.x < 0)
	{
		MetaString message = MetaString::createFromTextID("core.genrltxt.334");
		message.replaceTextID(caster->getCasterNameTextID());
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
		iw.text.appendTextID("core.genrltxt.336"); //%s tried to summon a boat, but failed.
		iw.text.replaceTextID(parameters.caster->getCasterNameTextID());
		env->apply(iw);
		return ESpellCastResult::OK;
	}

	//try to find unoccupied boat to summon
	const CGBoat * nearest = nullptr;

	if (useExistingBoat)
	{
		// H3: only boats that are owned by caster or not owned by anyone can be summoned
		// the boat last used by the caster is preferred, then the nearest one in Manhattan metric (levels are ignored), the last one wins ties
		const auto * caster = parameters.caster->getHeroCaster();
		const int3 heroPos = caster->visitablePos();

		auto isCandidate = [&](const CGBoat * b)
		{
			if(b->getBoardedHero() || b->layer != EPathfindingLayer::SAIL)
				return false;
			return !b->tempOwner.isValidPlayer() || b->tempOwner == parameters.caster->getCasterOwner();
		};

		auto rank = [&](const CGBoat * b)
		{
			const int3 boatPos = b->visitablePos();
			const int dist = std::abs(boatPos.x - heroPos.x) + std::abs(boatPos.y - heroPos.y);
			return std::make_pair(b->getLastHeroID() != caster->id, dist);
		};

		const auto boats = env->getMap()->getObjects<CGBoat>();
		std::vector<const CGBoat *> candidates;
		std::copy_if(boats.begin(), boats.end(), std::back_inserter(candidates), isCandidate);

		// reversed iteration makes the last of equally ranked boats win
		auto best = std::ranges::min_element(candidates.rbegin(), candidates.rend(), [&](const CGBoat * l, const CGBoat * r)
		{
			return rank(l) < rank(r);
		});
		if(best != candidates.rend())
			nearest = *best;
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
		iw.text.appendTextID("core.genrltxt.335"); //There are no boats to summon.
		env->apply(iw);
		return ESpellCastResult::ERROR;
	}
	else //create boat
	{
		env->createBoat(summonPos, createdBoat, parameters.caster->getCasterOwner());
	}
	return ESpellCastResult::OK;
}

/*
 * ScuttleBoatMechanics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ScuttleBoatMechanics.h"

#include "../CSpellHandler.h"

#include "../../callback/IGameInfoCallback.h"
#include "../../mapObjects/CGHeroInstance.h"
#include "../../mapping/CMap.h"
#include "../../networkPacks/PacksForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

ScuttleBoatMechanics::ScuttleBoatMechanics(const CSpell * s)
	: AdventureSpellMechanics(s)
{
}

bool ScuttleBoatMechanics::canBeCastAtImpl(spells::Problem & problem, const IGameInfoCallback * cb, const spells::Caster * caster, const int3 & pos) const
{
	if(!cb->isInTheMap(pos))
		return false;

	if(caster->getHeroCaster())
	{
		int3 casterPosition = caster->getHeroCaster()->getSightCenter();

		if(!isInScreenRange(casterPosition, pos))
			return false;
	}

	if(!cb->isVisibleFor(pos, caster->getCasterOwner()))
		return false;

	const TerrainTile * t = cb->getTile(pos);
	if(!t || t->visitableObjects.empty())
		return false;

	const CGObjectInstance * topObject = cb->getObj(t->visitableObjects.back());
	if(topObject->ID != Obj::BOAT)
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

VCMI_LIB_NAMESPACE_END

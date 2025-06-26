/*
 * SideInBattle.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SideInBattle.h"

#include "../callback/IGameInfoCallback.h"
#include "../mapObjects/CGHeroInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

void SideInBattle::init(const CGHeroInstance * Hero, const CArmedInstance * Army)
{
	armyObjectID = Army->id;
	if (Hero)
		heroID = Hero->id;

	switch(Army->ID.toEnum())
	{
		case Obj::CREATURE_GENERATOR1:
		case Obj::CREATURE_GENERATOR2:
		case Obj::CREATURE_GENERATOR3:
		case Obj::CREATURE_GENERATOR4:
			color = PlayerColor::NEUTRAL;
			break;
		default:
			color = Army->getOwner();
	}

	if(color == PlayerColor::UNFLAGGABLE)
		color = PlayerColor::NEUTRAL;
}

const CArmedInstance * SideInBattle::getArmy() const
{
	if (armyObjectID.hasValue())
		return dynamic_cast<const CArmedInstance*>(cb->getObjInstance(armyObjectID));
	return nullptr;
}

const CGHeroInstance * SideInBattle::getHero() const
{
	if (heroID.hasValue())
		return cb->getHero(heroID);
	return nullptr;
}

VCMI_LIB_NAMESPACE_END

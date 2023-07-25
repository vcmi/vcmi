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
#include "../mapObjects/CArmedInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

void SideInBattle::init(const CGHeroInstance * Hero, const CArmedInstance * Army)
{
	hero = Hero;
	armyObject = Army;

	switch(armyObject->ID)
	{
		case Obj::CREATURE_GENERATOR1:
		case Obj::CREATURE_GENERATOR2:
		case Obj::CREATURE_GENERATOR3:
		case Obj::CREATURE_GENERATOR4:
			color = PlayerColor::NEUTRAL;
			break;
		default:
			color = armyObject->getOwner();
	}

	if(color == PlayerColor::UNFLAGGABLE)
		color = PlayerColor::NEUTRAL;
}

VCMI_LIB_NAMESPACE_END

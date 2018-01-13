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

SideInBattle::SideInBattle()
{
	color = PlayerColor::CANNOT_DETERMINE;
	hero = nullptr;
	armyObject = nullptr;
	castSpellsCount = 0;
	enchanterCounter = 0;
}

void SideInBattle::init(const CGHeroInstance * Hero, const CArmedInstance * Army)
{
	hero = Hero;
	armyObject = Army;
	color = armyObject->getOwner();

	if(color == PlayerColor::UNFLAGGABLE)
		color = PlayerColor::NEUTRAL;
}

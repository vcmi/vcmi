#pragma once

class CStack;

/*
 * SStackAttackedInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

/// Small struct which contains information about the id of the attacked stack, the damage dealt,...
struct SStackAttackedInfo
{
	const CStack * defender; //attacked stack
	int dmg; //damage dealt
	int amountKilled; //how many creatures in stack has been killed
	const CStack * attacker; //attacking stack
	bool byShooting; //if true, stack has been attacked by shooting
	bool killed; //if true, stack has been killed
	bool rebirth; //if true, play rebirth animation after all
};
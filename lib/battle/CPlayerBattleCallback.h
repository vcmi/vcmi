/*
 * CPlayerBattleCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "CBattleInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;

class DLL_LINKAGE CPlayerBattleCallback : public CBattleInfoCallback
{
public:
	bool battleCanFlee() const; //returns true if caller can flee from the battle
	TStacks battleGetStacks(EStackOwnership whose = MINE_AND_ENEMY, bool onlyAlive = true) const; //returns stacks on battlefield

	int battleGetSurrenderCost() const; //returns cost of surrendering battle, -1 if surrendering is not possible

	const CGHeroInstance * battleGetMyHero() const;
	InfoAboutHero battleGetEnemyHero() const;
};


VCMI_LIB_NAMESPACE_END

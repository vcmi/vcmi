/*
 * CExchangeController.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
 
#include "../lib/mapObjects/CGHeroInstance.h"
 
class CExchangeController
{
public:
	CExchangeController(ObjectInstanceID hero1, ObjectInstanceID hero2);
	void swapArmy();
	void moveArmy(bool leftToRight, std::optional<SlotID> heldSlot);
	void moveStack(bool leftToRight, SlotID sourceSlot);
	void moveSingleStackCreature(bool leftToRight, SlotID sourceSlot, bool forceEmptySlotTarget);
	void swapArtifacts(bool equipped, bool baclpack);
	void moveArtifacts(bool leftToRight, bool equipped, bool baclpack);

private:
	const CGHeroInstance * left;
	const CGHeroInstance * right;
};

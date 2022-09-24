/*
 * IUnitInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CCreature;

namespace battle
{

class Unit;

class DLL_LINKAGE IUnitEnvironment
{
public:
	virtual bool unitHasAmmoCart(const Unit * unit) const = 0; //todo: handle ammo cart with bonus system

	virtual PlayerColor unitEffectiveOwner(const Unit * unit) const = 0;
};

class DLL_LINKAGE IUnitInfo
{
public:
	virtual int32_t unitBaseAmount() const = 0;

	virtual uint32_t unitId() const = 0;
	virtual ui8 unitSide() const = 0;
	virtual PlayerColor unitOwner() const = 0;
	virtual SlotID unitSlot() const = 0;

	virtual const CCreature * unitType() const = 0;
};

}

VCMI_LIB_NAMESPACE_END

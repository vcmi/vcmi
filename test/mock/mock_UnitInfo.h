/*
 * mock_UnitInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/battle/IUnitInfo.h"

class UnitInfoMock : public battle::IUnitInfo
{
public:
	MOCK_CONST_METHOD0(unitBaseAmount, int32_t());

	MOCK_CONST_METHOD0(unitId, uint32_t());
	MOCK_CONST_METHOD0(unitSide, ui8());
	MOCK_CONST_METHOD0(unitOwner, PlayerColor());

	MOCK_CONST_METHOD0(unitSlot, SlotID());

	MOCK_CONST_METHOD0(unitType, const CCreature * ());
};


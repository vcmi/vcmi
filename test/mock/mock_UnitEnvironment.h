/*
 * mock_UnitEnvironment.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/battle/IUnitInfo.h"

class UnitEnvironmentMock : public battle::IUnitEnvironment
{
public:
	MOCK_CONST_METHOD1(unitHasAmmoCart, bool(const battle::Unit *));
	MOCK_CONST_METHOD1(unitEffectiveOwner, PlayerColor(const battle::Unit *));
};

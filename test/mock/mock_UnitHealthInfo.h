/*
 * mock_UnitHealthInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../../lib/CStack.h"

class UnitHealthInfoMock : public IUnitHealthInfo
{
public:
	MOCK_CONST_METHOD0(unitMaxHealth, int32_t());
	MOCK_CONST_METHOD0(unitBaseAmount, int32_t());
};




/*
 * mock_events_ApplyDamage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/events/ApplyDamage.h>


namespace events
{

class ApplyDamageMock : public ApplyDamage
{
public:
	MOCK_CONST_METHOD0(getInitalDamage, int64_t());
	MOCK_CONST_METHOD0(getDamage, int64_t());
	MOCK_METHOD1(setDamage, void(int64_t));
	MOCK_METHOD0(execute, void());
	MOCK_CONST_METHOD0(getTarget, const battle::Unit *());
};


}

/*
 * mock_CreatureService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/CreatureService.h>

class CreatureServiceMock : public CreatureService
{
public:
	MOCK_CONST_METHOD1(getCreature, const Creature *(const CreatureID &));
};

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
	MOCK_CONST_METHOD1(getBaseByIndex, const Entity *(const int32_t));
	MOCK_CONST_METHOD1(forEachBase, void(const std::function<void(const Entity *, bool &)> &));
	MOCK_CONST_METHOD1(getById, const Creature *(const CreatureID &));
	MOCK_CONST_METHOD1(getByIndex, const Creature *(const int32_t));
	MOCK_CONST_METHOD1(forEach, void(const std::function<void(const Creature *, bool &)> &));
};

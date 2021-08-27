/*
 * mock_spells_Spell.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/spells/Service.h>

namespace spells
{

class SpellServiceMock : public Service
{
public:
	MOCK_CONST_METHOD1(getBaseByIndex, const Entity *(const int32_t));
	MOCK_CONST_METHOD1(forEachBase, void(const std::function<void(const Entity *, bool &)> &));
	MOCK_CONST_METHOD1(getById, const Spell *(const SpellID &));
	MOCK_CONST_METHOD1(getByIndex, const Spell *(const int32_t));
	MOCK_CONST_METHOD1(forEach, void(const std::function<void(const Spell *, bool &)> &));
};

}

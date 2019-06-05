/*
 * mock_Creature.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Creature.h>

class CreatureMock : public Creature
{
public:
	MOCK_CONST_METHOD0(getIndex, int32_t());
	MOCK_CONST_METHOD0(getJsonKey, const std::string &());
	MOCK_CONST_METHOD0(getName, const std::string &());
	MOCK_CONST_METHOD0(getId, CreatureID());
	MOCK_CONST_METHOD1(registerIcons, void(const IconRegistar &));
	MOCK_CONST_METHOD0(getPluralName, const std::string &());
	MOCK_CONST_METHOD0(getSingularName, const std::string &());
	MOCK_CONST_METHOD0(getMaxHealth, uint32_t());
};

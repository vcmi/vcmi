/*
 * mock_spells_effects_Registry.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/spells/effects/Registry.h"

namespace spells
{
namespace effects
{

class EffectFactoryMock : public IEffectFactory
{
public:
	MOCK_CONST_METHOD0(create, Effect *());
};

class RegistryMock : public Registry
{
public:
	MOCK_CONST_METHOD1(find, const IEffectFactory *(const std::string &));
	MOCK_METHOD2(add, void(const std::string &, FactoryPtr));
};

}
}

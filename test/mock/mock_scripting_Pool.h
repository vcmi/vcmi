/*
 * mock_scripting_Pool.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>

namespace scripting
{

class PoolMock : public Pool
{
public:
	MOCK_METHOD1(getContext, std::shared_ptr<Context>(const Script *));
	MOCK_METHOD2(serializeState, void(const bool, JsonNode &));
};

}

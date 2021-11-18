/*
 * mock_scripting_Context.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>

#include "../../../lib/JsonNode.h"

namespace scripting
{

class ContextMock : public Context
{
public:
	MOCK_METHOD1(run, void(const JsonNode &));
	MOCK_METHOD2(run, void(ServerCallback *, const JsonNode &));

	MOCK_METHOD2(callGlobal, JsonNode(const std::string &, const JsonNode &));
	MOCK_METHOD3(callGlobal, JsonNode(ServerCallback *, const std::string &, const JsonNode &));

	MOCK_METHOD2(getGlobal, void(const std::string &, int &));
	MOCK_METHOD2(getGlobal, void(const std::string &, std::string &));
	MOCK_METHOD2(getGlobal, void(const std::string &, double &));
	MOCK_METHOD2(getGlobal, void(const std::string &, JsonNode &));

	MOCK_METHOD2(setGlobal, void(const std::string &, int));
	MOCK_METHOD2(setGlobal, void(const std::string &, const std::string &));
	MOCK_METHOD2(setGlobal, void(const std::string &, double));
	MOCK_METHOD2(setGlobal, void(const std::string &, const JsonNode &));

	MOCK_METHOD0(saveState, JsonNode());
};

}

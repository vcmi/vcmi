/*
 * SetScriptVariable.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "NetPacksBase.h"
#include "../json/JsonNode.h"

struct DLL_LINKAGE SetScriptVariable : public CPackForClient
{
	std::string scope;
	std::string name;
	JsonNode value;

	SetScriptVariable() = default;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & scope;
		h & name;
		h & value;
	}
};

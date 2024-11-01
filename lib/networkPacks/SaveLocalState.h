/*
 * SaveLocalState.h, part of VCMI engine
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

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE SaveLocalState : public CPackForServer
{
	JsonNode data;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & data;
	}
};

VCMI_LIB_NAMESPACE_END

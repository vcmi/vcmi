/*
 * CampaignRegionsHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CampaignRegions.h"

#include "../IHandlerBase.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Managed campaign region sets - "map" of campaign locations, with selectable scenarios
/// Used only for .h3c campaigns. .vmap's embed campaign regions layout in its format
class DLL_LINKAGE CampaignRegionsHandler : public IHandlerBase
{
public:
	std::vector<JsonNode> loadLegacyData() override;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	const CampaignRegions * getByIndex(int index) const
	{
		return objects.at(index).get();
	}

private:
	std::vector<std::shared_ptr<CampaignRegions>> objects;
};

VCMI_LIB_NAMESPACE_END

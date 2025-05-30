/*
 * CampaignRegions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../Point.h"
#include "../filesystem/ResourcePath.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CampaignRegions
{
	// Campaign editor
	friend class CampaignEditor;
	friend class CampaignProperties;
	friend class ScenarioProperties;

	std::string campPrefix;
	std::vector<std::string> campSuffix;
	std::string campBackground;
	int colorSuffixLength = 0;

	struct DLL_LINKAGE RegionDescription
	{
		std::string infix;
		Point pos;
		std::optional<Point> labelPos;

		template <typename Handler> void serialize(Handler &h)
		{
			h & infix;
			h & pos;
			h & labelPos;
		}

		static CampaignRegions::RegionDescription fromJson(const JsonNode & node);
		static JsonNode toJson(CampaignRegions::RegionDescription & rd);
	};

	std::vector<RegionDescription> regions;

	ImagePath getNameFor(CampaignScenarioID which, int color, const std::string & type) const;

public:
	CampaignRegions() = default;
	explicit CampaignRegions(const JsonNode & node);

	ImagePath getBackgroundName() const;
	Point getPosition(CampaignScenarioID which) const;
	std::optional<Point> getLabelPosition(CampaignScenarioID which) const;
	ImagePath getAvailableName(CampaignScenarioID which, int color) const;
	ImagePath getSelectedName(CampaignScenarioID which, int color) const;
	ImagePath getConqueredName(CampaignScenarioID which, int color) const;
	int regionsCount() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & campPrefix;
		h & colorSuffixLength;
		h & regions;
		h & campSuffix;
		h & campBackground;
	}


	static JsonNode toJson(CampaignRegions cr);
	static CampaignRegions getLegacy(int campId);
};

VCMI_LIB_NAMESPACE_END

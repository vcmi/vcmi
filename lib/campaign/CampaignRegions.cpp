/*
 * CampaignRegions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CampaignRegions.h"

#include "../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

CampaignRegions::RegionDescription CampaignRegions::RegionDescription::fromJson(const JsonNode & node)
{
	CampaignRegions::RegionDescription rd;
	rd.infix = node["infix"].String();
	rd.pos = Point(static_cast<int>(node["x"].Float()), static_cast<int>(node["y"].Float()));
	if(!node["labelPos"].isNull())
		rd.labelPos = Point(static_cast<int>(node["labelPos"]["x"].Float()), static_cast<int>(node["labelPos"]["y"].Float()));
	else
		rd.labelPos = std::nullopt;
	return rd;
}

JsonNode CampaignRegions::RegionDescription::toJson(CampaignRegions::RegionDescription & rd)
{
	JsonNode node;
	node["infix"].String() = rd.infix;
	node["x"].Float() = rd.pos.x;
	node["y"].Float() = rd.pos.y;
	if(rd.labelPos != std::nullopt)
	{
		node["labelPos"]["x"].Float() = (*rd.labelPos).x;
		node["labelPos"]["y"].Float() = (*rd.labelPos).y;
	}
	else
		node["labelPos"].clear();
	return node;
}

CampaignRegions::CampaignRegions(const JsonNode & node)
{
	campPrefix = node["prefix"].String();
	colorSuffixLength = static_cast<int>(node["colorSuffixLength"].Float());
	campSuffix = node["suffix"].isNull() ? std::vector<std::string>() : std::vector<std::string>{node["suffix"].Vector()[0].String(), node["suffix"].Vector()[1].String(), node["suffix"].Vector()[2].String()};
	campBackground = node["background"].isNull() ? "" : node["background"].String();

	for(const JsonNode & desc : node["desc"].Vector())
		regions.push_back(CampaignRegions::RegionDescription::fromJson(desc));
}

JsonNode CampaignRegions::toJson(CampaignRegions cr)
{
	JsonNode node;
	node["prefix"].String() = cr.campPrefix;
	node["colorSuffixLength"].Float() = cr.colorSuffixLength;
	if(cr.campSuffix.empty())
		node["suffix"].clear();
	else
		node["suffix"].Vector() = JsonVector{ JsonNode(cr.campSuffix[0]), JsonNode(cr.campSuffix[1]), JsonNode(cr.campSuffix[2]) };
	if(cr.campBackground.empty())
		node["background"].clear();
	else
		node["background"].String() = cr.campBackground;
	node["desc"].Vector() = JsonVector();
	for(auto & region : cr.regions)
		node["desc"].Vector().push_back(CampaignRegions::RegionDescription::toJson(region));
	return node;
}

CampaignRegions CampaignRegions::getLegacy(int campId)
{
	static std::vector<CampaignRegions> campDescriptions;
	if(campDescriptions.empty()) //read once
	{
		const JsonNode config(JsonPath::builtin("config/campaign_regions.json"));
		for(const JsonNode & campaign : config["campaign_regions"].Vector())
			campDescriptions.push_back(CampaignRegions(campaign));
	}

	return campDescriptions.at(campId);
}

ImagePath CampaignRegions::getBackgroundName() const
{
	if(campBackground.empty())
		return ImagePath::builtin(campPrefix + "_BG.BMP");
	else
		return ImagePath::builtin(campBackground);
}

Point CampaignRegions::getPosition(CampaignScenarioID which) const
{
	const auto & region = regions[which.getNum()];
	return region.pos;
}

std::optional<Point> CampaignRegions::getLabelPosition(CampaignScenarioID which) const
{
	const auto & region = regions[which.getNum()];
	return region.labelPos;
}

ImagePath CampaignRegions::getNameFor(CampaignScenarioID which, int colorIndex, const std::string & type) const
{
	const auto & region = regions[which.getNum()];

	static const std::array<std::array<std::string, 8>, 3> colors = {{
			{ "", "", "", "", "", "", "", "" },
			{ "R", "B", "N", "G", "O", "V", "T", "P" },
			{ "Re", "Bl", "Br", "Gr", "Or", "Vi", "Te", "Pi" }
		}};

	std::string color = colors[colorSuffixLength][colorIndex];

	return ImagePath::builtin(campPrefix + region.infix + "_" + type + color + ".BMP");
}

ImagePath CampaignRegions::getAvailableName(CampaignScenarioID which, int color) const
{
	if(campSuffix.empty())
		return getNameFor(which, color, "En");
	else
		return getNameFor(which, color, campSuffix[0]);
}

ImagePath CampaignRegions::getSelectedName(CampaignScenarioID which, int color) const
{
	if(campSuffix.empty())
		return getNameFor(which, color, "Se");
	else
		return getNameFor(which, color, campSuffix[1]);
}

ImagePath CampaignRegions::getConqueredName(CampaignScenarioID which, int color) const
{
	if(campSuffix.empty())
		return getNameFor(which, color, "Co");
	else
		return getNameFor(which, color, campSuffix[2]);
}

int CampaignRegions::regionsCount() const
{
	return regions.size();
}

VCMI_LIB_NAMESPACE_END

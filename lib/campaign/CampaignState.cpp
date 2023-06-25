/*
 * CCampaignHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CampaignState.h"

#include "../JsonNode.h"
#include "../filesystem/ResourceID.h"
#include "../VCMI_Lib.h"
#include "../CGeneralTextHandler.h"
#include "../mapping/CMapService.h"
#include "../mapping/CMapInfo.h"
#include "../mapping/CMap.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"

VCMI_LIB_NAMESPACE_BEGIN

void CampaignScenario::loadPreconditionRegions(ui32 regions)
{
	for (int i=0; i<32; i++) //for each bit in region. h3c however can only hold up to 16
	{
		if ( (1 << i) & regions)
			preconditionRegions.insert(static_cast<CampaignScenarioID>(i));
	}
}

CampaignRegions::RegionDescription CampaignRegions::RegionDescription::fromJson(const JsonNode & node)
{
	CampaignRegions::RegionDescription rd;
	rd.infix = node["infix"].String();
	rd.xpos = static_cast<int>(node["x"].Float());
	rd.ypos = static_cast<int>(node["y"].Float());
	return rd;
}

CampaignRegions CampaignRegions::fromJson(const JsonNode & node)
{
	CampaignRegions cr;
	cr.campPrefix = node["prefix"].String();
	cr.colorSuffixLength = static_cast<int>(node["color_suffix_length"].Float());

	for(const JsonNode & desc : node["desc"].Vector())
		cr.regions.push_back(CampaignRegions::RegionDescription::fromJson(desc));

	return cr;
}

CampaignRegions CampaignRegions::getLegacy(int campId)
{
	static std::vector<CampaignRegions> campDescriptions;
	if(campDescriptions.empty()) //read once
	{
		const JsonNode config(ResourceID("config/campaign_regions.json"));
		for(const JsonNode & campaign : config["campaign_regions"].Vector())
			campDescriptions.push_back(CampaignRegions::fromJson(campaign));
	}

	return campDescriptions.at(campId);
}


bool CampaignBonus::isBonusForHero() const
{
	return type == CampaignBonusType::SPELL ||
		   type == CampaignBonusType::MONSTER ||
		   type == CampaignBonusType::ARTIFACT ||
		   type == CampaignBonusType::SPELL_SCROLL ||
		   type == CampaignBonusType::PRIMARY_SKILL ||
		   type == CampaignBonusType::SECONDARY_SKILL;
}

void CampaignHeader::loadLegacyData(ui8 campId)
{
	campaignRegions = CampaignRegions::getLegacy(campId);
	numberOfScenarios = VLC->generaltexth->getCampaignLength(campId);
}

bool CampaignState::conquerable(CampaignScenarioID whichScenario) const
{
	//check for void scenraio
	if (!scenarios.at(whichScenario).isNotVoid())
	{
		return false;
	}

	if (scenarios.at(whichScenario).conquered)
	{
		return false;
	}
	//check preconditioned regions
	for (auto const & it : scenarios.at(whichScenario).preconditionRegions)
	{
		if (!scenarios.at(it).conquered)
			return false;
	}
	return true;
}

bool CampaignScenario::isNotVoid() const
{
	return !mapName.empty();
}

const CGHeroInstance * CampaignScenario::strongestHero(const PlayerColor & owner)
{
	std::function<bool(JsonNode & node)> isOwned = [owner](JsonNode & node)
	{
		auto * h = CampaignState::crossoverDeserialize(node);
		bool result = h->tempOwner == owner;
		vstd::clear_pointer(h);
		return result;
	};
	auto ownedHeroes = crossoverHeroes | boost::adaptors::filtered(isOwned);

	auto i = vstd::maxElementByFun(ownedHeroes, [](JsonNode & node)
	{
		auto * h = CampaignState::crossoverDeserialize(node);
		double result = h->getHeroStrength();
		vstd::clear_pointer(h);
		return result;
	});
	return i == ownedHeroes.end() ? nullptr : CampaignState::crossoverDeserialize(*i);
}

std::vector<CGHeroInstance *> CampaignScenario::getLostCrossoverHeroes()
{
	std::vector<CGHeroInstance *> lostCrossoverHeroes;
	if(conquered)
	{
		for(auto node2 : placedCrossoverHeroes)
		{
			auto * hero = CampaignState::crossoverDeserialize(node2);
			auto it = range::find_if(crossoverHeroes, [hero](JsonNode node)
			{
				auto * h = CampaignState::crossoverDeserialize(node);
				bool result = hero->subID == h->subID;
				vstd::clear_pointer(h);
				return result;
			});
			if(it == crossoverHeroes.end())
			{
				lostCrossoverHeroes.push_back(hero);
			}
		}
	}
	return lostCrossoverHeroes;
}

void CampaignState::setCurrentMapAsConquered(const std::vector<CGHeroInstance *> & heroes)
{
	scenarios.at(*currentMap).crossoverHeroes.clear();
	for(CGHeroInstance * hero : heroes)
	{
		scenarios.at(*currentMap).crossoverHeroes.push_back(crossoverSerialize(hero));
	}

	mapsConquered.push_back(*currentMap);
	mapsRemaining -= *currentMap;
	scenarios.at(*currentMap).conquered = true;
}

std::optional<CampaignBonus> CampaignState::getBonusForCurrentMap() const
{
	auto bonuses = getCurrentScenario().travelOptions.bonusesToChoose;
	assert(chosenCampaignBonuses.count(*currentMap) || bonuses.size() == 0);

	if(bonuses.empty())
		return std::optional<CampaignBonus>();
	else
		return bonuses[currentBonusID()];
}

const CampaignScenario & CampaignState::getCurrentScenario() const
{
	return scenarios.at(*currentMap);
}

CampaignScenario & CampaignState::getCurrentScenario()
{
	return scenarios.at(*currentMap);
}

ui8 CampaignState::currentBonusID() const
{
	return chosenCampaignBonuses.at(*currentMap);
}

std::unique_ptr<CMap> CampaignState::getMap(CampaignScenarioID scenarioId) const
{
	// FIXME: there is certainly better way to handle maps inside campaigns
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	CMapService mapService;
	std::string scenarioName = header.filename.substr(0, header.filename.find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(static_cast<int>(scenarioId));
	const std::string & mapContent = mapPieces.find(scenarioId)->second;
	const auto * buffer = reinterpret_cast<const ui8 *>(mapContent.data());
	return mapService.loadMap(buffer, static_cast<int>(mapContent.size()), scenarioName, header.modName, header.encoding);
}

std::unique_ptr<CMapHeader> CampaignState::getMapHeader(CampaignScenarioID scenarioId) const
{
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	CMapService mapService;
	std::string scenarioName = header.filename.substr(0, header.filename.find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(static_cast<int>(scenarioId));
	const std::string & mapContent = mapPieces.find(scenarioId)->second;
	const auto * buffer = reinterpret_cast<const ui8 *>(mapContent.data());
	return mapService.loadMapHeader(buffer, static_cast<int>(mapContent.size()), scenarioName, header.modName, header.encoding);
}

std::shared_ptr<CMapInfo> CampaignState::getMapInfo(CampaignScenarioID scenarioId) const
{
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->fileURI = header.filename;
	mapInfo->mapHeader = getMapHeader(scenarioId);
	mapInfo->countPlayers();
	return mapInfo;
}

JsonNode CampaignState::crossoverSerialize(CGHeroInstance * hero)
{
	JsonNode node;
	JsonSerializer handler(nullptr, node);
	hero->serializeJsonOptions(handler);
	return node;
}

CGHeroInstance * CampaignState::crossoverDeserialize(JsonNode & node)
{
	JsonDeserializer handler(nullptr, node);
	auto * hero = new CGHeroInstance();
	hero->ID = Obj::HERO;
	hero->serializeJsonOptions(handler);
	return hero;
}

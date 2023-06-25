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

bool CampaignHeader::playerSelectedDifficulty() const
{
	return difficultyChoosenByPlayer;
}

bool CampaignHeader::formatVCMI() const
{
	return version == CampaignVersion::VCMI;
}

std::string CampaignHeader::getDescription() const
{
	return description;
}

std::string CampaignHeader::getName() const
{
	return name;
}

std::string CampaignHeader::getFilename() const
{
	return filename;
}

const CampaignRegions & CampaignHeader::getRegions() const
{
	return campaignRegions;
}

bool CampaignState::isConquered(CampaignScenarioID whichScenario) const
{
	return vstd::contains(mapsConquered, whichScenario);
}

bool CampaignState::isAvailable(CampaignScenarioID whichScenario) const
{
	//check for void scenraio
	if (!scenario(whichScenario).isNotVoid())
	{
		return false;
	}

	if (vstd::contains(mapsConquered, whichScenario))
	{
		return false;
	}
	//check preconditioned regions
	for (auto const & it : scenario(whichScenario).preconditionRegions)
	{
		if (!vstd::contains(mapsConquered, it))
			return false;
	}
	return true;
}

bool CampaignScenario::isNotVoid() const
{
	return !mapName.empty();
}

const CGHeroInstance * CampaignState::strongestHero(CampaignScenarioID scenarioId, const PlayerColor & owner) const
{
	std::function<bool(const JsonNode & node)> isOwned = [owner](const JsonNode & node)
	{
		auto * h = CampaignState::crossoverDeserialize(node);
		bool result = h->tempOwner == owner;
		vstd::clear_pointer(h);
		return result;
	};
	auto ownedHeroes = crossover.placedHeroes.at(scenarioId) | boost::adaptors::filtered(isOwned);

	auto i = vstd::maxElementByFun(ownedHeroes, [](const JsonNode & node)
	{
		auto * h = CampaignState::crossoverDeserialize(node);
		double result = h->getHeroStrength();
		vstd::clear_pointer(h);
		return result;
	});
	return i == ownedHeroes.end() ? nullptr : CampaignState::crossoverDeserialize(*i);
}

std::vector<CGHeroInstance *> CampaignState::getLostCrossoverHeroes(CampaignScenarioID scenarioId) const
{
	std::vector<CGHeroInstance *> lostCrossoverHeroes;

	for(auto node2 :  crossover.placedHeroes.at(scenarioId))
	{
		auto * hero = CampaignState::crossoverDeserialize(node2);
		auto it = range::find_if(crossover.crossoverHeroes.at(scenarioId), [hero](JsonNode node)
		{
				  auto * h = CampaignState::crossoverDeserialize(node);
				  bool result = hero->subID == h->subID;
				  vstd::clear_pointer(h);
				  return result;
	});
		if(it == crossover.crossoverHeroes.at(scenarioId).end())
		{
			lostCrossoverHeroes.push_back(hero);
		}
	}

	return lostCrossoverHeroes;
}

std::vector<JsonNode> CampaignState::getCrossoverHeroes(CampaignScenarioID scenarioId) const
{
	return crossover.crossoverHeroes.at(scenarioId);
}

void CampaignState::setCurrentMapAsConquered(const std::vector<CGHeroInstance *> & heroes)
{
	crossover.crossoverHeroes[*currentMap].clear();
	for(CGHeroInstance * hero : heroes)
	{
		crossover.crossoverHeroes[*currentMap].push_back(crossoverSerialize(hero));
	}

	mapsConquered.push_back(*currentMap);
}

std::optional<CampaignBonus> CampaignState::getBonus(CampaignScenarioID which) const
{
	auto bonuses = scenario(which).travelOptions.bonusesToChoose;
	assert(chosenCampaignBonuses.count(*currentMap) || bonuses.size() == 0);

	if(bonuses.empty())
		return std::optional<CampaignBonus>();

	if (!getBonusID(which))
		return std::optional<CampaignBonus>();

	return bonuses[getBonusID(which).value()];
}

std::optional<ui8> CampaignState::getBonusID(CampaignScenarioID which) const
{
	if (!chosenCampaignBonuses.count(which))
		return std::nullopt;

	return chosenCampaignBonuses.at(which);
}

std::unique_ptr<CMap> CampaignState::getMap(CampaignScenarioID scenarioId) const
{
	// FIXME: there is certainly better way to handle maps inside campaigns
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	CMapService mapService;
	std::string scenarioName = filename.substr(0, filename.find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(static_cast<int>(scenarioId));
	const std::string & mapContent = mapPieces.find(scenarioId)->second;
	const auto * buffer = reinterpret_cast<const ui8 *>(mapContent.data());
	return mapService.loadMap(buffer, static_cast<int>(mapContent.size()), scenarioName, modName, encoding);
}

std::unique_ptr<CMapHeader> CampaignState::getMapHeader(CampaignScenarioID scenarioId) const
{
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	CMapService mapService;
	std::string scenarioName = filename.substr(0, filename.find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(static_cast<int>(scenarioId));
	const std::string & mapContent = mapPieces.find(scenarioId)->second;
	const auto * buffer = reinterpret_cast<const ui8 *>(mapContent.data());
	return mapService.loadMapHeader(buffer, static_cast<int>(mapContent.size()), scenarioName, modName, encoding);
}

std::shared_ptr<CMapInfo> CampaignState::getMapInfo(CampaignScenarioID scenarioId) const
{
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->fileURI = filename;
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

CGHeroInstance * CampaignState::crossoverDeserialize(const JsonNode & node)
{
	JsonDeserializer handler(nullptr, const_cast<JsonNode&>(node));
	auto * hero = new CGHeroInstance();
	hero->ID = Obj::HERO;
	hero->serializeJsonOptions(handler);
	return hero;
}

void CampaignState::setCurrentMap(CampaignScenarioID which)
{
	assert(scenario(which).isNotVoid());

	currentMap = which;
}

void CampaignState::setCurrentMapBonus(ui8 which)
{
	chosenCampaignBonuses[*currentMap] = which;
}

std::optional<CampaignScenarioID> CampaignState::currentScenario() const
{
	return currentMap;
}

std::optional<CampaignScenarioID> CampaignState::lastScenario() const
{
	if (mapsConquered.empty())
		return std::nullopt;
	return mapsConquered.back();
}

std::set<CampaignScenarioID> CampaignState::conqueredScenarios() const
{
	std::set<CampaignScenarioID> result;
	result.insert(mapsConquered.begin(), mapsConquered.end());
	return result;
}

std::set<CampaignScenarioID> Campaign::allScenarios() const
{
	std::set<CampaignScenarioID> result;

	for (auto const & entry : scenarios)
	{
		if (entry.second.isNotVoid())
			result.insert(entry.first);
	}

	return result;
}

int Campaign::scenariosCount() const
{
	return allScenarios().size();
}

const CampaignScenario & Campaign::scenario(CampaignScenarioID which) const
{
	assert(scenarios.count(which));
	assert(scenarios.at(which).isNotVoid());

	return scenarios.at(which);
}

bool CampaignState::isCampaignFinished() const
{
	return conqueredScenarios() == allScenarios();
}

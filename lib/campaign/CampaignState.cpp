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

std::string CampaignRegions::getBackgroundName() const
{
	return campPrefix + "_BG.BMP";
}

Point CampaignRegions::getPosition(CampaignScenarioID which) const
{
	auto const & region = regions[static_cast<int>(which)];
	return Point(region.xpos, region.ypos);
}

std::string CampaignRegions::getNameFor(CampaignScenarioID which, int colorIndex, std::string type) const
{
	auto const & region = regions[static_cast<int>(which)];

	static const std::string colors[2][8] =
	{
		{"R", "B", "N", "G", "O", "V", "T", "P"},
		{"Re", "Bl", "Br", "Gr", "Or", "Vi", "Te", "Pi"}
	};

	std::string color = colors[colorSuffixLength - 1][colorIndex];

	return campPrefix + region.infix + "_" + type + color + ".BMP";
}

std::string CampaignRegions::getAvailableName(CampaignScenarioID which, int color) const
{
	return getNameFor(which, color, "En");
}

std::string CampaignRegions::getSelectedName(CampaignScenarioID which, int color) const
{
	return getNameFor(which, color, "Se");
}

std::string CampaignRegions::getConqueredName(CampaignScenarioID which, int color) const
{
	return getNameFor(which, color, "Co");
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

std::string CampaignHeader::getModName() const
{
	return modName;
}

std::string CampaignHeader::getEncoding() const
{
	return encoding;
}

std::string CampaignHeader::getMusic() const
{
	return music;
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

std::set<HeroTypeID> CampaignState::getReservedHeroes() const
{
	std::set<HeroTypeID> result;

	for (auto const & scenarioID : allScenarios())
	{
		if (isConquered(scenarioID))
			continue;

		auto header = getMapHeader(scenarioID);

		result.insert(header->reservedCampaignHeroes.begin(), header->reservedCampaignHeroes.end());
	}

	return result;
}

const CGHeroInstance * CampaignState::strongestHero(CampaignScenarioID scenarioId, const PlayerColor & owner) const
{
	std::function<bool(const JsonNode & node)> isOwned = [owner](const JsonNode & node)
	{
		auto * h = CampaignState::crossoverDeserialize(node, nullptr);
		bool result = h->tempOwner == owner;
		vstd::clear_pointer(h);
		return result;
	};
	auto ownedHeroes = scenarioHeroPool.at(scenarioId) | boost::adaptors::filtered(isOwned);

	if (ownedHeroes.empty())
		return nullptr;

	return CampaignState::crossoverDeserialize(ownedHeroes.front(), nullptr);
}

/// Returns heroes that can be instantiated as hero placeholders by power
const std::vector<JsonNode> & CampaignState::getHeroesByPower(CampaignScenarioID scenarioId) const
{
	static const std::vector<JsonNode> emptyVector;

	if (scenarioHeroPool.count(scenarioId))
		return scenarioHeroPool.at(scenarioId);

	return emptyVector;
}

/// Returns hero for instantiation as placeholder by type
/// May return empty JsonNode if such hero was not found
const JsonNode & CampaignState::getHeroByType(HeroTypeID heroID) const
{
	static const JsonNode emptyNode;

	if (!getReservedHeroes().count(heroID))
		return emptyNode;

	if (!globalHeroPool.count(heroID))
		return emptyNode;

	return globalHeroPool.at(heroID);
}

void CampaignState::setCurrentMapAsConquered(std::vector<CGHeroInstance *> heroes)
{
	range::sort(heroes, [](const CGHeroInstance * a, const CGHeroInstance * b)
	{
		return a->getHeroStrength() > b->getHeroStrength();
	});

	logGlobal->info("Scenario %d of campaign %s (%s) has been completed", static_cast<int>(*currentMap), getFilename(), getName());

	mapsConquered.push_back(*currentMap);
	auto reservedHeroes = getReservedHeroes();

	for (auto * hero : heroes)
	{
		HeroTypeID heroType(hero->subID);
		JsonNode node = CampaignState::crossoverSerialize(hero);

		if (reservedHeroes.count(heroType))
		{
			logGlobal->info("Hero crossover: %d (%s) exported to global pool", hero->subID, hero->getNameTranslated());
			globalHeroPool[heroType] = node;
		}
		else
		{
			logGlobal->info("Hero crossover: %d (%s) exported to scenario pool", hero->subID, hero->getNameTranslated());
			scenarioHeroPool[*currentMap].push_back(node);
		}
	}
}

std::optional<CampaignBonus> CampaignState::getBonus(CampaignScenarioID which) const
{
	auto bonuses = scenario(which).travelOptions.bonusesToChoose;
	assert(chosenCampaignBonuses.count(*currentMap) || bonuses.empty());

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
	std::string scenarioName = getFilename().substr(0, getFilename().find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(static_cast<int>(scenarioId));
	const auto & mapContent = mapPieces.find(scenarioId)->second;
	return mapService.loadMap(mapContent.data(), mapContent.size(), scenarioName, getModName(), getEncoding());
}

std::unique_ptr<CMapHeader> CampaignState::getMapHeader(CampaignScenarioID scenarioId) const
{
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	CMapService mapService;
	std::string scenarioName = getFilename().substr(0, getFilename().find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(static_cast<int>(scenarioId));
	const auto & mapContent = mapPieces.find(scenarioId)->second;
	return mapService.loadMapHeader(mapContent.data(), mapContent.size(), scenarioName, getModName(), getEncoding());
}

std::shared_ptr<CMapInfo> CampaignState::getMapInfo(CampaignScenarioID scenarioId) const
{
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->fileURI = getFilename();
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

CGHeroInstance * CampaignState::crossoverDeserialize(const JsonNode & node, CMap * map)
{
	JsonDeserializer handler(nullptr, const_cast<JsonNode&>(node));
	auto * hero = new CGHeroInstance();
	hero->ID = Obj::HERO;
	hero->serializeJsonOptions(handler);
	if (map)
		hero->serializeJsonArtifacts(handler, "artifacts", map);
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

VCMI_LIB_NAMESPACE_END

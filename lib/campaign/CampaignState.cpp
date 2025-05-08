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

#include "../Point.h"
#include "../filesystem/ResourcePath.h"
#include "../GameLibrary.h"
#include "../texts/CGeneralTextHandler.h"
#include "../mapping/CMapService.h"
#include "../mapping/CMapInfo.h"
#include "../mapping/CMap.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonSerializer.h"
#include "../json/JsonUtils.h"

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

CampaignRegions CampaignRegions::fromJson(const JsonNode & node)
{
	CampaignRegions cr;
	cr.campPrefix = node["prefix"].String();
	cr.colorSuffixLength = static_cast<int>(node["colorSuffixLength"].Float());
	cr.campSuffix = node["suffix"].isNull() ? std::vector<std::string>() : std::vector<std::string>{node["suffix"].Vector()[0].String(), node["suffix"].Vector()[1].String(), node["suffix"].Vector()[2].String()};
	cr.campBackground = node["background"].isNull() ? "" : node["background"].String();

	for(const JsonNode & desc : node["desc"].Vector())
		cr.regions.push_back(CampaignRegions::RegionDescription::fromJson(desc));

	return cr;
}

JsonNode CampaignRegions::toJson(CampaignRegions cr)
{
	JsonNode node;
	node["prefix"].String() = cr.campPrefix;
	node["colorSuffixLength"].Float() = cr.colorSuffixLength;
	if(!cr.campSuffix.size())
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
			campDescriptions.push_back(CampaignRegions::fromJson(campaign));
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
	auto const & region = regions[which.getNum()];
	return region.pos;
}

std::optional<Point> CampaignRegions::getLabelPosition(CampaignScenarioID which) const
{
	auto const & region = regions[which.getNum()];
	return region.labelPos;
}

ImagePath CampaignRegions::getNameFor(CampaignScenarioID which, int colorIndex, std::string type) const
{
	auto const & region = regions[which.getNum()];

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
	numberOfScenarios = LIBRARY->generaltexth->getCampaignLength(campId);
}

void CampaignHeader::loadLegacyData(CampaignRegions regions, int numOfScenario)
{
	campaignRegions = regions;
	numberOfScenarios = numOfScenario;
}

bool CampaignHeader::playerSelectedDifficulty() const
{
	return difficultyChosenByPlayer;
}

bool CampaignHeader::formatVCMI() const
{
	return version == CampaignVersion::VCMI;
}

std::string CampaignHeader::getDescriptionTranslated() const
{
	return description.toString();
}

std::string CampaignHeader::getNameTranslated() const
{
	return name.toString();
}

std::string CampaignHeader::getAuthor() const
{
	return authorContact.toString();
}

std::string CampaignHeader::getAuthorContact() const
{
	return authorContact.toString();
}

std::string CampaignHeader::getCampaignVersion() const
{
	return campaignVersion.toString();
}

time_t CampaignHeader::getCreationDateTime() const
{
	return creationDateTime;
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

AudioPath CampaignHeader::getMusic() const
{
	return music;
}

ImagePath CampaignHeader::getLoadingBackground() const
{
	return loadingBackground;
}

ImagePath CampaignHeader::getVideoRim() const
{
	return videoRim;
}

VideoPath CampaignHeader::getIntroVideo() const
{
	return introVideo;
}

VideoPath CampaignHeader::getOutroVideo() const
{
	return outroVideo;
}

const CampaignRegions & CampaignHeader::getRegions() const
{
	return campaignRegions;
}

TextContainerRegistrable & CampaignHeader::getTexts()
{
	return textContainer;
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

std::shared_ptr<CGHeroInstance> CampaignState::strongestHero(CampaignScenarioID scenarioId, const PlayerColor & owner) const
{
	std::function<bool(const JsonNode & node)> isOwned = [&](const JsonNode & node)
	{
		auto h = CampaignState::crossoverDeserialize(node, nullptr);
		bool result = h->tempOwner == owner;
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
	boost::range::sort(heroes, [](const CGHeroInstance * a, const CGHeroInstance * b)
	{
		return a->getValueForCampaign() > b->getValueForCampaign();
	});

	logGlobal->info("Scenario %d of campaign %s (%s) has been completed", currentMap->getNum(), getFilename(), getNameTranslated());

	mapsConquered.push_back(*currentMap);
	auto reservedHeroes = getReservedHeroes();

	for (auto * hero : heroes)
	{
		JsonNode node = CampaignState::crossoverSerialize(hero);

		if (reservedHeroes.count(hero->getHeroTypeID()))
		{
			logGlobal->info("Hero crossover: %d (%s) exported to global pool", hero->getHeroTypeID(), hero->getNameTranslated());
			globalHeroPool[hero->getHeroTypeID()] = node;
		}
		else
		{
			logGlobal->info("Hero crossover: %d (%s) exported to scenario pool", hero->getHeroTypeID(), hero->getNameTranslated());
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

std::unique_ptr<CMap> CampaignState::getMap(CampaignScenarioID scenarioId, IGameCallback * cb)
{
	// FIXME: there is certainly better way to handle maps inside campaigns
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	CMapService mapService;
	std::string scenarioName = getFilename().substr(0, getFilename().find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(scenarioId.getNum());

	if(!mapPieces.count(scenarioId))
		return nullptr;

	const auto & mapContent = mapPieces.find(scenarioId)->second;
	auto result = mapService.loadMap(mapContent.data(), mapContent.size(), scenarioName, getModName(), getEncoding(), cb);

	mapTranslations[scenarioId] = result->texts;
	return result;
}

std::unique_ptr<CMapHeader> CampaignState::getMapHeader(CampaignScenarioID scenarioId) const
{
	if(scenarioId == CampaignScenarioID::NONE)
		scenarioId = currentMap.value();

	CMapService mapService;
	std::string scenarioName = getFilename().substr(0, getFilename().find('.'));
	boost::to_lower(scenarioName);
	scenarioName += ':' + std::to_string(scenarioId.getNum());
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

JsonNode CampaignState::crossoverSerialize(CGHeroInstance * hero) const
{
	JsonNode node;
	JsonSerializer handler(nullptr, node);
	hero->serializeJsonOptions(handler);
	return node;
}

std::shared_ptr<CGHeroInstance> CampaignState::crossoverDeserialize(const JsonNode & node, CMap * map) const
{
	JsonDeserializer handler(nullptr, const_cast<JsonNode&>(node));
	auto hero = std::make_shared<CGHeroInstance>(map ? map->cb : nullptr);
	hero->ID = Obj::HERO;
	hero->serializeJsonOptions(handler);
	if (map)
	{
		hero->serializeJsonArtifacts(handler, "artifacts", map);
	}
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

void Campaign::overrideCampaign()
{
	const JsonNode node = JsonUtils::assembleFromFiles("config/campaignOverrides.json");
	for (auto & entry : node.Struct())
		if(filename == entry.first)
		{
			if(!entry.second["regions"].isNull() && !entry.second["scenarioCount"].isNull())
				loadLegacyData(CampaignRegions::fromJson(entry.second["regions"]), entry.second["scenarioCount"].Integer());
			if(!entry.second["loadingBackground"].isNull())
				loadingBackground = ImagePath::builtin(entry.second["loadingBackground"].String());
			if(!entry.second["videoRim"].isNull())
				videoRim = ImagePath::builtin(entry.second["videoRim"].String());
			if(!entry.second["introVideo"].isNull())
				introVideo = VideoPath::builtin(entry.second["introVideo"].String());
			if(!entry.second["outroVideo"].isNull())
				outroVideo = VideoPath::builtin(entry.second["outroVideo"].String());
		}
}

void Campaign::overrideCampaignScenarios()
{
	const JsonNode node = JsonUtils::assembleFromFiles("config/campaignOverrides.json");
	for (auto & entry : node.Struct())
		if(filename == entry.first)
		{
			if(!entry.second["scenarios"].isNull())
			{
				auto sc = entry.second["scenarios"].Vector();
				for(int i = 0; i < sc.size(); i++)
				{
					auto it = scenarios.begin();
					std::advance(it, i);
					if(!sc.at(i)["voiceProlog"].isNull())
						it->second.prolog.prologVoice = AudioPath::builtin(sc.at(i)["voiceProlog"].String());
					if(!sc.at(i)["voiceEpilog"].isNull())
						it->second.epilog.prologVoice = AudioPath::builtin(sc.at(i)["voiceEpilog"].String());
				}
			}
		}
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

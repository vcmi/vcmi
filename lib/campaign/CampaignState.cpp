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
#include "../mapping/MapFormatSettings.h"
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

bool CampaignHeader::playerSelectedDifficulty() const
{
	return difficultyChosenByPlayer;
}

CampaignVersion CampaignHeader::getFormat() const
{
	return version;
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
		return CGHeroInstance::compareCampaignValue(a, b);
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

std::unique_ptr<CMap> CampaignState::getMap(CampaignScenarioID scenarioId, IGameInfoCallback * cb)
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
	const JsonNode & overrides = LIBRARY->mapFormat->campaignOverrides(filename);

	if(!overrides["regions"].isNull())
		campaignRegions = CampaignRegions(overrides["regions"]);
	if (!overrides["scenarioCount"].isNull())
		numberOfScenarios = overrides["scenarioCount"].Integer();
	if(!overrides["loadingBackground"].isNull())
		loadingBackground = ImagePath::builtin(overrides["loadingBackground"].String());
	if(!overrides["videoRim"].isNull())
		videoRim = ImagePath::builtin(overrides["videoRim"].String());
	if(!overrides["introVideo"].isNull())
		introVideo = VideoPath::builtin(overrides["introVideo"].String());
	if(!overrides["outroVideo"].isNull())
		outroVideo = VideoPath::builtin(overrides["outroVideo"].String());
	if(!overrides["heroGemSorceress"].isNull())
		gemSorceressID = HeroTypeID(*LIBRARY->identifiersHandler->getIdentifier("hero", overrides["heroGemSorceress"]));
	if(!overrides["heroYogWizard"].isNull())
		yogWizardID = HeroTypeID(*LIBRARY->identifiersHandler->getIdentifier("hero", overrides["heroYogWizard"]));

	restrictGarrisonsAI	= overrides["restrictedGarrisonsForAI"].Bool();
}

void Campaign::overrideCampaignScenarios()
{
	const JsonNode & overrides = LIBRARY->mapFormat->campaignOverrides(filename);

	if(!overrides["scenarios"].isNull())
	{
		auto sc = overrides["scenarios"].Vector();
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

HeroTypeID CampaignHeader::getYogWizardID() const
{
	return yogWizardID;
}
HeroTypeID CampaignHeader::getGemSorceressID() const
{
	return gemSorceressID;
}
bool CampaignHeader::restrictedGarrisonsForAI() const
{
	return restrictGarrisonsAI;
}

VCMI_LIB_NAMESPACE_END

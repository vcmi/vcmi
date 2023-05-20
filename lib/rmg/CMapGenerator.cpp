/*
 * CMapGenerator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMapGenerator.h"

#include "../mapping/CMap.h"
#include "../VCMI_Lib.h"
#include "../CGeneralTextHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../CTownHandler.h"
#include "../StringConstants.h"
#include "../filesystem/Filesystem.h"
#include "CZonePlacer.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "TileInfo.h"
#include "Zone.h"
#include "Functions.h"
#include "RmgMap.h"
#include "threadpool/ThreadPool.h"
#include "modificators/ObjectManager.h"
#include "modificators/TreasurePlacer.h"
#include "modificators/RoadPlacer.h"

VCMI_LIB_NAMESPACE_BEGIN

CMapGenerator::CMapGenerator(CMapGenOptions& mapGenOptions, int RandomSeed) :
	mapGenOptions(mapGenOptions), randomSeed(RandomSeed),
	allowedPrisons(0), monolithIndex(0)
{
	loadConfig();
	rand.setSeed(this->randomSeed);
	mapGenOptions.finalize(rand);
	map = std::make_unique<RmgMap>(mapGenOptions);
	placer = std::make_shared<CZonePlacer>(*map);
}

int CMapGenerator::getRandomSeed() const
{
	return randomSeed;
}

void CMapGenerator::loadConfig()
{
	static const ResourceID path("config/randomMap.json");
	JsonNode randomMapJson(path);

	config.shipyardGuard = randomMapJson["waterZone"]["shipyard"]["value"].Integer();
	for(auto & treasure : randomMapJson["waterZone"]["treasure"].Vector())
	{
		config.waterTreasure.emplace_back(treasure["min"].Integer(), treasure["max"].Integer(), treasure["density"].Integer());
	}
	config.mineExtraResources = randomMapJson["mines"]["extraResourcesLimit"].Integer();
	config.minGuardStrength = randomMapJson["minGuardStrength"].Integer();
	config.defaultRoadType = randomMapJson["defaultRoadType"].String();
	config.secondaryRoadType = randomMapJson["secondaryRoadType"].String();
	config.treasureValueLimit = randomMapJson["treasureValueLimit"].Integer();
	for(auto & i : randomMapJson["prisons"]["experience"].Vector())
		config.prisonExperience.push_back(i.Integer());
	for(auto & i : randomMapJson["prisons"]["value"].Vector())
		config.prisonValues.push_back(i.Integer());
	for(auto & i : randomMapJson["scrolls"]["value"].Vector())
		config.scrollValues.push_back(i.Integer());
	for(auto & i : randomMapJson["pandoras"]["creaturesValue"].Vector())
		config.pandoraCreatureValues.push_back(i.Integer());
	for(auto & i : randomMapJson["quests"]["value"].Vector())
		config.questValues.push_back(i.Integer());
	for(auto & i : randomMapJson["quests"]["rewardValue"].Vector())
		config.questRewardValues.push_back(i.Integer());
	config.pandoraMultiplierGold = randomMapJson["pandoras"]["valueMultiplierGold"].Integer();
	config.pandoraMultiplierExperience = randomMapJson["pandoras"]["valueMultiplierExperience"].Integer();
	config.pandoraMultiplierSpells = randomMapJson["pandoras"]["valueMultiplierSpells"].Integer();
	config.pandoraSpellSchool = randomMapJson["pandoras"]["valueSpellSchool"].Integer();
	config.pandoraSpell60 = randomMapJson["pandoras"]["valueSpell60"].Integer();
	//override config with game options
	if(!mapGenOptions.isRoadEnabled(config.secondaryRoadType))
		config.secondaryRoadType = "";
	if(!mapGenOptions.isRoadEnabled(config.defaultRoadType))
		config.defaultRoadType = config.secondaryRoadType;
}

const CMapGenerator::Config & CMapGenerator::getConfig() const
{
	return config;
}

//must be instantiated in .cpp file for access to complete types of all member fields
CMapGenerator::~CMapGenerator() = default;

const CMapGenOptions& CMapGenerator::getMapGenOptions() const
{
	return mapGenOptions;
}

void CMapGenerator::initPrisonsRemaining()
{
	allowedPrisons = 0;
	for (auto isAllowed : map->getMap(this).allowedHeroes)
	{
		if (isAllowed)
			allowedPrisons++;
	}
	allowedPrisons = std::max<int> (0, allowedPrisons - 16 * mapGenOptions.getPlayerCount()); //so at least 16 heroes will be available for every player
}

void CMapGenerator::initQuestArtsRemaining()
{
	//TODO: Move to QuestArtifactPlacer?
	for (auto art : VLC->arth->objects)
	{
		//Don't use parts of combined artifacts
		if (art->aClass == CArtifact::ART_TREASURE && VLC->arth->legalArtifact(art->getId()) && art->constituentOf.empty())
			questArtifacts.push_back(art->getId());
	}
}

std::unique_ptr<CMap> CMapGenerator::generate()
{
	Load::Progress::reset();
	Load::Progress::setupStepsTill(5, 30);
	try
	{
		addHeaderInfo();
		map->initTiles(*this, rand);
		Load::Progress::step();
		initPrisonsRemaining();
		initQuestArtsRemaining();
		genZones();
		Load::Progress::step();
		map->getMap(this).calculateGuardingGreaturePositions(); //clear map so that all tiles are unguarded
		map->addModificators();
		Load::Progress::step(3);
		fillZones();
		//updated guarded tiles will be calculated in CGameState::initMapObjects()
		map->getZones().clear();
	}
	catch (rmgException &e)
	{
		logGlobal->error("Random map generation received exception: %s", e.what());
	}
	Load::Progress::finish();
	return std::move(map->mapInstance);
}

std::string CMapGenerator::getMapDescription() const
{
	assert(map);

	const std::string waterContentStr[3] = { "none", "normal", "islands" };
	const std::string monsterStrengthStr[3] = { "weak", "normal", "strong" };

	int monsterStrengthIndex = mapGenOptions.getMonsterStrength() - EMonsterStrength::GLOBAL_WEAK; //does not start from 0
	const auto * mapTemplate = mapGenOptions.getMapTemplate();

	if(!mapTemplate)
		throw rmgException("Map template for Random Map Generator is not found. Could not start the game.");

    std::stringstream ss;
    ss << boost::str(boost::format(std::string("Map created by the Random Map Generator.\nTemplate was %s, Random seed was %d, size %dx%d") +
        ", levels %d, players %d, computers %d, water %s, monster %s, VCMI map") % mapTemplate->getName() %
		randomSeed % map->width() % map->height() % static_cast<int>(map->levels()) % static_cast<int>(mapGenOptions.getPlayerCount()) %
		static_cast<int>(mapGenOptions.getCompOnlyPlayerCount()) % waterContentStr[mapGenOptions.getWaterContent()] %
		monsterStrengthStr[monsterStrengthIndex]);

	for(const auto & pair : mapGenOptions.getPlayersSettings())
	{
		const auto & pSettings = pair.second;
		if(pSettings.getPlayerType() == EPlayerType::HUMAN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor().getNum()] << " is human";
		}
		if(pSettings.getStartingTown() != CMapGenOptions::CPlayerSettings::RANDOM_TOWN)
		{
			ss << ", " << GameConstants::PLAYER_COLOR_NAMES[pSettings.getColor().getNum()]
			   << " town choice is " << (*VLC->townh)[pSettings.getStartingTown()]->getNameTranslated();
		}
	}

	return ss.str();
}

void CMapGenerator::addPlayerInfo()
{
	// Calculate which team numbers exist

	enum ETeams {CPHUMAN = 0, CPUONLY = 1, AFTER_LAST = 2};
	std::array<std::list<int>, 2> teamNumbers;
	std::set<int> teamsTotal;

	int teamOffset = 0;
	int playerCount = 0;
	int teamCount = 0;

	for (int i = CPHUMAN; i < AFTER_LAST; ++i)
	{
		if (i == CPHUMAN)
		{
			playerCount = mapGenOptions.getPlayerCount();
			teamCount = mapGenOptions.getTeamCount();
		}
		else
		{
			playerCount = mapGenOptions.getCompOnlyPlayerCount();
			teamCount = mapGenOptions.getCompOnlyTeamCount();
		}

		if(playerCount == 0)
		{
			continue;
		}
		int playersPerTeam = playerCount / (teamCount == 0 ? playerCount : teamCount);
		int teamCountNorm = teamCount;
		if(teamCountNorm == 0)
		{
			teamCountNorm = playerCount;
		}
		for(int j = 0; j < teamCountNorm; ++j)
		{
			for(int k = 0; k < playersPerTeam; ++k)
			{
				teamNumbers[i].push_back(j + teamOffset);
			}
		}
		for(int j = 0; j < playerCount - teamCountNorm * playersPerTeam; ++j)
		{
			teamNumbers[i].push_back(j + teamOffset);
		}
		teamOffset += teamCountNorm;
	}

	// Team numbers are assigned randomly to every player
	//TODO: allow customize teams in rmg template
	for(const auto & pair : mapGenOptions.getPlayersSettings())
	{
		const auto & pSettings = pair.second;
		PlayerInfo player;
		player.canComputerPlay = true;
		int j = (pSettings.getPlayerType() == EPlayerType::COMP_ONLY) ? CPUONLY : CPHUMAN;
		if (j == CPHUMAN)
		{
			player.canHumanPlay = true;
		}

		if(pSettings.getTeam() != TeamID::NO_TEAM)
		{
			player.team = pSettings.getTeam();
		}
		else
		{
			if (teamNumbers[j].empty())
			{
				logGlobal->error("Not enough places in team for %s player", ((j == CPUONLY) ? "CPU" : "CPU or human"));
				assert (teamNumbers[j].size());
			}
			auto itTeam = RandomGeneratorUtil::nextItem(teamNumbers[j], rand);
			player.team = TeamID(*itTeam);
			teamNumbers[j].erase(itTeam);
		}
		teamsTotal.insert(player.team.getNum());
		map->getMap(this).players[pSettings.getColor().getNum()] = player;
	}

	map->getMap(this).howManyTeams = teamsTotal.size();
}

void CMapGenerator::genZones()
{
	placer->placeZones(&rand);
	placer->assignZones(&rand);

	logGlobal->info("Zones generated successfully");
}

void CMapGenerator::addWaterTreasuresInfo()
{
	if (!getZoneWater())
		return;

	//add treasures on water
	for (const auto& treasureInfo : getConfig().waterTreasure)
	{
		getZoneWater()->addTreasureInfo(treasureInfo);
	}
}

void CMapGenerator::fillZones()
{
	addWaterTreasuresInfo();

	logGlobal->info("Started filling zones");

	size_t numZones = map->getZones().size();

	//we need info about all town types to evaluate dwellings and pandoras with creatures properly
	//place main town in the middle
	
	Load::Progress::setupStepsTill(numZones, 50);
	for (const auto& it : map->getZones())
	{
		it.second->initFreeTiles();
		it.second->initModificators();
		Progress::Progress::step();
	}

	std::vector<std::shared_ptr<Zone>> treasureZones;

	ThreadPool pool;

	std::vector<boost::future<void>> futures;
	//At most one Modificator can run for every zone
	pool.init(std::min<int>(boost::thread::hardware_concurrency(), numZones));

	TModificators allJobs;
	for (auto & it : map->getZones())
	{
		allJobs.splice(allJobs.end(), it.second->getModificators());
	}

	Load::Progress::setupStepsTill(allJobs.size(), 240);

	while (!allJobs.empty())
	{
		for (auto it = allJobs.begin(); it != allJobs.end();)
		{
			if ((*it)->isFinished())
			{
				it = allJobs.erase(it);
				Progress::Progress::step();
			}
			else if ((*it)->isReady())
			{
				auto jobCopy = *it;
				futures.emplace_back(pool.async([this, jobCopy]() -> void
					{
						jobCopy->run();
						Progress::Progress::step(); //Update progress bar
					}
				));
				it = allJobs.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	//Wait for all the tasks
	for (auto& fut : futures)
	{
		fut.get();
	}

	for (const auto& it : map->getZones())
	{
		if (it.second->getType() == ETemplateZoneType::TREASURE)
			treasureZones.push_back(it.second);
	}

	//find place for Grail
	if (treasureZones.empty())
	{
		for (const auto& it : map->getZones())
			if (it.second->getType() != ETemplateZoneType::WATER)
				treasureZones.push_back(it.second);
	}
	auto grailZone = *RandomGeneratorUtil::nextItem(treasureZones, rand);

	map->getMap(this).grailPos = *RandomGeneratorUtil::nextItem(grailZone->freePaths().getTiles(), rand);

	logGlobal->info("Zones filled successfully");

	Load::Progress::set(250);
}

void CMapGenerator::addHeaderInfo()
{
	auto& m = map->getMap(this);
	m.version = EMapFormat::VCMI;
	m.width = mapGenOptions.getWidth();
	m.height = mapGenOptions.getHeight();
	m.twoLevel = mapGenOptions.getHasTwoLevels();
	m.name = VLC->generaltexth->allTexts[740];
	m.description = getMapDescription();
	m.difficulty = 1;
	addPlayerInfo();
}

int CMapGenerator::getNextMonlithIndex()
{
	while (true)
	{
		if (monolithIndex >= VLC->objtypeh->knownSubObjects(Obj::MONOLITH_TWO_WAY).size())
			throw rmgException(boost::to_string(boost::format("There is no Monolith Two Way with index %d available!") % monolithIndex));
		else
		{
			//Skip modded Monoliths which can't beplaced on every terrain
			auto templates = VLC->objtypeh->getHandlerFor(Obj::MONOLITH_TWO_WAY, monolithIndex)->getTemplates();
			if (templates.empty() || !templates[0]->canBePlacedAtAnyTerrain())
			{
				monolithIndex++;
			}
			else
			{
				return monolithIndex++;
			}
		}
	}
}

int CMapGenerator::getPrisonsRemaning() const
{
	return allowedPrisons;
}

std::shared_ptr<CZonePlacer> CMapGenerator::getZonePlacer() const
{
	return placer;
}

const std::vector<ArtifactID> & CMapGenerator::getAllPossibleQuestArtifacts() const
{
	return questArtifacts;
}

const std::vector<HeroTypeID> CMapGenerator::getAllPossibleHeroes() const
{
	//Skip heroes that were banned, including the ones placed in prisons
	std::vector<HeroTypeID> ret;
	for (int j = 0; j < map->getMap(this).allowedHeroes.size(); j++)
	{
		if (map->getMap(this).allowedHeroes[j])
			ret.push_back(HeroTypeID(j));
	}
	return ret;
}

void CMapGenerator::banQuestArt(const ArtifactID & id)
{
	//TODO: Protect with mutex
	map->getMap(this).allowedArtifact[id] = false;
}

void CMapGenerator::banHero(const HeroTypeID & id)
{
	//TODO: Protect with mutex
	map->getMap(this).allowedHeroes[id] = false;
}

Zone * CMapGenerator::getZoneWater() const
{
	for(auto & z : map->getZones())
		if(z.second->getType() == ETemplateZoneType::WATER)
			return z.second.get();
	return nullptr;
}

VCMI_LIB_NAMESPACE_END

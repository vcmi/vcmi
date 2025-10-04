/*
 * GameStatistics.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GameStatistics.h"
#include "../CPlayerState.h"
#include "../constants/StringConstants.h"
#include "../VCMIDirs.h"
#include "CGameState.h"
#include "TerrainHandler.h"
#include "StartInfo.h"
#include "HighScore.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/CGObjectInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapping/CMap.h"
#include "../entities/building/CBuilding.h"
#include "../serializer/JsonDeserializer.h"
#include "../serializer/JsonUpdater.h"
#include "../entities/ResourceTypeHandler.h"


VCMI_LIB_NAMESPACE_BEGIN

void StatisticDataSet::add(StatisticDataSetEntry entry)
{
	data.push_back(entry);
}

StatisticDataSetEntry StatisticDataSet::createEntry(const PlayerState * ps, const CGameState * gs, const StatisticDataSet & accumulatedData)
{
	StatisticDataSetEntry data;

	HighScoreParameter param = HighScore::prepareHighScores(gs, ps->color, false);
	HighScoreCalculation scenarioHighScores;
	scenarioHighScores.parameters.push_back(param);
	scenarioHighScores.isCampaign = false;

	data.map = gs->getMap().name.toString();
	data.timestamp = std::time(nullptr);
	data.day = gs->getDate(Date::DAY);
	data.player = ps->color;
	data.playerName = gs->getStartInfo()->playerInfos.at(ps->color).name;
	data.team = ps->team;
	data.isHuman = ps->isHuman();
	data.status = ps->status;
	data.resources = ps->resources;
	data.numberHeroes = ps->getHeroes().size();
	data.numberTowns = gs->howManyTowns(ps->color);
	data.numberArtifacts = Statistic::getNumberOfArts(ps);
	data.numberDwellings = Statistic::getNumberOfDwellings(ps);
	data.armyStrength = Statistic::getArmyStrength(ps, true);
	data.totalExperience = Statistic::getTotalExperience(ps);
	data.income = Statistic::getIncome(gs, ps);
	data.mapExploredRatio = Statistic::getMapExploredRatio(gs, ps->color);
	data.obeliskVisitedRatio = Statistic::getObeliskVisitedRatio(gs, ps->team);
	data.townBuiltRatio = Statistic::getTownBuiltRatio(ps);
	data.hasGrail = param.hasGrail;
	data.numMines = Statistic::getNumMines(gs, ps);
	data.score = scenarioHighScores.calculate().total;
	data.maxHeroLevel = Statistic::findBestHero(gs, ps->color) ? Statistic::findBestHero(gs, ps->color)->level : 0;
	data.numBattlesNeutral = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).numBattlesNeutral : 0;
	data.numBattlesPlayer = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).numBattlesPlayer : 0;
	data.numWinBattlesNeutral = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).numWinBattlesNeutral : 0;
	data.numWinBattlesPlayer = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).numWinBattlesPlayer : 0;
	data.numHeroSurrendered = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).numHeroSurrendered : 0;
	data.numHeroEscaped = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).numHeroEscaped : 0;
	data.spentResourcesForArmy = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).spentResourcesForArmy : TResources();
	data.spentResourcesForBuildings = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).spentResourcesForBuildings : TResources();
	data.tradeVolume = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).tradeVolume : TResources();
	data.eventCapturedTown = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).lastCapturedTownDay == gs->getDate(Date::DAY) : false;
	data.eventDefeatedStrongestHero = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).lastDefeatedStrongestHeroDay == gs->getDate(Date::DAY) : false;
	data.movementPointsUsed = accumulatedData.accumulatedValues.count(ps->color) ? accumulatedData.accumulatedValues.at(ps->color).movementPointsUsed : 0;

	return data;
}

void StatisticDataSetEntry::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeString("map", map);
	handler.serializeInt("timestamp", timestamp);
	handler.serializeInt("day", day);
	handler.serializeId("player", player, PlayerColor::CANNOT_DETERMINE);
	handler.serializeString("playerName", playerName);
	handler.serializeInt("team", team);
	handler.serializeBool("isHuman", isHuman);
	handler.serializeEnum("status", status, {"ingame", "loser", "winner"});
	resources.serializeJson(handler, "resources");
	handler.serializeInt("numberHeroes", numberHeroes);
	handler.serializeInt("numberTowns", numberTowns);
	handler.serializeInt("numberArtifacts", numberArtifacts);
	handler.serializeInt("numberDwellings", numberDwellings);
	handler.serializeInt("armyStrength", armyStrength);
	handler.serializeInt("totalExperience", totalExperience);
	handler.serializeInt("income", income);
	handler.serializeFloat("mapExploredRatio", mapExploredRatio);
	handler.serializeFloat("obeliskVisitedRatio", obeliskVisitedRatio);
	handler.serializeFloat("townBuiltRatio", townBuiltRatio);
	handler.serializeBool("hasGrail", hasGrail);
	{
		auto zonesData = handler.enterStruct("numMines");
		for(auto & idx : LIBRARY->resourceTypeHandler->getAllObjects())
			handler.serializeInt(idx.toResource()->getJsonKey(), numMines[idx], 0);
	}
	handler.serializeInt("score", score);
	handler.serializeInt("maxHeroLevel", maxHeroLevel);
	handler.serializeInt("numBattlesNeutral", numBattlesNeutral);
	handler.serializeInt("numBattlesPlayer", numBattlesPlayer);
	handler.serializeInt("numWinBattlesNeutral", numWinBattlesNeutral);
	handler.serializeInt("numWinBattlesPlayer", numWinBattlesPlayer);
	handler.serializeInt("numHeroSurrendered", numHeroSurrendered);
	handler.serializeInt("numHeroEscaped", numHeroEscaped);
	spentResourcesForArmy.serializeJson(handler, "spentResourcesForArmy");
	spentResourcesForBuildings.serializeJson(handler, "spentResourcesForBuildings");
	tradeVolume.serializeJson(handler, "tradeVolume");
	handler.serializeBool("eventCapturedTown", eventCapturedTown);
	handler.serializeBool("eventDefeatedStrongestHero", eventDefeatedStrongestHero);
	handler.serializeInt("movementPointsUsed", movementPointsUsed);
}

void StatisticDataSet::PlayerAccumulatedValueStorage::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeInt("numBattlesNeutral", numBattlesNeutral);
	handler.serializeInt("numBattlesPlayer", numBattlesPlayer);
	handler.serializeInt("numWinBattlesNeutral", numWinBattlesNeutral);
	handler.serializeInt("numWinBattlesPlayer", numWinBattlesPlayer);
	handler.serializeInt("numHeroSurrendered", numHeroSurrendered);
	handler.serializeInt("numHeroEscaped", numHeroEscaped);
	spentResourcesForArmy.serializeJson(handler, "spentResourcesForArmy");
	spentResourcesForBuildings.serializeJson(handler, "spentResourcesForBuildings");
	tradeVolume.serializeJson(handler, "tradeVolume");
	handler.serializeInt("movementPointsUsed", movementPointsUsed);
	handler.serializeInt("lastCapturedTownDay", lastCapturedTownDay);
	handler.serializeInt("lastDefeatedStrongestHeroDay", lastDefeatedStrongestHeroDay);
}

void StatisticDataSet::serializeJson(JsonSerializeFormat & handler)
{
	{
		auto eventsHandler = handler.enterArray("data");
		eventsHandler.syncSize(data, JsonNode::JsonType::DATA_VECTOR);
		eventsHandler.serializeStruct(data);
	}
	{
		auto eventsHandler = handler.enterStruct("accumulatedValues");
		for(auto & val : accumulatedValues)
			eventsHandler->serializeStruct(GameConstants::PLAYER_COLOR_NAMES[val.first], val.second);
	}
}

std::string StatisticDataSet::toCsv(std::string sep) const
{
	std::stringstream ss;

	auto resources = LIBRARY->resourceTypeHandler->getAllObjects();

	ss << "Map" << sep;
	ss << "Timestamp" << sep;
	ss << "Day" << sep;
	ss << "Player" << sep;
	ss << "PlayerName" << sep;
	ss << "Team" << sep;
	ss << "IsHuman" << sep;
	ss << "Status" << sep;
	ss << "NumberHeroes" << sep;
	ss << "NumberTowns" << sep;
	ss << "NumberArtifacts" << sep;
	ss << "NumberDwellings" << sep;
	ss << "ArmyStrength" << sep;
	ss << "TotalExperience" << sep;
	ss << "Income" << sep;
	ss << "MapExploredRatio" << sep;
	ss << "ObeliskVisitedRatio" << sep;
	ss << "TownBuiltRatio" << sep;
	ss << "HasGrail" << sep;
	ss << "Score" << sep;
	ss << "MaxHeroLevel" << sep;
	ss << "NumBattlesNeutral" << sep;
	ss << "NumBattlesPlayer" << sep;
	ss << "NumWinBattlesNeutral" << sep;
	ss << "NumWinBattlesPlayer" << sep;
	ss << "NumHeroSurrendered" << sep;
	ss << "NumHeroEscaped" << sep;
	ss << "EventCapturedTown" << sep;
	ss << "EventDefeatedStrongestHero" << sep;
	ss << "MovementPointsUsed";
	for(auto & resource : resources)
		ss << sep << resource.toResource()->getJsonKey();
	for(auto & resource : resources)
		ss << sep << resource.toResource()->getJsonKey() + "Mines";
	for(auto & resource : resources)
		ss << sep << resource.toResource()->getJsonKey() + "SpentResourcesForArmy";
	for(auto & resource : resources)
		ss << sep << resource.toResource()->getJsonKey() + "SpentResourcesForBuildings";
	for(auto & resource : resources)
		ss << sep << resource.toResource()->getJsonKey() + "TradeVolume";
	ss << "\r\n";

	for(auto & entry : data)
	{
		ss << entry.map << sep;
		ss << vstd::getFormattedDateTime(entry.timestamp, "%Y-%m-%dT%H:%M:%S") << sep;
		ss << entry.day << sep;
		ss << GameConstants::PLAYER_COLOR_NAMES[entry.player] << sep;
		ss << entry.playerName << sep;
		ss << entry.team.getNum() << sep;
		ss << entry.isHuman << sep;
		ss << static_cast<int>(entry.status) << sep;
		ss << entry.numberHeroes << sep;
		ss << entry.numberTowns <<  sep;
		ss << entry.numberArtifacts << sep;
		ss << entry.numberDwellings << sep;
		ss << entry.armyStrength << sep;
		ss << entry.totalExperience << sep;
		ss << entry.income << sep;
		ss << entry.mapExploredRatio << sep;
		ss << entry.obeliskVisitedRatio << sep;
		ss << entry.townBuiltRatio << sep;
		ss << entry.hasGrail << sep;
		ss << entry.score << sep;
		ss << entry.maxHeroLevel << sep;
		ss << entry.numBattlesNeutral << sep;
		ss << entry.numBattlesPlayer << sep;
		ss << entry.numWinBattlesNeutral << sep;
		ss << entry.numWinBattlesPlayer << sep;
		ss << entry.numHeroSurrendered << sep;
		ss << entry.numHeroEscaped << sep;
		ss << entry.eventCapturedTown << sep;
		ss << entry.eventDefeatedStrongestHero << sep;
		ss << entry.movementPointsUsed;
		for(auto & resource : resources)
			ss << sep << entry.resources[resource];
		for(auto & resource : resources)
			ss << sep << entry.numMines.at(resource);
		for(auto & resource : resources)
			ss << sep << entry.spentResourcesForArmy[resource];
		for(auto & resource : resources)
			ss << sep << entry.spentResourcesForBuildings[resource];
		for(auto & resource : resources)
			ss << sep << entry.tradeVolume[resource];
		ss << "\r\n";
	}

	return ss.str();
}

std::string StatisticDataSet::writeCsv() const
{
	const boost::filesystem::path outPath = VCMIDirs::get().userCachePath() / "statistic";
	boost::filesystem::create_directories(outPath);

	const boost::filesystem::path filePath = outPath / (vstd::getDateTimeISO8601Basic(std::time(nullptr)) + ".csv");
	std::ofstream file(filePath.c_str());
	std::string csv = toCsv(";");
	file << csv;

	return filePath.string();
}

//calculates total number of artifacts that belong to given player
int Statistic::getNumberOfArts(const PlayerState * ps)
{
	int ret = 0;
	for(auto h : ps->getHeroes())
	{
		ret += h->artifactsInBackpack.size() + h->artifactsWorn.size();
	}
	return ret;
}

int Statistic::getNumberOfDwellings(const PlayerState * ps)
{
	int ret = 0;
	for(const auto * obj : ps->getOwnedObjects())
		if (!obj->asOwnable()->providedCreatures().empty())
			ret	+= 1;

	return ret;
}

// get total strength of player army
si64 Statistic::getArmyStrength(const PlayerState * ps, bool withTownGarrison)
{
	si64 str = 0;

	for(auto h : ps->getHeroes())
	{
		if(!h->isGarrisoned() || withTownGarrison)		//original h3 behavior
			str += h->getArmyStrength();
	}
	return str;
}

// get total experience of all heroes
si64 Statistic::getTotalExperience(const PlayerState * ps)
{
	si64 tmp = 0;

	for(auto h : ps->getHeroes())
		tmp += h->exp;
	
	return tmp;
}

// get total gold income
int Statistic::getIncome(const CGameState * gs, const PlayerState * ps)
{
	int totalIncome = 0;

	//Heroes can produce gold as well - skill, specialty or arts
	for(const auto & object : ps->getOwnedObjects())
		totalIncome += object->asOwnable()->dailyIncome()[EGameResID::GOLD];

	return totalIncome;
}

float Statistic::getMapExploredRatio(const CGameState * gs, PlayerColor player)
{
	float visible = 0.0;
	float numTiles = 0.0;

	for(int layer = 0; layer < gs->getMap().levels(); layer++)
		for(int y = 0; y < gs->getMap().height; ++y)
			for(int x = 0; x < gs->getMap().width; ++x)
			{
				TerrainTile tile = gs->getMap().getTile(int3(x, y, layer));

				if(tile.blocked() && !tile.visitable())
					continue;

				if(gs->isVisibleFor(int3(x, y, layer), player))
					visible++;
				numTiles++;
			}
	
	return visible / numTiles;
}

const CGHeroInstance * Statistic::findBestHero(const CGameState * gs, const PlayerColor & color)
{
	const auto &h = gs->players.at(color).getHeroes();
	if(h.empty())
		return nullptr;
	//best hero will be that with highest exp
	int best = 0;
	for(int b=1; b<h.size(); ++b)
	{
		if(h[b]->exp > h[best]->exp)
		{
			best = b;
		}
	}
	return h[best];
}

std::vector<std::vector<PlayerColor>> Statistic::getRank(std::vector<std::pair<PlayerColor, si64>> stats)
{
	std::sort(stats.begin(), stats.end(), [](const std::pair<PlayerColor, si64> & a, const std::pair<PlayerColor, si64> & b) { return a.second > b.second; });

	//put first element
	std::vector< std::vector<PlayerColor> > ret;
	ret.push_back( { stats[0].first } );

	//the rest of elements
	for(int g=1; g<stats.size(); ++g)
	{
		if(stats[g].second == stats[g-1].second)
		{
			(ret.end()-1)->push_back( stats[g].first );
		}
		else
		{
			//create next occupied rank
			ret.push_back( { stats[g].first });
		}
	}

	return ret;
}

int Statistic::getObeliskVisited(const CGameState * gs, const TeamID & t)
{
	if(gs->getMap().obelisksVisited.count(t))
		return gs->getMap().obelisksVisited.at(t);
	else
		return 0;
}

float Statistic::getObeliskVisitedRatio(const CGameState * gs, const TeamID & t)
{
	if(!gs->getMap().obeliskCount)
		return 0;
	return static_cast<float>(getObeliskVisited(gs, t)) / gs->getMap().obeliskCount;
}

std::map<EGameResID, int> Statistic::getNumMines(const CGameState * gs, const PlayerState * ps)
{
	std::map<EGameResID, int> tmp;

	for(auto & res : LIBRARY->resourceTypeHandler->getAllObjects())
		tmp[res] = 0;

	for(const auto * object : ps->getOwnedObjects())
	{
		//Mines
		if(object->ID == Obj::MINE || object->ID == Obj::ABANDONED_MINE)
		{
			const auto * mine = dynamic_cast<const CGMine *>(object);
			assert(mine);
			tmp[mine->producedResource]++;
		}
	}
	return tmp;
}

float Statistic::getTownBuiltRatio(const PlayerState * ps)
{
	float built = 0.0;
	float total = 0.0;

	for(const auto & t : ps->getTowns())
	{
		built += t->getBuildings().size();
		for(const auto & b : t->getTown()->buildings)
			if(!t->forbiddenBuildings.count(b.first))
				total += 1;
	}

	if(total < 1)
		return 0;
	
	return built / total;
}

VCMI_LIB_NAMESPACE_END

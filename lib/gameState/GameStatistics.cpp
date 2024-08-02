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
#include "CGameState.h"
#include "TerrainHandler.h"
#include "CHeroHandler.h"
#include "StartInfo.h"
#include "HighScore.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/CGObjectInstance.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapping/CMap.h"

VCMI_LIB_NAMESPACE_BEGIN

void StatisticDataSet::add(StatisticDataSetEntry entry)
{
	data.push_back(entry);
}

StatisticDataSetEntry StatisticDataSet::createEntry(const PlayerState * ps, const CGameState * gs)
{
	StatisticDataSetEntry data;

	HighScoreParameter param = HighScore::prepareHighScores(gs, ps->color, false);
	HighScoreCalculation scenarioHighScores;
	scenarioHighScores.parameters.push_back(param);
	scenarioHighScores.isCampaign = false;

	data.map = gs->map->name.toString();
	data.timestamp = std::time(0);
	data.day = gs->getDate(Date::DAY);
	data.player = ps->color;
	data.team = ps->team;
	data.isHuman = ps->isHuman();
	data.status = ps->status;
	data.resources = ps->resources;
	data.numberHeroes = ps->heroes.size();
	data.numberTowns = gs->howManyTowns(ps->color);
	data.numberArtifacts = Statistic::getNumberOfArts(ps);
	data.armyStrength = Statistic::getArmyStrength(ps, true);
	data.income = Statistic::getIncome(gs, ps);
	data.mapVisitedRatio = Statistic::getMapVisitedRatio(gs, ps->color);
	data.obeliskVisited = Statistic::getObeliskVisited(gs, ps->team);
	data.mightMagicRatio = Statistic::getMightMagicRatio(ps);
	data.numMines = Statistic::getNumMines(gs, ps);
	data.score = scenarioHighScores.calculate().total;
	data.maxHeroLevel = Statistic::findBestHero(gs, ps->color) ? Statistic::findBestHero(gs, ps->color)->level : 0;
	data.numBattlesNeutral = gs->statistic.values.numBattlesNeutral.count(ps->color) ? gs->statistic.values.numBattlesNeutral.at(ps->color) : 0;
	data.numBattlesPlayer = gs->statistic.values.numBattlesPlayer.count(ps->color) ? gs->statistic.values.numBattlesPlayer.at(ps->color) : 0;
	data.numWinBattlesNeutral = gs->statistic.values.numWinBattlesNeutral.count(ps->color) ? gs->statistic.values.numWinBattlesNeutral.at(ps->color) : 0;
	data.numWinBattlesPlayer = gs->statistic.values.numWinBattlesPlayer.count(ps->color) ? gs->statistic.values.numWinBattlesPlayer.at(ps->color) : 0;
	data.numHeroSurrendered = gs->statistic.values.numHeroSurrendered.count(ps->color) ? gs->statistic.values.numHeroSurrendered.at(ps->color) : 0;
	data.numHeroEscaped = gs->statistic.values.numHeroEscaped.count(ps->color) ? gs->statistic.values.numHeroEscaped.at(ps->color) : 0;

	return data;
}

std::string StatisticDataSet::toCsv()
{
	std::stringstream ss;

	auto resources = std::vector<EGameResID>{EGameResID::GOLD, EGameResID::WOOD, EGameResID::MERCURY, EGameResID::ORE, EGameResID::SULFUR, EGameResID::CRYSTAL, EGameResID::GEMS};

	ss << "Map" << ";";
	ss << "Timestamp" << ";";
	ss << "Day" << ";";
	ss << "Player" << ";";
	ss << "Team" << ";";
	ss << "IsHuman" << ";";
	ss << "Status" << ";";
	ss << "NumberHeroes" << ";";
	ss << "NumberTowns" << ";";
	ss << "NumberArtifacts" << ";";
	ss << "ArmyStrength" << ";";
	ss << "Income" << ";";
	ss << "MapVisitedRatio" << ";";
	ss << "ObeliskVisited" << ";";
	ss << "MightMagicRatio" << ";";
	ss << "Score" << ";";
	ss << "MaxHeroLevel" << ";";
	ss << "NumBattlesNeutral" << ";";
	ss << "NumBattlesPlayer" << ";";
	ss << "NumWinBattlesNeutral" << ";";
	ss << "NumWinBattlesPlayer" << ";";
	ss << "NumHeroSurrendered" << ";";
	ss << "NumHeroEscaped";
	for(auto & resource : resources)
		ss << ";" << GameConstants::RESOURCE_NAMES[resource];
	for(auto & resource : resources)
		ss << ";" << GameConstants::RESOURCE_NAMES[resource] + "Mines";
	ss << "\r\n";

	for(auto & entry : data)
	{
		ss << entry.map << ";";
		ss << vstd::getFormattedDateTime(entry.timestamp, "%Y-%m-%dT%H-%M-%S") << ";";
		ss << entry.day << ";";
		ss << GameConstants::PLAYER_COLOR_NAMES[entry.player] << ";";
		ss << entry.team.getNum() << ";";
		ss << entry.isHuman << ";";
		ss << (int)entry.status << ";";
		ss << entry.numberHeroes << ";";
		ss << entry.numberTowns <<  ";";
		ss << entry.numberArtifacts << ";";
		ss << entry.armyStrength << ";";
		ss << entry.income << ";";
		ss << entry.mapVisitedRatio << ";";
		ss << entry.obeliskVisited << ";";
		ss << entry.mightMagicRatio << ";";
		ss << entry.score << ";";
		ss << entry.maxHeroLevel << ";";
		ss << entry.numBattlesNeutral << ";";
		ss << entry.numBattlesPlayer << ";";
		ss << entry.numWinBattlesNeutral << ";";
		ss << entry.numWinBattlesPlayer << ";";
		ss << entry.numHeroSurrendered << ";";
		ss << entry.numHeroEscaped;
		for(auto & resource : resources)
			ss << ";" << entry.resources[resource];
		for(auto & resource : resources)
			ss << ";" << entry.numMines[resource];
		ss << "\r\n";
	}

	return ss.str();
}

std::vector<const CGMine *> Statistic::getMines(const CGameState * gs, const PlayerState * ps)
{
	std::vector<const CGMine *> tmp;

	/// FIXME: Dirty dirty hack
	/// Stats helper need some access to gamestate.
	std::vector<const CGObjectInstance *> ownedObjects;
	for(const CGObjectInstance * obj : gs->map->objects)
	{
		if(obj && obj->tempOwner == ps->color)
			ownedObjects.push_back(obj);
	}
	/// This is code from CPlayerSpecificInfoCallback::getMyObjects
	/// I'm really need to find out about callback interface design...

	for(const auto * object : ownedObjects)
	{
		//Mines
		if ( object->ID == Obj::MINE )
		{
			const auto * mine = dynamic_cast<const CGMine *>(object);
			assert(mine);

			tmp.push_back(mine);
		}
	}

	return tmp;
}

//calculates total number of artifacts that belong to given player
int Statistic::getNumberOfArts(const PlayerState * ps)
{
	int ret = 0;
	for(auto h : ps->heroes)
	{
		ret += (int)h->artifactsInBackpack.size() + (int)h->artifactsWorn.size();
	}
	return ret;
}

// get total strength of player army
si64 Statistic::getArmyStrength(const PlayerState * ps, bool withTownGarrison)
{
	si64 str = 0;

	for(auto h : ps->heroes)
	{
		if(!h->inTownGarrison || withTownGarrison)		//original h3 behavior
			str += h->getArmyStrength();
	}
	return str;
}

// get total gold income
int Statistic::getIncome(const CGameState * gs, const PlayerState * ps)
{
	int percentIncome = gs->getStartInfo()->getIthPlayersSettings(ps->color).handicap.percentIncome;
	int totalIncome = 0;
	const CGObjectInstance * heroOrTown = nullptr;

	//Heroes can produce gold as well - skill, specialty or arts
	for(const auto & h : ps->heroes)
	{
		totalIncome += h->valOfBonuses(Selector::typeSubtype(BonusType::GENERATE_RESOURCE, BonusSubtypeID(GameResID(GameResID::GOLD)))) * percentIncome / 100;

		if(!heroOrTown)
			heroOrTown = h;
	}

	//Add town income of all towns
	for(const auto & t : ps->towns)
	{
		totalIncome += t->dailyIncome()[EGameResID::GOLD];

		if(!heroOrTown)
			heroOrTown = t;
	}

	for(const CGMine * mine : getMines(gs, ps))
	{
		if (mine->producedResource == EGameResID::GOLD)
			totalIncome += mine->getProducedQuantity();
	}

	return totalIncome;
}

double Statistic::getMapVisitedRatio(const CGameState * gs, PlayerColor player)
{
	double visible = 0.0;
	double numTiles = 0.0;

	for(int layer = 0; layer < (gs->map->twoLevel ? 2 : 1); layer++)
		for(int y = 0; y < gs->map->height; ++y)
			for(int x = 0; x < gs->map->width; ++x)
			{
				TerrainTile tile = gs->map->getTile(int3(x, y, layer));

				if(tile.blocked && (!tile.visitable))
					continue;

				if(gs->isVisible(int3(x, y, layer), player))
					visible++;
				numTiles++;
			}
	
	return visible / numTiles;
}

const CGHeroInstance * Statistic::findBestHero(const CGameState * gs, const PlayerColor & color)
{
	auto &h = gs->players.at(color).heroes;
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

std::vector<std::vector<PlayerColor>> Statistic::getRank(std::vector<TStat> stats)
{
	std::sort(stats.begin(), stats.end(), [](const TStat & a, const TStat & b) { return a.second > b.second; });

	//put first element
	std::vector< std::vector<PlayerColor> > ret;
	std::vector<PlayerColor> tmp;
	tmp.push_back( stats[0].first );
	ret.push_back( tmp );

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
			std::vector<PlayerColor> tmp;
			tmp.push_back(stats[g].first);
			ret.push_back(tmp);
		}
	}

	return ret;
}

int Statistic::getObeliskVisited(const CGameState * gs, const TeamID & t)
{
	if(gs->map->obelisksVisited.count(t))
		return gs->map->obelisksVisited.at(t);
	else
		return 0;
}

double Statistic::getMightMagicRatio(const PlayerState * ps)
{
	double numMight = 0;

	for(auto h : ps->heroes)
		if(h->type->heroClass->affinity == CHeroClass::EClassAffinity::MIGHT)
			numMight++;

	return numMight / ps->heroes.size();
}

std::map<EGameResID, int> Statistic::getNumMines(const CGameState * gs, const PlayerState * ps)
{
	std::map<EGameResID, int> tmp;

	for(auto & res : EGameResID::ALL_RESOURCES())
		tmp[res] = 0;

	for(const CGMine * mine : getMines(gs, ps))
		tmp[mine->producedResource]++;
	
	return tmp;
}

VCMI_LIB_NAMESPACE_END

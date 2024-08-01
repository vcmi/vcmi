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

	data.day = gs->getDate(Date::DAY);
	data.player = ps->color;
	data.team = ps->team;
	data.isHuman = ps->isHuman();
	data.status = ps->status;
	data.resources = ps->resources;
	data.numberHeroes = ps->heroes.size();
	data.numberTowns = ps->towns.size();
	data.numberArtifacts = Statistic::getNumberOfArts(ps);
	data.armyStrength = Statistic::getArmyStrength(ps);
	data.income = Statistic::getIncome(ps);
	data.mapVisitedRatio = Statistic::getMapVisitedRatio(gs, ps->color);

	return data;
}

std::string StatisticDataSet::toCsv()
{
	std::stringstream ss;

	auto resources = std::vector<EGameResID>{EGameResID::GOLD, EGameResID::WOOD, EGameResID::MERCURY, EGameResID::ORE, EGameResID::SULFUR, EGameResID::CRYSTAL, EGameResID::GEMS};

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
	ss << "MapVisitedRatio";
	for(auto & resource : resources)
		ss << ";" << GameConstants::RESOURCE_NAMES[resource];
	ss << "\r\n";

	for(auto & entry : data)
	{
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
		ss << entry.mapVisitedRatio;
		for(auto & resource : resources)
			ss << ";" << entry.resources[resource];
		ss << "\r\n";
	}

	return ss.str();
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
si64 Statistic::getArmyStrength(const PlayerState * ps)
{
	si64 str = 0;

	for(auto h : ps->heroes)
	{
		if(!h->inTownGarrison)		//original h3 behavior
			str += h->getArmyStrength();
	}
	return str;
}

// get total gold income
int Statistic::getIncome(const PlayerState * ps)
{
	int totalIncome = 0;
	const CGObjectInstance * heroOrTown = nullptr;

	//Heroes can produce gold as well - skill, specialty or arts
	for(const auto & h : ps->heroes)
	{
		totalIncome += h->valOfBonuses(Selector::typeSubtype(BonusType::GENERATE_RESOURCE, BonusSubtypeID(GameResID(GameResID::GOLD))));

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

	/// FIXME: Dirty dirty hack
	/// Stats helper need some access to gamestate.
	std::vector<const CGObjectInstance *> ownedObjects;
	for(const CGObjectInstance * obj : heroOrTown->cb->gameState()->map->objects)
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

			if (mine->producedResource == EGameResID::GOLD)
				totalIncome += mine->producedQuantity;
		}
	}

	return totalIncome;
}

double Statistic::getMapVisitedRatio(const CGameState * gs, PlayerColor player)
{
	double visible = 0.0;
	double numTiles = 0.0;

	for(int layer = 0; layer < (gs->map->twoLevel ? 2 : 1); layer++)
		for (int y = 0; y < gs->map->height; ++y)
			for (int x = 0; x < gs->map->width; ++x)
			{
				TerrainTile tile = gs->map->getTile(int3(x, y, layer));

				if (tile.blocked && (!tile.visitable))
					continue;

				if(gs->isVisible(int3(x, y, layer), player))
					visible++;
				numTiles++;
			}
	
	return visible / (numTiles);
}

VCMI_LIB_NAMESPACE_END

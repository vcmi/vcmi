/*
 * Goals.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "GatherArmy.h"
#include "../VCAI.h"
#include "../SectorMap.h"
#include "lib/mapping/CMap.h" //for victory conditions
#include "lib/CPathfinder.h"
#include "../Tasks/VisitTile.h"
#include "../Tasks/BuildStructure.h"
#include "../Tasks/RecruitHero.h"
#include "../Tasks/BuyArmyInTown.h"

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;

using namespace Goals;

ui64 howManyReinforcementsCanGet(HeroPtr h, const CArmedInstance * t)
{
	ui64 ret = 0;
	int freeHeroSlots = GameConstants::ARMY_SIZE - h->stacksCount();
	std::vector<ui64> toMove;
	for (auto const slot : t->Slots())
	{
		//can be merged woth another stack?
		SlotID dst = h->getSlotFor(slot.second->getCreatureID());
		ui64 power = t->getPower(slot.first);

		if (h->hasStackAtSlot(dst))
			ret += power;
		else
			toMove.push_back(power);
	}

	boost::sort(toMove);

	for (auto & stack : boost::adaptors::reverse(toMove))
	{
		if (freeHeroSlots)
		{
			ret += stack;
			freeHeroSlots--;
		}
		else
			break;
	}
	return ret;
}

ui64 howManyReinforcementsCanBuy(HeroPtr h, const CGTownInstance * t)
{
	ui64 ret = 0;
	int freeHeroSlots = GameConstants::ARMY_SIZE - h->stacksCount();
	std::vector<ui64> toMove;
	auto resources = cb->getResourceAmount();

	for (int i = t->creatures.size() - 1; i >= 0; i--)
	{
		if (!t->creatures[i].second.size())
			continue;

		int count = t->creatures[i].first;

		if (!count) {
			continue;
		}

		CreatureID creID = t->creatures[i].second.back();
		const CCreature *c = creID.toCreature();
		ui64 power = c->AIValue * count;
		TResources cost = c->cost * count;

		if (!resources.canAfford(cost)) {
			logAi->trace("Dwelling %s has creatures %s x %i, not enugh resources", t->getObjectName(), c->namePl, count);
			break;
		}

		resources -= cost;

		SlotID dst = h->getSlotFor(creID);
		if (h->hasStackAtSlot(dst))
			ret += power;
		else
			toMove.push_back(power);

		logAi->trace("Dwelling %s has creatures %s x %i", t->getObjectName(), c->namePl, count);
	}

	boost::sort(toMove);

	for (auto & stack : boost::adaptors::reverse(toMove))
	{
		if (freeHeroSlots)
		{
			ret += stack;
			freeHeroSlots--;
		}
		else
			break;
	}
	return ret;
}

Tasks::TaskList GatherArmy::getTasks() {
	Tasks::TaskList tasks;

	auto heroes = cb->getHeroesInfo();
	auto towns = cb->getTownsInfo();
	
	if (!this->hero) {
		if (heroes.empty()) {
			return tasks;
		}

		for (auto town : towns) {
			bool allowBuyArmy = town->hasBuilt(BuildingID::CITY_HALL);
			
			int3 pos = town->visitablePos();
			std::vector<const CGHeroInstance*> copy = heroes;

			vstd::erase_if(copy, [town, allowBuyArmy](const CGHeroInstance* h) -> bool {
				auto minimalQuote = h->getArmyStrength() / 4;
				bool army = ai->howManyArmyCanGet(h, town->getUpperArmy());

				if (allowBuyArmy) {
					army += howManyReinforcementsCanBuy(h, town);
				}

				return army < minimalQuote;
			});

			if (copy.empty()) {
				continue;
			}

			auto carrier = getNearestHero(copy, pos);

			addTask(tasks, Tasks::VisitTile(pos, carrier, town), 1);

			if (allowBuyArmy && howManyReinforcementsCanBuy(carrier, town)) {
				addTask(tasks, Tasks::BuyArmyInTown(town));
			}
		}

		return tasks;
	}


	auto targetHeroPosition = this->hero->visitablePos();
	auto pathsInfo = cb->getPathsInfo(this->hero.get());

	for (HeroPtr hero : heroes) {
		auto isStronger = isLevelHigher(hero, this->hero);
		auto isAccessible = ai->isAccessibleForHero(targetHeroPosition, hero, true);

		if (hero.h == this->hero.h
			|| isStronger
			|| !isAccessible) {
			continue;
		}

		auto additionalArmy = ai->howManyArmyCanGet(this->hero.get(), hero.get());

		if (additionalArmy < this->hero->getArmyStrength() / 4
			|| !force && additionalArmy < requiredAmmount / 4) {
			continue;
		}

		auto priority = std::min(0.0, 1 - (double)distanceToTile(pathsInfo, hero->visitablePos()) / hero->maxMovePoints(true));

		addTask(tasks, Tasks::VisitTile(targetHeroPosition, hero, this->hero.get()), 0.8 * priority);

		if (!force) {
			break;
		}
	}
	//TODO take town if it is closer than hero
	boost::sort(towns, CDistanceSorter(this->hero.get()));

	for (auto town : towns) {
		auto pos = town->visitablePos();
		auto pathInfo = cb->getPathsInfo(this->hero.get())->getPathInfo(pos);
		auto additionalArmy = howManyReinforcementsCanBuy(hero, town);

		if (pathInfo->reachable() && additionalArmy > this->hero->getArmyStrength() / 4
			&& (force || additionalArmy > requiredAmmount / 4))
		{
			auto priority = std::min(0.0, 1 - (double)distanceToTile(pathsInfo, town->visitablePos()) / hero->maxMovePoints(true));

			addTask(tasks, Tasks::BuyArmyInTown(town), priority);

			if (!force) {
				break;
			}
		}
	}

	return tasks;
}

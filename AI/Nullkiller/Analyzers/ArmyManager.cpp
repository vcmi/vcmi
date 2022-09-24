/*
* BuildingManager.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#include "StdInc.h"
#include "ArmyManager.h"
#include "../Engine/Nullkiller.h"
#include "../../../CCallback.h"
#include "../../../lib/mapObjects/MapObjects.h"

class StackUpgradeInfo
{
public:
	CreatureID initialCreature;
	CreatureID upgradedCreature;
	TResources cost;
	int count;
	uint64_t upgradeValue;

	StackUpgradeInfo(CreatureID initial, CreatureID upgraded, int count)
		:initialCreature(initial), upgradedCreature(upgraded), count(count)
	{
		cost = (upgradedCreature.toCreature()->cost - initialCreature.toCreature()->cost) * count;
		upgradeValue = (upgradedCreature.toCreature()->AIValue - initialCreature.toCreature()->AIValue) * count;
	}
};

uint64_t ArmyManager::howManyReinforcementsCanGet(const CGHeroInstance * hero, const CCreatureSet * source) const
{
	return howManyReinforcementsCanGet(hero, hero, source);
}

std::vector<SlotInfo> ArmyManager::getSortedSlots(const CCreatureSet * target, const CCreatureSet * source) const
{
	const CCreatureSet * armies[] = { target, source };

	//we calculate total strength for each creature type available in armies
	std::map<const CCreature *, SlotInfo> creToPower;
	std::vector<SlotInfo> resultingArmy;

	for(auto armyPtr : armies)
	{
		for(auto & i : armyPtr->Slots())
		{
			auto & slotInfp = creToPower[i.second->type];

			slotInfp.creature = i.second->type;
			slotInfp.power += i.second->getPower();
			slotInfp.count += i.second->count;
		}
	}

	for(auto pair : creToPower)
		resultingArmy.push_back(pair.second);

	boost::sort(resultingArmy, [](const SlotInfo & left, const SlotInfo & right) -> bool
	{
		return left.power > right.power;
	});

	return resultingArmy;
}

std::vector<SlotInfo>::iterator ArmyManager::getWeakestCreature(std::vector<SlotInfo> & army) const
{
	auto weakest = boost::min_element(army, [](const SlotInfo & left, const SlotInfo & right) -> bool
	{
		if(left.creature->level != right.creature->level)
			return left.creature->level < right.creature->level;
		
		return left.creature->Speed() > right.creature->Speed();
	});

	return weakest;
}

class TemporaryArmy : public CArmedInstance
{
public:
	void armyChanged() override {}
	TemporaryArmy()
		:CArmedInstance(true)
	{
	}
};

std::vector<SlotInfo> ArmyManager::getBestArmy(const IBonusBearer * armyCarrier, const CCreatureSet * target, const CCreatureSet * source) const
{
	auto sortedSlots = getSortedSlots(target, source);
	std::map<TFaction, uint64_t> alignmentMap;

	for(auto & slot : sortedSlots)
	{
		alignmentMap[slot.creature->faction] += slot.power;
	}

	std::set<TFaction> allowedFactions;
	std::vector<SlotInfo> resultingArmy;
	uint64_t armyValue = 0;

	TemporaryArmy newArmyInstance;
	auto bonusModifiers = armyCarrier->getBonuses(Selector::type()(Bonus::MORALE));

	for(auto bonus : *bonusModifiers)
	{
		// army bonuses will change and object bonuses are temporary
		if(bonus->source != Bonus::ARMY || bonus->source != Bonus::OBJECT)
		{
			newArmyInstance.addNewBonus(std::make_shared<Bonus>(*bonus));
		}
	}

	while(allowedFactions.size() < alignmentMap.size())
	{
		auto strongestAlignment = vstd::maxElementByFun(alignmentMap, [&](std::pair<TFaction, uint64_t> pair) -> uint64_t
		{
			return vstd::contains(allowedFactions, pair.first) ? 0 : pair.second;
		});

		allowedFactions.insert(strongestAlignment->first);

		std::vector<SlotInfo> newArmy;
		uint64_t newValue = 0;
		newArmyInstance.clear();

		for(auto & slot : sortedSlots)
		{
			if(vstd::contains(allowedFactions, slot.creature->faction))
			{
				auto slotID = newArmyInstance.getSlotFor(slot.creature);

				if(slotID.validSlot())
				{
					newArmyInstance.setCreature(slotID, slot.creature->idNumber, slot.count);
					newArmy.push_back(slot);
				}
			}
		}

		newArmyInstance.updateMoraleBonusFromArmy();

		for(auto & slot : newArmyInstance.Slots())
		{
			auto morale = slot.second->MoraleVal();
			auto multiplier = 1.0f;

			const float BadMoraleChance = 0.083f;
			const float HighMoraleChance = 0.04f;

			if(morale < 0)
			{
				multiplier += morale * BadMoraleChance;
			}
			else if(morale > 0)
			{
				multiplier += morale * HighMoraleChance;
			}

			newValue += multiplier * slot.second->getPower();
		}

		if(armyValue >= newValue)
		{
			break;
		}

		resultingArmy = newArmy;
		armyValue = newValue;
	}

	if(resultingArmy.size() <= GameConstants::ARMY_SIZE
		&& allowedFactions.size() == alignmentMap.size()
		&& source->needsLastStack())
	{
		auto weakest = getWeakestCreature(resultingArmy);

		if(weakest->count == 1)
		{
			resultingArmy.erase(weakest);
		}
		else
		{
			weakest->power -= weakest->power / weakest->count;
			weakest->count--;
		}
	}

	return resultingArmy;
}

ui64 ArmyManager::howManyReinforcementsCanBuy(const CCreatureSet * h, const CGDwelling * t) const
{
	return howManyReinforcementsCanBuy(h, t, ai->getFreeResources());
}

std::shared_ptr<CCreatureSet> ArmyManager::getArmyAvailableToBuyAsCCreatureSet(
	const CGDwelling * dwelling,
	TResources availableRes) const
{
	std::vector<creInfo> creaturesInDwellings;
	auto army = std::make_shared<TemporaryArmy>();

	for(int i = dwelling->creatures.size() - 1; i >= 0; i--)
	{
		auto ci = infoFromDC(dwelling->creatures[i]);

		if(!ci.count || ci.creID == -1)
			continue;

		vstd::amin(ci.count, availableRes / ci.cre->cost); //max count we can afford

		if(!ci.count)
			continue;

		SlotID dst = army->getFreeSlot();

		if(!dst.validSlot())
			break;

		army->setCreature(dst, ci.creID, ci.count);
		availableRes -= ci.cre->cost * ci.count;
	}

	return army;
}

ui64 ArmyManager::howManyReinforcementsCanBuy(
	const CCreatureSet * targetArmy,
	const CGDwelling * dwelling,
	const TResources & availableResources) const
{
	ui64 aivalue = 0;
	auto army = getArmyAvailableToBuy(targetArmy, dwelling, availableResources);

	for(const creInfo & ci : army)
	{
		aivalue += ci.count * ci.cre->AIValue;
	}

	return aivalue;
}

std::vector<creInfo> ArmyManager::getArmyAvailableToBuy(const CCreatureSet * hero, const CGDwelling * dwelling) const
{
	return getArmyAvailableToBuy(hero, dwelling, ai->getFreeResources());
}

std::vector<creInfo> ArmyManager::getArmyAvailableToBuy(
	const CCreatureSet * hero,
	const CGDwelling * dwelling,
	TResources availableRes) const
{
	std::vector<creInfo> creaturesInDwellings;
	int freeHeroSlots = GameConstants::ARMY_SIZE - hero->stacksCount();

	for(int i = dwelling->creatures.size() - 1; i >= 0; i--)
	{
		auto ci = infoFromDC(dwelling->creatures[i]);

		if(!ci.count || ci.creID == -1)
			continue;

		SlotID dst = hero->getSlotFor(ci.creID);
		if(!hero->hasStackAtSlot(dst)) //need another new slot for this stack
		{
			if(!freeHeroSlots) //no more place for stacks
				continue;
			else
				freeHeroSlots--; //new slot will be occupied
		}

		vstd::amin(ci.count, availableRes / ci.cre->cost); //max count we can afford

		if(!ci.count)
			continue;

		ci.level = i; //this is important for Dungeon Summoning Portal
		creaturesInDwellings.push_back(ci);
		availableRes -= ci.cre->cost * ci.count;
	}

	return creaturesInDwellings;
}

ui64 ArmyManager::howManyReinforcementsCanGet(const IBonusBearer * armyCarrier, const CCreatureSet * target, const CCreatureSet * source) const
{
	auto bestArmy = getBestArmy(armyCarrier, target, source);
	uint64_t newArmy = 0;
	uint64_t oldArmy = target->getArmyStrength();

	for(auto & slot : bestArmy)
	{
		newArmy += slot.power;
	}

	return newArmy > oldArmy ? newArmy - oldArmy : 0;
}

uint64_t ArmyManager::evaluateStackPower(const CCreature * creature, int count) const
{
	return creature->AIValue * count;
}

SlotInfo ArmyManager::getTotalCreaturesAvailable(CreatureID creatureID) const
{
	auto creatureInfo = totalArmy.find(creatureID);

	return creatureInfo == totalArmy.end() ? SlotInfo() : creatureInfo->second;
}

void ArmyManager::update()
{
	logAi->trace("Start analysing army");

	std::vector<const CCreatureSet *> total;
	auto heroes = cb->getHeroesInfo();
	auto towns = cb->getTownsInfo();

	std::copy(heroes.begin(), heroes.end(), std::back_inserter(total));
	std::copy(towns.begin(), towns.end(), std::back_inserter(total));

	totalArmy.clear();

	for(auto army : total)
	{
		for(auto slot : army->Slots())
		{
			totalArmy[slot.second->getCreatureID()].count += slot.second->count;
		}
	}

	for(auto army : totalArmy)
	{
		army.second.creature = army.first.toCreature();
		army.second.power = evaluateStackPower(army.second.creature, army.second.count);
	}
}

std::vector<SlotInfo> ArmyManager::convertToSlots(const CCreatureSet * army) const
{
	std::vector<SlotInfo> result;

	for(auto slot : army->Slots())
	{
		SlotInfo slotInfo;

		slotInfo.creature = slot.second->getCreatureID().toCreature();
		slotInfo.count = slot.second->count;
		slotInfo.power = evaluateStackPower(slotInfo.creature, slotInfo.count);

		result.push_back(slotInfo);
	}

	return result;
}

std::vector<StackUpgradeInfo> ArmyManager::getHillFortUpgrades(const CCreatureSet * army) const
{
	std::vector<StackUpgradeInfo> upgrades;

	for(auto creature : army->Slots())
	{
		CreatureID initial = creature.second->getCreatureID();
		auto possibleUpgrades = initial.toCreature()->upgrades;

		if(possibleUpgrades.empty())
			continue;

		CreatureID strongestUpgrade = *vstd::minElementByFun(possibleUpgrades, [](CreatureID cre) -> uint64_t
		{
			return cre.toCreature()->AIValue;
		});

		StackUpgradeInfo upgrade = StackUpgradeInfo(initial, strongestUpgrade, creature.second->count);

		if(initial.toCreature()->level == 1)
			upgrade.cost = TResources();

		upgrades.push_back(upgrade);
	}

	return upgrades;
}

std::vector<StackUpgradeInfo> ArmyManager::getDwellingUpgrades(const CCreatureSet * army, const CGDwelling * dwelling) const
{
	std::vector<StackUpgradeInfo> upgrades;

	for(auto creature : army->Slots())
	{
		CreatureID initial = creature.second->getCreatureID();
		auto possibleUpgrades = initial.toCreature()->upgrades;

		vstd::erase_if(possibleUpgrades, [&](CreatureID creID) -> bool
		{
			for(auto pair : dwelling->creatures)
			{
				if(vstd::contains(pair.second, creID))
					return false;
			}

			return true;
		});

		if(possibleUpgrades.empty())
			continue;

		CreatureID strongestUpgrade = *vstd::minElementByFun(possibleUpgrades, [](CreatureID cre) -> uint64_t
		{
			return cre.toCreature()->AIValue;
		});

		StackUpgradeInfo upgrade = StackUpgradeInfo(initial, strongestUpgrade, creature.second->count);

		upgrades.push_back(upgrade);
	}

	return upgrades;
}

std::vector<StackUpgradeInfo> ArmyManager::getPossibleUpgrades(const CCreatureSet * army, const CGObjectInstance * upgrader) const
{
	std::vector<StackUpgradeInfo> upgrades;

	if(upgrader->ID == Obj::HILL_FORT)
	{
		upgrades = getHillFortUpgrades(army);
	}
	else
	{
		auto dwelling = dynamic_cast<const CGDwelling *>(upgrader);

		if(dwelling)
		{
			upgrades = getDwellingUpgrades(army, dwelling);
		}
	}

	return upgrades;
}

ArmyUpgradeInfo ArmyManager::calculateCreaturesUpgrade(
	const CCreatureSet * army,
	const CGObjectInstance * upgrader,
	const TResources & availableResources) const
{
	if(!upgrader)
		return ArmyUpgradeInfo();

	std::vector<StackUpgradeInfo> upgrades = getPossibleUpgrades(army, upgrader);

	vstd::erase_if(upgrades, [&](const StackUpgradeInfo & u) -> bool
	{
		return !availableResources.canAfford(u.cost);
	});

	if(upgrades.empty())
		return ArmyUpgradeInfo();

	std::sort(upgrades.begin(), upgrades.end(), [](const StackUpgradeInfo & u1, const StackUpgradeInfo & u2) -> bool
	{
		return u1.upgradeValue > u2.upgradeValue;
	});

	TResources resourcesLeft = availableResources;
	ArmyUpgradeInfo result;
	
	result.resultingArmy = convertToSlots(army);

	for(auto upgrade : upgrades)
	{
		if(resourcesLeft.canAfford(upgrade.cost))
		{
			SlotInfo upgradedArmy;

			upgradedArmy.creature = upgrade.upgradedCreature.toCreature();
			upgradedArmy.count = upgrade.count;
			upgradedArmy.power = evaluateStackPower(upgradedArmy.creature, upgradedArmy.count);

			auto slotToReplace = std::find_if(result.resultingArmy.begin(), result.resultingArmy.end(), [&](const SlotInfo & slot) -> bool {
				return slot.count == upgradedArmy.count && slot.creature->idNumber == upgrade.initialCreature;
			});

			resourcesLeft -= upgrade.cost;
			result.upgradeCost += upgrade.cost;
			result.upgradeValue += upgrade.upgradeValue;

			*slotToReplace = upgradedArmy;
		}
	}

	return result;
}

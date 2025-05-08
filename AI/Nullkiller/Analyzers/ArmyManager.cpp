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
#include "../../../lib/mapping/CMapDefines.h"
#include "../../../lib/IGameSettings.h"
#include "../../../lib/GameConstants.h"
#include "../../../lib/TerrainHandler.h"

namespace NKAI
{
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
		cost = (upgradedCreature.toCreature()->getFullRecruitCost() - initialCreature.toCreature()->getFullRecruitCost()) * count;
		upgradeValue = (upgradedCreature.toCreature()->getAIValue() - initialCreature.toCreature()->getAIValue()) * count;
	}
};

void ArmyUpgradeInfo::addArmyToBuy(std::vector<SlotInfo> army)
{
	for(auto slot : army)
	{
		resultingArmy.push_back(slot);

		upgradeValue += slot.power;
		upgradeCost += slot.creature->getFullRecruitCost() * slot.count;
	}
}

void ArmyUpgradeInfo::addArmyToGet(std::vector<SlotInfo> army)
{
	for(auto slot : army)
	{
		resultingArmy.push_back(slot);

		upgradeValue += slot.power;
	}
}

std::vector<SlotInfo> ArmyManager::toSlotInfo(std::vector<creInfo> army) const
{
	std::vector<SlotInfo> result;

	for(auto i : army)
	{
		SlotInfo slot;

		slot.creature = i.creID.toCreature();
		slot.count = i.count;
		slot.power = evaluateStackPower(i.creID.toCreature(), i.count);

		result.push_back(slot);
	}

	return result;
}

uint64_t ArmyManager::howManyReinforcementsCanGet(const CGHeroInstance * hero, const CCreatureSet * source) const
{
	return howManyReinforcementsCanGet(hero, hero, source, ai->cb->getTile(hero->visitablePos())->getTerrainID());
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
			auto cre = dynamic_cast<const CCreature*>(i.second->getType());
			auto & slotInfp = creToPower[cre];

			slotInfp.creature = cre;
			slotInfp.power += i.second->getPower();
			slotInfp.count += i.second->getCount();
		}
	}

	for(auto & pair : creToPower)
		resultingArmy.push_back(pair.second);

	boost::sort(resultingArmy, [](const SlotInfo & left, const SlotInfo & right) -> bool
	{
		return left.power > right.power;
	});

	return resultingArmy;
}

std::vector<SlotInfo>::iterator ArmyManager::getBestUnitForScout(std::vector<SlotInfo> & army, const TerrainId & armyTerrain) const
{
	uint64_t totalPower = 0;

	for (const auto & unit : army)
		totalPower += unit.power;

	int baseMovementCost = cb->getSettings().getInteger(EGameSettings::HEROES_MOVEMENT_COST_BASE);
	bool terrainHasPenalty = armyTerrain.hasValue() && armyTerrain.toEntity(LIBRARY)->moveCost != baseMovementCost;

	// arbitrary threshold - don't give scout more than specified part of total AI value of our army
	uint64_t maxUnitValue = totalPower / 100;

	const auto & movementPointsLimits = cb->getSettings().getVector(EGameSettings::HEROES_MOVEMENT_POINTS_LAND);

	auto fastest = boost::min_element(army, [&](const SlotInfo & left, const SlotInfo & right) -> bool
	{
		uint64_t leftUnitPower = left.power / left.count;
		uint64_t rightUnitPower = left.power / left.count;
		bool leftUnitIsWeak = leftUnitPower < maxUnitValue || left.creature->getLevel() < 4;
		bool rightUnitIsWeak = rightUnitPower < maxUnitValue || right.creature->getLevel() < 4;

		if (leftUnitIsWeak != rightUnitIsWeak)
			return leftUnitIsWeak;

		if (terrainHasPenalty)
		{
			auto leftNativeTerrain = left.creature->getFactionID().toFaction()->nativeTerrain;
			auto rightNativeTerrain = right.creature->getFactionID().toFaction()->nativeTerrain;

			if (leftNativeTerrain != rightNativeTerrain)
			{
				if (leftNativeTerrain == armyTerrain)
					return true;

				if (rightNativeTerrain == armyTerrain)
					return false;
			}
		}

		int leftEffectiveMovement = std::min<int>(movementPointsLimits.size() - 1, left.creature->getMovementRange());
		int rightEffectiveMovement = std::min<int>(movementPointsLimits.size() - 1, right.creature->getMovementRange());

		int leftMovementPointsLimit = movementPointsLimits[leftEffectiveMovement];
		int rightMovementPointsLimit = movementPointsLimits[rightEffectiveMovement];

		if (leftMovementPointsLimit != rightMovementPointsLimit)
			return leftMovementPointsLimit > rightMovementPointsLimit;

		return leftUnitPower < rightUnitPower;
	});

	return fastest;
}

class TemporaryArmy : public CArmedInstance
{
public:
	void armyChanged() override {}
	TemporaryArmy()
		:CArmedInstance(nullptr, true)
	{
	}
};

std::vector<SlotInfo> ArmyManager::getBestArmy(const IBonusBearer * armyCarrier, const CCreatureSet * target, const CCreatureSet * source, const TerrainId & armyTerrain) const
{
	auto sortedSlots = getSortedSlots(target, source);

	if(source->stacksCount() == 0)
		return sortedSlots;

	std::map<FactionID, uint64_t> alignmentMap;

	for(auto & slot : sortedSlots)
	{
		alignmentMap[slot.creature->getFactionID()] += slot.power;
	}

	std::set<FactionID> allowedFactions;
	std::vector<SlotInfo> resultingArmy;
	uint64_t armyValue = 0;

	TemporaryArmy newArmyInstance;

	while(allowedFactions.size() < alignmentMap.size())
	{
		auto strongestAlignment = vstd::maxElementByFun(alignmentMap, [&](std::pair<FactionID, uint64_t> pair) -> uint64_t
		{
			return vstd::contains(allowedFactions, pair.first) ? 0 : pair.second;
		});

		allowedFactions.insert(strongestAlignment->first);

		std::vector<SlotInfo> newArmy;
		uint64_t newValue = 0;
		newArmyInstance.clearSlots();

		for(auto & slot : sortedSlots)
		{
			if(vstd::contains(allowedFactions, slot.creature->getFactionID()))
			{
				auto slotID = newArmyInstance.getSlotFor(slot.creature->getId());

				if(slotID.validSlot())
				{
					newArmyInstance.setCreature(slotID, slot.creature->getId(), slot.count);
					newArmy.push_back(slot);
				}
			}
		}

		newArmyInstance.updateMoraleBonusFromArmy();

		for(auto & slot : newArmyInstance.Slots())
		{
			auto morale = slot.second->moraleVal();
			auto multiplier = 1.0f;

			const auto & badMoraleChance = cb->getSettings().getVector(EGameSettings::COMBAT_BAD_MORALE_CHANCE);
			const auto & highMoraleChance = cb->getSettings().getVector(EGameSettings::COMBAT_GOOD_MORALE_CHANCE);
			int moraleDiceSize = cb->getSettings().getInteger(EGameSettings::COMBAT_MORALE_DICE_SIZE);

			if(morale < 0 && !badMoraleChance.empty())
			{
				size_t chanceIndex = std::min<size_t>(badMoraleChance.size(), -morale) - 1;
				multiplier -= 1.0 / moraleDiceSize * badMoraleChance.at(chanceIndex);
			}
			else if(morale > 0 && !highMoraleChance.empty())
			{
				size_t chanceIndex = std::min<size_t>(highMoraleChance.size(), morale) - 1;
				multiplier += 1.0 / moraleDiceSize * highMoraleChance.at(chanceIndex);
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
		auto weakest = getBestUnitForScout(resultingArmy, armyTerrain);

		if(weakest->count == 1) 
		{
			if (resultingArmy.size() == 1)
				logAi->warn("Unexpected resulting army size!");

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

		if(!ci.count || ci.creID == CreatureID::NONE)
			continue;

		vstd::amin(ci.count, availableRes / ci.creID.toCreature()->getFullRecruitCost()); //max count we can afford

		if(!ci.count)
			continue;

		SlotID dst = army->getFreeSlot();

		if(!dst.validSlot())
			break;

		army->setCreature(dst, ci.creID, ci.count);
		availableRes -= ci.creID.toCreature()->getFullRecruitCost() * ci.count;
	}

	return army;
}

ui64 ArmyManager::howManyReinforcementsCanBuy(
	const CCreatureSet * targetArmy,
	const CGDwelling * dwelling,
	const TResources & availableResources,
	uint8_t turn) const
{
	ui64 aivalue = 0;
	auto army = getArmyAvailableToBuy(targetArmy, dwelling, availableResources);

	for(const creInfo & ci : army)
	{
		aivalue += ci.count * ci.creID.toCreature()->getAIValue();
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
	TResources availableRes,
	uint8_t turn) const
{
	std::vector<creInfo> creaturesInDwellings;
	int freeHeroSlots = GameConstants::ARMY_SIZE - hero->stacksCount();
	bool countGrowth = (cb->getDate(Date::DAY_OF_WEEK) + turn) > 7;

	const CGTownInstance * town = dwelling->ID == Obj::TOWN
		? dynamic_cast<const CGTownInstance *>(dwelling)
		: nullptr;

	std::set<SlotID> alreadyDisbanded;

	for(int i = dwelling->creatures.size() - 1; i >= 0; i--)
	{
		auto ci = infoFromDC(dwelling->creatures[i]);

		if(ci.creID == CreatureID::NONE) continue;

		if(i < GameConstants::CREATURES_PER_TOWN && countGrowth)
		{
			ci.count += town ? town->creatureGrowth(i) : ci.creID.toCreature()->getGrowth();
		}

		if(!ci.count) continue;

		// Calculate the market value of the new stack
		TResources newStackValue = ci.creID.toCreature()->getFullRecruitCost() * ci.count;

		SlotID dst = hero->getSlotFor(ci.creID);

		// Keep track of the least valuable slot in the hero's army
		SlotID leastValuableSlot;
		TResources leastValuableStackValue;
		leastValuableStackValue[6] = std::numeric_limits<int>::max();
		bool shouldDisband = false;
		if(!hero->hasStackAtSlot(dst)) //need another new slot for this stack
		{
			if(!freeHeroSlots) // No free slots; consider replacing
			{
				// Check for the least valuable existing stack
				for (auto& slot : hero->Slots())
				{
					if (alreadyDisbanded.find(slot.first) != alreadyDisbanded.end())
						continue;

					if(slot.second->getCreatureID() != CreatureID::NONE)
					{
						TResources currentStackValue = slot.second->getCreatureID().toCreature()->getFullRecruitCost() * slot.second->getCount();

						if (town && slot.second->getCreatureID().toCreature()->getFactionID() == town->getFactionID())
							continue;

						if(currentStackValue.marketValue() < leastValuableStackValue.marketValue())
						{
							leastValuableStackValue = currentStackValue;
							leastValuableSlot = slot.first;
						}
					}
				}

				// Decide whether to replace the least valuable stack
				if(newStackValue.marketValue() <= leastValuableStackValue.marketValue())
				{
					continue; // Skip if the new stack isn't worth replacing
				}
				else
				{
					shouldDisband = true;
				}
			}
			else
			{
				freeHeroSlots--; //new slot will be occupied
			}
		}

		vstd::amin(ci.count, availableRes / ci.creID.toCreature()->getFullRecruitCost()); //max count we can afford

		int disbandMalus = 0;
		
		if (shouldDisband)
		{
			disbandMalus = leastValuableStackValue / ci.creID.toCreature()->getFullRecruitCost();
			alreadyDisbanded.insert(leastValuableSlot);
		}

		ci.count -= disbandMalus;

		if(ci.count <= 0)
			continue;

		ci.level = i; //this is important for Dungeon Summoning Portal
		creaturesInDwellings.push_back(ci);
		availableRes -= ci.creID.toCreature()->getFullRecruitCost() * ci.count;
	}

	return creaturesInDwellings;
}

ui64 ArmyManager::howManyReinforcementsCanGet(const IBonusBearer * armyCarrier, const CCreatureSet * target, const CCreatureSet * source, const TerrainId & armyTerrain) const
{
	if(source->stacksCount() == 0)
	{
		return 0;
	}

	auto bestArmy = getBestArmy(armyCarrier, target, source, armyTerrain);
	uint64_t newArmy = 0;
	uint64_t oldArmy = target->getArmyStrength();

	for(auto & slot : bestArmy)
	{
		newArmy += slot.power;
	}

	return newArmy > oldArmy ? newArmy - oldArmy : 0;
}

uint64_t ArmyManager::evaluateStackPower(const Creature * creature, int count) const
{
	return creature->getAIValue() * count;
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
		for(const auto & slot : army->Slots())
		{
			totalArmy[slot.second->getCreatureID()].count += slot.second->getCount();
		}
	}

	for(auto & army : totalArmy)
	{
		army.second.creature = army.first.toCreature();
		army.second.power = evaluateStackPower(army.second.creature, army.second.count);
	}
}

std::vector<SlotInfo> ArmyManager::convertToSlots(const CCreatureSet * army) const
{
	std::vector<SlotInfo> result;

	for(const auto & slot : army->Slots())
	{
		SlotInfo slotInfo;

		slotInfo.creature = slot.second->getCreatureID().toCreature();
		slotInfo.count = slot.second->getCount();
		slotInfo.power = evaluateStackPower(slotInfo.creature, slotInfo.count);

		result.push_back(slotInfo);
	}

	return result;
}

std::vector<StackUpgradeInfo> ArmyManager::getHillFortUpgrades(const CCreatureSet * army) const
{
	std::vector<StackUpgradeInfo> upgrades;

	for(const auto & creature : army->Slots())
	{
		CreatureID initial = creature.second->getCreatureID();
		auto possibleUpgrades = initial.toCreature()->upgrades;

		if(possibleUpgrades.empty())
			continue;

		CreatureID strongestUpgrade = *vstd::minElementByFun(possibleUpgrades, [](CreatureID cre) -> uint64_t
		{
			return cre.toCreature()->getAIValue();
		});

		StackUpgradeInfo upgrade = StackUpgradeInfo(initial, strongestUpgrade, creature.second->getCount());

		if(initial.toCreature()->getLevel() == 1)
			upgrade.cost = TResources();

		upgrades.push_back(upgrade);
	}

	return upgrades;
}

std::vector<StackUpgradeInfo> ArmyManager::getDwellingUpgrades(const CCreatureSet * army, const CGDwelling * dwelling) const
{
	std::vector<StackUpgradeInfo> upgrades;

	for(const auto & creature : army->Slots())
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
			return cre.toCreature()->getAIValue();
		});

		StackUpgradeInfo upgrade = StackUpgradeInfo(initial, strongestUpgrade, creature.second->getCount());

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
				return slot.count == upgradedArmy.count && slot.creature->getId() == upgrade.initialCreature;
			});

			resourcesLeft -= upgrade.cost;
			result.upgradeCost += upgrade.cost;
			result.upgradeValue += upgrade.upgradeValue;

			*slotToReplace = upgradedArmy;
		}
	}

	return result;
}

}

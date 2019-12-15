/*
* AIhelper.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"

#include "AIhelper.h"

AIhelper::AIhelper()
{
	pathfindingManager.reset(new PathfindingManager());
	armyManager.reset(new ArmyManager());
	heroManager.reset(new HeroManager());
}

AIhelper::~AIhelper()
{
}

void AIhelper::init(CPlayerSpecificInfoCallback * CB)
{
	pathfindingManager->init(CB);
	armyManager->init(CB);
	heroManager->init(CB);
}

void AIhelper::setAI(VCAI * AI)
{
	pathfindingManager->setAI(AI);
	armyManager->setAI(AI);
	heroManager->setAI(AI);
}

void AIhelper::update()
{
	armyManager->update();
	heroManager->update();
}

std::vector<AIPath> AIhelper::getPathsToTile(const HeroPtr & hero, const int3 & tile) const
{
	return pathfindingManager->getPathsToTile(hero, tile);
}

std::vector<AIPath> AIhelper::getPathsToTile(const int3 & tile) const
{
	return pathfindingManager->getPathsToTile(tile);
}

void AIhelper::updatePaths(std::vector<HeroPtr> heroes, bool useHeroChain)
{
	pathfindingManager->updatePaths(heroes, useHeroChain);
}

uint64_t AIhelper::evaluateStackPower(const CCreature * creature, int count) const
{
	return armyManager->evaluateStackPower(creature, count);
}

SlotInfo AIhelper::getTotalCreaturesAvailable(CreatureID creatureID) const
{
	return armyManager->getTotalCreaturesAvailable(creatureID);
}

bool AIhelper::canGetArmy(const CArmedInstance * army, const CArmedInstance * source) const
{
	return armyManager->canGetArmy(army, source);
}

ui64 AIhelper::howManyReinforcementsCanBuy(const CCreatureSet * h, const CGDwelling * t) const
{
	return armyManager->howManyReinforcementsCanBuy(h, t);
}

ui64 AIhelper::howManyReinforcementsCanGet(const CCreatureSet * target, const CCreatureSet * source) const
{
	return armyManager->howManyReinforcementsCanGet(target, source);
}

std::vector<SlotInfo> AIhelper::getBestArmy(const CCreatureSet * target, const CCreatureSet * source) const
{
	return armyManager->getBestArmy(target, source);
}

std::vector<SlotInfo>::iterator AIhelper::getWeakestCreature(std::vector<SlotInfo> & army) const
{
	return armyManager->getWeakestCreature(army);
}

std::vector<SlotInfo> AIhelper::getSortedSlots(const CCreatureSet * target, const CCreatureSet * source) const
{
	return armyManager->getSortedSlots(target, source);
}

std::vector<creInfo> AIhelper::getArmyAvailableToBuy(const CCreatureSet * hero, const CGDwelling * dwelling) const
{
	return armyManager->getArmyAvailableToBuy(hero, dwelling);
}

ArmyUpgradeInfo AIhelper::calculateCreateresUpgrade(
	const CCreatureSet * army,
	const CGObjectInstance * upgrader,
	const TResources & availableResources) const
{
	return armyManager->calculateCreateresUpgrade(army, upgrader, availableResources);
}

int AIhelper::selectBestSkill(const HeroPtr & hero, const std::vector<SecondarySkill> & skills) const
{
	return heroManager->selectBestSkill(hero, skills);
}

const std::map<HeroPtr, HeroRole> & AIhelper::getHeroRoles() const
{
	return heroManager->getHeroRoles();
}

HeroRole AIhelper::getHeroRole(const HeroPtr & hero) const
{
	return heroManager->getHeroRole(hero);
}

float AIhelper::evaluateSecSkill(SecondarySkill skill, const CGHeroInstance * hero) const
{
	return heroManager->evaluateSecSkill(skill, hero);
}

float AIhelper::evaluateHero(const CGHeroInstance * hero) const
{
	return heroManager->evaluateHero(hero);
}
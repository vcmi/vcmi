/*
* AINodeStorage.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Actors.h"
#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"
#include "../../../lib/mapObjects/MapObjects.h"
#include "../../../lib/mapping/TerrainTile.h"
#include "../../../lib/pathfinder/TurnInfo.h"
#include "Actions/BuyArmyAction.h"

using namespace NK2AI;

const CCreatureSet emptyArmy;

bool HeroExchangeArmy::needsLastStack() const
{
	return true;
}

std::shared_ptr<SpecialAction> HeroExchangeArmy::getActorAction() const
{
	std::shared_ptr<SpecialAction> result;

	if(requireBuyArmy)
	{
		result.reset(new AIPathfinding::BuyArmyAction());
	}

	return result;
}

ChainActor::ChainActor(const CGHeroInstance * hero, HeroRole heroRole, uint64_t chainMask)
	:hero(hero), heroRole(heroRole), isMovable(true), chainMask(chainMask), creatureSet(hero),
	baseActor(this), carrierParent(nullptr), otherParent(nullptr), actorExchangeCount(1), armyCost(), actorAction()
{
	initialPosition = hero->visitablePos();
	layer = hero->inBoat() ? hero->getBoat()->layer : EPathfindingLayer::LAND;
	initialMovement = hero->movementPointsRemaining();
	initialTurn = 0;
	armyValue = getHeroArmyStrengthWithCommander(hero, hero);
	heroFightingStrength = hero->getHeroStrength();
}

ChainActor::ChainActor(const ChainActor * carrier, const ChainActor * other, const CCreatureSet * heroArmy)
	:hero(carrier->hero), heroRole(carrier->heroRole), isMovable(true), creatureSet(heroArmy), chainMask(carrier->chainMask | other->chainMask),
	baseActor(this), carrierParent(carrier), otherParent(other), heroFightingStrength(carrier->heroFightingStrength),
	actorExchangeCount(carrier->actorExchangeCount + other->actorExchangeCount), armyCost(carrier->armyCost + other->armyCost), actorAction()
{
	armyValue = getHeroArmyStrengthWithCommander(hero, heroArmy);
}

ChainActor::ChainActor(const CGObjectInstance * obj, const CCreatureSet * creatureSet, uint64_t chainMask, int initialTurn)
	:hero(nullptr), heroRole(HeroRole::MAIN), isMovable(false), creatureSet(creatureSet), chainMask(chainMask),
	baseActor(this), carrierParent(nullptr), otherParent(nullptr), initialTurn(initialTurn), initialMovement(0),
	heroFightingStrength(0), actorExchangeCount(1), armyCost(), actorAction()
{
	initialPosition = obj->visitablePos();
	layer = EPathfindingLayer::LAND;
	armyValue = creatureSet->getArmyStrength();
}

int ChainActor::maxMovePoints(CGPathNode::ELayer layer)
{
#if NK2AI_TRACE_LEVEL > 0
	if(!hero)
		throw std::logic_error("Asking movement points for static actor");
#endif

	return hero->movementPointsLimit(layer != EPathfindingLayer::SAIL);
}

std::string ChainActor::toString() const
{
	return hero->getNameTranslated();
}

ObjectActor::ObjectActor(const CGObjectInstance * obj, const CCreatureSet * army, uint64_t chainMask, int initialTurn)
	:ChainActor(obj, army, chainMask, initialTurn), object(obj)
{
}

const CGObjectInstance * ObjectActor::getActorObject() const
{
	return object;
}

std::string ObjectActor::toString() const
{
	return object->getObjectName() + " at " + object->visitablePos().toString();
}

HeroActor::HeroActor(const CGHeroInstance * hero, HeroRole heroRole, uint64_t chainMask, const Nullkiller * aiNk)
	:ChainActor(hero, heroRole, chainMask)
{
	exchangeMap.reset(new HeroExchangeMap(this, aiNk));
	setupSpecialActors();
}

HeroActor::HeroActor(
	const ChainActor * carrier,
	const ChainActor * other,
	const HeroExchangeArmy * army,
	const Nullkiller * aiNk)
	:ChainActor(carrier, other, army)
{
	exchangeMap.reset(new HeroExchangeMap(this, aiNk));
	armyCost += army->armyCost;
	actorAction = army->getActorAction();
	setupSpecialActors();
}

void ChainActor::setBaseActor(HeroActor * base)
{
	baseActor = base;
	hero = base->hero;
	heroRole = base->heroRole;
	layer = base->layer;
	initialMovement = base->initialMovement;
	initialTurn = base->initialTurn;
	armyValue = base->armyValue;
	chainMask = base->chainMask;
	creatureSet = base->creatureSet;
	isMovable = base->isMovable;
	heroFightingStrength = base->heroFightingStrength;
	armyCost = base->armyCost;
	actorAction = base->actorAction;
	actorExchangeCount = base->actorExchangeCount;
}

void HeroActor::setupSpecialActors()
{
	auto allActors = std::vector<ChainActor *>{this};

	for(ChainActor & specialActor : specialActors)
	{
		specialActor.setBaseActor(this);
		allActors.push_back(&specialActor);
	}

	for(int i = 0; i <= SPECIAL_ACTORS_COUNT; i++)
	{
		ChainActor * actor = allActors[i];

		actor->allowBattle = (i & 1) > 0;
		actor->allowSpellCast = (i & 2) > 0;
		actor->allowUseResources = (i & 4) > 0;
		actor->battleActor = allActors[i | 1];
		actor->castActor = allActors[i | 2];
		actor->resourceActor = allActors[i | 4];
	}
}

ExchangeResult ChainActor::tryExchangeNoLock(const ChainActor * specialActor, const ChainActor * other) const
{
	if(!isMovable) return ExchangeResult();

	return baseActor->tryExchangeNoLock(specialActor, other);
}

ExchangeResult HeroActor::tryExchangeNoLock(const ChainActor * specialActor, const ChainActor * other) const
{
	const ChainActor * otherBase = other->baseActor;
	ExchangeResult result = exchangeMap->tryExchangeNoLock(otherBase);

	if(!result.actor || !result.lockAcquired) return result;

	if(specialActor == this)
		return result;

	int index = vstd::find_pos_if(specialActors, [specialActor](const ChainActor & actor) -> bool
	{
		return &actor == specialActor;
	});

	result.actor = &(dynamic_cast<HeroActor *>(result.actor)->specialActors.at(index));

	return result;
}

HeroExchangeMap::HeroExchangeMap(const HeroActor * actor, const Nullkiller * aiNk)
	:actor(actor), aiNk(aiNk), sync()
{
}

HeroExchangeMap::~HeroExchangeMap()
{
	for(auto & exchange : exchangeMap)
	{
		if(!exchange.second) continue;

		delete exchange.second->creatureSet;
	}

	for(auto & exchange : exchangeMap)
	{
		if(!exchange.second) continue;

		delete exchange.second;
	}

	exchangeMap.clear();

}

ExchangeResult HeroExchangeMap::tryExchangeNoLock(const ChainActor * other)
{
	ExchangeResult result;

	{
		std::shared_lock lock(sync, std::try_to_lock);

		if(!lock.owns_lock())
		{
			result.lockAcquired = false;

			return result;
		}

		auto position = exchangeMap.find(other);

		if(position != exchangeMap.end())
		{
			result.actor = position->second;

			return result;
		}
	}

	{
		std::unique_lock uniqueLock(sync, std::try_to_lock);

		if(!uniqueLock.owns_lock())
		{
			result.lockAcquired = false;

			return result;
		}

		auto inserted = exchangeMap.insert(std::pair<const ChainActor *, HeroActor *>(other, nullptr));

		if(!inserted.second)
		{
			result.actor = inserted.first->second;

			return result; // already inserted
		}

		auto differentMasks = (actor->chainMask & other->chainMask) == 0;

		if(!differentMasks) return result;

		if(actor->allowSpellCast || other->allowSpellCast)
			return result;

		TResources resources = aiNk->cc->getResourceAmount();

		if(!resources.canAfford(actor->armyCost + other->armyCost))
		{
#if NK2AI_PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace(
				"Can not afford exchange because of total cost %s but we have %s",
				(actor->armyCost + other->armyCost).toString(),
				resources.toString());
#endif
			return result;
		}

		if(other->isMovable && other->armyValue <= actor->armyValue / 10 && other->armyValue < MIN_ARMY_STRENGTH_FOR_CHAIN)
			return result;

		TResources availableResources = resources - actor->armyCost - other->armyCost;
		HeroExchangeArmy * upgradedInitialArmy = tryUpgrade(actor->creatureSet, other->getActorObject(), availableResources);
		HeroExchangeArmy * newArmy;

		if(other->creatureSet->Slots().size())
		{
			if(upgradedInitialArmy)
			{
				newArmy = pickBestCreatures(upgradedInitialArmy, other->creatureSet);
				newArmy->armyCost = upgradedInitialArmy->armyCost;
				newArmy->requireBuyArmy = upgradedInitialArmy->requireBuyArmy;

				delete upgradedInitialArmy;
			}
			else
			{
				newArmy = pickBestCreatures(actor->creatureSet, other->creatureSet);
			}
		}
		else
		{
			newArmy = upgradedInitialArmy;
		}

		if(!newArmy) return result;

		auto newArmyStrength = newArmy->getArmyStrength();
		auto oldArmyStrength = actor->creatureSet->getArmyStrength();

		if(newArmyStrength <= oldArmyStrength) return result;

		auto reinforcement = newArmyStrength - oldArmyStrength;

	#if NK2AI_PATHFINDER_TRACE_LEVEL >= 2
		logAi->trace(
			"Exchange %s->%s reinforcement: %d, %f%%",
			actor->toString(),
			other->toString(),
			reinforcement,
			100.0f * reinforcement / actor->armyValue);
	#endif

		if(reinforcement <= actor->armyValue / 10 && reinforcement < MIN_ARMY_STRENGTH_FOR_CHAIN)
		{
			delete newArmy;

			return result;
		}

		auto * exchanged = new HeroActor(actor, other, newArmy, aiNk);

		exchanged->armyCost += newArmy->armyCost;
		result.actor = exchanged;
		exchangeMap[other] = exchanged;

		return result;
	}
}

HeroExchangeArmy * HeroExchangeMap::tryUpgrade(
	const CCreatureSet * army,
	const CGObjectInstance * upgrader,
	TResources resources) const
{
	auto * target = new HeroExchangeArmy();
	auto upgradeInfo = aiNk->armyManager->calculateCreaturesUpgrade(army, upgrader, resources);

	if(upgradeInfo.upgradeValue)
	{
		for(auto & slotInfo : upgradeInfo.resultingArmy)
		{
			auto targetSlot = target->getFreeSlot();

			target->addToSlot(targetSlot, slotInfo.creature->getId(), TQuantity(slotInfo.count));
		}

		resources -= upgradeInfo.upgradeCost;
		target->armyCost += upgradeInfo.upgradeCost;
	}
	else
	{
		for(const auto & slot : army->Slots())
		{
			const auto & targetSlot = target->getSlotFor(slot.second->getCreatureID());

			target->addToSlot(targetSlot, slot.second->getCreatureID(), slot.second->getCount());
		}
	}

	if(upgrader->ID == Obj::TOWN)
	{
		auto buyArmy = aiNk->armyManager->getArmyAvailableToBuy(target, aiNk->cc->getTown(upgrader->id), resources);

		for(auto & creatureToBuy : buyArmy)
		{
			auto targetSlot = target->getSlotFor(creatureToBuy.creID.toCreature());
			if (targetSlot.validSlot())
			{
				target->addToSlot(targetSlot, creatureToBuy.creID, creatureToBuy.count);
				target->armyCost += creatureToBuy.creID.toCreature()->getFullRecruitCost() * creatureToBuy.count;
				target->requireBuyArmy = true;
			}
		}
	}

	if(target->getArmyStrength() <= army->getArmyStrength())
	{
		delete target;

		return nullptr;
	}

	return target;
}

HeroExchangeArmy * HeroExchangeMap::pickBestCreatures(const CCreatureSet * army1, const CCreatureSet * army2) const
{
	auto * target = new HeroExchangeArmy();
	auto bestArmy = aiNk->armyManager->getBestArmy(actor->hero, army1, army2, aiNk->cc->getTile(actor->hero->visitablePos())->getTerrainID());

	for(auto & slotInfo : bestArmy)
	{
		auto targetSlot = target->getFreeSlot();

		target->addToSlot(targetSlot, slotInfo.creature->getId(), TQuantity(slotInfo.count));
	}

	return target;
}

HillFortActor::HillFortActor(const CGObjectInstance * hillFort, uint64_t chainMask)
	:ObjectActor(hillFort, &emptyArmy, chainMask, 0)
{
}

DwellingActor::DwellingActor(const CGDwelling * dwelling, uint64_t chainMask, bool waitForGrowth, int dayOfWeek)
	: ObjectActor(
		dwelling,
		getDwellingCreatures(dwelling, waitForGrowth),
		chainMask,
		getInitialTurn(waitForGrowth, dayOfWeek)),
	dwelling(dwelling)
{
	for(auto & slot : creatureSet->Slots())
	{
		armyCost += slot.second->getCreatureID().toCreature()->getFullRecruitCost() * slot.second->getCount();
	}
}

DwellingActor::~DwellingActor()
{
	delete creatureSet;
}

int DwellingActor::getInitialTurn(bool waitForGrowth, int dayOfWeek)
{
	if(!waitForGrowth)
		return 0;

	return 8 - dayOfWeek;
}

std::string DwellingActor::toString() const
{
	return dwelling->getTypeName() + dwelling->visitablePos().toString();
}

CCreatureSet * DwellingActor::getDwellingCreatures(const CGDwelling * dwelling, bool waitForGrowth)
{
	auto * dwellingCreatures = new CCreatureSet();

	for(auto & creatureInfo : dwelling->creatures)
	{
		if(!creatureInfo.second.size())
			continue;

		auto creature = creatureInfo.second.back().toCreature();
		dwellingCreatures->addToSlot(
			dwellingCreatures->getSlotFor(creature),
			creature->getId(),
			TQuantity(creatureInfo.first));
	}

	return dwellingCreatures;
}

TownGarrisonActor::TownGarrisonActor(const CGTownInstance * town, uint64_t chainMask)
	:ObjectActor(town, town->getUpperArmy(), chainMask, 0), town(town)
{
}

std::string TownGarrisonActor::toString() const
{
	return town->getNameTranslated();
}

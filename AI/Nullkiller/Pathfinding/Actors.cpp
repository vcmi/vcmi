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
#include "../VCAI.h"
#include "../Engine/Nullkiller.h"
#include "../../../CCallback.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/mapObjects/MapObjects.h"
#include "Actions/BuyArmyAction.h"

CCreatureSet emptyArmy;

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
	layer = hero->boat ? EPathfindingLayer::SAIL : EPathfindingLayer::LAND;
	initialMovement = hero->movement;
	initialTurn = 0;
	armyValue = hero->getArmyStrength();
	heroFightingStrength = hero->getFightingStrength();
}

ChainActor::ChainActor(const ChainActor * carrier, const ChainActor * other, const CCreatureSet * heroArmy)
	:hero(carrier->hero), heroRole(carrier->heroRole), isMovable(true), creatureSet(heroArmy), chainMask(carrier->chainMask | other->chainMask),
	baseActor(this), carrierParent(carrier), otherParent(other), heroFightingStrength(carrier->heroFightingStrength),
	actorExchangeCount(carrier->actorExchangeCount + other->actorExchangeCount), armyCost(carrier->armyCost + other->armyCost), actorAction()
{
	armyValue = heroArmy->getArmyStrength();
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

std::string ChainActor::toString() const
{
	return hero->name;
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

HeroActor::HeroActor(const CGHeroInstance * hero, HeroRole heroRole, uint64_t chainMask, const Nullkiller * ai)
	:ChainActor(hero, heroRole, chainMask)
{
	exchangeMap.reset(new HeroExchangeMap(this, ai));
	setupSpecialActors();
}

HeroActor::HeroActor(
	const ChainActor * carrier, 
	const ChainActor * other, 
	const HeroExchangeArmy * army, 
	const Nullkiller * ai)
	:ChainActor(carrier, other,	army)
{
	exchangeMap.reset(new HeroExchangeMap(this, ai));
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
}

void HeroActor::setupSpecialActors()
{
	auto allActors = std::vector<ChainActor *>{ this };

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

ChainActor * ChainActor::exchange(const ChainActor * specialActor, const ChainActor * other) const
{
	return baseActor->exchange(specialActor, other);
}

bool ChainActor::canExchange(const ChainActor * other) const
{
	return isMovable && baseActor->canExchange(other);
}

namespace vstd
{
	template <class M, class Key, class F>
	typename M::mapped_type & getOrCompute(M &m, Key const& k, F f)
	{
		typedef typename M::mapped_type V;

		std::pair<typename M::iterator, bool> r = m.insert(typename M::value_type(k, V()));
		V &v = r.first->second;

		if(r.second)
			f(v);

		return v;
	}
}

bool HeroActor::canExchange(const ChainActor * other) const
{
	return exchangeMap->canExchange(other);
}

bool HeroExchangeMap::canExchange(const ChainActor * other)
{
	return vstd::getOrCompute(canExchangeCache, other, [&](bool & result) {
		result = (actor->chainMask & other->chainMask) == 0;

		if(result)
		{
			TResources resources = ai->cb->getResourceAmount();

			if(!resources.canAfford(actor->armyCost + other->armyCost))
			{
				result = false;
#if PATHFINDER_TRACE_LEVEL >= 2
				logAi->trace(
					"Can not afford exchange because of total cost %s but we have %s",
					(actor->armyCost + other->armyCost).toString(),
					resources.toString());
#endif
				return;
			}

			TResources availableResources = resources - actor->armyCost - other->armyCost;

			auto upgradeInfo = ai->armyManager->calculateCreateresUpgrade(
				actor->creatureSet, 
				other->getActorObject(),
				availableResources);

			uint64_t reinforcment = upgradeInfo.upgradeValue;
			
			if(other->creatureSet->Slots().size())
				reinforcment += ai->armyManager->howManyReinforcementsCanGet(actor->hero, actor->creatureSet, other->creatureSet);
			
			auto obj = other->getActorObject();
			if(obj && obj->ID == Obj::TOWN)
			{
				reinforcment += ai->armyManager->howManyReinforcementsCanBuy(
					actor->creatureSet,
					ai->cb->getTown(obj->id),
					availableResources - upgradeInfo.upgradeCost);
			}

#if PATHFINDER_TRACE_LEVEL >= 2
			logAi->trace(
				"Exchange %s->%s reinforcement: %d, %f%%",
				actor->toString(),
				other->toString(),
				reinforcment,
				100.0f * reinforcment / actor->armyValue);
#endif

			result = reinforcment > actor->armyValue / 10 || reinforcment > 1000;
		}
	});
}

ChainActor * HeroActor::exchange(const ChainActor * specialActor, const ChainActor * other) const
{
	const ChainActor * otherBase = other->baseActor;
	HeroActor * result = exchangeMap->exchange(otherBase);

	if(specialActor == this)
		return result;

	int index = vstd::find_pos_if(specialActors, [specialActor](const ChainActor & actor) -> bool
	{
		return &actor == specialActor;
	});

	return &result->specialActors[index];
}

HeroExchangeMap::HeroExchangeMap(const HeroActor * actor, const Nullkiller * ai)
	:actor(actor), ai(ai)
{
}

HeroExchangeMap::~HeroExchangeMap()
{
	for(auto & exchange : exchangeMap)
	{
		delete exchange.second->creatureSet;
		delete exchange.second;
	}

	exchangeMap.clear();
}

HeroActor * HeroExchangeMap::exchange(const ChainActor * other)
{
	HeroActor * result;

	if(vstd::contains(exchangeMap, other))
		result = exchangeMap.at(other);
	else 
	{
		TResources availableResources = ai->cb->getResourceAmount() - actor->armyCost - other->armyCost;
		HeroExchangeArmy * upgradedInitialArmy = tryUpgrade(actor->creatureSet, other->getActorObject(), availableResources);
		HeroExchangeArmy * newArmy;
		
		if(other->creatureSet->Slots().size())
		{
			if(upgradedInitialArmy)
			{
				newArmy = pickBestCreatures(upgradedInitialArmy, other->creatureSet);
				newArmy->armyCost = upgradedInitialArmy->armyCost;

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

		result = new HeroActor(actor, other, newArmy, ai);
		result->armyCost += newArmy->armyCost;
		exchangeMap[other] = result;
	}

	return result;
}

HeroExchangeArmy * HeroExchangeMap::tryUpgrade(
	const CCreatureSet * army,
	const CGObjectInstance * upgrader,
	TResources resources) const
{
	HeroExchangeArmy * target = new HeroExchangeArmy();
	auto upgradeInfo = ai->armyManager->calculateCreateresUpgrade(army, upgrader, resources);

	if(upgradeInfo.upgradeValue)
	{
		for(auto & slotInfo : upgradeInfo.resultingArmy)
		{
			auto targetSlot = target->getFreeSlot();

			target->addToSlot(targetSlot, slotInfo.creature->idNumber, TQuantity(slotInfo.count));
		}

		resources -= upgradeInfo.upgradeCost;
		target->armyCost += upgradeInfo.upgradeCost;
	}
	else
	{
		for(auto slot : army->Slots())
		{
			auto targetSlot = target->getSlotFor(slot.second->getCreatureID());

			target->addToSlot(targetSlot, slot.second->getCreatureID(), slot.second->count);
		}
	}

	if(upgrader->ID == Obj::TOWN)
	{
		auto buyArmy = ai->armyManager->getArmyAvailableToBuy(target, ai->cb->getTown(upgrader->id), resources);

		for(auto creatureToBuy : buyArmy)
		{
			auto targetSlot = target->getSlotFor(creatureToBuy.cre);

			target->addToSlot(targetSlot, creatureToBuy.creID, creatureToBuy.count);
			target->armyCost += creatureToBuy.cre->cost * creatureToBuy.count;
			target->requireBuyArmy = true;
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
	HeroExchangeArmy * target = new HeroExchangeArmy();
	auto bestArmy = ai->armyManager->getBestArmy(actor->hero, army1, army2);

	for(auto & slotInfo : bestArmy)
	{
		auto targetSlot = target->getFreeSlot();

		target->addToSlot(targetSlot, slotInfo.creature->idNumber, TQuantity(slotInfo.count));
	}

	return target;
}

HillFortActor::HillFortActor(const CGObjectInstance * hillFort, uint64_t chainMask)
	:ObjectActor(hillFort, &emptyArmy, chainMask, 0)
{
}

DwellingActor::DwellingActor(const CGDwelling * dwelling, uint64_t chainMask, bool waitForGrowth, int dayOfWeek)
	:ObjectActor(
		dwelling, 
		getDwellingCreatures(dwelling, waitForGrowth), 
		chainMask, 
		getInitialTurn(waitForGrowth, dayOfWeek)),
	dwelling(dwelling)
{
	for(auto & slot : creatureSet->Slots())
	{
		armyCost += slot.second->getCreatureID().toCreature()->cost * slot.second->count;
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
	return dwelling->typeName + dwelling->visitablePos().toString();
}

CCreatureSet * DwellingActor::getDwellingCreatures(const CGDwelling * dwelling, bool waitForGrowth)
{
	CCreatureSet * dwellingCreatures = new CCreatureSet();

	for(auto & creatureInfo : dwelling->creatures)
	{
		if(!creatureInfo.second.size())
			continue;

		auto creature = creatureInfo.second.back().toCreature();
		auto count = creatureInfo.first;
			
		if(waitForGrowth)
		{
			const CGTownInstance * town = dynamic_cast<const CGTownInstance *>(dwelling);

			count += town ? town->creatureGrowth(creature->level) : creature->growth;
		}

		dwellingCreatures->addToSlot(
			dwellingCreatures->getSlotFor(creature),
			creature->idNumber,
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
	return town->name;
}
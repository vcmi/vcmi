/*
* AINodeStorage.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "../../../lib/CPathfinder.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../AIUtility.h"
#include "Actions/ISpecialAction.h"

class HeroActor;
class VCAI;

class ChainActor
{
protected:
	HeroActor * baseActor;
	ChainActor(const CGHeroInstance * hero, int chainMask);
	ChainActor(const ChainActor * carrier, const ChainActor * other, const CCreatureSet * heroArmy);

public:
	uint64_t chainMask;
	bool isMovable;
	bool allowUseResources;
	bool allowBattle;
	bool allowSpellCast;
	const CGHeroInstance * hero;
	const CCreatureSet * creatureSet;
	const ChainActor * battleActor;
	const ChainActor * castActor;
	const ChainActor * resourceActor;
	const ChainActor * carrierParent;
	const ChainActor * otherParent;
	int3 initialPosition;
	EPathfindingLayer layer;
	uint32_t initialMovement;
	uint32_t initialTurn;
	uint64_t armyValue;

	ChainActor(){}
	ChainActor * exchange(const ChainActor * other) const;
	bool canExchange(const ChainActor * other, const VCAI * ai) const;
	void setBaseActor(HeroActor * base);
	const HeroActor * getBaseActor() const { return baseActor; }
	std::shared_ptr<ISpecialAction> getExchangeAction() const;

};

class HeroActor : public ChainActor
{
public:
	static const int SPECIAL_ACTORS_COUNT = 7;

private:
	ChainActor specialActors[SPECIAL_ACTORS_COUNT];
	std::map<const ChainActor *, HeroActor *> exchangeMap;
	std::map<const HeroActor *, bool> canExchangeCache;

	void setupSpecialActors();

public:
	std::shared_ptr<ISpecialAction> exchangeAction;
	// chain flags, can be combined meaning hero exchange and so on

	HeroActor()
	{
	}

	HeroActor(const CGHeroInstance * hero, int chainMask);
	HeroActor(const ChainActor * carrier, const ChainActor * other);
	ChainActor * exchange(const ChainActor * specialActor, const ChainActor * other);
	bool canExchange(const HeroActor * other, const VCAI * ai);
	CCreatureSet * pickBestCreatures(const CCreatureSet * army1, const CCreatureSet * army2) const;
};

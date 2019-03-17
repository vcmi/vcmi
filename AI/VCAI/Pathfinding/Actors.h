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
	ChainActor(const CGHeroInstance * hero, uint64_t chainMask);
	ChainActor(const ChainActor * carrier, const ChainActor * other, const CCreatureSet * heroArmy);
	ChainActor(const CGObjectInstance * obj, const CCreatureSet * army, uint64_t chainMask, int initialTurn);

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
	const ChainActor * baseActor;
	int3 initialPosition;
	EPathfindingLayer layer;
	uint32_t initialMovement;
	uint32_t initialTurn;
	uint64_t armyValue;

	ChainActor(){}

	virtual bool canExchange(const ChainActor * other) const;
	ChainActor * exchange(const ChainActor * other) const { return exchange(this, other); }
	void setBaseActor(HeroActor * base);

protected:
	virtual ChainActor * exchange(const ChainActor * specialActor, const ChainActor * other) const;
};

class HeroExchangeMap
{
private:
	const HeroActor * actor;
	std::map<const ChainActor *, HeroActor *> exchangeMap;
	std::map<const ChainActor *, bool> canExchangeCache;
	const VCAI * ai;

public:
	HeroExchangeMap(const HeroActor * actor, const VCAI * ai)
		:actor(actor), ai(ai)
	{
	}

	HeroActor * exchange(const ChainActor * other);
	bool canExchange(const ChainActor * other);

private:
	CCreatureSet * pickBestCreatures(const CCreatureSet * army1, const CCreatureSet * army2) const;
};

class HeroActor : public ChainActor
{
public:
	static const int SPECIAL_ACTORS_COUNT = 7;

private:
	ChainActor specialActors[SPECIAL_ACTORS_COUNT];
	HeroExchangeMap * exchangeMap;

	void setupSpecialActors();

public:
	std::shared_ptr<ISpecialAction> exchangeAction;
	// chain flags, can be combined meaning hero exchange and so on

	HeroActor(const CGHeroInstance * hero, uint64_t chainMask, const VCAI * ai);
	HeroActor(const ChainActor * carrier, const ChainActor * other, const CCreatureSet * army, const VCAI * ai);

	virtual bool canExchange(const ChainActor * other) const override;

protected:
	virtual ChainActor * exchange(const ChainActor * specialActor, const ChainActor * other) const override;
};

class DwellingActor : public ChainActor
{
public:
	DwellingActor(const CGDwelling * dwelling, uint64_t chainMask, bool waitForGrowth, int dayOfWeek);
	~DwellingActor();

protected:
	int getInitialTurn(bool waitForGrowth, int dayOfWeek);
	CCreatureSet * getDwellingCreatures(const CGDwelling * dwelling, bool waitForGrowth);
};

class TownGarrisonActor : public ChainActor
{
public:
	TownGarrisonActor(const CGTownInstance * town, uint64_t chainMask);
};
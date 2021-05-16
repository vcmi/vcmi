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
#include "Actions/SpecialAction.h"

class HeroActor;
class VCAI;

class HeroExchangeArmy : public CCreatureSet
{
public:
	virtual bool needsLastStack() const override;
};

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
	float heroFightingStrength;
	uint8_t actorExchangeCount;
	TResources armyCost;

	ChainActor(){}

	virtual bool canExchange(const ChainActor * other) const;
	virtual std::string toString() const;
	ChainActor * exchange(const ChainActor * other) const { return exchange(this, other); }
	void setBaseActor(HeroActor * base);
	virtual const CGObjectInstance * getActorObject() const	{ return hero; }

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

	~HeroExchangeMap();

	HeroActor * exchange(const ChainActor * other);
	bool canExchange(const ChainActor * other);

private:
	CCreatureSet * pickBestCreatures(const CCreatureSet * army1, const CCreatureSet * army2) const;
	CCreatureSet * tryUpgrade(const CCreatureSet * army, const CGObjectInstance * upgrader, TResources resources) const;
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
	std::shared_ptr<SpecialAction> exchangeAction;
	// chain flags, can be combined meaning hero exchange and so on

	HeroActor(const CGHeroInstance * hero, uint64_t chainMask, const VCAI * ai);
	HeroActor(const ChainActor * carrier, const ChainActor * other, const CCreatureSet * army, const VCAI * ai);

	virtual bool canExchange(const ChainActor * other) const override;

protected:
	virtual ChainActor * exchange(const ChainActor * specialActor, const ChainActor * other) const override;
};

class ObjectActor : public ChainActor
{
private:
	const CGObjectInstance * object;

public:
	ObjectActor(const CGObjectInstance * obj, const CCreatureSet * army, uint64_t chainMask, int initialTurn);
	virtual std::string toString() const override;
	const CGObjectInstance * getActorObject() const override;
};

class HillFortActor : public ObjectActor
{
public:
	HillFortActor(const CGObjectInstance * hillFort, uint64_t chainMask);
};

class DwellingActor : public ObjectActor
{
private:
	const CGDwelling * dwelling;

public:
	DwellingActor(const CGDwelling * dwelling, uint64_t chainMask, bool waitForGrowth, int dayOfWeek);
	~DwellingActor();
	virtual std::string toString() const override;

protected:
	int getInitialTurn(bool waitForGrowth, int dayOfWeek);
	CCreatureSet * getDwellingCreatures(const CGDwelling * dwelling, bool waitForGrowth);
};

class TownGarrisonActor : public ObjectActor
{
private:
	const CGTownInstance * town;

public:
	TownGarrisonActor(const CGTownInstance * town, uint64_t chainMask);
	virtual std::string toString() const override;
};
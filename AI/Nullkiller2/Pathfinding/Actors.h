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

#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../AIUtility.h"
#include "Actions/SpecialAction.h"

namespace NK2AI
{

extern const uint64_t MIN_ARMY_STRENGTH_FOR_CHAIN;

class ChainActor;
class HeroActor;
class Nullkiller;

class HeroExchangeArmy : public CArmedInstance
{
public:
	TResources armyCost;
	bool requireBuyArmy;
	bool needsLastStack() const override;
	std::shared_ptr<SpecialAction> getActorAction() const;

	HeroExchangeArmy(): CArmedInstance(nullptr, BonusNodeType::UNKNOWN, true), requireBuyArmy(false) {}
};

struct ExchangeResult
{
	bool lockAcquired;
	ChainActor * actor;

	ExchangeResult() : lockAcquired(true), actor(nullptr) {}
};

class ChainActor
{
protected:
	ChainActor(const CGHeroInstance * hero, HeroRole heroRole, uint64_t chainMask);
	ChainActor(const ChainActor * carrier, const ChainActor * other, const CCreatureSet * heroArmy);
	ChainActor(const CGObjectInstance * obj, const CCreatureSet * army, uint64_t chainMask, int initialTurn);

public:
	uint64_t chainMask;
	bool isMovable = false;
	bool allowUseResources = false;
	bool allowBattle = false;
	bool allowSpellCast = false;
	std::shared_ptr<SpecialAction> actorAction;
	const CGHeroInstance * hero;
	HeroRole heroRole;
	const CCreatureSet * creatureSet = nullptr;
	const ChainActor * battleActor = nullptr;
	const ChainActor * castActor = nullptr;
	const ChainActor * resourceActor = nullptr;
	const ChainActor * carrierParent = nullptr;
	const ChainActor * otherParent = nullptr;
	const ChainActor * baseActor = nullptr;
	int3 initialPosition;
	EPathfindingLayer layer;
	uint32_t initialMovement = 0;
	uint32_t initialTurn = 0;
	uint64_t armyValue;
	float heroFightingStrength;
	uint8_t actorExchangeCount;
	TResources armyCost;

	ChainActor() = default;
	virtual ~ChainActor() = default;

	virtual std::string toString() const;
	ExchangeResult tryExchangeNoLock(const ChainActor * other) const { return tryExchangeNoLock(this, other); }
	void setBaseActor(HeroActor * base);
	virtual const CGObjectInstance * getActorObject() const	{ return hero; }
	int maxMovePoints(EPathfindingLayer layer);

protected:
	virtual ExchangeResult tryExchangeNoLock(const ChainActor * specialActor, const ChainActor * other) const;
};

class HeroExchangeMap
{
private:
	const HeroActor * actor;
	std::map<const ChainActor *, HeroActor *> exchangeMap;
	const Nullkiller * aiNk;
	std::shared_mutex sync;

public:
	HeroExchangeMap(const HeroActor * actor, const Nullkiller * aiNk);
	~HeroExchangeMap();

	ExchangeResult tryExchangeNoLock(const ChainActor * other);

private:
	HeroExchangeArmy * pickBestCreatures(const CCreatureSet * army1, const CCreatureSet * army2) const;
	HeroExchangeArmy * tryUpgrade(const CCreatureSet * army, const CGObjectInstance * upgrader, TResources resources) const;
};

class HeroActor : public ChainActor
{
public:
	static const int SPECIAL_ACTORS_COUNT = 7;

private:
	std::array<ChainActor, SPECIAL_ACTORS_COUNT> specialActors;
	std::unique_ptr<HeroExchangeMap> exchangeMap;

	void setupSpecialActors();

public:
	std::shared_ptr<SpecialAction> exchangeAction;
	// chain flags, can be combined meaning hero exchange and so on

	HeroActor(const CGHeroInstance * hero, HeroRole heroRole, uint64_t chainMask, const Nullkiller * aiNk);
	HeroActor(const ChainActor * carrier, const ChainActor * other, const HeroExchangeArmy * army, const Nullkiller * aiNk);

protected:
	ExchangeResult tryExchangeNoLock(const ChainActor * specialActor, const ChainActor * other) const override;
};

class ObjectActor : public ChainActor
{
private:
	const CGObjectInstance * object;

public:
	ObjectActor(const CGObjectInstance * obj, const CCreatureSet * army, uint64_t chainMask, int initialTurn);
	std::string toString() const override;
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
	std::string toString() const override;

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
	std::string toString() const override;
};

}

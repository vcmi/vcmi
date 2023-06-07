/*
* CommonConstructors.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "AObjectTypeHandler.h"
#include "CDefaultObjectTypeHandler.h"

#include "../mapObjects/CGMarket.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../LogicalExpression.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CGTownInstance;
class CGHeroInstance;
class CGDwelling;
class CHeroClass;
class CBank;
class CFaction;
class CStackBasicDescriptor;

class CObstacleConstructor : public CDefaultObjectTypeHandler<CGObjectInstance>
{
public:
	bool isStaticObject() override;
};

class CTownInstanceConstructor : public CDefaultObjectTypeHandler<CGTownInstance>
{
	JsonNode filtersJson;
protected:
	bool objectFilter(const CGObjectInstance * obj, std::shared_ptr<const ObjectTemplate> tmpl) const override;
	void initTypeData(const JsonNode & input) override;

public:
	CFaction * faction = nullptr;
	std::map<std::string, LogicalExpression<BuildingID>> filters;

	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;
	void afterLoadFinalization() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & filtersJson;
		h & faction;
		h & filters;
		h & static_cast<CDefaultObjectTypeHandler<CGTownInstance>&>(*this);
	}
};

class CHeroInstanceConstructor : public CDefaultObjectTypeHandler<CGHeroInstance>
{
	JsonNode filtersJson;
protected:
	bool objectFilter(const CGObjectInstance * obj, std::shared_ptr<const ObjectTemplate> tmpl) const override;
	void initTypeData(const JsonNode & input) override;

public:
	CHeroClass * heroClass = nullptr;
	std::map<std::string, LogicalExpression<HeroTypeID>> filters;

	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;
	void afterLoadFinalization() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & filtersJson;
		h & heroClass;
		h & filters;
		h & static_cast<CDefaultObjectTypeHandler<CGHeroInstance>&>(*this);
	}
};

class CDwellingInstanceConstructor : public CDefaultObjectTypeHandler<CGDwelling>
{
	std::vector<std::vector<const CCreature *>> availableCreatures;

	JsonNode guards;

protected:
	bool objectFilter(const CGObjectInstance * obj, std::shared_ptr<const ObjectTemplate> tmpl) const override;
	void initTypeData(const JsonNode & input) override;

public:
	bool hasNameTextID() const override;

	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;

	bool producesCreature(const CCreature * crea) const;
	std::vector<const CCreature *> getProducedCreatures() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & availableCreatures;
		h & guards;
		h & static_cast<CDefaultObjectTypeHandler<CGDwelling>&>(*this);
	}
};

class DLL_LINKAGE BoatInstanceConstructor : public CDefaultObjectTypeHandler<CGBoat>
{
protected:
	void initTypeData(const JsonNode & config) override;
	
	std::vector<Bonus> bonuses;
	EPathfindingLayer layer;
	bool onboardAssaultAllowed; //if true, hero can attack units from transport
	bool onboardVisitAllowed; //if true, hero can visit objects from transport
	
	std::string actualAnimation; //for OH3 boats those have actual animations
	std::string overlayAnimation; //waves animations
	std::array<std::string, PlayerColor::PLAYER_LIMIT_I> flagAnimations;
	
public:
	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;
	void afterLoadFinalization() override;

	/// Returns boat preview animation, for use in Shipyards
	std::string getBoatAnimationName() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CDefaultObjectTypeHandler<CGBoat>&>(*this);
		h & layer;
		h & onboardAssaultAllowed;
		h & onboardVisitAllowed;
		h & bonuses;
		h & actualAnimation;
		h & overlayAnimation;
		h & flagAnimations;
	}
};

class MarketInstanceConstructor : public CDefaultObjectTypeHandler<CGMarket>
{
protected:
	void initTypeData(const JsonNode & config) override;
	
	std::set<EMarketMode::EMarketMode> marketModes;
	JsonNode predefinedOffer;
	int marketEfficiency;
	
	std::string title, speech;
	
public:
	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CDefaultObjectTypeHandler<CGMarket>&>(*this);
		h & marketModes;
		h & marketEfficiency;
	}
};

VCMI_LIB_NAMESPACE_END

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

#include "CDefaultObjectTypeHandler.h"
#include "../LogicalExpression.h"

#include "../mapObjects/MiscObjects.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGArtifact;
class CGObjectInstance;
class CGTownInstance;
class CGHeroInstance;
class CGMarket;
class CHeroClass;
class CBank;
class CGBoat;
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

	void initializeObject(CGTownInstance * object) const override;
	void randomizeObject(CGTownInstance * object, CRandomGenerator & rng) const override;
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

	void initializeObject(CGHeroInstance * object) const override;
	void randomizeObject(CGHeroInstance * object, CRandomGenerator & rng) const override;
	void afterLoadFinalization() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & filtersJson;
		h & heroClass;
		h & filters;
		h & static_cast<CDefaultObjectTypeHandler<CGHeroInstance>&>(*this);
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
	void initializeObject(CGBoat * object) const override;

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
	CGMarket * createObject() const override;
	void initializeObject(CGMarket * object) const override;
	void randomizeObject(CGMarket * object, CRandomGenerator & rng) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CDefaultObjectTypeHandler<CGMarket>&>(*this);
		h & marketModes;
		h & marketEfficiency;
	}
};

VCMI_LIB_NAMESPACE_END

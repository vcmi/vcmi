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
#include "../mapObjects/CGCreature.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/ObstacleSetHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGArtifact;
class CGObjectInstance;
class CGTownInstance;
class CGHeroInstance;
class CGMarket;
class CHeroClass;
class CGCreature;
class CGBoat;
class CFaction;
class CStackBasicDescriptor;

class CObstacleConstructor : public CDefaultObjectTypeHandler<CGObjectInstance>
{
public:
	bool isStaticObject() override;

};

class CreatureInstanceConstructor : public CDefaultObjectTypeHandler<CGCreature>
{
public:
	bool hasNameTextID() const override;
	std::string getNameTextID() const override;
};

class ResourceInstanceConstructor : public CDefaultObjectTypeHandler<CGResource>
{
public:
	bool hasNameTextID() const override;
	std::string getNameTextID() const override;
};

class CTownInstanceConstructor : public CDefaultObjectTypeHandler<CGTownInstance>
{
	JsonNode filtersJson;
protected:
	bool objectFilter(const CGObjectInstance * obj, std::shared_ptr<const ObjectTemplate> tmpl) const override;
	void initTypeData(const JsonNode & input) override;

public:
	const CFaction * faction = nullptr;
	std::map<std::string, LogicalExpression<BuildingID>> filters;

	void initializeObject(CGTownInstance * object) const override;
	void randomizeObject(CGTownInstance * object, vstd::RNG & rng) const override;
	void afterLoadFinalization() override;

	bool hasNameTextID() const override;
	std::string getNameTextID() const override;
};

class CHeroInstanceConstructor : public CDefaultObjectTypeHandler<CGHeroInstance>
{
	struct HeroFilter
	{
		HeroTypeID fixedHero;
		bool allowMale;
		bool allowFemale;
	};

	std::map<std::string, HeroFilter> filters;
	const CHeroClass * heroClass = nullptr;

	std::shared_ptr<const ObjectTemplate> getOverride(TerrainId terrainType, const CGObjectInstance * object) const override;
	void initTypeData(const JsonNode & input) override;

public:
	void randomizeObject(CGHeroInstance * object, vstd::RNG & rng) const override;

	bool hasNameTextID() const override;
	std::string getNameTextID() const override;
};

class DLL_LINKAGE BoatInstanceConstructor : public CDefaultObjectTypeHandler<CGBoat>
{
protected:
	void initTypeData(const JsonNode & config) override;
	
	std::vector<Bonus> bonuses;
	EPathfindingLayer layer;
	bool onboardAssaultAllowed; //if true, hero can attack units from transport
	bool onboardVisitAllowed; //if true, hero can visit objects from transport
	
	AnimationPath actualAnimation; //for OH3 boats those have actual animations
	AnimationPath overlayAnimation; //waves animations
	std::array<AnimationPath, PlayerColor::PLAYER_LIMIT_I> flagAnimations;
	
public:
	void initializeObject(CGBoat * object) const override;

	/// Returns boat preview animation, for use in Shipyards
	AnimationPath getBoatAnimationName() const;
};

class MarketInstanceConstructor : public CDefaultObjectTypeHandler<CGMarket>
{
	std::string descriptionTextID;
	std::string speechTextID;
	
	std::set<EMarketMode> marketModes;
	JsonNode predefinedOffer;
	int marketEfficiency;

	void initTypeData(const JsonNode & config) override;
public:
	CGMarket * createObject(IGameCallback * cb) const override;
	void randomizeObject(CGMarket * object, vstd::RNG & rng) const override;

	const std::set<EMarketMode> & availableModes() const;
	bool hasDescription() const;

	std::string getSpeechTranslated() const;
	int getMarketEfficiency() const;
};

VCMI_LIB_NAMESPACE_END

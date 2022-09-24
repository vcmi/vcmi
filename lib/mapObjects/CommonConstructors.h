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

#include "CObjectClassesHandler.h"
#include "../CTownHandler.h" // for building ID-based filters
#include "MapObjects.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class CGTownInstance;
class CGHeroInstance;
class CGDwelling;
class CHeroClass;
class CBank;
class CStackBasicDescriptor;

/// Class that is used for objects that do not have dedicated handler
template<class ObjectType>
class CDefaultObjectTypeHandler : public AObjectTypeHandler
{
protected:
	ObjectType * createTyped(std::shared_ptr<const ObjectTemplate> tmpl /* = nullptr */) const
	{
		auto obj = new ObjectType();
		preInitObject(obj);

		if (tmpl)
		{
			obj->appearance = tmpl;
		}
		else
		{
			auto templates = getTemplates();
			if (templates.empty())
			{
				throw std::runtime_error("No handler for created object");
			}
			obj->appearance = templates.front(); //just any template for now, will be initialized later
		}

		return obj;
	}
public:
	CDefaultObjectTypeHandler() {}

	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override
	{
		return createTyped(tmpl);
	}

	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override
	{
	}

	virtual std::unique_ptr<IObjectInfo> getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const override
	{
		return nullptr;
	}
};

class CObstacleConstructor : public CDefaultObjectTypeHandler<CGObjectInstance>
{
public:
	CObstacleConstructor();
	bool isStaticObject() override;
};

class CTownInstanceConstructor : public CDefaultObjectTypeHandler<CGTownInstance>
{
	JsonNode filtersJson;
protected:
	bool objectFilter(const CGObjectInstance *, std::shared_ptr<const ObjectTemplate>) const override;
	void initTypeData(const JsonNode & input) override;

public:
	CFaction * faction;
	std::map<std::string, LogicalExpression<BuildingID>> filters;

	CTownInstanceConstructor();
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
	bool objectFilter(const CGObjectInstance *, std::shared_ptr<const ObjectTemplate>) const override;
	void initTypeData(const JsonNode & input) override;

public:
	CHeroClass * heroClass;
	std::map<std::string, LogicalExpression<HeroTypeID>> filters;

	CHeroInstanceConstructor();
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
	bool objectFilter(const CGObjectInstance *, std::shared_ptr<const ObjectTemplate> tmpl) const override;
	void initTypeData(const JsonNode & input) override;

public:

	CDwellingInstanceConstructor();
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

struct BankConfig
{
	BankConfig() { chance = upgradeChance = combatValue = value = 0; };
	ui32 value; //overall value of given things
	ui32 chance; //chance for this level being chosen
	ui32 upgradeChance; //chance for creatures to be in upgraded versions
	ui32 combatValue; //how hard are guards of this level
	std::vector<CStackBasicDescriptor> guards; //creature ID, amount
	Res::ResourceSet resources; //resources given in case of victory
	std::vector<CStackBasicDescriptor> creatures; //creatures granted in case of victory (creature ID, amount)
	std::vector<ArtifactID> artifacts; //artifacts given in case of victory
	std::vector<SpellID> spells; // granted spell(s), for Pyramid

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & chance;
		h & upgradeChance;
		h & guards;
		h & combatValue;
		h & resources;
		h & creatures;
		h & artifacts;
		h & value;
		h & spells;
	}
};

typedef std::vector<std::pair<ui8, IObjectInfo::CArmyStructure>> TPossibleGuards;

template <typename T>
struct DLL_LINKAGE PossibleReward
{
	int chance;
	T data;

	PossibleReward(int chance, const T & data) : chance(chance), data(data) {}
};

class DLL_LINKAGE CBankInfo : public IObjectInfo
{
	const JsonVector & config;
public:
	CBankInfo(const JsonVector & Config);

	TPossibleGuards getPossibleGuards() const;
	std::vector<PossibleReward<TResources>> getPossibleResourcesReward() const;
	std::vector<PossibleReward<CStackBasicDescriptor>> getPossibleCreaturesReward() const;

	// These functions should try to evaluate minimal possible/max possible guards to give provide information on possible thread to AI
	CArmyStructure minGuards() const override;
	CArmyStructure maxGuards() const override;

	bool givesResources() const override;
	bool givesArtifacts() const override;
	bool givesCreatures() const override;
	bool givesSpells() const override;
};

class CBankInstanceConstructor : public CDefaultObjectTypeHandler<CBank>
{
	BankConfig generateConfig(const JsonNode & conf, CRandomGenerator & rng) const;

	JsonVector levels;
protected:
	void initTypeData(const JsonNode & input) override;

public:
	// all banks of this type will be reset N days after clearing,
	si32 bankResetDuration;

	CBankInstanceConstructor();

	CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;

	std::unique_ptr<IObjectInfo> getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & levels;
		h & bankResetDuration;
		h & static_cast<CDefaultObjectTypeHandler<CBank>&>(*this);
	}
};

VCMI_LIB_NAMESPACE_END

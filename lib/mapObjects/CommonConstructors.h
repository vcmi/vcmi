#pragma once

#include "CObjectClassesHandler.h"
#include "../CTownHandler.h" // for building ID-based filters
#include "MapObjects.h"

/*
 * CommonConstructors.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGObjectInstance;
class CGTownInstance;
class CGHeroInstance;
class CGDwelling;
//class CGArtifact;
//class CGCreature;
class CHeroClass;
class CBank;
class CStackBasicDescriptor;

/// Class that is used for objects that do not have dedicated handler
template<class ObjectType>
class CDefaultObjectTypeHandler : public AObjectTypeHandler
{
protected:
	ObjectType * createTyped(ObjectTemplate tmpl) const
	{
		auto obj = new ObjectType();
		obj->ID = tmpl.id;
		obj->subID = tmpl.subid;
		obj->appearance = tmpl;
		return obj;
	}
public:
	CDefaultObjectTypeHandler(){}

	CGObjectInstance * create(ObjectTemplate tmpl) const override
	{
		return createTyped(tmpl);
	}

	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override
	{
	}

	virtual std::unique_ptr<IObjectInfo> getObjectInfo(ObjectTemplate tmpl) const override
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
	bool objectFilter(const CGObjectInstance *, const ObjectTemplate &) const override;
	void initTypeData(const JsonNode & input) override;

public:
	CFaction * faction;
	std::map<std::string, LogicalExpression<BuildingID>> filters;

	CTownInstanceConstructor();
	CGObjectInstance * create(ObjectTemplate tmpl) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;
	void afterLoadFinalization() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & filtersJson & faction & filters;
		h & static_cast<CDefaultObjectTypeHandler<CGTownInstance>&>(*this);
	}
};

class CHeroInstanceConstructor : public CDefaultObjectTypeHandler<CGHeroInstance>
{
	JsonNode filtersJson;
protected:
	bool objectFilter(const CGObjectInstance *, const ObjectTemplate &) const override;
	void initTypeData(const JsonNode & input) override;

public:
	CHeroClass * heroClass;
	std::map<std::string, LogicalExpression<HeroTypeID>> filters;

	CHeroInstanceConstructor();
	CGObjectInstance * create(ObjectTemplate tmpl) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;
	void afterLoadFinalization() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & filtersJson & heroClass & filters;
		h & static_cast<CDefaultObjectTypeHandler<CGHeroInstance>&>(*this);
	}
};

class CDwellingInstanceConstructor : public CDefaultObjectTypeHandler<CGDwelling>
{
	std::vector<std::vector<const CCreature *>> availableCreatures;

	JsonNode guards;

protected:
	bool objectFilter(const CGObjectInstance *, const ObjectTemplate &) const override;
	void initTypeData(const JsonNode & input) override;

public:

	CDwellingInstanceConstructor();
	CGObjectInstance * create(ObjectTemplate tmpl) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;

	bool producesCreature(const CCreature * crea) const;
	std::vector<const CCreature *> getProducedCreatures() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & availableCreatures & guards;
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
		h & chance & upgradeChance & guards & combatValue & resources & creatures & artifacts & value & spells;
	}
};

typedef std::vector<std::pair<ui8, IObjectInfo::CArmyStructure>> TPossibleGuards;

class DLL_LINKAGE CBankInfo : public IObjectInfo
{
	const JsonVector & config;
public:
	CBankInfo(const JsonVector & Config);

	TPossibleGuards getPossibleGuards() const;

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

	CGObjectInstance *create(ObjectTemplate tmpl) const override;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const override;

	std::unique_ptr<IObjectInfo> getObjectInfo(ObjectTemplate tmpl) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & levels & bankResetDuration;
		h & static_cast<CDefaultObjectTypeHandler<CBank>&>(*this);
	}
};

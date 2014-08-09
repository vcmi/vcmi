#pragma once

#include "CObjectClassesHandler.h"
#include "../CTownHandler.h" // for building ID-based filters

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

	CGObjectInstance * create(ObjectTemplate tmpl) const
	{
		return createTyped(tmpl);
	}

	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
	{
	}

	virtual std::unique_ptr<IObjectInfo> getObjectInfo(ObjectTemplate tmpl) const
	{
		return nullptr;
	}
};

class CObstacleConstructor : public CDefaultObjectTypeHandler<CGObjectInstance>
{
public:
	CObstacleConstructor();
	bool isStaticObject();
};

class CTownInstanceConstructor : public CDefaultObjectTypeHandler<CGTownInstance>
{
	JsonNode filtersJson;
protected:
	bool objectFilter(const CGObjectInstance *, const ObjectTemplate &) const;
	void initTypeData(const JsonNode & input);

public:
	CFaction * faction;
	std::map<std::string, LogicalExpression<BuildingID>> filters;

	CTownInstanceConstructor();
	CGObjectInstance * create(ObjectTemplate tmpl) const;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const;
	void afterLoadFinalization();

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
	bool objectFilter(const CGObjectInstance *, const ObjectTemplate &) const;
	void initTypeData(const JsonNode & input);

public:
	CHeroClass * heroClass;
	std::map<std::string, LogicalExpression<HeroTypeID>> filters;

	CHeroInstanceConstructor();
	CGObjectInstance * create(ObjectTemplate tmpl) const;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const;
	void afterLoadFinalization();

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
	bool objectFilter(const CGObjectInstance *, const ObjectTemplate &) const;
	void initTypeData(const JsonNode & input);

public:

	CDwellingInstanceConstructor();
	CGObjectInstance * create(ObjectTemplate tmpl) const;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const;

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

class CBankInfo : public IObjectInfo
{
	JsonVector config;
public:
	CBankInfo(JsonVector config);

	CArmyStructure minGuards() const;
	CArmyStructure maxGuards() const;
	bool givesResources() const;
	bool givesArtifacts() const;
	bool givesCreatures() const;
	bool givesSpells() const;
};

class CBankInstanceConstructor : public CDefaultObjectTypeHandler<CBank>
{
	BankConfig generateConfig(const JsonNode & conf, CRandomGenerator & rng) const;

	JsonVector levels;
protected:
	void initTypeData(const JsonNode & input);

public:
	// all banks of this type will be reset N days after clearing,
	si32 bankResetDuration;

	CBankInstanceConstructor();

	CGObjectInstance *create(ObjectTemplate tmpl) const;
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const;

	std::unique_ptr<IObjectInfo> getObjectInfo(ObjectTemplate tmpl) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & levels & bankResetDuration;
		h & static_cast<CDefaultObjectTypeHandler<CBank>&>(*this);
	}
};

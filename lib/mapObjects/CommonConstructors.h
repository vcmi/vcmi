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

class CGTownInstance;
class CGHeroInstance;
class CGDwelling;
//class CGArtifact;
//class CGCreature;
class CHeroClass;

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

	virtual const IObjectInfo * getObjectInfo(ObjectTemplate tmpl) const
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

public:
	CFaction * faction;
	std::map<std::string, LogicalExpression<BuildingID>> filters;

	CTownInstanceConstructor();
	CGObjectInstance * create(ObjectTemplate tmpl) const;
	void initTypeData(const JsonNode & input);
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

public:
	CHeroClass * heroClass;
	std::map<std::string, LogicalExpression<HeroTypeID>> filters;

	CHeroInstanceConstructor();
	CGObjectInstance * create(ObjectTemplate tmpl) const;
	void initTypeData(const JsonNode & input);
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

public:

	CDwellingInstanceConstructor();
	CGObjectInstance * create(ObjectTemplate tmpl) const;
	void initTypeData(const JsonNode & input);
	void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const;

	bool producesCreature(const CCreature * crea) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & availableCreatures & guards;
		h & static_cast<CDefaultObjectTypeHandler<CGDwelling>&>(*this);
	}
};

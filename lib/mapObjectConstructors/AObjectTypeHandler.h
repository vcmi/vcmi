/*
 * AObjectTypeHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "RandomMapInfo.h"
#include "SObjectSounds.h"
#include "../JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class ObjectTemplate;
class CGObjectInstance;
class CRandomGenerator;
class IObjectInfo;

/// Class responsible for creation of objects of specific type & subtype
class DLL_LINKAGE AObjectTypeHandler : public boost::noncopyable
{
	friend class CObjectClassesHandler;

	RandomMapInfo rmgInfo;

	JsonNode base; /// describes base template

	std::vector<std::shared_ptr<const ObjectTemplate>> templates;

	SObjectSounds sounds;

	std::optional<si32> aiValue;
	std::optional<std::string> battlefield;

	std::string modScope;
	std::string typeName;
	std::string subTypeName;

	si32 type;
	si32 subtype;

protected:
	void preInitObject(CGObjectInstance * obj) const;
	virtual bool objectFilter(const CGObjectInstance * obj, std::shared_ptr<const ObjectTemplate> tmpl) const;

	/// initialization for classes that inherit this one
	virtual void initTypeData(const JsonNode & input);
public:

	virtual ~AObjectTypeHandler() = default;

	si32 getIndex() const;
	si32 getSubIndex() const;

	std::string getTypeName() const;
	std::string getSubTypeName() const;

	/// loads generic data from Json structure and passes it towards type-specific constructors
	void init(const JsonNode & input);

	/// returns full form of identifier of this object in form of modName:objectName
	std::string getJsonKey() const;

	/// Returns object-specific name, if set
	SObjectSounds getSounds() const;

	void addTemplate(const std::shared_ptr<const ObjectTemplate> & templ);
	void addTemplate(JsonNode config);
	void clearTemplates();

	/// returns all templates matching parameters
	std::vector<std::shared_ptr<const ObjectTemplate>> getTemplates() const;
	std::vector<std::shared_ptr<const ObjectTemplate>> getTemplates(const TerrainId terrainType) const;

	/// returns preferred template for this object, if present (e.g. one of 3 possible templates for town - village, fort and castle)
	/// note that appearance will not be changed - this must be done separately (either by assignment or via pack from server)
	std::shared_ptr<const ObjectTemplate> getOverride(TerrainId terrainType, const CGObjectInstance * object) const;

	BattleField getBattlefield() const;

	const RandomMapInfo & getRMGInfo();

	std::optional<si32> getAiValue() const;

	/// returns true if this class provides custom text ID's instead of generic per-object name
	virtual bool hasNameTextID() const;

	/// returns object's name in form of translatable text ID
	virtual std::string getNameTextID() const;

	/// returns object's name in form of human-readable text
	std::string getNameTranslated() const;

	virtual bool isStaticObject();

	virtual void afterLoadFinalization();

	/// Creates object and set up core properties (like ID/subID). Object is NOT initialized
	/// to allow creating objects before game start (e.g. map loading)
	virtual CGObjectInstance * create(std::shared_ptr<const ObjectTemplate> tmpl = nullptr) const = 0;

	/// Configures object properties. Should be re-entrable, resetting state of the object if necessarily
	/// This should set remaining properties, including randomized or depending on map
	virtual void configureObject(CGObjectInstance * object, CRandomGenerator & rng) const = 0;

	/// Returns object configuration, if available. Otherwise returns NULL
	virtual std::unique_ptr<IObjectInfo> getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type;
		h & subtype;
		h & templates;
		h & rmgInfo;
		h & modScope;
		h & typeName;
		h & subTypeName;
		h & sounds;
		h & aiValue;
		h & battlefield;
	}
};

VCMI_LIB_NAMESPACE_END

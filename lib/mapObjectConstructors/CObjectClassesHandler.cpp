/*
 * CObjectClassesHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CConfigHandler.h"
#include "CObjectClassesHandler.h"

#include "../filesystem/Filesystem.h"
#include "../filesystem/CBinaryReader.h"
#include "../json/JsonUtils.h"
#include "../GameLibrary.h"
#include "../GameConstants.h"
#include "../constants/StringConstants.h"
#include "../IGameSettings.h"
#include "../CSoundBase.h"

#include "../mapObjectConstructors/CRewardableConstructor.h"
#include "../mapObjectConstructors/CommonConstructors.h"
#include "../mapObjectConstructors/DwellingInstanceConstructor.h"
#include "../mapObjectConstructors/FlaggableInstanceConstructor.h"
#include "../mapObjectConstructors/HillFortInstanceConstructor.h"
#include "../mapObjectConstructors/MarketInstanceConstructor.h"
#include "../mapObjectConstructors/ShipyardInstanceConstructor.h"

#include "../mapObjects/CGCreature.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGMarket.h"
#include "../mapObjects/CGPandoraBox.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapObjects/CQuest.h"
#include "../mapObjects/FlaggableMapObject.h"
#include "../mapObjects/MiscObjects.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../mapObjects/ObstacleSetHandler.h"

#include "../modding/IdentifierStorage.h"
#include "../modding/CModHandler.h"
#include "../modding/ModScope.h"
#include "../texts/CGeneralTextHandler.h"
#include "../texts/CLegacyConfigParser.h"

#include <vstd/StringUtils.h>

VCMI_LIB_NAMESPACE_BEGIN

CObjectClassesHandler::CObjectClassesHandler()
{
#define SET_HANDLER_CLASS(STRING, CLASSNAME) handlerConstructors[STRING] = std::make_shared<CLASSNAME>
#define SET_HANDLER(STRING, TYPENAME) handlerConstructors[STRING] = std::make_shared<CDefaultObjectTypeHandler<TYPENAME>>

	// list of all known handlers, hardcoded for now since the only way to add new objects is via C++ code
	//Note: should be in sync with registerTypesMapObjectTypes function
	SET_HANDLER_CLASS("configurable", CRewardableConstructor);
	SET_HANDLER_CLASS("dwelling", DwellingInstanceConstructor);
	SET_HANDLER_CLASS("hero", CHeroInstanceConstructor);
	SET_HANDLER_CLASS("town", CTownInstanceConstructor);
	SET_HANDLER_CLASS("boat", BoatInstanceConstructor);
	SET_HANDLER_CLASS("flaggable", FlaggableInstanceConstructor);
	SET_HANDLER_CLASS("market", MarketInstanceConstructor);
	SET_HANDLER_CLASS("hillFort", HillFortInstanceConstructor);
	SET_HANDLER_CLASS("shipyard", ShipyardInstanceConstructor);
	SET_HANDLER_CLASS("monster", CreatureInstanceConstructor);
	SET_HANDLER_CLASS("resource", ResourceInstanceConstructor);

	SET_HANDLER_CLASS("static", CObstacleConstructor);
	SET_HANDLER_CLASS("", CObstacleConstructor);

	SET_HANDLER("randomArtifact", CGArtifact);
	SET_HANDLER("randomHero", CGHeroInstance);
	SET_HANDLER("randomResource", CGResource);
	SET_HANDLER("randomTown", CGTownInstance);
	SET_HANDLER("randomMonster", CGCreature);
	SET_HANDLER("randomDwelling", CGDwelling);

	SET_HANDLER("generic", CGObjectInstance);
	SET_HANDLER("artifact", CGArtifact);
	SET_HANDLER("borderGate", CGBorderGate);
	SET_HANDLER("borderGuard", CGBorderGuard);
	SET_HANDLER("denOfThieves", CGDenOfthieves);
	SET_HANDLER("event", CGEvent);
	SET_HANDLER("garrison", CGGarrison);
	SET_HANDLER("heroPlaceholder", CGHeroPlaceholder);
	SET_HANDLER("keymaster", CGKeymasterTent);
	SET_HANDLER("magi", CGMagi);
	SET_HANDLER("mine", CGMine);
	SET_HANDLER("obelisk", CGObelisk);
	SET_HANDLER("pandora", CGPandoraBox);
	SET_HANDLER("prison", CGHeroInstance);
	SET_HANDLER("questGuard", CGQuestGuard);
	SET_HANDLER("seerHut", CGSeerHut);
	SET_HANDLER("sign", CGSignBottle);
	SET_HANDLER("siren", CGSirens);
	SET_HANDLER("monolith", CGMonolith);
	SET_HANDLER("subterraneanGate", CGSubterraneanGate);
	SET_HANDLER("whirlpool", CGWhirlpool);
	SET_HANDLER("terrain", CGTerrainPatch);

#undef SET_HANDLER_CLASS
#undef SET_HANDLER
}

CObjectClassesHandler::~CObjectClassesHandler() = default;

std::vector<JsonNode> CObjectClassesHandler::loadLegacyData()
{
	size_t dataSize = LIBRARY->engineSettings()->getInteger(EGameSettings::TEXTS_OBJECT);

	CLegacyConfigParser parser(TextPath::builtin("Data/Objects.txt"));
	auto totalNumber = static_cast<size_t>(parser.readNumber()); // first line contains number of objects to read and nothing else
	parser.endLine();

	for (size_t i = 0; i < totalNumber; i++)
	{
		auto tmpl = std::make_shared<ObjectTemplate>();

		tmpl->readTxt(parser);
		parser.endLine();

		std::pair key(tmpl->id, tmpl->subid);
		legacyTemplates.insert(std::make_pair(key, tmpl));
	}

	mapObjectTypes.resize(256);

	std::vector<JsonNode> ret(dataSize);// create storage for 256 objects
	assert(dataSize == 256);

	CLegacyConfigParser namesParser(TextPath::builtin("Data/ObjNames.txt"));
	for (size_t i=0; i<256; i++)
	{
		ret[i]["name"].String() = namesParser.readString();
		namesParser.endLine();
	}

	JsonNode cregen1;
	JsonNode cregen4;

	CLegacyConfigParser cregen1Parser(TextPath::builtin("data/crgen1"));
	do
	{
		JsonNode subObject;
		subObject["name"].String() = cregen1Parser.readString();
		cregen1.Vector().push_back(subObject);
	}
	while(cregen1Parser.endLine());

	CLegacyConfigParser cregen4Parser(TextPath::builtin("data/crgen4"));
	do
	{
		JsonNode subObject;
		subObject["name"].String() = cregen4Parser.readString();
		cregen4.Vector().push_back(subObject);
	}
	while(cregen4Parser.endLine());

	ret[Obj::CREATURE_GENERATOR1]["subObjects"] = cregen1;
	ret[Obj::CREATURE_GENERATOR4]["subObjects"] = cregen4;

	ret[Obj::REFUGEE_CAMP]["subObjects"].Vector().push_back(ret[Obj::REFUGEE_CAMP]);
	ret[Obj::WAR_MACHINE_FACTORY]["subObjects"].Vector().push_back(ret[Obj::WAR_MACHINE_FACTORY]);

	return ret;
}

void CObjectClassesHandler::loadSubObject(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * baseObject)
{
	auto subObject = loadSubObjectFromJson(scope, identifier, entry, baseObject, baseObject->objectTypeHandlers.size());

	assert(subObject);
	baseObject->objectTypeHandlers.push_back(subObject);

	registerObject(scope, baseObject->getJsonKey(), subObject->getSubTypeName(), subObject->subtype);
	for(const auto & compatID : entry["compatibilityIdentifiers"].Vector())
	{
		if (identifier != compatID.String())
			registerObject(scope, baseObject->getJsonKey(), compatID.String(), subObject->subtype);
		else
			logMod->warn("Mod '%s' map object '%s': compatibility identifier has same name as object itself!", scope, identifier);
	}
}

void CObjectClassesHandler::loadSubObject(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * baseObject, size_t index)
{
	auto subObject = loadSubObjectFromJson(scope, identifier, entry, baseObject, index);

	assert(subObject);
	if (baseObject->objectTypeHandlers.at(index) != nullptr)
		throw std::runtime_error("Attempt to load already loaded object:" + identifier);

	baseObject->objectTypeHandlers.at(index) = subObject;

	registerObject(scope, baseObject->getJsonKey(), subObject->getSubTypeName(), subObject->subtype);
	for(const auto & compatID : entry["compatibilityIdentifiers"].Vector())
	{
		if (identifier != compatID.String())
			registerObject(scope, baseObject->getJsonKey(), compatID.String(), subObject->subtype);
		else
			logMod->warn("Mod '%s' map object '%s': compatibility identifier has same name as object itself!");
	}
}

TObjectTypeHandler CObjectClassesHandler::loadSubObjectFromJson(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * baseObject, size_t index)
{
	assert(!scope.empty());

	if (settings["mods"]["validation"].String() != "off")
	{
		size_t separator = identifier.find(':');

		if (separator != std::string::npos)
		{
			std::string modName = identifier.substr(0, separator);
			std::string objectName = identifier.substr(separator + 1);
			logMod->warn("Mod %s: Map object type with format '%s' will add new map object, not modify it! Please use '%s' form and add dependency on mod '%s' instead!", scope, identifier, modName, identifier );
		}
	}

	std::string handler = baseObject->handlerName;
	if(!handlerConstructors.count(handler))
	{
		logMod->error("Handler with name %s was not found!", handler);
		// workaround for potential crash - if handler does not exists, continue with generic handler that is used for objects without any custom logc
		handler = "generic";
		assert(handlerConstructors.count(handler) != 0);
	}

	auto createdObject = handlerConstructors.at(handler)();

	createdObject->modScope = scope;
	createdObject->typeName = baseObject->identifier;
	createdObject->subTypeName = identifier;

	createdObject->type = baseObject->id;
	createdObject->subtype = index;
	createdObject->init(entry);

	bool staticObject = createdObject->isStaticObject();
	if (staticObject)
	{
		for (auto & templ : createdObject->getTemplates())
		{
			// Register templates for new objects from mods
			LIBRARY->biomeHandler->addTemplate(scope, templ->stringID, templ);
		}
	}

	auto range = legacyTemplates.equal_range(std::make_pair(baseObject->id, index));
	for (auto & templ : boost::make_iterator_range(range.first, range.second))
	{
		if (staticObject)
		{
			// Register legacy templates as "core"
			// FIXME: Why does it clear stringID?
			LIBRARY->biomeHandler->addTemplate("core", templ.second->stringID, templ.second);
		}

		createdObject->addTemplate(templ.second);

	}
	legacyTemplates.erase(range.first, range.second);

	logGlobal->debug("Loaded object %s(%d)::%s(%d)", baseObject->getJsonKey(), baseObject->id, identifier, index);

	return createdObject;
}

ObjectClass::ObjectClass() = default;
ObjectClass::~ObjectClass() = default;

std::string ObjectClass::getJsonKey() const
{
	return modScope + ':' + identifier;
}

std::string ObjectClass::getNameTextID() const
{
	return TextIdentifier("object", modScope, identifier, "name").get();
}

std::string ObjectClass::getNameTranslated() const
{
	return LIBRARY->generaltexth->translate(getNameTextID());
}

std::unique_ptr<ObjectClass> CObjectClassesHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & name, size_t index)
{
	auto newObject = std::make_unique<ObjectClass>();

	newObject->modScope = scope;
	newObject->identifier = name;
	newObject->handlerName = json["handler"].String();
	newObject->base = json["base"];
	newObject->id = index;

	LIBRARY->generaltexth->registerString(scope, newObject->getNameTextID(), json["name"]);

	newObject->objectTypeHandlers.resize(json["lastReservedIndex"].Float() + 1);

	for (auto subData : json["types"].Struct())
	{
		if (!subData.second["index"].isNull())
		{
			const std::string & subMeta = subData.second["index"].getModScope();

			if ( subMeta == "core")
			{
				size_t subIndex = subData.second["index"].Integer();
				loadSubObject(subData.second.getModScope(), subData.first, subData.second, newObject.get(), subIndex);
			}
			else
			{
				logMod->error("Object %s:%s.%s - attempt to load object with preset index! This option is reserved for built-in mod", subMeta, name, subData.first );
				loadSubObject(subData.second.getModScope(), subData.first, subData.second, newObject.get());
			}
		}
		else
			loadSubObject(subData.second.getModScope(), subData.first, subData.second, newObject.get());
	}

	if (newObject->id == MapObjectID::MONOLITH_TWO_WAY)
		generateExtraMonolithsForRMG(newObject.get());

	return newObject;
}

void CObjectClassesHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	mapObjectTypes.push_back(loadFromJson(scope, data, name, mapObjectTypes.size()));

	LIBRARY->identifiersHandler->registerObject(scope, "object", name, mapObjectTypes.back()->id);
}

void CObjectClassesHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	assert(mapObjectTypes.at(index) == nullptr); // ensure that this id was not loaded before

	mapObjectTypes.at(index) = loadFromJson(scope, data, name, index);
	LIBRARY->identifiersHandler->registerObject(scope, "object", name, mapObjectTypes.at(index)->id);
}

void CObjectClassesHandler::loadSubObject(const std::string & identifier, JsonNode config, MapObjectID ID, MapObjectSubID subID)
{
	config.setType(JsonNode::JsonType::DATA_STRUCT); // ensure that input is not NULL

	assert(mapObjectTypes.at(ID.getNum()));

	if (subID.getNum() >= mapObjectTypes.at(ID.getNum())->objectTypeHandlers.size())
	{
		mapObjectTypes.at(ID.getNum())->objectTypeHandlers.resize(subID.getNum() + 1);
	}

	JsonUtils::inherit(config, mapObjectTypes.at(ID.getNum())->base);
	for (auto & templ : config["templates"].Struct())
		JsonUtils::inherit(templ.second, config["base"]);

	if (settings["mods"]["validation"].String() != "off")
		JsonUtils::validate(config, "vcmi:objectType", identifier);

	loadSubObject(config.getModScope(), identifier, config, mapObjectTypes.at(ID.getNum()).get(), subID.getNum());
}

void CObjectClassesHandler::removeSubObject(MapObjectID ID, MapObjectSubID subID)
{
	assert(mapObjectTypes.at(ID.getNum()));
	mapObjectTypes.at(ID.getNum())->objectTypeHandlers.at(subID.getNum()) = nullptr;
}

TObjectTypeHandler CObjectClassesHandler::getHandlerFor(MapObjectID type, MapObjectSubID subtype) const
{
	try
	{
		if (mapObjectTypes.at(type.getNum()) == nullptr)
			return mapObjectTypes.front()->objectTypeHandlers.front();

		auto subID = subtype.getNum();
		if (type == Obj::PRISON || type == Obj::HERO_PLACEHOLDER || type == Obj::SPELL_SCROLL)
			subID = 0;
		auto result = mapObjectTypes.at(type.getNum())->objectTypeHandlers.at(subID);

		if (result != nullptr)
			return result;
	}
	catch (std::out_of_range & e)
	{
		// Leave catch block silently and handle error in block outside of try ... catch - all valid values should use 'return' in try block
	}

	std::string errorString = "Failed to find object of type " + std::to_string(type.getNum()) + "::" + std::to_string(subtype.getNum());
	logGlobal->error(errorString);
	throw std::out_of_range(errorString);
}

TObjectTypeHandler CObjectClassesHandler::getHandlerFor(const std::string & scope, const std::string & type, const std::string & subtype) const
{
	std::optional<si32> id = LIBRARY->identifiers()->getIdentifier(scope, "object", type);
	if(id)
	{
		const auto & object = mapObjectTypes.at(id.value());
		std::optional<si32> subID = LIBRARY->identifiers()->getIdentifier(scope, object->getJsonKey(), subtype);

		if (subID)
			return object->objectTypeHandlers.at(subID.value());
	}

	std::string errorString = "Failed to find object of type " + type + "::" + subtype;
	logGlobal->error(errorString);
	throw std::runtime_error(errorString);
}

TObjectTypeHandler CObjectClassesHandler::getHandlerFor(CompoundMapObjectID compoundIdentifier) const
{
	return getHandlerFor(compoundIdentifier.primaryID, compoundIdentifier.secondaryID);
}

CompoundMapObjectID CObjectClassesHandler::getCompoundIdentifier(const std::string & scope, const std::string & type, const std::string & subtype) const
{
	std::optional<si32> id;
	if (scope.empty())
	{
		id = LIBRARY->identifiers()->getIdentifier("object", type);
	}
	else
	{
		id = LIBRARY->identifiers()->getIdentifier(scope, "object", type);
	}

	if(id)
	{
		if (subtype.empty())
			return CompoundMapObjectID(id.value(), 0);

		const auto & object = mapObjectTypes.at(id.value());
		std::optional<si32> subID = LIBRARY->identifiers()->getIdentifier(scope, object->getJsonKey(), subtype);

		if (subID)
			return CompoundMapObjectID(id.value(), subID.value());
	}

	std::string errorString = "Failed to get id for object of type " + type + "." + subtype;
	logGlobal->error(errorString);
	throw std::runtime_error(errorString);
}

CompoundMapObjectID CObjectClassesHandler::getCompoundIdentifier(const std::string & objectName) const
{
	std::string subtype = "object"; //Default for objects with no subIds
	std::string type;

	auto scopeAndFullName = vstd::splitStringToPair(objectName, ':');
	logGlobal->debug("scopeAndFullName: %s, %s", scopeAndFullName.first, scopeAndFullName.second);
	
	auto typeAndName = vstd::splitStringToPair(scopeAndFullName.second, '.');
	logGlobal->debug("typeAndName: %s, %s", typeAndName.first, typeAndName.second);
	
	auto nameAndSubtype = vstd::splitStringToPair(typeAndName.second, '.');
	logGlobal->debug("nameAndSubtype: %s, %s", nameAndSubtype.first, nameAndSubtype.second);

	if (!nameAndSubtype.first.empty())
	{
		type = nameAndSubtype.first;
		subtype = nameAndSubtype.second;
	}
	else
	{
		type = typeAndName.second;
	}
	
	return getCompoundIdentifier(boost::to_lower_copy(scopeAndFullName.first), type, subtype);
}

std::set<MapObjectID> CObjectClassesHandler::knownObjects() const
{
	std::set<MapObjectID> ret;

	for(auto & entry : mapObjectTypes)
		if (entry)
			ret.insert(entry->id);

	return ret;
}

std::set<MapObjectSubID> CObjectClassesHandler::knownSubObjects(MapObjectID primaryID) const
{
	std::set<MapObjectSubID> ret;

	if (!mapObjectTypes.at(primaryID.getNum()))
	{
		logGlobal->error("Failed to find object %d", primaryID);
		return ret;
	}

	for(const auto & entry : mapObjectTypes.at(primaryID.getNum())->objectTypeHandlers)
		if (entry)
			ret.insert(entry->subtype);

	return ret;
}

void CObjectClassesHandler::beforeValidate(JsonNode & object)
{
	for (auto & entry : object["types"].Struct())
	{
		if (object.Struct().count("subObjects"))
		{
			const auto & vector = object["subObjects"].Vector();

			if (entry.second.Struct().count("index"))
			{
				size_t index = entry.second["index"].Integer();

				if (index < vector.size())
					JsonUtils::inherit(entry.second, vector[index]);
			}
		}

		JsonUtils::inherit(entry.second, object["base"]);
		for (auto & templ : entry.second["templates"].Struct())
			JsonUtils::inherit(templ.second, entry.second["base"]);
	}
	object.Struct().erase("subObjects");
}

void CObjectClassesHandler::afterLoadFinalization()
{
	for(auto & entry : mapObjectTypes)
	{
		if (!entry)
			continue;

		for(const auto & obj : entry->objectTypeHandlers)
		{
			if (!obj)
				continue;

			obj->afterLoadFinalization();
			if(obj->getTemplates().empty())
				logMod->debug("No templates found for %s:%s", entry->getJsonKey(), obj->getJsonKey());
		}
	}

	for(auto & entry : objectIdHandlers)
	{
		// Call function for each object id
		entry.second(entry.first);
	}
}

void CObjectClassesHandler::resolveObjectCompoundId(const std::string & id, std::function<void(CompoundMapObjectID)> callback)
{
	auto compoundId = getCompoundIdentifier(id);
	objectIdHandlers.push_back(std::make_pair(compoundId, callback));
}

void CObjectClassesHandler::generateExtraMonolithsForRMG(ObjectClass * container)
{
	//duplicate existing two-way portals to make reserve for RMG
	auto& portalVec = container->objectTypeHandlers;
	//FIXME: Monoliths  in this vector can be already not useful for every terrain
	const size_t portalCount = portalVec.size();

	//Invalid portals will be skipped and portalVec size stays unchanged
	for (size_t i = portalCount; portalVec.size() < 100; ++i)
	{
		auto index = static_cast<si32>(i % portalCount);
		auto portal = portalVec[index];
		auto templates = portal->getTemplates();
		if (templates.empty() || !templates[0]->canBePlacedAtAnyTerrain())
		{
			continue; //Do not clone HoTA water-only portals or any others we can't use
		}

		//deep copy of noncopyable object :?
		auto newPortal = std::make_shared<CDefaultObjectTypeHandler<CGMonolith>>();
		newPortal->rmgInfo = portal->getRMGInfo();
		newPortal->templates = portal->getTemplates();
		newPortal->sounds = portal->getSounds();
		newPortal->aiValue = portal->getAiValue();
		newPortal->battlefield = portal->battlefield; //getter is not initialized at this point
		newPortal->modScope = portal->modScope; //private
		newPortal->typeName = portal->getTypeName(); 
		newPortal->subTypeName = std::string("monolith") + std::to_string(portalVec.size());
		newPortal->type = portal->getIndex();

		// Inconsintent original indexing: monolith1 has index 0
		newPortal->subtype = portalVec.size() - 1; //indexes must be unique, they are returned as a set
		newPortal->blockVisit = portal->blockVisit;
		newPortal->removable = portal->removable;

		portalVec.push_back(newPortal);

		registerObject(newPortal->modScope, container->getJsonKey(), newPortal->subTypeName, newPortal->subtype);
	}
}

std::string CObjectClassesHandler::getObjectName(MapObjectID type, MapObjectSubID subtype) const
{
	const auto handler = getHandlerFor(type, subtype);
	if (handler && handler->hasNameTextID())
		return handler->getNameTranslated();

	if (mapObjectTypes.at(type.getNum()))
		return mapObjectTypes.at(type.getNum())->getNameTranslated();

	return mapObjectTypes.front()->getNameTranslated();
}

SObjectSounds CObjectClassesHandler::getObjectSounds(MapObjectID type, MapObjectSubID subtype) const
{
	// TODO: these objects may have subID's that does not have associated handler:
	// Prison: uses hero type as subID
	// Hero: uses hero type as subID, but registers hero classes as subtypes
	// Spell scroll: uses spell ID as subID
	if(type == Obj::PRISON || type == Obj::HERO || type == Obj::SPELL_SCROLL)
		subtype = 0;

	if(mapObjectTypes.at(type.getNum()))
		return getHandlerFor(type, subtype)->getSounds();
	else
		return mapObjectTypes.front()->objectTypeHandlers.front()->getSounds();
}

std::string CObjectClassesHandler::getObjectHandlerName(MapObjectID type) const
{
	if (mapObjectTypes.at(type.getNum()))
		return mapObjectTypes.at(type.getNum())->handlerName;
	else
		return mapObjectTypes.front()->handlerName;
}

std::string CObjectClassesHandler::getJsonKey(MapObjectID type) const
{
	if (mapObjectTypes.at(type.getNum()) != nullptr)
		return mapObjectTypes.at(type.getNum())->getJsonKey();

	logGlobal->warn("Unknown object of type %d!", type);
	return mapObjectTypes.front()->getJsonKey();
}

VCMI_LIB_NAMESPACE_END

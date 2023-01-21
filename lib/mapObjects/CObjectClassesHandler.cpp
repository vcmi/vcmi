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
#include "CObjectClassesHandler.h"

#include "../filesystem/Filesystem.h"
#include "../filesystem/CBinaryReader.h"
#include "../VCMI_Lib.h"
#include "../GameConstants.h"
#include "../StringConstants.h"
#include "../CGeneralTextHandler.h"
#include "../CModHandler.h"
#include "../JsonNode.h"
#include "../CSoundBase.h"

#include "CRewardableConstructor.h"
#include "CommonConstructors.h"
#include "MapObjects.h"

VCMI_LIB_NAMESPACE_BEGIN

CObjectClassesHandler::CObjectClassesHandler()
{
#define SET_HANDLER_CLASS(STRING, CLASSNAME) handlerConstructors[STRING] = std::make_shared<CLASSNAME>;
#define SET_HANDLER(STRING, TYPENAME) handlerConstructors[STRING] = std::make_shared<CDefaultObjectTypeHandler<TYPENAME>>

	// list of all known handlers, hardcoded for now since the only way to add new objects is via C++ code
	//Note: should be in sync with registerTypesMapObjectTypes function
	SET_HANDLER_CLASS("configurable", CRewardableConstructor);
	SET_HANDLER_CLASS("dwelling", CDwellingInstanceConstructor);
	SET_HANDLER_CLASS("hero", CHeroInstanceConstructor);
	SET_HANDLER_CLASS("town", CTownInstanceConstructor);
	SET_HANDLER_CLASS("bank", CBankInstanceConstructor);

	SET_HANDLER_CLASS("static", CObstacleConstructor);
	SET_HANDLER_CLASS("", CObstacleConstructor);

	SET_HANDLER("randomArtifact", CGArtifact);
	SET_HANDLER("randomHero", CGHeroInstance);
	SET_HANDLER("randomResource", CGResource);
	SET_HANDLER("randomTown", CGTownInstance);
	SET_HANDLER("randomMonster", CGCreature);
	SET_HANDLER("randomDwelling", CGDwelling);

	SET_HANDLER("generic", CGObjectInstance);
	SET_HANDLER("cartographer", CCartographer);
	SET_HANDLER("artifact", CGArtifact);
	SET_HANDLER("blackMarket", CGBlackMarket);
	SET_HANDLER("boat", CGBoat);
	SET_HANDLER("bonusingObject", CGBonusingObject);
	SET_HANDLER("borderGate", CGBorderGate);
	SET_HANDLER("borderGuard", CGBorderGuard);
	SET_HANDLER("monster", CGCreature);
	SET_HANDLER("denOfThieves", CGDenOfthieves);
	SET_HANDLER("event", CGEvent);
	SET_HANDLER("garrison", CGGarrison);
	SET_HANDLER("heroPlaceholder", CGHeroPlaceholder);
	SET_HANDLER("keymaster", CGKeymasterTent);
	SET_HANDLER("lighthouse", CGLighthouse);
	SET_HANDLER("magi", CGMagi);
	SET_HANDLER("magicSpring", CGMagicSpring);
	SET_HANDLER("magicWell", CGMagicWell);
	SET_HANDLER("market", CGMarket);
	SET_HANDLER("mine", CGMine);
	SET_HANDLER("obelisk", CGObelisk);
	SET_HANDLER("observatory", CGObservatory);
	SET_HANDLER("onceVisitable", CGOnceVisitable);
	SET_HANDLER("pandora", CGPandoraBox);
	SET_HANDLER("prison", CGHeroInstance);
	SET_HANDLER("questGuard", CGQuestGuard);
	SET_HANDLER("resource", CGResource);
	SET_HANDLER("scholar", CGScholar);
	SET_HANDLER("seerHut", CGSeerHut);
	SET_HANDLER("shipyard", CGShipyard);
	SET_HANDLER("shrine", CGShrine);
	SET_HANDLER("sign", CGSignBottle);
	SET_HANDLER("siren", CGSirens);
	SET_HANDLER("monolith", CGMonolith);
	SET_HANDLER("subterraneanGate", CGSubterraneanGate);
	SET_HANDLER("whirlpool", CGWhirlpool);
	SET_HANDLER("university", CGUniversity);
	SET_HANDLER("oncePerHero", CGVisitableOPH);
	SET_HANDLER("oncePerWeek", CGVisitableOPW);
	SET_HANDLER("witch", CGWitchHut);
	SET_HANDLER("terrain", CGTerrainPatch);

#undef SET_HANDLER_CLASS
#undef SET_HANDLER
}

CObjectClassesHandler::~CObjectClassesHandler()
{
	for(auto p : objects)
		delete p.second;
}

std::vector<JsonNode> CObjectClassesHandler::loadLegacyData(size_t dataSize)
{
	CLegacyConfigParser parser("Data/Objects.txt");
	size_t totalNumber = static_cast<size_t>(parser.readNumber()); // first line contains number of objects to read and nothing else
	parser.endLine();

	for (size_t i = 0; i < totalNumber; i++)
	{
		auto tmpl = new ObjectTemplate;

		tmpl->readTxt(parser);
		parser.endLine();

		std::pair<si32, si32> key(tmpl->id.num, tmpl->subid);
		legacyTemplates.insert(std::make_pair(key, std::shared_ptr<const ObjectTemplate>(tmpl)));
	}

	std::vector<JsonNode> ret(dataSize);// create storage for 256 objects
	assert(dataSize == 256);

	CLegacyConfigParser namesParser("Data/ObjNames.txt");
	for (size_t i=0; i<256; i++)
	{
		ret[i]["name"].String() = namesParser.readString();
		namesParser.endLine();
	}

	CLegacyConfigParser cregen1Parser("data/crgen1");
	do
		customNames[Obj::CREATURE_GENERATOR1].push_back(cregen1Parser.readString());
	while(cregen1Parser.endLine());

	CLegacyConfigParser cregen4Parser("data/crgen4");
	do
		customNames[Obj::CREATURE_GENERATOR4].push_back(cregen4Parser.readString());
	while(cregen4Parser.endLine());

	return ret;
}

/// selects preferred ID (or subID) for new object
template<typename Map>
si32 selectNextID(const JsonNode & fixedID, const Map & map, si32 fixedObjectsBound)
{
	assert(fixedObjectsBound > 0);
	if(fixedID.isNull())
	{
		auto lastID = map.empty() ? 0 : map.rbegin()->first;
		return lastID < fixedObjectsBound ? fixedObjectsBound : lastID + 1;
	}
	auto id = static_cast<si32>(fixedID.Float());
	if(id >= fixedObjectsBound)
		logGlobal->error("Getting next ID overflowed: %d >= %d", id, fixedObjectsBound);

	return id;
}

void CObjectClassesHandler::loadObjectEntry(const std::string & identifier, const JsonNode & entry, ObjectContainter * obj, bool isSubobject)
{
	static const si32 fixedObjectsBound = 1000; // legacy value for backward compatibilitty
	static const si32 fixedSubobjectsBound = 10000000; // large enough arbitrary value to avoid ID-collisions
	si32 usedBound = fixedObjectsBound;

	if(!handlerConstructors.count(obj->handlerName))
	{
		logGlobal->error("Handler with name %s was not found!", obj->handlerName);
		return;
	}
	const auto convertedId = VLC->modh->normalizeIdentifier(entry.meta, CModHandler::scopeBuiltin(), identifier);
	const auto & entryIndex = entry["index"];
	bool useSelectNextID = !isSubobject || entryIndex.isNull();

	if(useSelectNextID && isSubobject)
	{
		usedBound = fixedSubobjectsBound;
		logGlobal->error("Subobject index is Null. convertedId = '%s' obj->id = %d", convertedId, obj->id);
	}
	si32 id = useSelectNextID ? selectNextID(entryIndex, obj->subObjects, usedBound) 
		: (si32)entryIndex.Float();

	auto handler = handlerConstructors.at(obj->handlerName)();
	handler->setType(obj->id, id);
	handler->setTypeName(obj->identifier, convertedId);

	if (customNames.count(obj->id) && customNames.at(obj->id).size() > id)
		handler->init(entry, customNames.at(obj->id).at(id));
	else
		handler->init(entry);

	//if (handler->getTemplates().empty())
	{
		auto range = legacyTemplates.equal_range(std::make_pair(obj->id, id));
		for (auto & templ : boost::make_iterator_range(range.first, range.second))
		{
			handler->addTemplate(templ.second);
		}
		legacyTemplates.erase(range.first, range.second);
	}

	logGlobal->debug("Loaded object %s(%d)::%s(%d)", obj->identifier, obj->id, convertedId, id);

	//some mods redefine content handlers in the decoration.json in such way:
	//"core:sign" : { "types" : { "forgeSign" : { ...
	static const std::vector<std::string> breakersRMG
	{
		"hota.hota decorations:hotaPandoraBox"
		, "hota.hota decorations:hotaSubterreanGate"
	};
	const bool isExistingKey = obj->subObjects.count(id) > 0;
	const bool isBreaker = std::any_of(breakersRMG.begin(), breakersRMG.end(),
		[&handler](const std::string & str)
		{
			return str.compare(handler->subTypeName) == 0;
		});
	const bool passedHandler = !isExistingKey && !isBreaker;

	if(passedHandler)
	{
		obj->subObjects[id] = handler;
		obj->subIds[convertedId] = id;
	}
	else if(isExistingKey) //It's supposed that fan mods handlers are not overridden by default handlers
	{
		logGlobal->trace("Handler '%s' has not been overridden with handler '%s' in object %s(%d)::%s(%d)",
			obj->subObjects[id]->subTypeName, obj->handlerName, obj->identifier, obj->id, convertedId, id);
	}
	else
	{
		logGlobal->warn("Handler '%s' for object %s(%d)::%s(%d) has not been activated as RMG breaker",
			obj->handlerName, obj->identifier, obj->id, convertedId, id);
	}
}

CObjectClassesHandler::ObjectContainter * CObjectClassesHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & name)
{
	auto obj = new ObjectContainter();
	static const si32 fixedObjectsBound = 256; //Legacy value for backward compatibility

	obj->identifier = name;
	obj->name = json["name"].String();
	obj->handlerName = json["handler"].String();
	obj->base = json["base"];
	obj->id = selectNextID(json["index"], objects, fixedObjectsBound);

	if(json["defaultAiValue"].isNull())
		obj->groupDefaultAiValue = boost::none;
	else
		obj->groupDefaultAiValue = static_cast<boost::optional<si32>>(json["defaultAiValue"].Integer());

	for (auto entry : json["types"].Struct())
		loadObjectEntry(entry.first, entry.second, obj);

	return obj;
}

void CObjectClassesHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(scope, data, VLC->modh->normalizeIdentifier(scope, CModHandler::scopeBuiltin(), name));
	objects[object->id] = object;
	VLC->modh->identifiers.registerObject(scope, "object", name, object->id);
}

void CObjectClassesHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(scope, data, VLC->modh->normalizeIdentifier(scope, CModHandler::scopeBuiltin(), name));
	assert(objects[(si32)index] == nullptr); // ensure that this id was not loaded before
	objects[(si32)index] = object;
	VLC->modh->identifiers.registerObject(scope, "object", name, object->id);
}

void CObjectClassesHandler::loadSubObject(const std::string & identifier, JsonNode config, si32 ID, boost::optional<si32> subID)
{
	static const bool isSubObject = true;

	config.setType(JsonNode::JsonType::DATA_STRUCT); // ensure that input is not NULL
	assert(objects.count(ID));
	if (subID)
	{
		assert(objects.at(ID)->subObjects.count(subID.get()) == 0);
		assert(config["index"].isNull());
		config["index"].Float() = subID.get();
		config["index"].setMeta(config.meta);
	}
	JsonUtils::inherit(config, objects.at(ID)->base);
	loadObjectEntry(identifier, config, objects[ID], isSubObject);
}

void CObjectClassesHandler::removeSubObject(si32 ID, si32 subID)
{
	assert(objects.count(ID));
	assert(objects.at(ID)->subObjects.count(subID));
	objects.at(ID)->subObjects.erase(subID); //TODO: cleanup string id map
}

std::vector<bool> CObjectClassesHandler::getDefaultAllowed() const
{
	return std::vector<bool>(); //TODO?
}

TObjectTypeHandler CObjectClassesHandler::getHandlerFor(si32 type, si32 subtype) const
{
	if (objects.count(type))
	{
		if (objects.at(type)->subObjects.count(subtype))
			return objects.at(type)->subObjects.at(subtype);
	}
	std::string errorString = "Failed to find object of type " + std::to_string(type) + "::" + std::to_string(subtype);
	logGlobal->error(errorString);
	throw std::runtime_error(errorString);
}

TObjectTypeHandler CObjectClassesHandler::getHandlerFor(std::string scope, std::string type, std::string subtype) const
{
	boost::optional<si32> id = VLC->modh->identifiers.getIdentifier(scope, "object", type, false);
	if(id)
	{
		auto object = objects.at(id.get());
		if(object->subIds.count(subtype))
		{
			si32 subId = object->subIds.at(subtype);

			return object->subObjects.at(subId);
		}
	}
	std::string errorString = "Failed to find object of type " + type + "::" + subtype;
	logGlobal->error(errorString);
	throw std::runtime_error(errorString);
}

TObjectTypeHandler CObjectClassesHandler::getHandlerFor(CompoundMapObjectID compoundIdentifier) const
{
	return getHandlerFor(compoundIdentifier.primaryID, compoundIdentifier.secondaryID);
}

std::set<si32> CObjectClassesHandler::knownObjects() const
{
	std::set<si32> ret;

	for (auto entry : objects)
		ret.insert(entry.first);

	return ret;
}

std::set<si32> CObjectClassesHandler::knownSubObjects(si32 primaryID) const
{
	std::set<si32> ret;

	if (objects.count(primaryID))
	{
		for (auto entry : objects.at(primaryID)->subObjects)
			ret.insert(entry.first);
	}
	return ret;
}

void CObjectClassesHandler::beforeValidate(JsonNode & object)
{
	for (auto & entry : object["types"].Struct())
	{
		JsonUtils::inherit(entry.second, object["base"]);
		for (auto & templ : entry.second["templates"].Struct())
			JsonUtils::inherit(templ.second, entry.second["base"]);
	}
}

void CObjectClassesHandler::afterLoadFinalization()
{
	for(auto entry : objects)
	{
		for(auto obj : entry.second->subObjects)
		{
			obj.second->afterLoadFinalization();
			if(obj.second->getTemplates().empty())
				logGlobal->warn("No templates found for %d:%d", entry.first, obj.first);
		}
	}

	//duplicate existing two-way portals to make reserve for RMG
	auto& portalVec = objects[Obj::MONOLITH_TWO_WAY]->subObjects;
	size_t portalCount = portalVec.size();
	size_t currentIndex = portalCount;
	while(portalVec.size() < 100)
	{
		portalVec[(si32)currentIndex] = portalVec[static_cast<si32>(currentIndex % portalCount)];
		currentIndex++;
	}
}

std::string CObjectClassesHandler::getObjectName(si32 type) const
{
	if (objects.count(type))
		return objects.at(type)->name;
	logGlobal->error("Access to non existing object of type %d", type);
	return "";
}

std::string CObjectClassesHandler::getObjectName(si32 type, si32 subtype) const
{
	if (knownSubObjects(type).count(subtype))
	{
		auto name = getHandlerFor(type, subtype)->getCustomName();
		if (name)
			return name.get();
	}
	return getObjectName(type);
}

SObjectSounds CObjectClassesHandler::getObjectSounds(si32 type) const
{
	if(objects.count(type))
		return objects.at(type)->sounds;
	logGlobal->error("Access to non existing object of type %d", type);
	return SObjectSounds();
}

SObjectSounds CObjectClassesHandler::getObjectSounds(si32 type, si32 subtype) const
{
	if(knownSubObjects(type).count(subtype))
		return getHandlerFor(type, subtype)->getSounds();
	else
		return getObjectSounds(type);
}

std::string CObjectClassesHandler::getObjectHandlerName(si32 type) const
{
	return objects.at(type)->handlerName;
}

boost::optional<si32> CObjectClassesHandler::getObjGroupAiValue(si32 primaryID) const
{
	return objects.at(primaryID)->groupDefaultAiValue;
}

AObjectTypeHandler::AObjectTypeHandler():
	type(-1), subtype(-1)
{

}

AObjectTypeHandler::~AObjectTypeHandler()
{
}

void AObjectTypeHandler::setType(si32 type, si32 subtype)
{
	this->type = type;
	this->subtype = subtype;
}

void AObjectTypeHandler::setTypeName(std::string type, std::string subtype)
{
	this->typeName = type;
	this->subTypeName = subtype;
}

static ui32 loadJsonOrMax(const JsonNode & input)
{
	if (input.isNull())
		return std::numeric_limits<ui32>::max();
	else
		return static_cast<ui32>(input.Float());
}

void AObjectTypeHandler::init(const JsonNode & input, boost::optional<std::string> name)
{
	base = input["base"];

	if (!input["rmg"].isNull())
	{
		rmgInfo.value =     static_cast<ui32>(input["rmg"]["value"].Float());
		rmgInfo.mapLimit =  loadJsonOrMax(input["rmg"]["mapLimit"]);
		rmgInfo.zoneLimit = loadJsonOrMax(input["rmg"]["zoneLimit"]);
		rmgInfo.rarity =    static_cast<ui32>(input["rmg"]["rarity"].Float());
	} // else block is not needed - set in constructor

	for (auto entry : input["templates"].Struct())
	{
		entry.second.setType(JsonNode::JsonType::DATA_STRUCT);
		JsonUtils::inherit(entry.second, base);

		auto tmpl = new ObjectTemplate;
		tmpl->id = Obj(type);
		tmpl->subid = subtype;
		tmpl->stringID = entry.first; // FIXME: create "fullID" - type.object.template?
		try
		{
			tmpl->readJson(entry.second);
			templates.push_back(std::shared_ptr<const ObjectTemplate>(tmpl));
		}
		catch (const std::exception & e)
		{
			logGlobal->warn("Failed to load terrains for object %s: %s", entry.first, e.what());
		}
	}

	if (input["name"].isNull())
		objectName = name;
	else
		objectName.reset(input["name"].String());

	for(const JsonNode & node : input["sounds"]["ambient"].Vector())
		sounds.ambient.push_back(node.String());

	for(const JsonNode & node : input["sounds"]["visit"].Vector())
		sounds.visit.push_back(node.String());

	for(const JsonNode & node : input["sounds"]["removal"].Vector())
		sounds.removal.push_back(node.String());

	if(input["aiValue"].isNull())
		aiValue = boost::none;
	else
		aiValue = static_cast<boost::optional<si32>>(input["aiValue"].Integer());

	if(input["battleground"].getType() == JsonNode::JsonType::DATA_STRING)
		battlefield = input["battleground"].String();
	else
		battlefield = boost::none;

	initTypeData(input);
}

bool AObjectTypeHandler::objectFilter(const CGObjectInstance *, std::shared_ptr<const ObjectTemplate>) const
{
	return false; // by default there are no overrides
}

void AObjectTypeHandler::preInitObject(CGObjectInstance * obj) const
{
	obj->ID = Obj(type);
	obj->subID = subtype;
	obj->typeName = typeName;
	obj->subTypeName = subTypeName;
}

void AObjectTypeHandler::initTypeData(const JsonNode & input)
{
	// empty implementation for overrides
}

boost::optional<std::string> AObjectTypeHandler::getCustomName() const
{
	return objectName;
}

SObjectSounds AObjectTypeHandler::getSounds() const
{
	return sounds;
}

void AObjectTypeHandler::addTemplate(std::shared_ptr<const ObjectTemplate> templ)
{
	templates.push_back(templ);
}

void AObjectTypeHandler::addTemplate(JsonNode config)
{
	config.setType(JsonNode::JsonType::DATA_STRUCT); // ensure that input is not null
	JsonUtils::inherit(config, base);
	auto tmpl = new ObjectTemplate;
	tmpl->id = Obj(type);
	tmpl->subid = subtype;
	tmpl->stringID.clear(); // TODO?
	tmpl->readJson(config);
	templates.emplace_back(tmpl);
}

std::vector<std::shared_ptr<const ObjectTemplate>> AObjectTypeHandler::getTemplates() const
{
	return templates;
}

BattleField AObjectTypeHandler::getBattlefield() const
{
	return battlefield ? BattleField::fromString(battlefield.get()) : BattleField::NONE;
}

std::vector<std::shared_ptr<const ObjectTemplate>>AObjectTypeHandler::getTemplates(TerrainId terrainType) const
{
	std::vector<std::shared_ptr<const ObjectTemplate>> templates = getTemplates();
	std::vector<std::shared_ptr<const ObjectTemplate>> filtered;

	std::copy_if(templates.begin(), templates.end(), std::back_inserter(filtered), [&](std::shared_ptr<const ObjectTemplate> obj)
	{
		return obj->canBePlacedAt(terrainType);
	});
	// H3 defines allowed terrains in a weird way - artifacts, monsters and resources have faulty masks here
	// Perhaps we should re-define faulty templates and remove this workaround (already done for resources)
	if (type == Obj::ARTIFACT || type == Obj::MONSTER)
		return templates;
	else
		return filtered;
}

std::shared_ptr<const ObjectTemplate> AObjectTypeHandler::getOverride(TerrainId terrainType, const CGObjectInstance * object) const
{
	std::vector<std::shared_ptr<const ObjectTemplate>> ret = getTemplates(terrainType);
	for (const auto & tmpl: ret)
	{
		if (objectFilter(object, tmpl))
			return tmpl;
	}
	return std::shared_ptr<const ObjectTemplate>(); //empty
}

const RandomMapInfo & AObjectTypeHandler::getRMGInfo()
{
	return rmgInfo;
}

boost::optional<si32> AObjectTypeHandler::getAiValue() const
{
	return aiValue;
}

bool AObjectTypeHandler::isStaticObject()
{
	return false; // most of classes are not static
}

void AObjectTypeHandler::afterLoadFinalization()
{
}

VCMI_LIB_NAMESPACE_END

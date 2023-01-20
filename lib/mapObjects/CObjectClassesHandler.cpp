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
	SET_HANDLER("market", CGMarket);
	SET_HANDLER("mine", CGMine);
	SET_HANDLER("obelisk", CGObelisk);
	SET_HANDLER("observatory", CGObservatory);
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
	SET_HANDLER("witch", CGWitchHut);
	SET_HANDLER("terrain", CGTerrainPatch);

#undef SET_HANDLER_CLASS
#undef SET_HANDLER
}

CObjectClassesHandler::~CObjectClassesHandler()
{
	for(auto p : objects)
		delete p;
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

	objects.resize(256);

	std::vector<JsonNode> ret(dataSize);// create storage for 256 objects
	assert(dataSize == 256);

	CLegacyConfigParser namesParser("Data/ObjNames.txt");
	for (size_t i=0; i<256; i++)
	{
		ret[i]["name"].String() = namesParser.readString();
		namesParser.endLine();
	}

	JsonNode cregen1;
	JsonNode cregen4;

	CLegacyConfigParser cregen1Parser("data/crgen1");
	do
	{
		JsonNode subObject;
		subObject["name"].String() = cregen1Parser.readString();
		cregen1.Vector().push_back(subObject);
	}
	while(cregen1Parser.endLine());

	CLegacyConfigParser cregen4Parser("data/crgen4");
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

void CObjectClassesHandler::loadSubObject(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj)
{
	auto object = loadSubObjectFromJson(scope, identifier, entry, obj, obj->objects.size());

	assert(object);
	obj->objects.push_back(object);

	registerObject(scope, "mapObject", obj->getJsonKey() + "." + object->getSubTypeName(), object->subtype);
}

void CObjectClassesHandler::loadSubObject(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj, size_t index)
{
	auto object = loadSubObjectFromJson(scope, identifier, entry, obj, index);

	assert(object);
	assert(obj->objects[index] == nullptr); // ensure that this id was not loaded before
	obj->objects[index] = object;

	registerObject(scope, "mapObject", obj->getJsonKey() + "." + object->getSubTypeName(), object->subtype);
}

TObjectTypeHandler CObjectClassesHandler::loadSubObjectFromJson(const std::string & scope, const std::string & identifier, const JsonNode & entry, ObjectClass * obj, size_t index)
{
	assert(identifier.find(':') == std::string::npos);
	assert(!scope.empty());

	if(!handlerConstructors.count(obj->handlerName))
	{
		logGlobal->error("Handler with name %s was not found!", obj->handlerName);
		return nullptr;
	}

	auto createdObject = handlerConstructors.at(obj->handlerName)();

	createdObject->modScope = scope;
	createdObject->typeName = obj->identifier;;
	createdObject->subTypeName = identifier;

	createdObject->type = obj->id;
	createdObject->subtype = index;
	createdObject->init(entry);

	auto range = legacyTemplates.equal_range(std::make_pair(obj->id, index));
	for (auto & templ : boost::make_iterator_range(range.first, range.second))
	{
		createdObject->addTemplate(templ.second);
	}
	legacyTemplates.erase(range.first, range.second);

	logGlobal->debug("Loaded object %s(%d)::%s(%d)", obj->getJsonKey(), obj->id, identifier, index);

	return createdObject;
}

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
	return VLC->generaltexth->translate(getNameTextID());
}

ObjectClass * CObjectClassesHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & name, size_t index)
{
	auto obj = new ObjectClass();

	obj->modScope = scope;
	obj->identifier = name;
	obj->handlerName = json["handler"].String();
	obj->base = json["base"];
	obj->id = index;

	VLC->generaltexth->registerString(obj->getNameTextID(), json["name"].String());

	obj->objects.resize(json["lastReservedIndex"].Float() + 1);

	for (auto subData : json["types"].Struct())
	{
		if (!subData.second["index"].isNull())
		{
			std::string const & subMeta = subData.second["index"].meta;

			if ( subMeta != "core")
				logMod->warn("Object %s:%s.%s - attempt to load object with preset index! This option is reserved for built-in mod", subMeta, name, subData.first );
			size_t subIndex = subData.second["index"].Integer();
			loadSubObject(subData.second.meta, subData.first, subData.second, obj, subIndex);
		}
		else
			loadSubObject(subData.second.meta, subData.first, subData.second, obj);
	}
	return obj;
}

void CObjectClassesHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(scope, data, name, objects.size());
	objects.push_back(object);
	VLC->modh->identifiers.registerObject(scope, "object", name, object->id);
}

void CObjectClassesHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(scope, data, name, index);
	assert(objects[(si32)index] == nullptr); // ensure that this id was not loaded before
	objects[(si32)index] = object;
	VLC->modh->identifiers.registerObject(scope, "object", name, object->id);
}

void CObjectClassesHandler::loadSubObject(const std::string & identifier, JsonNode config, si32 ID, si32 subID)
{
	config.setType(JsonNode::JsonType::DATA_STRUCT); // ensure that input is not NULL
	assert(ID < objects.size());
	assert(objects[ID]);

	if ( subID >= objects[ID]->objects.size())
		objects[ID]->objects.resize(subID+1);

	JsonUtils::inherit(config, objects.at(ID)->base);
	loadSubObject(config.meta, identifier, config, objects[ID], subID);
}

void CObjectClassesHandler::removeSubObject(si32 ID, si32 subID)
{
	assert(ID < objects.size());
	assert(objects[ID]);
	assert(subID < objects[ID]->objects.size());
	objects[ID]->objects[subID] = nullptr;
}

std::vector<bool> CObjectClassesHandler::getDefaultAllowed() const
{
	return std::vector<bool>(); //TODO?
}

TObjectTypeHandler CObjectClassesHandler::getHandlerFor(si32 type, si32 subtype) const
{
	assert(type < objects.size());
	assert(objects[type]);
	assert(subtype < objects[type]->objects.size());

	return objects.at(type)->objects.at(subtype);
}

TObjectTypeHandler CObjectClassesHandler::getHandlerFor(std::string scope, std::string type, std::string subtype) const
{
	boost::optional<si32> id = VLC->modh->identifiers.getIdentifier(scope, "object", type, false);
	if(id)
	{
		auto object = objects[id.get()];
		boost::optional<si32> subID = VLC->modh->identifiers.getIdentifier(scope, object->getJsonKey(), subtype, false);

		if (subID)
			return object->objects[subID.get()];
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
		if (entry)
			ret.insert(entry->id);

	return ret;
}

std::set<si32> CObjectClassesHandler::knownSubObjects(si32 primaryID) const
{
	assert(primaryID < objects.size());
	assert(objects[primaryID]);

	std::set<si32> ret;

	for (auto entry : objects.at(primaryID)->objects)
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
			auto const & vector = object["subObjects"].Vector();

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
	for(auto entry : objects)
	{
		if (!entry)
			continue;

		for(auto obj : entry->objects)
		{
			if (!obj)
				continue;

			obj->afterLoadFinalization();
			if(obj->getTemplates().empty())
				logGlobal->warn("No templates found for %s:%s", entry->getJsonKey(), obj->getJsonKey());
		}
	}

	//duplicate existing two-way portals to make reserve for RMG
	auto& portalVec = objects[Obj::MONOLITH_TWO_WAY]->objects;
	size_t portalCount = portalVec.size();

	for (size_t i = portalCount; i < 100; ++i)
		portalVec.push_back(portalVec[static_cast<si32>(i % portalCount)]);
}

std::string CObjectClassesHandler::getObjectName(si32 type, si32 subtype) const
{
	auto const handler = getHandlerFor(type, subtype);
	if (handler->hasNameTextID())
		return handler->getNameTranslated();
	else
		return objects[type]->getNameTranslated();
}

SObjectSounds CObjectClassesHandler::getObjectSounds(si32 type, si32 subtype) const
{
	// TODO: these objects may have subID's that does not have associated handler:
	// Prison: uses hero type as subID
	// Hero: uses hero type as subID, but registers hero classes as subtypes
	// Spell scroll: uses spell ID as subID
	if(type == Obj::PRISON || type == Obj::HERO || type == Obj::SPELL_SCROLL)
		subtype = 0;

	assert(type < objects.size());
	assert(objects[type]);
	assert(subtype < objects[type]->objects.size());

	return getHandlerFor(type, subtype)->getSounds();
}

std::string CObjectClassesHandler::getObjectHandlerName(si32 type) const
{
	return objects.at(type)->handlerName;
}

AObjectTypeHandler::AObjectTypeHandler():
	type(-1), subtype(-1)
{

}

AObjectTypeHandler::~AObjectTypeHandler()
{
}

std::string AObjectTypeHandler::getJsonKey() const
{
	return modScope + ':' + subTypeName;
}

si32 AObjectTypeHandler::getIndex() const
{
	return type;
}

si32 AObjectTypeHandler::getSubIndex() const
{
	return subtype;
}

std::string AObjectTypeHandler::getTypeName() const
{
	return typeName;
}

std::string AObjectTypeHandler::getSubTypeName() const
{
	return subTypeName;
}

static ui32 loadJsonOrMax(const JsonNode & input)
{
	if (input.isNull())
		return std::numeric_limits<ui32>::max();
	else
		return static_cast<ui32>(input.Float());
}

void AObjectTypeHandler::init(const JsonNode & input)
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

bool AObjectTypeHandler::hasNameTextID() const
{
	return false;
}

std::string AObjectTypeHandler::getNameTextID() const
{
	return TextIdentifier("mapObject", modScope, typeName, subTypeName, "name").get();
}

std::string AObjectTypeHandler::getNameTranslated() const
{
	return VLC->generaltexth->translate(getNameTextID());
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

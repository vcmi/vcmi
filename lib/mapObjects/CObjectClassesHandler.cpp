#include "StdInc.h"
#include "CObjectClassesHandler.h"

#include "filesystem/Filesystem.h"
#include "filesystem/CBinaryReader.h"
#include "../lib/VCMI_Lib.h"
#include "GameConstants.h"
#include "StringConstants.h"
#include "CGeneralTextHandler.h"
#include "CModHandler.h"
#include "JsonNode.h"

#include "CRewardableConstructor.h"
#include "MapObjects.h"

/*
 * CObjectClassesHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CObjectClassesHandler::CObjectClassesHandler()
{
#define SET_HANDLER_CLASS(STRING, CLASSNAME) handlerConstructors[STRING] = std::make_shared<CLASSNAME>;
#define SET_HANDLER(STRING, TYPENAME) handlerConstructors[STRING] = std::make_shared<CDefaultObjectTypeHandler<TYPENAME> >

	// list of all known handlers, hardcoded for now since the only way to add new objects is via C++ code
	//WARNING: should be in sync with registerTypesMapObjectTypes function
	SET_HANDLER_CLASS("configurable", CRewardableConstructor);

	SET_HANDLER("", CGObjectInstance);
	SET_HANDLER("generic", CGObjectInstance);

	SET_HANDLER("market", CGMarket);
	SET_HANDLER("bank", CBank);
	SET_HANDLER("cartographer", CCartographer);
	SET_HANDLER("artifact", CGArtifact);
	SET_HANDLER("blackMarket", CGBlackMarket);
	SET_HANDLER("boat", CGBoat);
	SET_HANDLER("bonusingObject", CGBonusingObject);
	SET_HANDLER("borderGate", CGBorderGate);
	SET_HANDLER("borderGuard", CGBorderGuard);
	SET_HANDLER("monster", CGCreature);
	SET_HANDLER("denOfThieves", CGDenOfthieves);
	SET_HANDLER("dwelling", CGDwelling);
	SET_HANDLER("event", CGEvent);
	SET_HANDLER("garrison", CGGarrison);
	SET_HANDLER("hero", CGHeroInstance);
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
	SET_HANDLER("pickable", CGPickable);
	SET_HANDLER("pyramid", CGPyramid);
	SET_HANDLER("questGuard", CGQuestGuard);
	SET_HANDLER("resource", CGResource);
	SET_HANDLER("scholar", CGScholar);
	SET_HANDLER("seerHut", CGSeerHut);
	SET_HANDLER("shipyard", CGShipyard);
	SET_HANDLER("shrine", CGShrine);
	SET_HANDLER("sign", CGSignBottle);
	SET_HANDLER("siren", CGSirens);
	SET_HANDLER("teleport", CGTeleport);
	SET_HANDLER("town", CGTownInstance);
	SET_HANDLER("university", CGUniversity);
	SET_HANDLER("oncePerHero", CGVisitableOPH);
	SET_HANDLER("oncePerWeek", CGVisitableOPW);
	SET_HANDLER("witch", CGWitchHut);

#undef SET_HANDLER_CLASS
#undef SET_HANDLER
}

template<typename Container>
void readTextFile(Container & objects, std::string path)
{
	CLegacyConfigParser parser(path);
	size_t totalNumber = parser.readNumber(); // first line contains number of objects to read and nothing else
	parser.endLine();

	for (size_t i=0; i<totalNumber; i++)
	{
		ObjectTemplate templ;
		templ.readTxt(parser);
		parser.endLine();
		typename Container::key_type key(templ.id.num, templ.subid);
		objects.insert(std::make_pair(key, templ));
	}
}

std::vector<JsonNode> CObjectClassesHandler::loadLegacyData(size_t dataSize)
{
	readTextFile(legacyTemplates, "Data/Objects.txt");
	readTextFile(legacyTemplates, "Data/Heroes.txt");

	std::vector<JsonNode> ret(dataSize);// create storage for 256 objects
	assert(dataSize == 256);

	CLegacyConfigParser parser("Data/ObjNames.txt");
	for (size_t i=0; i<256; i++)
	{
		ret[i]["name"].String() = parser.readString();
		parser.endLine();
	}
	return ret;
}

/// selects preferred ID (or subID) for new object
template<typename Map>
si32 selectNextID(const JsonNode & fixedID, const Map & map, si32 defaultID)
{
	if (!fixedID.isNull() && fixedID.Float() < defaultID)
		return fixedID.Float(); // H3M object with fixed ID

	if (map.empty())
		return defaultID; // no objects loaded, keep gap for H3M objects
	if (map.rbegin()->first > defaultID)
		return map.rbegin()->first + 1; // some modded objects loaded, return next available

	return defaultID; // some H3M objects loaded, first modded found
}

void CObjectClassesHandler::loadObjectEntry(const JsonNode & entry, ObjectContainter * obj)
{
	auto handler = handlerConstructors.at(obj->handlerName)();
	handler->init(entry);

	si32 id = selectNextID(entry["index"], obj->objects, 1000);
	handler->setType(obj->id, id);

	if (handler->getTemplates().empty())
	{
		auto range = legacyTemplates.equal_range(std::make_pair(obj->id, id));
		for (auto & templ : boost::make_iterator_range(range.first, range.second))
		{
			handler->addTemplate(templ.second);
		}
		legacyTemplates.erase(range.first, range.second);
	}
	
	obj->objects[id] = handler;
}

CObjectClassesHandler::ObjectContainter * CObjectClassesHandler::loadFromJson(const JsonNode & json)
{
	auto obj = new ObjectContainter();
	obj->name = json["name"].String();
	obj->handlerName = json["handler"].String();
	obj->base = json["base"]; // FIXME: when this data will be actually merged?
	obj->id = selectNextID(json["index"], objects, 256);
	for (auto entry : json["types"].Struct())
	{
		loadObjectEntry(entry.second, obj);
	}
	return obj;
}

void CObjectClassesHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(data);
	objects[object->id] = object;

	VLC->modh->identifiers.registerObject(scope, "object", name, object->id);
}

void CObjectClassesHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(data);

	assert(objects[index] == nullptr); // ensure that this id was not loaded before
	objects[index] = object;

	VLC->modh->identifiers.registerObject(scope, "object", name, object->id);
}

void CObjectClassesHandler::createObject(std::string name, JsonNode config, si32 ID, boost::optional<si32> subID)
{
	config.setType(JsonNode::DATA_STRUCT); // ensure that input is not NULL
	assert(objects.count(ID));
	if (subID)
	{
		assert(objects.at(ID)->objects.count(subID.get()) == 0);
		assert(config["index"].isNull());
		config["index"].Float() = subID.get();
	}

	JsonUtils::inherit(config, objects.at(ID)->base);

	loadObjectEntry(config, objects[ID]);
}

void CObjectClassesHandler::eraseObject(si32 ID, si32 subID)
{
	assert(objects.count(ID));
	assert(objects.at(ID)->objects.count(subID));
	objects.at(ID)->objects.erase(subID);
}

std::vector<bool> CObjectClassesHandler::getDefaultAllowed() const
{
	return std::vector<bool>(); //TODO?
}

TObjectTypeHandler CObjectClassesHandler::getHandlerFor(si32 type, si32 subtype) const
{
	if (objects.count(type))
	{
		if (objects.at(type)->objects.count(subtype))
			return objects.at(type)->objects.at(subtype);
	}
	logGlobal->errorStream() << "Failed to find object of type " << type << ":" << subtype;
	assert(0); // FIXME: throw error?
	return nullptr;
}

void CObjectClassesHandler::beforeValidate(JsonNode & object)
{
	for (auto & entry : object["types"].Struct())
	{
		JsonUtils::inherit(entry.second, object["base"]);
		for (auto & templ : entry.second["templates"].Struct())
		{
			JsonUtils::inherit(templ.second, entry.second["base"]);
		}
	}
}

void CObjectClassesHandler::afterLoadFinalization()
{
	legacyTemplates.clear(); // whatever left there is no longer needed
	for (auto entry : objects)
	{
		for (auto obj : entry.second->objects)
		{
			if (obj.second->getTemplates().empty())
				logGlobal->warnStream() << "No templates found for " << entry.first << ":" << obj.first;
		}
	}
}

std::string CObjectClassesHandler::getObjectName(si32 type) const
{
	assert(objects.count(type));
	return objects.at(type)->name;
}

void AObjectTypeHandler::setType(si32 type, si32 subtype)
{
	this->type = type;
	this->subtype = subtype;
}

void AObjectTypeHandler::init(const JsonNode & input)
{
	base = input["base"];
	for (auto entry : input["templates"].Struct())
	{
		entry.second.setType(JsonNode::DATA_STRUCT);
		JsonUtils::inherit(entry.second, base);

		ObjectTemplate tmpl;
		tmpl.id = Obj(type);
		tmpl.subid = subtype;
		tmpl.stringID = entry.first; // FIXME: create "fullID" - type.object.template?
		tmpl.readJson(entry.second);
		templates.push_back(tmpl);
	}
}

bool AObjectTypeHandler::objectFilter(const CGObjectInstance *, const ObjectTemplate &) const
{
	return true; // by default - accept all.
}

void AObjectTypeHandler::addTemplate(ObjectTemplate templ)
{
	templ.id = Obj(type);
	templ.subid = subtype;
	templates.push_back(templ);
}

void AObjectTypeHandler::addTemplate(JsonNode config)
{
	config.setType(JsonNode::DATA_STRUCT); // ensure that input is not null
	JsonUtils::inherit(config, base);
	ObjectTemplate tmpl;
	tmpl.id = Obj(type);
	tmpl.subid = subtype;
	tmpl.stringID = ""; // TODO?
	tmpl.readJson(config);
	addTemplate(tmpl);
}

std::vector<ObjectTemplate> AObjectTypeHandler::getTemplates() const
{
	return templates;
}

std::vector<ObjectTemplate> AObjectTypeHandler::getTemplates(si32 terrainType) const// FIXME: replace with ETerrainType
{
	std::vector<ObjectTemplate> ret = getTemplates();
	std::vector<ObjectTemplate> filtered;

	std::copy_if(ret.begin(), ret.end(), std::back_inserter(filtered), [&](const ObjectTemplate & obj)
	{
		return obj.canBePlacedAt(ETerrainType(terrainType));
	});
	// it is possible that there are no templates usable on specific terrain. In this case - return list before filtering
	return filtered.empty() ? ret : filtered;
}

boost::optional<ObjectTemplate> AObjectTypeHandler::getOverride(si32 terrainType, const CGObjectInstance * object) const
{
	std::vector<ObjectTemplate> ret = getTemplates(terrainType);
	for (auto & tmpl : ret)
	{
		if (objectFilter(object, tmpl))
			return tmpl;
	}
	return boost::optional<ObjectTemplate>();
}

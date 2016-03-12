/*
 * JsonNode.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonNode.h"

#include "ScopeGuard.h"

#include "HeroBonus.h"
#include "filesystem/Filesystem.h"
#include "VCMI_Lib.h" //for identifier resolution
#include "CModHandler.h"
#include "CGeneralTextHandler.h"
#include "JsonDetail.h"

using namespace JsonDetail;

class LibClasses;
class CModHandler;

static const JsonNode nullNode;


std::ostream & operator<<(std::ostream &out, const JsonNode &node)
{
	JsonWriter writer(out, node);
	return out << "\n";
}

JsonNode::JsonNode(JsonType Type):
	type(DATA_NULL)
{
	setType(Type);
}

JsonNode::JsonNode(const char *data, size_t datasize):
	type(DATA_NULL)
{
	JsonParser parser(data, datasize);
	*this = parser.parse("<unknown>");
}

JsonNode::JsonNode(ResourceID && fileURI):
	type(DATA_NULL)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
}

JsonNode::JsonNode(const ResourceID & fileURI):
	type(DATA_NULL)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
}

JsonNode::JsonNode(ResourceID && fileURI, bool &isValidSyntax):
	type(DATA_NULL)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
	isValidSyntax = parser.isValid();
}

JsonNode::JsonNode(const JsonNode &copy):
	type(DATA_NULL),
	meta(copy.meta)
{
	setType(copy.getType());
	switch(type)
	{
		break; case DATA_NULL:
		break; case DATA_BOOL:   Bool() =   copy.Bool();
		break; case DATA_FLOAT:  Float() =  copy.Float();
		break; case DATA_STRING: String() = copy.String();
		break; case DATA_VECTOR: Vector() = copy.Vector();
		break; case DATA_STRUCT: Struct() = copy.Struct();
	}
}

JsonNode::~JsonNode()
{
	setType(DATA_NULL);
}

void JsonNode::swap(JsonNode &b)
{
	using std::swap;
	swap(meta, b.meta);
	swap(data, b.data);
	swap(type, b.type);
}

JsonNode & JsonNode::operator =(JsonNode node)
{
	swap(node);
	return *this;
}

bool JsonNode::operator == (const JsonNode &other) const
{
	if (getType() == other.getType())
	{
		switch(type)
		{
			case DATA_NULL:   return true;
			case DATA_BOOL:   return Bool() == other.Bool();
			case DATA_FLOAT:  return Float() == other.Float();
			case DATA_STRING: return String() == other.String();
			case DATA_VECTOR: return Vector() == other.Vector();
			case DATA_STRUCT: return Struct() == other.Struct();
		}
	}
	return false;
}

bool JsonNode::operator != (const JsonNode &other) const
{
	return !(*this == other);
}

JsonNode::JsonType JsonNode::getType() const
{
	return type;
}

void JsonNode::setMeta(std::string metadata, bool recursive)
{
	meta = metadata;
	if (recursive)
	{
		switch (type)
		{
			break; case DATA_VECTOR:
			{
				for(auto & node : Vector())
				{
					node.setMeta(metadata);
				}
			}
			break; case DATA_STRUCT:
			{
				for(auto & node : Struct())
				{
					node.second.setMeta(metadata);
				}
			}
		}
	}
}

void JsonNode::setType(JsonType Type)
{
	if (type == Type)
		return;

	//Reset node to nullptr
	if (Type != DATA_NULL)
		setType(DATA_NULL);

	switch (type)
	{
		break; case DATA_STRING:  delete data.String;
		break; case DATA_VECTOR:  delete data.Vector;
		break; case DATA_STRUCT:  delete data.Struct;
		break; default:
		break;
	}
	//Set new node type
	type = Type;
	switch(type)
	{
		break; case DATA_NULL:
		break; case DATA_BOOL:   data.Bool = false;
		break; case DATA_FLOAT:  data.Float = 0;
		break; case DATA_STRING: data.String = new std::string();
		break; case DATA_VECTOR: data.Vector = new JsonVector();
		break; case DATA_STRUCT: data.Struct = new JsonMap();
	}
}

bool JsonNode::isNull() const
{
	return type == DATA_NULL;
}

void JsonNode::clear()
{
	setType(DATA_NULL);
}

bool & JsonNode::Bool()
{
	setType(DATA_BOOL);
	return data.Bool;
}

double & JsonNode::Float()
{
	setType(DATA_FLOAT);
	return data.Float;
}

std::string & JsonNode::String()
{
	setType(DATA_STRING);
	return *data.String;
}

JsonVector & JsonNode::Vector()
{
	setType(DATA_VECTOR);
	return *data.Vector;
}

JsonMap & JsonNode::Struct()
{
	setType(DATA_STRUCT);
	return *data.Struct;
}

const bool boolDefault = false;
const bool & JsonNode::Bool() const
{
	if (type == DATA_NULL)
		return boolDefault;
	assert(type == DATA_BOOL);
	return data.Bool;
}

const double floatDefault = 0;
const double & JsonNode::Float() const
{
	if (type == DATA_NULL)
		return floatDefault;
	assert(type == DATA_FLOAT);
	return data.Float;
}

const std::string stringDefault = std::string();
const std::string & JsonNode::String() const
{
	if (type == DATA_NULL)
		return stringDefault;
	assert(type == DATA_STRING);
	return *data.String;
}

const JsonVector vectorDefault = JsonVector();
const JsonVector & JsonNode::Vector() const
{
	if (type == DATA_NULL)
		return vectorDefault;
	assert(type == DATA_VECTOR);
	return *data.Vector;
}

const JsonMap mapDefault = JsonMap();
const JsonMap & JsonNode::Struct() const
{
	if (type == DATA_NULL)
		return mapDefault;
	assert(type == DATA_STRUCT);
	return *data.Struct;
}

JsonNode & JsonNode::operator[](std::string child)
{
	return Struct()[child];
}

const JsonNode & JsonNode::operator[](std::string child) const
{
	auto it = Struct().find(child);
	if (it != Struct().end())
		return it->second;
	return nullNode;
}

// to avoid duplicating const and non-const code
template<typename Node>
Node & resolvePointer(Node & in, const std::string & pointer)
{
	if (pointer.empty())
		return in;
	assert(pointer[0] == '/');

	size_t splitPos = pointer.find('/', 1);

	std::string entry   =   pointer.substr(1, splitPos -1);
	std::string remainer =  splitPos == std::string::npos ? "" : pointer.substr(splitPos);

	if (in.getType() == JsonNode::DATA_VECTOR)
	{
		if (entry.find_first_not_of("0123456789") != std::string::npos) // non-numbers in string
			throw std::runtime_error("Invalid Json pointer");

		if (entry.size() > 1 && entry[0] == '0') // leading zeros are not allowed
			throw std::runtime_error("Invalid Json pointer");

		size_t index = boost::lexical_cast<size_t>(entry);

		if (in.Vector().size() > index)
			return in.Vector()[index].resolvePointer(remainer);
	}
	return in[entry].resolvePointer(remainer);
}

const JsonNode & JsonNode::resolvePointer(const std::string &jsonPointer) const
{
	return ::resolvePointer(*this, jsonPointer);
}

JsonNode & JsonNode::resolvePointer(const std::string &jsonPointer)
{
	return ::resolvePointer(*this, jsonPointer);
}

///JsonUtils

void JsonUtils::parseTypedBonusShort(const JsonVector& source, Bonus *dest)
{
	dest->val = source[1].Float();
	resolveIdentifier(source[2],dest->subtype);
	dest->additionalInfo = source[3].Float();
	dest->duration = Bonus::PERMANENT; //TODO: handle flags (as integer)
	dest->turnsRemain = 0;
}


Bonus * JsonUtils::parseBonus (const JsonVector &ability_vec) //TODO: merge with AddAbility, create universal parser for all bonus properties
{
	auto  b = new Bonus();
	std::string type = ability_vec[0].String();
	auto it = bonusNameMap.find(type);
	if (it == bonusNameMap.end())
	{
		logGlobal->errorStream() << "Error: invalid ability type " << type;
		return b;
	}
	b->type = it->second;

	parseTypedBonusShort(ability_vec, b);
	return b;
}
template <typename T>
const T & parseByMap(const std::map<std::string, T> & map, const JsonNode * val, std::string err)
{
	static T defaultValue = T();
	if (!val->isNull())
	{
		std::string type = val->String();
		auto it = map.find(type);
		if (it == map.end())
		{
			logGlobal->errorStream() << "Error: invalid " << err << type;
			return defaultValue;
		}
		else
		{
			return it->second;
		}
	}
	else
		return defaultValue;
}

void JsonUtils::resolveIdentifier (si32 &var, const JsonNode &node, std::string name)
{
	const JsonNode &value = node[name];
	if (!value.isNull())
	{
		switch (value.getType())
		{
			case JsonNode::DATA_FLOAT:
				var = value.Float();
				break;
			case JsonNode::DATA_STRING:
				VLC->modh->identifiers.requestIdentifier(value, [&](si32 identifier)
				{
					var = identifier;
				});
				break;
			default:
				logGlobal->errorStream() << "Error! Wrong identifier used for value of " << name;
		}
	}
}

void JsonUtils::resolveIdentifier (const JsonNode &node, si32 &var)
{
	switch (node.getType())
	{
		case JsonNode::DATA_FLOAT:
			var = node.Float();
			break;
		case JsonNode::DATA_STRING:
			VLC->modh->identifiers.requestIdentifier (node, [&](si32 identifier)
			{
				var = identifier;
			});
			break;
		default:
			logGlobal->errorStream() << "Error! Wrong identifier used for identifier!";
	}
}

Bonus * JsonUtils::parseBonus (const JsonNode &ability)
{

	auto  b = new Bonus();
	const JsonNode *value;

	std::string type = ability["type"].String();
	auto it = bonusNameMap.find(type);
	if (it == bonusNameMap.end())
	{
		logGlobal->errorStream() << "Error: invalid ability type " << type;
		return b;
	}
	b->type = it->second;

	resolveIdentifier (b->subtype, ability, "subtype");

	b->val = ability["val"].Float();

	value = &ability["valueType"];
	if (!value->isNull())
		b->valType = static_cast<Bonus::ValueType>(parseByMap(bonusValueMap, value, "value type "));

	resolveIdentifier (b->additionalInfo, ability, "addInfo");

	b->turnsRemain = ability["turns"].Float();

	b->sid = ability["sourceID"].Float();

	b->description = ability["description"].String();

	value = &ability["effectRange"];
	if (!value->isNull())
		b->effectRange = static_cast<Bonus::LimitEffect>(parseByMap(bonusLimitEffect, value, "effect range "));

	value = &ability["duration"];
	if (!value->isNull())
	{
		switch (value->getType())
		{
		case JsonNode::DATA_STRING:
			b->duration = parseByMap(bonusDurationMap, value, "duration type ");
			break;
		case JsonNode::DATA_VECTOR:
			{
				ui16 dur = 0;
				for (const JsonNode & d : value->Vector())
				{
					dur |= parseByMap(bonusDurationMap, &d, "duration type ");
				}
				b->duration = dur;
			}
			break;
		default:
			logGlobal->errorStream() << "Error! Wrong bonus duration format.";
		}
	}

	value = &ability["source"];
	if (!value->isNull())
		b->source = static_cast<Bonus::BonusSource>(parseByMap(bonusSourceMap, value, "source type "));

	value = &ability["limiters"];
	if (!value->isNull())
	{
		for (const JsonNode & limiter : value->Vector())
		{
			switch (limiter.getType())
			{
				case JsonNode::DATA_STRING: //pre-defined limiters
					b->limiter = parseByMap(bonusLimiterMap, &limiter, "limiter type ");
					break;
				case JsonNode::DATA_STRUCT: //customizable limiters
					{
						std::shared_ptr<ILimiter> l;
						if (limiter["type"].String() == "CREATURE_TYPE_LIMITER")
						{
							std::shared_ptr<CCreatureTypeLimiter> l2 = std::make_shared<CCreatureTypeLimiter>(); //TODO: How the hell resolve pointer to creature?
							const JsonVector vec = limiter["parameters"].Vector();
							VLC->modh->identifiers.requestIdentifier("creature", vec[0], [=](si32 creature)
							{
								l2->setCreature (CreatureID(creature));
							});
							if (vec.size() > 1)
							{
								l2->includeUpgrades = vec[1].Bool();
							}
							else
								l2->includeUpgrades = false;

							l = l2;
						}
						if (limiter["type"].String() == "HAS_ANOTHER_BONUS_LIMITER")
						{
							std::shared_ptr<HasAnotherBonusLimiter> l2 = std::make_shared<HasAnotherBonusLimiter>();
							const JsonVector vec = limiter["parameters"].Vector();
							std::string anotherBonusType = vec[0].String();

							auto it = bonusNameMap.find (anotherBonusType);
							if (it == bonusNameMap.end())
							{
								logGlobal->errorStream() << "Error: invalid ability type " << anotherBonusType;
								continue;
							}
							l2->type = it->second;

							if (vec.size() > 1 )
							{
								resolveIdentifier (vec[1], l2->subtype);
								l2->isSubtypeRelevant = true;
							}
							l = l2;
						}
						b->addLimiter(l);
					}
					break;
			}
		}
	}

	value = &ability["propagator"];
	if (!value->isNull())
		b->propagator = parseByMap(bonusPropagatorMap, value, "propagator type ");

	return b;
}

//returns first Key with value equal to given one
template<class Key, class Val>
Key reverseMapFirst(const Val & val, const std::map<Key, Val> & map)
{
	for(auto it : map)
	{
		if(it.second == val)
		{
			return it.first;
		}
	}
	assert(0);
	return "";
}

void JsonUtils::unparseBonus( JsonNode &node, const Bonus * bonus )
{
	node["type"].String() = reverseMapFirst<std::string, Bonus::BonusType>(bonus->type, bonusNameMap);
	node["subtype"].Float() = bonus->subtype;
	node["val"].Float() = bonus->val;
	node["valueType"].String() = reverseMapFirst<std::string, Bonus::ValueType>(bonus->valType, bonusValueMap);
	node["additionalInfo"].Float() = bonus->additionalInfo;
	node["turns"].Float() = bonus->turnsRemain;
	node["sourceID"].Float() = bonus->source;
	node["description"].String() = bonus->description;
	node["effectRange"].String() = reverseMapFirst<std::string, Bonus::LimitEffect>(bonus->effectRange, bonusLimitEffect);
	node["duration"].String() = reverseMapFirst<std::string, ui16>(bonus->duration, bonusDurationMap);
	node["source"].String() = reverseMapFirst<std::string, Bonus::BonusSource>(bonus->source, bonusSourceMap);
	if(bonus->limiter)
	{
		node["limiter"].String() = reverseMapFirst<std::string, TLimiterPtr>(bonus->limiter, bonusLimiterMap);
	}
	if(bonus->propagator)
	{
		node["propagator"].String() = reverseMapFirst<std::string, TPropagatorPtr>(bonus->propagator, bonusPropagatorMap);
	}
}

void minimizeNode(JsonNode & node, const JsonNode & schema)
{
	if (schema["type"].String() == "object")
	{
		std::set<std::string> foundEntries;

		for(auto & entry : schema["required"].Vector())
		{
			std::string name = entry.String();
			foundEntries.insert(name);

			minimizeNode(node[name], schema["properties"][name]);

			if (vstd::contains(node.Struct(), name) &&
				node[name] == schema["properties"][name]["default"])
			{
				node.Struct().erase(name);
			}
		}

		// erase all unhandled entries
		for (auto it = node.Struct().begin(); it != node.Struct().end();)
		{
			if (!vstd::contains(foundEntries, it->first))
				it = node.Struct().erase(it);
			else
				it++;
		}
	}
}

void JsonUtils::minimize(JsonNode & node, std::string schemaName)
{
	minimizeNode(node, getSchema(schemaName));
}

// FIXME: except for several lines function is identical to minimizeNode. Some way to reduce duplication?
void maximizeNode(JsonNode & node, const JsonNode & schema)
{
	// "required" entry can only be found in object/struct
	if (schema["type"].String() == "object")
	{
		std::set<std::string> foundEntries;

		// check all required entries that have default version
		for(auto & entry : schema["required"].Vector())
		{
			std::string name = entry.String();
			foundEntries.insert(name);

			if (node[name].isNull() &&
				!schema["properties"][name]["default"].isNull())
			{
				node[name] = schema["properties"][name]["default"];
			}
			maximizeNode(node[name], schema["properties"][name]);
		}

		// erase all unhandled entries
		for (auto it = node.Struct().begin(); it != node.Struct().end();)
		{
			if (!vstd::contains(foundEntries, it->first))
				it = node.Struct().erase(it);
			else
				it++;
		}
	}
}

void JsonUtils::maximize(JsonNode & node, std::string schemaName)
{
	maximizeNode(node, getSchema(schemaName));
}

bool JsonUtils::validate(const JsonNode &node, std::string schemaName, std::string dataName)
{
	std::string log = Validation::check(schemaName, node);
	if (!log.empty())
	{
		logGlobal->warnStream() << "Data in " << dataName << " is invalid!";
		logGlobal->warnStream() << log;
	}
	return log.empty();
}

const JsonNode & getSchemaByName(std::string name)
{
	// cached schemas to avoid loading json data multiple times
	static std::map<std::string, JsonNode> loadedSchemas;

	if (vstd::contains(loadedSchemas, name))
		return loadedSchemas[name];

	std::string filename = "config/schemas/" + name + ".json";

	if (CResourceHandler::get()->existsResource(ResourceID(filename)))
	{
		loadedSchemas[name] = JsonNode(ResourceID(filename));
		return loadedSchemas[name];
	}

	logGlobal->errorStream() << "Error: missing schema with name " << name << "!";
	assert(0);
	return nullNode;
}

const JsonNode & JsonUtils::getSchema(std::string URI)
{
	std::vector<std::string> segments;

	size_t posColon = URI.find(':');
	size_t posHash  = URI.find('#');
	assert(posColon != std::string::npos);

	std::string protocolName = URI.substr(0, posColon);
	std::string filename =     URI.substr(posColon + 1, posHash - posColon - 1);

	if (protocolName != "vcmi")
	{
		logGlobal->errorStream() << "Error: unsupported URI protocol for schema: " << segments[0];
		return nullNode;
	}

	// check if json pointer if present (section after hash in string)
	if (posHash == std::string::npos || posHash == URI.size() - 1)
		return getSchemaByName(filename);
	else
		return getSchemaByName(filename).resolvePointer(URI.substr(posHash + 1));
}

void JsonUtils::merge(JsonNode & dest, JsonNode & source)
{
	if (dest.getType() == JsonNode::DATA_NULL)
	{
		std::swap(dest, source);
		return;
	}

	switch (source.getType())
	{
		case JsonNode::DATA_NULL:
		{
			dest.clear();
			break;
		}
		case JsonNode::DATA_BOOL:
		case JsonNode::DATA_FLOAT:
		case JsonNode::DATA_STRING:
		case JsonNode::DATA_VECTOR:
		{
			std::swap(dest, source);
			break;
		}
		case JsonNode::DATA_STRUCT:
		{
			//recursively merge all entries from struct
			for(auto & node : source.Struct())
				merge(dest[node.first], node.second);
		}
	}
}

void JsonUtils::mergeCopy(JsonNode & dest, JsonNode source)
{
	// uses copy created in stack to safely merge two nodes
	merge(dest, source);
}

void JsonUtils::inherit(JsonNode & descendant, const JsonNode & base)
{
	JsonNode inheritedNode(base);
	merge(inheritedNode,descendant);
	descendant.swap(inheritedNode);
}

JsonNode JsonUtils::assembleFromFiles(std::vector<std::string> files)
{
	bool isValid;
	return assembleFromFiles(files, isValid);
}

JsonNode JsonUtils::assembleFromFiles(std::vector<std::string> files, bool &isValid)
{
	isValid = true;
	JsonNode result;

	for(std::string file : files)
	{
		bool isValidFile;
		JsonNode section(ResourceID(file, EResType::TEXT), isValidFile);
		merge(result, section);
		isValid |= isValidFile;
	}
	return result;
}

JsonNode JsonUtils::assembleFromFiles(std::string filename)
{
	JsonNode result;
	ResourceID resID(filename, EResType::TEXT);

	for(auto & loader : CResourceHandler::get()->getResourcesWithName(resID))
	{
		// FIXME: some way to make this code more readable
		auto stream = loader->load(resID);
		std::unique_ptr<ui8[]> textData(new ui8[stream->getSize()]);
		stream->read(textData.get(), stream->getSize());

		JsonNode section((char*)textData.get(), stream->getSize());
		merge(result, section);
	}
	return result;
}

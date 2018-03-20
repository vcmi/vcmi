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

JsonNode::JsonNode(JsonType Type):
	type(JsonType::DATA_NULL)
{
	setType(Type);
}

JsonNode::JsonNode(const char *data, size_t datasize):
	type(JsonType::DATA_NULL)
{
	JsonParser parser(data, datasize);
	*this = parser.parse("<unknown>");
}

JsonNode::JsonNode(ResourceID && fileURI):
	type(JsonType::DATA_NULL)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
}

JsonNode::JsonNode(const ResourceID & fileURI):
	type(JsonType::DATA_NULL)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
}

JsonNode::JsonNode(ResourceID && fileURI, bool &isValidSyntax):
	type(JsonType::DATA_NULL)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
	isValidSyntax = parser.isValid();
}

JsonNode::JsonNode(const JsonNode &copy):
	type(JsonType::DATA_NULL),
	meta(copy.meta),
	flags(copy.flags)
{
	setType(copy.getType());
	switch(type)
	{
		break; case JsonType::DATA_NULL:
		break; case JsonType::DATA_BOOL:   Bool()    = copy.Bool();
		break; case JsonType::DATA_FLOAT:  Float()   = copy.Float();
		break; case JsonType::DATA_STRING: String()  = copy.String();
		break; case JsonType::DATA_VECTOR: Vector()  = copy.Vector();
		break; case JsonType::DATA_STRUCT: Struct()  = copy.Struct();
		break; case JsonType::DATA_INTEGER:Integer() = copy.Integer();
	}
}

JsonNode::~JsonNode()
{
	setType(JsonType::DATA_NULL);
}

void JsonNode::swap(JsonNode &b)
{
	using std::swap;
	swap(meta, b.meta);
	swap(data, b.data);
	swap(type, b.type);
	swap(flags, b.flags);
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
			case JsonType::DATA_NULL:   return true;
			case JsonType::DATA_BOOL:   return Bool() == other.Bool();
			case JsonType::DATA_FLOAT:  return Float() == other.Float();
			case JsonType::DATA_STRING: return String() == other.String();
			case JsonType::DATA_VECTOR: return Vector() == other.Vector();
			case JsonType::DATA_STRUCT: return Struct() == other.Struct();
			case JsonType::DATA_INTEGER:return Integer()== other.Integer();
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
			break; case JsonType::DATA_VECTOR:
			{
				for(auto & node : Vector())
				{
					node.setMeta(metadata);
				}
			}
			break; case JsonType::DATA_STRUCT:
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

	//float<->int conversion
	if(type == JsonType::DATA_FLOAT && Type == JsonType::DATA_INTEGER)
	{
		si64 converted = data.Float;
		type = Type;
		data.Integer = converted;
		return;
	}
	else if(type == JsonType::DATA_INTEGER && Type == JsonType::DATA_FLOAT)
	{
		double converted = data.Integer;
		type = Type;
		data.Float = converted;
		return;
	}

	//Reset node to nullptr
	if (Type != JsonType::DATA_NULL)
		setType(JsonType::DATA_NULL);

	switch (type)
	{
		break; case JsonType::DATA_STRING:  delete data.String;
		break; case JsonType::DATA_VECTOR:  delete data.Vector;
		break; case JsonType::DATA_STRUCT:  delete data.Struct;
		break; default:
		break;
	}
	//Set new node type
	type = Type;
	switch(type)
	{
		break; case JsonType::DATA_NULL:
		break; case JsonType::DATA_BOOL:   data.Bool = false;
		break; case JsonType::DATA_FLOAT:  data.Float = 0;
		break; case JsonType::DATA_STRING: data.String = new std::string();
		break; case JsonType::DATA_VECTOR: data.Vector = new JsonVector();
		break; case JsonType::DATA_STRUCT: data.Struct = new JsonMap();
		break; case JsonType::DATA_INTEGER: data.Integer = 0;
	}
}

bool JsonNode::isNull() const
{
	return type == JsonType::DATA_NULL;
}

bool JsonNode::isNumber() const
{
	return type == JsonType::DATA_INTEGER || type == JsonType::DATA_FLOAT;
}

bool JsonNode::containsBaseData() const
{
	switch(type)
	{
	case JsonType::DATA_NULL:
		return false;
	case JsonType::DATA_STRUCT:
		for(auto elem : *data.Struct)
		{
			if(elem.second.containsBaseData())
				return true;
		}
		return false;
	default:
		//other types (including vector) cannot be extended via merge
		return true;
	}
}

bool JsonNode::isCompact() const
{
	switch(type)
	{
	case JsonType::DATA_VECTOR:
		for(JsonNode & elem : *data.Vector)
		{
			if(!elem.isCompact())
				return false;
		}
		return true;
	case JsonType::DATA_STRUCT:
		{
			int propertyCount = data.Struct->size();
			if(propertyCount == 0)
				return true;
			else if(propertyCount == 1)
				return data.Struct->begin()->second.isCompact();
		}
		return false;
	default:
		return true;
	}
}

void JsonNode::clear()
{
	setType(JsonType::DATA_NULL);
}

bool & JsonNode::Bool()
{
	setType(JsonType::DATA_BOOL);
	return data.Bool;
}

double & JsonNode::Float()
{
	setType(JsonType::DATA_FLOAT);
	return data.Float;
}

si64 & JsonNode::Integer()
{
	setType(JsonType::DATA_INTEGER);
	return data.Integer;
}

std::string & JsonNode::String()
{
	setType(JsonType::DATA_STRING);
	return *data.String;
}

JsonVector & JsonNode::Vector()
{
	setType(JsonType::DATA_VECTOR);
	return *data.Vector;
}

JsonMap & JsonNode::Struct()
{
	setType(JsonType::DATA_STRUCT);
	return *data.Struct;
}

const bool boolDefault = false;
bool JsonNode::Bool() const
{
	if (type == JsonType::DATA_NULL)
		return boolDefault;
	assert(type == JsonType::DATA_BOOL);
	return data.Bool;
}

const double floatDefault = 0;
double JsonNode::Float() const
{
	if(type == JsonType::DATA_NULL)
		return floatDefault;
	else if(type == JsonType::DATA_INTEGER)
		return data.Integer;

	assert(type == JsonType::DATA_FLOAT);
	return data.Float;
}

const si64 integetDefault = 0;
si64 JsonNode::Integer() const
{
	if(type == JsonType::DATA_NULL)
		return integetDefault;
	else if(type == JsonType::DATA_FLOAT)
		return data.Float;

	assert(type == JsonType::DATA_INTEGER);
	return data.Integer;
}

const std::string stringDefault = std::string();
const std::string & JsonNode::String() const
{
	if (type == JsonType::DATA_NULL)
		return stringDefault;
	assert(type == JsonType::DATA_STRING);
	return *data.String;
}

const JsonVector vectorDefault = JsonVector();
const JsonVector & JsonNode::Vector() const
{
	if (type == JsonType::DATA_NULL)
		return vectorDefault;
	assert(type == JsonType::DATA_VECTOR);
	return *data.Vector;
}

const JsonMap mapDefault = JsonMap();
const JsonMap & JsonNode::Struct() const
{
	if (type == JsonType::DATA_NULL)
		return mapDefault;
	assert(type == JsonType::DATA_STRUCT);
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

	if (in.getType() == JsonNode::JsonType::DATA_VECTOR)
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

std::string JsonNode::toJson(bool compact) const
{
	std::ostringstream out;
	JsonWriter writer(out, compact);
	writer.writeNode(*this);
	return out.str();
}

///JsonUtils

void JsonUtils::parseTypedBonusShort(const JsonVector& source, std::shared_ptr<Bonus> dest)
{
	dest->val = source[1].Float();
	resolveIdentifier(source[2],dest->subtype);
	dest->additionalInfo = source[3].Float();
	dest->duration = Bonus::PERMANENT; //TODO: handle flags (as integer)
	dest->turnsRemain = 0;
}


std::shared_ptr<Bonus> JsonUtils::parseBonus (const JsonVector &ability_vec) //TODO: merge with AddAbility, create universal parser for all bonus properties
{
	auto b = std::make_shared<Bonus>();
	std::string type = ability_vec[0].String();
	auto it = bonusNameMap.find(type);
	if (it == bonusNameMap.end())
	{
		logMod->error("Error: invalid ability type %s.", type);
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
			logMod->error("Error: invalid %s%s.", err, type);
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

void JsonUtils::resolveIdentifier(si32 &var, const JsonNode &node, std::string name)
{
	const JsonNode &value = node[name];
	if (!value.isNull())
	{
		switch (value.getType())
		{
			case JsonNode::JsonType::DATA_INTEGER:
				var = value.Integer();
				break;
			case JsonNode::JsonType::DATA_FLOAT:
				var = value.Float();
				break;
			case JsonNode::JsonType::DATA_STRING:
				VLC->modh->identifiers.requestIdentifier(value, [&](si32 identifier)
				{
					var = identifier;
				});
				break;
			default:
				logMod->error("Error! Wrong identifier used for value of %s.", name);
		}
	}
}

void JsonUtils::resolveAddInfo(CAddInfo & var, const JsonNode & node)
{
	const JsonNode & value = node["addInfo"];
	if (!value.isNull())
	{
		switch (value.getType())
		{
		case JsonNode::JsonType::DATA_INTEGER:
			var = value.Integer();
			break;
		case JsonNode::JsonType::DATA_FLOAT:
			var = value.Float();
			break;
		case JsonNode::JsonType::DATA_STRING:
			VLC->modh->identifiers.requestIdentifier(value, [&](si32 identifier)
			{
				var = identifier;
			});
			break;
		case JsonNode::JsonType::DATA_VECTOR:
			{
				const JsonVector & vec = value.Vector();
				var.resize(vec.size());
				for(int i = 0; i < vec.size(); i++)
				{
					switch(vec[i].getType())
					{
						case JsonNode::JsonType::DATA_INTEGER:
							var[i] = vec[i].Integer();
							break;
						case JsonNode::JsonType::DATA_FLOAT:
							var[i] = vec[i].Float();
							break;
						case JsonNode::JsonType::DATA_STRING:
							VLC->modh->identifiers.requestIdentifier(vec[i], [&var,i](si32 identifier)
							{
								var[i] = identifier;
							});
							break;
						default:
							logMod->error("Error! Wrong identifier used for value of addInfo[%d].", i);
					}
				}
				break;
			}
		default:
			logMod->error("Error! Wrong identifier used for value of addInfo.");
		}
	}
}

void JsonUtils::resolveIdentifier(const JsonNode &node, si32 &var)
{
	switch (node.getType())
	{
		case JsonNode::JsonType::DATA_INTEGER:
			var = node.Integer();
			break;
		case JsonNode::JsonType::DATA_FLOAT:
			var = node.Float();
			break;
		case JsonNode::JsonType::DATA_STRING:
			VLC->modh->identifiers.requestIdentifier(node, [&](si32 identifier)
			{
				var = identifier;
			});
			break;
		default:
			logMod->error("Error! Wrong identifier used for identifier!");
	}
}

std::shared_ptr<Bonus> JsonUtils::parseBonus(const JsonNode &ability)
{
	auto b = std::make_shared<Bonus>();
	if (!parseBonus(ability, b.get()))
	{
		return nullptr;
	}
	return b;
}

bool JsonUtils::parseBonus(const JsonNode &ability, Bonus *b)
{
	const JsonNode *value;

	std::string type = ability["type"].String();
	auto it = bonusNameMap.find(type);
	if (it == bonusNameMap.end())
	{
		logMod->error("Error: invalid ability type %s.", type);
		return false;
	}
	b->type = it->second;

	resolveIdentifier(b->subtype, ability, "subtype");

	b->val = ability["val"].Float();

	value = &ability["valueType"];
	if (!value->isNull())
		b->valType = static_cast<Bonus::ValueType>(parseByMap(bonusValueMap, value, "value type "));

	b->stacking = ability["stacking"].String();

	resolveAddInfo(b->additionalInfo, ability);

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
		case JsonNode::JsonType::DATA_STRING:
			b->duration = parseByMap(bonusDurationMap, value, "duration type ");
			break;
		case JsonNode::JsonType::DATA_VECTOR:
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
			logMod->error("Error! Wrong bonus duration format.");
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
				case JsonNode::JsonType::DATA_STRING: //pre-defined limiters
					b->limiter = parseByMap(bonusLimiterMap, &limiter, "limiter type ");
					break;
				case JsonNode::JsonType::DATA_STRUCT: //customizable limiters
					{
						std::shared_ptr<ILimiter> l;
						if (limiter["type"].String() == "CREATURE_TYPE_LIMITER")
						{
							std::shared_ptr<CCreatureTypeLimiter> l2 = std::make_shared<CCreatureTypeLimiter>(); //TODO: How the hell resolve pointer to creature?
							const JsonVector vec = limiter["parameters"].Vector();
							VLC->modh->identifiers.requestIdentifier("creature", vec[0], [=](si32 creature)
							{
								l2->setCreature(CreatureID(creature));
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

							auto it = bonusNameMap.find(anotherBonusType);
							if (it == bonusNameMap.end())
							{
								logMod->error("Error: invalid ability type %s.", anotherBonusType);
								continue;
							}
							l2->type = it->second;

							if (vec.size() > 1 )
							{
								resolveIdentifier(vec[1], l2->subtype);
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

	value = &ability["updater"];
	if(!value->isNull())
	{
		const JsonNode & updaterJson = *value;
		switch(updaterJson.getType())
		{
		case JsonNode::JsonType::DATA_STRING:
			b->addUpdater(parseByMap(bonusUpdaterMap, &updaterJson, "updater type "));
			break;
		case JsonNode::JsonType::DATA_STRUCT:
			if(updaterJson["type"].String() == "GROWS_WITH_LEVEL")
			{
				std::shared_ptr<GrowsWithLevelUpdater> updater = std::make_shared<GrowsWithLevelUpdater>();
				const JsonVector param = updaterJson["parameters"].Vector();
				updater->valPer20 = param[0].Integer();
				if(param.size() > 1)
					updater->stepSize = param[1].Integer();
				b->addUpdater(updater);
			}
			else
				logMod->warn("Unknown updater type \"%s\"", updaterJson["type"].String());
			break;
		}
	}

	return true;
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
		logMod->warn("Data in %s is invalid!", dataName);
		logMod->warn(log);
		logMod->trace("%s json: %s", dataName, node.toJson(true));
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

	logMod->error("Error: missing schema with name %s!", name);
	assert(0);
	return nullNode;
}

const JsonNode & JsonUtils::getSchema(std::string URI)
{
	size_t posColon = URI.find(':');
	size_t posHash  = URI.find('#');
	if(posColon == std::string::npos)
	{
		logMod->error("Invalid schema URI:%s", URI);
		return nullNode;
	}

	std::string protocolName = URI.substr(0, posColon);
	std::string filename =     URI.substr(posColon + 1, posHash - posColon - 1);

	if(protocolName != "vcmi")
	{
		logMod->error("Error: unsupported URI protocol for schema: %s", URI);
		return nullNode;
	}

	// check if json pointer if present (section after hash in string)
	if(posHash == std::string::npos || posHash == URI.size() - 1)
		return getSchemaByName(filename);
	else
		return getSchemaByName(filename).resolvePointer(URI.substr(posHash + 1));
}

void JsonUtils::merge(JsonNode & dest, JsonNode & source, bool noOverride)
{
	if (dest.getType() == JsonNode::JsonType::DATA_NULL)
	{
		std::swap(dest, source);
		return;
	}

	switch (source.getType())
	{
		case JsonNode::JsonType::DATA_NULL:
		{
			dest.clear();
			break;
		}
		case JsonNode::JsonType::DATA_BOOL:
		case JsonNode::JsonType::DATA_FLOAT:
		case JsonNode::JsonType::DATA_INTEGER:
		case JsonNode::JsonType::DATA_STRING:
		case JsonNode::JsonType::DATA_VECTOR:
		{
			std::swap(dest, source);
			break;
		}
		case JsonNode::JsonType::DATA_STRUCT:
		{
			if(!noOverride && vstd::contains(source.flags, "override"))
			{
				std::swap(dest, source);
			}
			else
			{
				//recursively merge all entries from struct
				for(auto & node : source.Struct())
					merge(dest[node.first], node.second, noOverride);
			}
		}
	}
}

void JsonUtils::mergeCopy(JsonNode & dest, JsonNode source, bool noOverride)
{
	// uses copy created in stack to safely merge two nodes
	merge(dest, source, noOverride);
}

void JsonUtils::inherit(JsonNode & descendant, const JsonNode & base)
{
	JsonNode inheritedNode(base);
	merge(inheritedNode, descendant, true);
	descendant.swap(inheritedNode);
}

JsonNode JsonUtils::intersect(const std::vector<JsonNode> & nodes, bool pruneEmpty)
{
	if(nodes.size() == 0)
		return nullNode;

	JsonNode result = nodes[0];
	for(int i = 1; i < nodes.size(); i++)
	{
		if(result.isNull())
			break;
		result = JsonUtils::intersect(result, nodes[i], pruneEmpty);
	}
	return result;
}

JsonNode JsonUtils::intersect(const JsonNode & a, const JsonNode & b, bool pruneEmpty)
{
	if(a.getType() == JsonNode::JsonType::DATA_STRUCT && b.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		// intersect individual properties
		JsonNode result(JsonNode::JsonType::DATA_STRUCT);
		for(auto property : a.Struct())
		{
			if(vstd::contains(b.Struct(), property.first))
			{
				JsonNode propertyIntersect = JsonUtils::intersect(property.second, b.Struct().find(property.first)->second);
				if(pruneEmpty && !propertyIntersect.containsBaseData())
					continue;
				result[property.first] = propertyIntersect;
			}
		}
		return result;
	}
	else
	{
		// not a struct - same or different, no middle ground
		if(a == b)
			return a;
	}
	return nullNode;
}

JsonNode JsonUtils::difference(const JsonNode & node, const JsonNode & base)
{
	auto addsInfo = [](JsonNode diff) -> bool
	{
		switch(diff.getType())
		{
		case JsonNode::JsonType::DATA_NULL:
			return false;
		case JsonNode::JsonType::DATA_STRUCT:
			return diff.Struct().size() > 0;
		default:
			return true;
		}
	};

	if(node.getType() == JsonNode::JsonType::DATA_STRUCT && base.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		// subtract individual properties
		JsonNode result(JsonNode::JsonType::DATA_STRUCT);
		for(auto property : node.Struct())
		{
			if(vstd::contains(base.Struct(), property.first))
			{
				const JsonNode propertyDifference = JsonUtils::difference(property.second, base.Struct().find(property.first)->second);
				if(addsInfo(propertyDifference))
					result[property.first] = propertyDifference;
			}
			else
			{
				result[property.first] = property.second;
			}
		}
		return result;
	}
	else
	{
		if(node == base)
			return nullNode;
	}
	return node;
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

DLL_LINKAGE JsonNode JsonUtils::boolNode(bool value)
{
	JsonNode node;
	node.Bool() = value;
	return node;
}

DLL_LINKAGE JsonNode JsonUtils::floatNode(double value)
{
	JsonNode node;
	node.Float() = value;
	return node;
}

DLL_LINKAGE JsonNode JsonUtils::stringNode(std::string value)
{
	JsonNode node;
	node.String() = value;
	return node;
}

DLL_LINKAGE JsonNode JsonUtils::intNode(si64 value)
{
	JsonNode node;
	node.Integer() = value;
	return node;
}

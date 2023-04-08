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
#include "StringConstants.h"
#include "battle/BattleHex.h"

namespace
{
// to avoid duplicating const and non-const code
template<typename Node>
Node & resolvePointer(Node & in, const std::string & pointer)
{
	if(pointer.empty())
		return in;
	assert(pointer[0] == '/');

	size_t splitPos = pointer.find('/', 1);

	std::string entry = pointer.substr(1, splitPos - 1);
	std::string remainer = splitPos == std::string::npos ? "" : pointer.substr(splitPos);

	if(in.getType() == VCMI_LIB_WRAP_NAMESPACE(JsonNode)::JsonType::DATA_VECTOR)
	{
		if(entry.find_first_not_of("0123456789") != std::string::npos) // non-numbers in string
			throw std::runtime_error("Invalid Json pointer");

		if(entry.size() > 1 && entry[0] == '0') // leading zeros are not allowed
			throw std::runtime_error("Invalid Json pointer");

		auto index = boost::lexical_cast<size_t>(entry);

		if (in.Vector().size() > index)
			return in.Vector()[index].resolvePointer(remainer);
	}
	return in[entry].resolvePointer(remainer);
}
}

VCMI_LIB_NAMESPACE_BEGIN

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

JsonNode::JsonNode(const std::string & idx, const ResourceID & fileURI):
type(JsonType::DATA_NULL)
{
	auto file = CResourceHandler::get(idx)->load(fileURI)->readAll();
	
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

void JsonNode::setMeta(const std::string & metadata, bool recursive)
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
		si64 converted = static_cast<si64>(data.Float);
		type = Type;
		data.Integer = converted;
		return;
	}
	else if(type == JsonType::DATA_INTEGER && Type == JsonType::DATA_FLOAT)
	{
		auto converted = static_cast<double>(data.Integer);
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

bool JsonNode::isString() const
{
	return type == JsonType::DATA_STRING;
}

bool JsonNode::isVector() const
{
	return type == JsonType::DATA_VECTOR;
}

bool JsonNode::isStruct() const
{
	return type == JsonType::DATA_STRUCT;
}

bool JsonNode::containsBaseData() const
{
	switch(type)
	{
	case JsonType::DATA_NULL:
		return false;
	case JsonType::DATA_STRUCT:
		for(const auto & elem : *data.Struct)
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
			auto propertyCount = data.Struct->size();
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

bool JsonNode::TryBoolFromString(bool & success) const
{
	success = true;
	if(type == JsonNode::JsonType::DATA_BOOL)
		return Bool();

	success = type == JsonNode::JsonType::DATA_STRING;
	if(success)
	{
		auto boolParamStr = String();
		boost::algorithm::trim(boolParamStr);
		boost::algorithm::to_lower(boolParamStr);
		success = boolParamStr == "true";

		if(success)
			return true;
		
		success = boolParamStr == "false";
	}
	return false;
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
		return static_cast<double>(data.Integer);

	assert(type == JsonType::DATA_FLOAT);
	return data.Float;
}

const si64 integetDefault = 0;
si64 JsonNode::Integer() const
{
	if(type == JsonType::DATA_NULL)
		return integetDefault;
	else if(type == JsonType::DATA_FLOAT)
		return static_cast<si64>(data.Float);

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

JsonNode & JsonNode::operator[](const std::string & child)
{
	return Struct()[child];
}

const JsonNode & JsonNode::operator[](const std::string & child) const
{
	auto it = Struct().find(child);
	if (it != Struct().end())
		return it->second;
	return nullNode;
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

void JsonUtils::parseTypedBonusShort(const JsonVector & source, const std::shared_ptr<Bonus> & dest)
{
	dest->val = static_cast<si32>(source[1].Float());
	resolveIdentifier(source[2],dest->subtype);
	dest->additionalInfo = static_cast<si32>(source[3].Float());
	dest->duration = Bonus::PERMANENT; //TODO: handle flags (as integer)
	dest->turnsRemain = 0;
}

std::shared_ptr<Bonus> JsonUtils::parseBonus(const JsonVector & ability_vec)
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
const T parseByMap(const std::map<std::string, T> & map, const JsonNode * val, const std::string & err)
{
	static T defaultValue = T();
	if (!val->isNull())
	{
		const std::string & type = val->String();
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

template <typename T>
const T parseByMapN(const std::map<std::string, T> & map, const JsonNode * val, const std::string & err)
{
	if(val->isNumber())
		return static_cast<T>(val->Integer());
	else
		return parseByMap<T>(map, val, err);
}

void JsonUtils::resolveIdentifier(si32 & var, const JsonNode & node, const std::string & name)
{
	const JsonNode &value = node[name];
	if (!value.isNull())
	{
		switch (value.getType())
		{
			case JsonNode::JsonType::DATA_INTEGER:
				var = static_cast<si32>(value.Integer());
				break;
			case JsonNode::JsonType::DATA_FLOAT:
				var = static_cast<si32>(value.Float());
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
			var = static_cast<si32>(value.Integer());
			break;
		case JsonNode::JsonType::DATA_FLOAT:
			var = static_cast<si32>(value.Float());
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
							var[i] = static_cast<si32>(vec[i].Integer());
							break;
						case JsonNode::JsonType::DATA_FLOAT:
							var[i] = static_cast<si32>(vec[i].Float());
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
			var = static_cast<si32>(node.Integer());
			break;
		case JsonNode::JsonType::DATA_FLOAT:
			var = static_cast<si32>(node.Float());
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

std::shared_ptr<ILimiter> JsonUtils::parseLimiter(const JsonNode & limiter)
{
	switch(limiter.getType())
	{
	case JsonNode::JsonType::DATA_VECTOR:
		{
			const JsonVector & subLimiters = limiter.Vector();
			if(subLimiters.empty())
			{
				logMod->warn("Warning: empty limiter list");
				return std::make_shared<AllOfLimiter>();
			}
			std::shared_ptr<AggregateLimiter> result;
			int offset = 1;
			// determine limiter type and offset for sub-limiters
			if(subLimiters[0].getType() == JsonNode::JsonType::DATA_STRING)
			{
				const std::string & aggregator = subLimiters[0].String();
				if(aggregator == AllOfLimiter::aggregator)
					result = std::make_shared<AllOfLimiter>();
				else if(aggregator == AnyOfLimiter::aggregator)
					result = std::make_shared<AnyOfLimiter>();
				else if(aggregator == NoneOfLimiter::aggregator)
					result = std::make_shared<NoneOfLimiter>();
			}
			if(!result)
			{
				// collapse for single limiter without explicit aggregate operator
				if(subLimiters.size() == 1)
					return parseLimiter(subLimiters[0]);
				// implicit aggregator must be allOf
				result = std::make_shared<AllOfLimiter>();
				offset = 0;
			}
			if(subLimiters.size() == offset)
				logMod->warn("Warning: empty sub-limiter list");
			for(int sl = offset; sl < subLimiters.size(); ++sl)
				result->add(parseLimiter(subLimiters[sl]));
			return result;
		}
		break;
	case JsonNode::JsonType::DATA_STRING: //pre-defined limiters
		return parseByMap(bonusLimiterMap, &limiter, "limiter type ");
		break;
	case JsonNode::JsonType::DATA_STRUCT: //customizable limiters
		{
			std::string limiterType = limiter["type"].String();
			const JsonVector & parameters = limiter["parameters"].Vector();
			if(limiterType == "CREATURE_TYPE_LIMITER")
			{
				std::shared_ptr<CCreatureTypeLimiter> creatureLimiter = std::make_shared<CCreatureTypeLimiter>();
				VLC->modh->identifiers.requestIdentifier("creature", parameters[0], [=](si32 creature)
				{
					creatureLimiter->setCreature(CreatureID(creature));
				});
				auto includeUpgrades = false;

				if(parameters.size() > 1)
				{
					bool success = true;
					includeUpgrades = parameters[1].TryBoolFromString(success);

					if(!success)
						logMod->error("Second parameter of '%s' limiter should be Bool", limiterType);
				}
				creatureLimiter->includeUpgrades = includeUpgrades;
				return creatureLimiter;
			}
			else if(limiterType == "HAS_ANOTHER_BONUS_LIMITER")
			{
				std::string anotherBonusType = parameters[0].String();
				auto it = bonusNameMap.find(anotherBonusType);
				if(it == bonusNameMap.end())
				{
					logMod->error("Error: invalid ability type %s.", anotherBonusType);
				}
				else
				{
					std::shared_ptr<HasAnotherBonusLimiter> bonusLimiter = std::make_shared<HasAnotherBonusLimiter>();
					bonusLimiter->type = it->second;
					auto findSource = [&](const JsonNode & parameter)
					{
						if(parameter.getType() == JsonNode::JsonType::DATA_STRUCT)
						{
							auto sourceIt = bonusSourceMap.find(parameter["type"].String());
							if(sourceIt != bonusSourceMap.end())
							{
								bonusLimiter->source = sourceIt->second;
								bonusLimiter->isSourceRelevant = true;
								if(!parameter["id"].isNull()) {
									resolveIdentifier(parameter["id"], bonusLimiter->sid);
									bonusLimiter->isSourceIDRelevant = true;
								}
							}
						}
						return false;
					};
					if(parameters.size() > 1)
					{
						if(findSource(parameters[1]) && parameters.size() == 2)
							return bonusLimiter;
						else
						{
							resolveIdentifier(parameters[1], bonusLimiter->subtype);
							bonusLimiter->isSubtypeRelevant = true;
							if(parameters.size() > 2)
								findSource(parameters[2]);
						}
					}
					return bonusLimiter;
				}
			}
			else if(limiterType == "CREATURE_ALIGNMENT_LIMITER")
			{
				int alignment = vstd::find_pos(GameConstants::ALIGNMENT_NAMES, parameters[0].String());
				if(alignment == -1)
					logMod->error("Error: invalid alignment %s.", parameters[0].String());
				else
					return std::make_shared<CreatureAlignmentLimiter>(static_cast<EAlignment>(alignment));
			}
			else if(limiterType == "CREATURE_FACTION_LIMITER")
			{
				std::shared_ptr<CreatureFactionLimiter> factionLimiter = std::make_shared<CreatureFactionLimiter>();
				VLC->modh->identifiers.requestIdentifier("faction", parameters[0], [=](si32 faction)
				{
					factionLimiter->faction = faction;
				});
				return factionLimiter;
			}
			else if(limiterType == "CREATURE_TERRAIN_LIMITER")
			{
				std::shared_ptr<CreatureTerrainLimiter> terrainLimiter = std::make_shared<CreatureTerrainLimiter>();
				if(!parameters.empty())
				{
					VLC->modh->identifiers.requestIdentifier("terrain", parameters[0], [=](si32 terrain)
					{
						//TODO: support limiters
						//terrainLimiter->terrainType = terrain;
					});
				}
				return terrainLimiter;
			}
			else if(limiterType == "UNIT_ON_HEXES") {
				auto hexLimiter = std::make_shared<UnitOnHexLimiter>();
				if(!parameters.empty())
				{
					for (const auto & parameter: parameters){
						if(parameter.isNumber())
							hexLimiter->applicableHexes.insert(BattleHex(parameter.Integer()));
					}
				}
				return hexLimiter;
			}
			else
			{
				logMod->error("Error: invalid customizable limiter type %s.", limiterType);
			}
		}
		break;
	default:
		break;
	}
	return nullptr;
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

std::shared_ptr<Bonus> JsonUtils::parseBuildingBonus(const JsonNode & ability, const BuildingID & building, const std::string & description)
{
	/*	duration = Bonus::PERMANENT
		source = Bonus::TOWN_STRUCTURE
		bonusType, val, subtype - get from json
	*/
	auto b = std::make_shared<Bonus>(Bonus::PERMANENT, Bonus::NONE, Bonus::TOWN_STRUCTURE, 0, building, description, -1);

	if(!parseBonus(ability, b.get()))
		return nullptr;
	return b;
}

static BonusParams convertDeprecatedBonus(const JsonNode &ability)
{
	if(vstd::contains(deprecatedBonusSet, ability["type"].String()))
	{
		logMod->warn("There is deprecated bonus found:\n%s\nTrying to convert...", ability.toJson());
		auto params = BonusParams(ability["type"].String(),
											ability["subtype"].isString() ? ability["subtype"].String() : "",
											   ability["subtype"].isNumber() ? ability["subtype"].Integer() : -1);
		if(params.isConverted)
		{
			if(!params.valRelevant) {
				params.val = static_cast<si32>(ability["val"].Float());
				params.valRelevant = true;
			}
			Bonus::ValueType valueType = Bonus::ADDITIVE_VALUE;
			if(!ability["valueType"].isNull())
				valueType = bonusValueMap.find(ability["valueType"].String())->second;

			if(ability["type"].String() == "SECONDARY_SKILL_PREMY" && valueType == Bonus::PERCENT_TO_BASE) //assume secondary skill special
			{
				params.valueType = Bonus::PERCENT_TO_TARGET_TYPE;
				params.targetType = Bonus::SECONDARY_SKILL;
				params.targetTypeRelevant = true;
			}

			if(!params.valueTypeRelevant) {
				params.valueType = valueType;
				params.valueTypeRelevant = true;
			}
			logMod->warn("Please, use this bonus:\n%s\nConverted sucessfully!", params.toJson().toJson());
			return params;
		}
		else
			logMod->error("Cannot convert bonus!\n%s", ability.toJson());
	}
	BonusParams ret;
	ret.isConverted = false;
	return ret;
}

static TUpdaterPtr parseUpdater(const JsonNode & updaterJson)
{
	switch(updaterJson.getType())
	{
	case JsonNode::JsonType::DATA_STRING:
		return parseByMap(bonusUpdaterMap, &updaterJson, "updater type ");
		break;
	case JsonNode::JsonType::DATA_STRUCT:
		if(updaterJson["type"].String() == "GROWS_WITH_LEVEL")
		{
			std::shared_ptr<GrowsWithLevelUpdater> updater = std::make_shared<GrowsWithLevelUpdater>();
			const JsonVector param = updaterJson["parameters"].Vector();
			updater->valPer20 = static_cast<int>(param[0].Integer());
			if(param.size() > 1)
				updater->stepSize = static_cast<int>(param[1].Integer());
			return updater;
		}
		else if (updaterJson["type"].String() == "ARMY_MOVEMENT")
		{
			std::shared_ptr<ArmyMovementUpdater> updater = std::make_shared<ArmyMovementUpdater>();
			if(updaterJson["parameters"].isVector())
			{
				const auto & param = updaterJson["parameters"].Vector();
				if(param.size() < 4)
					logMod->warn("Invalid ARMY_MOVEMENT parameters, using default!");
				else
				{
					updater->base = static_cast<si32>(param.at(0).Integer());
					updater->divider = static_cast<si32>(param.at(1).Integer());
					updater->multiplier = static_cast<si32>(param.at(2).Integer());
					updater->max = static_cast<si32>(param.at(3).Integer());
				}
				return updater;
			}
		}
		else
			logMod->warn("Unknown updater type \"%s\"", updaterJson["type"].String());
		break;
	}
	return nullptr;
}

bool JsonUtils::parseBonus(const JsonNode &ability, Bonus *b)
{
	const JsonNode * value = nullptr;

	std::string type = ability["type"].String();
	auto it = bonusNameMap.find(type);
	auto params = std::make_unique<BonusParams>(false);
	if (it == bonusNameMap.end())
	{
		params = std::make_unique<BonusParams>(convertDeprecatedBonus(ability));
		if(!params->isConverted)
		{
			logMod->error("Error: invalid ability type %s.", type);
			return false;
		}
		b->type = params->type;
		b->val = params->val;
		b->valType = params->valueType;
		if(params->targetTypeRelevant)
			b->targetSourceType = params->targetType;
	}
	else
		b->type = it->second;

	resolveIdentifier(b->subtype, params->isConverted ? params->toJson() : ability, "subtype");

	if(!params->isConverted)
	{
		b->val = static_cast<si32>(ability["val"].Float());

		value = &ability["valueType"];
		if (!value->isNull())
			b->valType = static_cast<Bonus::ValueType>(parseByMapN(bonusValueMap, value, "value type "));
	}

	b->stacking = ability["stacking"].String();

	resolveAddInfo(b->additionalInfo, ability);

	b->turnsRemain = static_cast<si32>(ability["turns"].Float());

	b->sid = static_cast<si32>(ability["sourceID"].Float());

	if(!ability["description"].isNull())
	{
		if (ability["description"].isString())
			b->description = ability["description"].String();
		if (ability["description"].isNumber())
			b->description = VLC->generaltexth->translate("core.arraytxt", ability["description"].Integer());
	}

	value = &ability["effectRange"];
	if (!value->isNull())
		b->effectRange = static_cast<Bonus::LimitEffect>(parseByMapN(bonusLimitEffect, value, "effect range "));

	value = &ability["duration"];
	if (!value->isNull())
	{
		switch (value->getType())
		{
		case JsonNode::JsonType::DATA_STRING:
			b->duration = static_cast<Bonus::BonusDuration>(parseByMap(bonusDurationMap, value, "duration type "));
			break;
		case JsonNode::JsonType::DATA_VECTOR:
			{
				ui16 dur = 0;
				for (const JsonNode & d : value->Vector())
				{
					dur |= parseByMapN(bonusDurationMap, &d, "duration type ");
				}
				b->duration = static_cast<Bonus::BonusDuration>(dur);
			}
			break;
		default:
			logMod->error("Error! Wrong bonus duration format.");
		}
	}

	value = &ability["sourceType"];
	if (!value->isNull())
		b->source = static_cast<Bonus::BonusSource>(parseByMap(bonusSourceMap, value, "source type "));

	value = &ability["targetSourceType"];
	if (!value->isNull())
		b->targetSourceType = static_cast<Bonus::BonusSource>(parseByMap(bonusSourceMap, value, "target type "));

	value = &ability["limiters"];
	if (!value->isNull())
		b->limiter = parseLimiter(*value);

	value = &ability["propagator"];
	if (!value->isNull())
		b->propagator = parseByMap(bonusPropagatorMap, value, "propagator type ");

	value = &ability["updater"];
	if(!value->isNull())
		b->addUpdater(parseUpdater(*value));
	value = &ability["propagationUpdater"];
	if(!value->isNull())
		b->propagationUpdater = parseUpdater(*value);
	return true;
}

CSelector JsonUtils::parseSelector(const JsonNode & ability)
{
	CSelector ret = Selector::all;

	// Recursive parsers for anyOf, allOf, noneOf
	const auto * value = &ability["allOf"];
	if(value->isVector())
	{
		for(const auto & andN : value->Vector())
			ret = ret.And(parseSelector(andN));
	}

	value = &ability["anyOf"];
	if(value->isVector())
	{
		CSelector base = Selector::none;
		for(const auto & andN : value->Vector())
			base.Or(parseSelector(andN));
		
		ret = ret.And(base);
	}

	value = &ability["noneOf"];
	if(value->isVector())
	{
		CSelector base = Selector::all;
		for(const auto & andN : value->Vector())
			base.And(parseSelector(andN));
		
		ret = ret.And(base.Not());
	}

	// Actual selector parser
	value = &ability["type"];
	if(value->isString())
	{
		auto it = bonusNameMap.find(value->String());
		if(it != bonusNameMap.end())
			ret = ret.And(Selector::type()(it->second));
	}
	value = &ability["subtype"];
	if(!value->isNull())
	{
		TBonusSubtype subtype;
		resolveIdentifier(subtype, ability, "subtype");
		ret = ret.And(Selector::subtype()(subtype));
	}
	value = &ability["sourceType"];
	Bonus::BonusSource src = Bonus::OTHER; //Fixes for GCC false maybe-uninitialized
	si32 id = 0;
	auto sourceIDRelevant = false;
	auto sourceTypeRelevant = false;
	if(value->isString())
	{
		auto it = bonusSourceMap.find(value->String());
		if(it != bonusSourceMap.end())
		{
			src = it->second;
			sourceTypeRelevant = true;
		}

	}
	value = &ability["sourceID"];
	if(!value->isNull())
	{
		sourceIDRelevant = true;
		resolveIdentifier(id, ability, "sourceID");
	}

	if(sourceIDRelevant && sourceTypeRelevant)
		ret = ret.And(Selector::source(src, id));
	else if(sourceTypeRelevant)
		ret = ret.And(Selector::sourceTypeSel(src));

	
	value = &ability["targetSourceType"];
	if(value->isString())
	{
		auto it = bonusSourceMap.find(value->String());
		if(it != bonusSourceMap.end())
			ret = ret.And(Selector::targetSourceType()(it->second));
	}
	value = &ability["valueType"];
	if(value->isString())
	{
		auto it = bonusValueMap.find(value->String());
		if(it != bonusValueMap.end())
			ret = ret.And(Selector::valueType(it->second));
	}
	CAddInfo info;
	value = &ability["addInfo"];
	if(!value->isNull())
	{
		resolveAddInfo(info, ability["addInfo"]);
		ret = ret.And(Selector::info()(info));
	}
	value = &ability["effectRange"];
	if(value->isString())
	{
		auto it = bonusLimitEffect.find(value->String());
		if(it != bonusLimitEffect.end())
			ret = ret.And(Selector::effectRange()(it->second));
	}
	value = &ability["lastsTurns"];
	if(value->isNumber())
		ret = ret.And(Selector::turns(value->Integer()));
	value = &ability["lastsDays"];
	if(value->isNumber())
		ret = ret.And(Selector::days(value->Integer()));

	return ret;
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

		for(const auto & entry : schema["required"].Vector())
		{
			const std::string & name = entry.String();
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

void JsonUtils::minimize(JsonNode & node, const std::string & schemaName)
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
		for(const auto & entry : schema["required"].Vector())
		{
			const std::string & name = entry.String();
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

void JsonUtils::maximize(JsonNode & node, const std::string & schemaName)
{
	maximizeNode(node, getSchema(schemaName));
}

bool JsonUtils::validate(const JsonNode & node, const std::string & schemaName, const std::string & dataName)
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

const JsonNode & getSchemaByName(const std::string & name)
{
	// cached schemas to avoid loading json data multiple times
	static std::map<std::string, JsonNode> loadedSchemas;

	if (vstd::contains(loadedSchemas, name))
		return loadedSchemas[name];

	std::string filename = "config/schemas/" + name;

	if (CResourceHandler::get()->existsResource(ResourceID(filename)))
	{
		loadedSchemas[name] = JsonNode(ResourceID(filename));
		return loadedSchemas[name];
	}

	logMod->error("Error: missing schema with name %s!", name);
	assert(0);
	return nullNode;
}

const JsonNode & JsonUtils::getSchema(const std::string & URI)
{
	size_t posColon = URI.find(':');
	size_t posHash  = URI.find('#');
	std::string filename;
	if(posColon == std::string::npos)
	{
		filename = URI.substr(0, posHash);
	}
	else
	{
		std::string protocolName = URI.substr(0, posColon);
		filename = URI.substr(posColon + 1, posHash - posColon - 1) + ".json";
		if(protocolName != "vcmi")
		{
			logMod->error("Error: unsupported URI protocol for schema: %s", URI);
			return nullNode;
		}
	}

	// check if json pointer if present (section after hash in string)
	if(posHash == std::string::npos || posHash == URI.size() - 1)
		return getSchemaByName(filename);
	else
		return getSchemaByName(filename).resolvePointer(URI.substr(posHash + 1));
}

void JsonUtils::merge(JsonNode & dest, JsonNode & source, bool ignoreOverride, bool copyMeta)
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
			if(!ignoreOverride && vstd::contains(source.flags, "override"))
			{
				std::swap(dest, source);
			}
			else
			{
				if (copyMeta)
					dest.meta = source.meta;

				//recursively merge all entries from struct
				for(auto & node : source.Struct())
					merge(dest[node.first], node.second, ignoreOverride);
			}
		}
	}
}

void JsonUtils::mergeCopy(JsonNode & dest, JsonNode source, bool ignoreOverride, bool copyMeta)
{
	// uses copy created in stack to safely merge two nodes
	merge(dest, source, ignoreOverride, copyMeta);
}

void JsonUtils::inherit(JsonNode & descendant, const JsonNode & base)
{
	JsonNode inheritedNode(base);
	merge(inheritedNode, descendant, true, true);
	descendant.swap(inheritedNode);
}

JsonNode JsonUtils::intersect(const std::vector<JsonNode> & nodes, bool pruneEmpty)
{
	if(nodes.empty())
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
		for(const auto & property : a.Struct())
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
			return !diff.Struct().empty();
		default:
			return true;
		}
	};

	if(node.getType() == JsonNode::JsonType::DATA_STRUCT && base.getType() == JsonNode::JsonType::DATA_STRUCT)
	{
		// subtract individual properties
		JsonNode result(JsonNode::JsonType::DATA_STRUCT);
		for(const auto & property : node.Struct())
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

JsonNode JsonUtils::assembleFromFiles(const std::vector<std::string> & files)
{
	bool isValid = false;
	return assembleFromFiles(files, isValid);
}

JsonNode JsonUtils::assembleFromFiles(const std::vector<std::string> & files, bool & isValid)
{
	isValid = true;
	JsonNode result;

	for(const std::string & file : files)
	{
		bool isValidFile = false;
		JsonNode section(ResourceID(file, EResType::TEXT), isValidFile);
		merge(result, section);
		isValid |= isValidFile;
	}
	return result;
}

JsonNode JsonUtils::assembleFromFiles(const std::string & filename)
{
	JsonNode result;
	ResourceID resID(filename, EResType::TEXT);

	for(auto & loader : CResourceHandler::get()->getResourcesWithName(resID))
	{
		// FIXME: some way to make this code more readable
		auto stream = loader->load(resID);
		std::unique_ptr<ui8[]> textData(new ui8[stream->getSize()]);
		stream->read(textData.get(), stream->getSize());

		JsonNode section(reinterpret_cast<char *>(textData.get()), stream->getSize());
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

DLL_LINKAGE JsonNode JsonUtils::stringNode(const std::string & value)
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

VCMI_LIB_NAMESPACE_END

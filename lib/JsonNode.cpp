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

#include "bonuses/BonusParams.h"
#include "bonuses/Bonus.h"
#include "bonuses/Limiters.h"
#include "bonuses/Propagators.h"
#include "bonuses/Updaters.h"
#include "filesystem/Filesystem.h"
#include "modding/IdentifierStorage.h"
#include "VCMI_Lib.h" //for identifier resolution
#include "CGeneralTextHandler.h"
#include "JsonDetail.h"
#include "constants/StringConstants.h"
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

JsonNode::JsonNode(JsonType Type)
{
	setType(Type);
}

JsonNode::JsonNode(const char *data, size_t datasize)
{
	JsonParser parser(data, datasize);
	*this = parser.parse("<unknown>");
}

JsonNode::JsonNode(const JsonPath & fileURI)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
}

JsonNode::JsonNode(const std::string & idx, const JsonPath & fileURI)
{
	auto file = CResourceHandler::get(idx)->load(fileURI)->readAll();
	
	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
}

JsonNode::JsonNode(const JsonPath & fileURI, bool &isValidSyntax)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
	isValidSyntax = parser.isValid();
}

bool JsonNode::operator == (const JsonNode &other) const
{
	return data == other.data;
}

bool JsonNode::operator != (const JsonNode &other) const
{
	return !(*this == other);
}

JsonNode::JsonType JsonNode::getType() const
{
	return static_cast<JsonType>(data.index());
}

void JsonNode::setMeta(const std::string & metadata, bool recursive)
{
	meta = metadata;
	if (recursive)
	{
		switch (getType())
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
	if (getType() == Type)
		return;

	//float<->int conversion
	if(getType() == JsonType::DATA_FLOAT && Type == JsonType::DATA_INTEGER)
	{
		si64 converted = static_cast<si64>(std::get<double>(data));
		data = JsonData(converted);
		return;
	}
	else if(getType() == JsonType::DATA_INTEGER && Type == JsonType::DATA_FLOAT)
	{
		double converted = static_cast<double>(std::get<si64>(data));
		data = JsonData(converted);
		return;
	}

	//Set new node type
	switch(Type)
	{
		break; case JsonType::DATA_NULL:    data = JsonData();
		break; case JsonType::DATA_BOOL:    data = JsonData(false);
		break; case JsonType::DATA_FLOAT:   data = JsonData(static_cast<double>(0.0));
		break; case JsonType::DATA_STRING:  data = JsonData(std::string());
		break; case JsonType::DATA_VECTOR:  data = JsonData(JsonVector());
		break; case JsonType::DATA_STRUCT:  data = JsonData(JsonMap());
		break; case JsonType::DATA_INTEGER: data = JsonData(static_cast<si64>(0));
	}
}

bool JsonNode::isNull() const
{
	return getType() == JsonType::DATA_NULL;
}

bool JsonNode::isNumber() const
{
	return getType() == JsonType::DATA_INTEGER || getType() == JsonType::DATA_FLOAT;
}

bool JsonNode::isString() const
{
	return getType() == JsonType::DATA_STRING;
}

bool JsonNode::isVector() const
{
	return getType() == JsonType::DATA_VECTOR;
}

bool JsonNode::isStruct() const
{
	return getType() == JsonType::DATA_STRUCT;
}

bool JsonNode::containsBaseData() const
{
	switch(getType())
	{
	case JsonType::DATA_NULL:
		return false;
	case JsonType::DATA_STRUCT:
		for(const auto & elem : Struct())
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
	switch(getType())
	{
	case JsonType::DATA_VECTOR:
		for(const JsonNode & elem : Vector())
		{
			if(!elem.isCompact())
				return false;
		}
		return true;
	case JsonType::DATA_STRUCT:
		{
			auto propertyCount = Struct().size();
			if(propertyCount == 0)
				return true;
			else if(propertyCount == 1)
				return Struct().begin()->second.isCompact();
		}
		return false;
	default:
		return true;
	}
}

bool JsonNode::TryBoolFromString(bool & success) const
{
	success = true;
	if(getType() == JsonNode::JsonType::DATA_BOOL)
		return Bool();

	success = getType() == JsonNode::JsonType::DATA_STRING;
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
	return std::get<bool>(data);
}

double & JsonNode::Float()
{
	setType(JsonType::DATA_FLOAT);
	return std::get<double>(data);
}

si64 & JsonNode::Integer()
{
	setType(JsonType::DATA_INTEGER);
	return std::get<si64>(data);
}

std::string & JsonNode::String()
{
	setType(JsonType::DATA_STRING);
	return std::get<std::string>(data);
}

JsonVector & JsonNode::Vector()
{
	setType(JsonType::DATA_VECTOR);
	return std::get<JsonVector>(data);
}

JsonMap & JsonNode::Struct()
{
	setType(JsonType::DATA_STRUCT);
	return std::get<JsonMap>(data);
}

const bool boolDefault = false;
bool JsonNode::Bool() const
{
	if (getType() == JsonType::DATA_NULL)
		return boolDefault;
	assert(getType() == JsonType::DATA_BOOL);
	return std::get<bool>(data);
}

const double floatDefault = 0;
double JsonNode::Float() const
{
	if(getType() == JsonType::DATA_NULL)
		return floatDefault;

	if(getType() == JsonType::DATA_INTEGER)
		return static_cast<double>(std::get<si64>(data));

	assert(getType() == JsonType::DATA_FLOAT);
	return std::get<double>(data);
}

const si64 integetDefault = 0;
si64 JsonNode::Integer() const
{
	if(getType() == JsonType::DATA_NULL)
		return integetDefault;

	if(getType() == JsonType::DATA_FLOAT)
		return static_cast<si64>(std::get<double>(data));

	assert(getType() == JsonType::DATA_INTEGER);
	return std::get<si64>(data);
}

const std::string stringDefault = std::string();
const std::string & JsonNode::String() const
{
	if (getType() == JsonType::DATA_NULL)
		return stringDefault;
	assert(getType() == JsonType::DATA_STRING);
	return std::get<std::string>(data);
}

const JsonVector vectorDefault = JsonVector();
const JsonVector & JsonNode::Vector() const
{
	if (getType() == JsonType::DATA_NULL)
		return vectorDefault;
	assert(getType() == JsonType::DATA_VECTOR);
	return std::get<JsonVector>(data);
}

const JsonMap mapDefault = JsonMap();
const JsonMap & JsonNode::Struct() const
{
	if (getType() == JsonType::DATA_NULL)
		return mapDefault;
	assert(getType() == JsonType::DATA_STRUCT);
	return std::get<JsonMap>(data);
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

JsonNode & JsonNode::operator[](size_t child)
{
	if (child >= Vector().size() )
		Vector().resize(child + 1);
	return Vector()[child];
}

const JsonNode & JsonNode::operator[](size_t child) const
{
	if (child < Vector().size() )
		return Vector()[child];

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

static void loadBonusSubtype(BonusSubtypeID & subtype, BonusType type, const JsonNode & node)
{
	if (node.isNull())
	{
		subtype = BonusSubtypeID();
		return;
	}

	if (node.isNumber()) // Compatibility code for 1.3 or older
	{
		logMod->warn("Bonus subtype must be string!");
		subtype = BonusCustomSubtype(node.Integer());
		return;
	}

	if (!node.isString())
	{
		logMod->warn("Bonus subtype must be string!");
		subtype = BonusSubtypeID();
		return;
	}

	switch (type)
	{
		case BonusType::MAGIC_SCHOOL_SKILL:
		case BonusType::SPELL_DAMAGE:
		case BonusType::SPELLS_OF_SCHOOL:
		case BonusType::SPELL_DAMAGE_REDUCTION:
		case BonusType::SPELL_SCHOOL_IMMUNITY:
		case BonusType::NEGATIVE_EFFECTS_IMMUNITY:
		{
			VLC->identifiers()->requestIdentifier( "spellSchool", node, [&subtype](int32_t identifier)
			{
				subtype = SpellSchool(identifier);
			});
			break;
		}
		case BonusType::NO_TERRAIN_PENALTY:
		{
			VLC->identifiers()->requestIdentifier( "terrain", node, [&subtype](int32_t identifier)
			{
				subtype = TerrainId(identifier);
			});
			break;
		}
		case BonusType::PRIMARY_SKILL:
		{
			VLC->identifiers()->requestIdentifier( "primarySkill", node, [&subtype](int32_t identifier)
			{
				subtype = PrimarySkill(identifier);
			});
			break;
		}
		case BonusType::IMPROVED_NECROMANCY:
		case BonusType::HERO_GRANTS_ATTACKS:
		case BonusType::BONUS_DAMAGE_CHANCE:
		case BonusType::BONUS_DAMAGE_PERCENTAGE:
		case BonusType::SPECIAL_UPGRADE:
		case BonusType::HATE:
		case BonusType::SUMMON_GUARDIANS:
		case BonusType::MANUAL_CONTROL:
		{
			VLC->identifiers()->requestIdentifier( "creature", node, [&subtype](int32_t identifier)
			{
				subtype = CreatureID(identifier);
			});
			break;
		}
		case BonusType::SPELL_IMMUNITY:
		case BonusType::SPELL_DURATION:
		case BonusType::SPECIAL_ADD_VALUE_ENCHANT:
		case BonusType::SPECIAL_FIXED_VALUE_ENCHANT:
		case BonusType::SPECIAL_PECULIAR_ENCHANT:
		case BonusType::SPECIAL_SPELL_LEV:
		case BonusType::SPECIFIC_SPELL_DAMAGE:
		case BonusType::SPELL:
		case BonusType::OPENING_BATTLE_SPELL:
		case BonusType::SPELL_LIKE_ATTACK:
		case BonusType::CATAPULT:
		case BonusType::CATAPULT_EXTRA_SHOTS:
		case BonusType::HEALER:
		case BonusType::SPELLCASTER:
		case BonusType::ENCHANTER:
		case BonusType::SPELL_AFTER_ATTACK:
		case BonusType::SPELL_BEFORE_ATTACK:
		case BonusType::SPECIFIC_SPELL_POWER:
		case BonusType::ENCHANTED:
		case BonusType::MORE_DAMAGE_FROM_SPELL:
		case BonusType::NOT_ACTIVE:
		{
			VLC->identifiers()->requestIdentifier( "spell", node, [&subtype](int32_t identifier)
			{
				subtype = SpellID(identifier);
			});
			break;
		}
		case BonusType::GENERATE_RESOURCE:
		{
			VLC->identifiers()->requestIdentifier( "resource", node, [&subtype](int32_t identifier)
			{
				subtype = GameResID(identifier);
			});
			break;
		}
		case BonusType::MOVEMENT:
		case BonusType::WATER_WALKING:
		case BonusType::FLYING_MOVEMENT:
		case BonusType::NEGATE_ALL_NATURAL_IMMUNITIES:
		case BonusType::CREATURE_DAMAGE:
		case BonusType::FLYING:
		case BonusType::GENERAL_DAMAGE_REDUCTION:
		case BonusType::PERCENTAGE_DAMAGE_BOOST:
		case BonusType::SOUL_STEAL:
		case BonusType::TRANSMUTATION:
		case BonusType::DESTRUCTION:
		case BonusType::DEATH_STARE:
		case BonusType::REBIRTH:
		case BonusType::VISIONS:
		case BonusType::SPELLS_OF_LEVEL: // spell level
		case BonusType::CREATURE_GROWTH: // creature level
		{
			VLC->identifiers()->requestIdentifier( "bonusSubtype", node, [&subtype](int32_t identifier)
			{
				subtype = BonusCustomSubtype(identifier);
			});
			break;
		}
		default:
			for(const auto & i : bonusNameMap)
				if(i.second == type)
					logMod->warn("Bonus type %s does not supports subtypes!", i.first );
			subtype =  BonusSubtypeID();
	}
}

static void loadBonusSourceInstance(BonusSourceID & sourceInstance, BonusSource sourceType, const JsonNode & node)
{
	if (node.isNull())
	{
		sourceInstance = BonusCustomSource();
		return;
	}

	if (node.isNumber()) // Compatibility code for 1.3 or older
	{
		logMod->warn("Bonus source must be string!");
		sourceInstance = BonusCustomSource(node.Integer());
		return;
	}

	if (!node.isString())
	{
		logMod->warn("Bonus source must be string!");
		sourceInstance = BonusCustomSource();
		return;
	}

	switch (sourceType)
	{
		case BonusSource::ARTIFACT:
		case BonusSource::ARTIFACT_INSTANCE:
		{
			VLC->identifiers()->requestIdentifier( "artifact", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = ArtifactID(identifier);
			});
			break;
		}
		case BonusSource::OBJECT_TYPE:
		{
			VLC->identifiers()->requestIdentifier( "object", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = Obj(identifier);
			});
			break;
		}
		case BonusSource::OBJECT_INSTANCE:
		case BonusSource::HERO_BASE_SKILL:
			sourceInstance = ObjectInstanceID(ObjectInstanceID::decode(node.String()));
			break;
		case BonusSource::CREATURE_ABILITY:
		{
			VLC->identifiers()->requestIdentifier( "creature", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = CreatureID(identifier);
			});
			break;
		}
		case BonusSource::TERRAIN_OVERLAY:
		{
			VLC->identifiers()->requestIdentifier( "spell", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = BattleField(identifier);
			});
			break;
		}
		case BonusSource::SPELL_EFFECT:
		{
			VLC->identifiers()->requestIdentifier( "spell", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = SpellID(identifier);
			});
			break;
		}
		case BonusSource::TOWN_STRUCTURE:
			assert(0); // TODO
			sourceInstance = BuildingTypeUniqueID();
			break;
		case BonusSource::SECONDARY_SKILL:
		{
			VLC->identifiers()->requestIdentifier( "secondarySkill", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = SecondarySkill(identifier);
			});
			break;
		}
		case BonusSource::HERO_SPECIAL:
		{
			VLC->identifiers()->requestIdentifier( "hero", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = HeroTypeID(identifier);
			});
			break;
		}
		case BonusSource::CAMPAIGN_BONUS:
			sourceInstance = CampaignScenarioID(CampaignScenarioID::decode(node.String()));
			break;
		case BonusSource::ARMY:
		case BonusSource::STACK_EXPERIENCE:
		case BonusSource::COMMANDER:
		case BonusSource::GLOBAL:
		case BonusSource::TERRAIN_NATIVE:
		case BonusSource::OTHER:
		default:
			sourceInstance = BonusSourceID();
			break;
	}
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

	b->val = static_cast<si32>(ability_vec[1].Float());
	loadBonusSubtype(b->subtype, b->type, ability_vec[2]);
	b->additionalInfo = static_cast<si32>(ability_vec[3].Float());
	b->duration = BonusDuration::PERMANENT; //TODO: handle flags (as integer)
	b->turnsRemain = 0;
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
			VLC->identifiers()->requestIdentifier(value, [&](si32 identifier)
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
							VLC->identifiers()->requestIdentifier(vec[i], [&var,i](si32 identifier)
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
				VLC->identifiers()->requestIdentifier("creature", parameters[0], [=](si32 creature)
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
									loadBonusSourceInstance(bonusLimiter->sid, bonusLimiter->source, parameter["id"]);
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
							loadBonusSubtype(bonusLimiter->subtype, bonusLimiter->type, parameters[1]);
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
			else if(limiterType == "FACTION_LIMITER" || limiterType == "CREATURE_FACTION_LIMITER") //Second name is deprecated, 1.2 compat
			{
				std::shared_ptr<FactionLimiter> factionLimiter = std::make_shared<FactionLimiter>();
				VLC->identifiers()->requestIdentifier("faction", parameters[0], [=](si32 faction)
				{
					factionLimiter->faction = FactionID(faction);
				});
				return factionLimiter;
			}
			else if(limiterType == "CREATURE_LEVEL_LIMITER")
			{
				auto levelLimiter = std::make_shared<CreatureLevelLimiter>();
				if(!parameters.empty()) //If parameters is empty, level limiter works as CREATURES_ONLY limiter
				{
					levelLimiter->minLevel = parameters[0].Integer();
					if(parameters[1].isNumber())
						levelLimiter->maxLevel = parameters[1].Integer();
				}
				return levelLimiter;
			}
			else if(limiterType == "CREATURE_TERRAIN_LIMITER")
			{
				std::shared_ptr<CreatureTerrainLimiter> terrainLimiter = std::make_shared<CreatureTerrainLimiter>();
				if(!parameters.empty())
				{
					VLC->identifiers()->requestIdentifier("terrain", parameters[0], [=](si32 terrain)
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
		// caller code can not handle this case and presumes that returned bonus is always valid
		logGlobal->error("Failed to parse bonus! Json config was %S ", ability.toJson());
		b->type = BonusType::NONE;
		return b;
	}
	return b;
}

std::shared_ptr<Bonus> JsonUtils::parseBuildingBonus(const JsonNode & ability, const FactionID & faction, const BuildingID & building, const std::string & description)
{
	/*	duration = BonusDuration::PERMANENT
		source = BonusSource::TOWN_STRUCTURE
		bonusType, val, subtype - get from json
	*/
	auto b = std::make_shared<Bonus>(BonusDuration::PERMANENT, BonusType::NONE, BonusSource::TOWN_STRUCTURE, 0, BuildingTypeUniqueID(faction, building), description);

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
			if(ability["type"].String() == "SECONDARY_SKILL_PREMY" && bonusValueMap.find(ability["valueType"].String())->second == BonusValueType::PERCENT_TO_BASE) //assume secondary skill special
			{
				params.valueType = BonusValueType::PERCENT_TO_TARGET_TYPE;
				params.targetType = BonusSource::SECONDARY_SKILL;
			}

			logMod->warn("Please, use this bonus:\n%s\nConverted successfully!", params.toJson().toJson());
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
		b->val = params->val.value_or(0);
		b->valType = params->valueType.value_or(BonusValueType::ADDITIVE_VALUE);
		if(params->targetType)
			b->targetSourceType = params->targetType.value();
	}
	else
		b->type = it->second;

	loadBonusSubtype(b->subtype, b->type, params->isConverted ? params->toJson()["subtype"] : ability["subtype"]);

	if(!params->isConverted)
	{
		b->val = static_cast<si32>(ability["val"].Float());

		value = &ability["valueType"];
		if (!value->isNull())
			b->valType = static_cast<BonusValueType>(parseByMapN(bonusValueMap, value, "value type "));
	}

	b->stacking = ability["stacking"].String();

	resolveAddInfo(b->additionalInfo, ability);

	b->turnsRemain = static_cast<si32>(ability["turns"].Float());

	if(!ability["description"].isNull())
	{
		if (ability["description"].isString())
			b->description = ability["description"].String();
		if (ability["description"].isNumber())
			b->description = VLC->generaltexth->translate("core.arraytxt", ability["description"].Integer());
	}

	value = &ability["effectRange"];
	if (!value->isNull())
		b->effectRange = static_cast<BonusLimitEffect>(parseByMapN(bonusLimitEffect, value, "effect range "));

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
				BonusDuration::Type dur = 0;
				for (const JsonNode & d : value->Vector())
					dur |= parseByMapN(bonusDurationMap, &d, "duration type ");
				b->duration = dur;
			}
			break;
		default:
			logMod->error("Error! Wrong bonus duration format.");
		}
	}

	value = &ability["sourceType"];
	if (!value->isNull())
		b->source = static_cast<BonusSource>(parseByMap(bonusSourceMap, value, "source type "));

	if (!ability["sourceID"].isNull())
		loadBonusSourceInstance(b->sid, b->source, ability["sourceID"]);

	value = &ability["targetSourceType"];
	if (!value->isNull())
		b->targetSourceType = static_cast<BonusSource>(parseByMap(bonusSourceMap, value, "target type "));

	value = &ability["limiters"];
	if (!value->isNull())
		b->limiter = parseLimiter(*value);

	value = &ability["propagator"];
	if (!value->isNull())
	{
		//ALL_CREATURES old propagator compatibility
		if(value->String() == "ALL_CREATURES") 
		{
			logMod->warn("ALL_CREATURES propagator is deprecated. Use GLOBAL_EFFECT propagator with CREATURES_ONLY limiter");
			b->addLimiter(std::make_shared<CreatureLevelLimiter>());
			b->propagator = bonusPropagatorMap.at("GLOBAL_EFFECT");
		}
		else
			b->propagator = parseByMap(bonusPropagatorMap, value, "propagator type ");
	}

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
			base = base.Or(parseSelector(andN));
		
		ret = ret.And(base);
	}

	value = &ability["noneOf"];
	if(value->isVector())
	{
		CSelector base = Selector::none;
		for(const auto & andN : value->Vector())
			base = base.Or(parseSelector(andN));
		
		ret = ret.And(base.Not());
	}

	BonusType type = BonusType::NONE;

	// Actual selector parser
	value = &ability["type"];
	if(value->isString())
	{
		auto it = bonusNameMap.find(value->String());
		if(it != bonusNameMap.end())
		{
			type = it->second;
			ret = ret.And(Selector::type()(it->second));
		}
	}
	value = &ability["subtype"];
	if(!value->isNull() && type != BonusType::NONE)
	{
		BonusSubtypeID subtype;
		loadBonusSubtype(subtype, type, ability);
		ret = ret.And(Selector::subtype()(subtype));
	}
	value = &ability["sourceType"];
	std::optional<BonusSource> src = std::nullopt; //Fixes for GCC false maybe-uninitialized
	std::optional<BonusSourceID> id = std::nullopt;
	if(value->isString())
	{
		auto it = bonusSourceMap.find(value->String());
		if(it != bonusSourceMap.end())
			src = it->second;
	}

	value = &ability["sourceID"];
	if(!value->isNull() && src.has_value())
	{
		loadBonusSourceInstance(*id, *src, ability);
	}

	if(src && id)
		ret = ret.And(Selector::source(*src, *id));
	else if(src)
		ret = ret.And(Selector::sourceTypeSel(*src));

	
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

static JsonNode getDefaultValue(const JsonNode & schema, std::string fieldName)
{
	const JsonNode & fieldProps = schema["properties"][fieldName];

#if defined(VCMI_IOS)
	if (!fieldProps["defaultIOS"].isNull())
		return fieldProps["defaultIOS"];
#elif defined(VCMI_ANDROID)
	if (!fieldProps["defaultAndroid"].isNull())
		return fieldProps["defaultAndroid"];
#elif !defined(VCMI_MOBILE)
	if (!fieldProps["defaultDesktop"].isNull())
		return fieldProps["defaultDesktop"];
#endif
	return fieldProps["default"];
}

static void eraseOptionalNodes(JsonNode & node, const JsonNode & schema)
{
	assert(schema["type"].String() == "object");

	std::set<std::string> foundEntries;

	for(const auto & entry : schema["required"].Vector())
		foundEntries.insert(entry.String());

	vstd::erase_if(node.Struct(), [&](const auto & node){
		return !vstd::contains(foundEntries, node.first);
	});
}

static void minimizeNode(JsonNode & node, const JsonNode & schema)
{
	if (schema["type"].String() != "object")
		return;

	for(const auto & entry : schema["required"].Vector())
	{
		const std::string & name = entry.String();
		minimizeNode(node[name], schema["properties"][name]);

		if (vstd::contains(node.Struct(), name) && node[name] == getDefaultValue(schema, name))
			node.Struct().erase(name);
	}
	eraseOptionalNodes(node, schema);
}

static void maximizeNode(JsonNode & node, const JsonNode & schema)
{
	// "required" entry can only be found in object/struct
	if (schema["type"].String() != "object")
		return;

	// check all required entries that have default version
	for(const auto & entry : schema["required"].Vector())
	{
		const std::string & name = entry.String();

		if (node[name].isNull() && !getDefaultValue(schema, name).isNull())
			node[name] = getDefaultValue(schema, name);

		maximizeNode(node[name], schema["properties"][name]);
	}

	eraseOptionalNodes(node, schema);
}

void JsonUtils::minimize(JsonNode & node, const std::string & schemaName)
{
	minimizeNode(node, getSchema(schemaName));
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

	auto filename = JsonPath::builtin("config/schemas/" + name);

	if (CResourceHandler::get()->existsResource(filename))
	{
		loadedSchemas[name] = JsonNode(filename);
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
	{
		auto const & result = getSchemaByName(filename);
		if (result.isNull())
			logMod->error("Error: missing schema %s", URI);
		return result;
	}
	else
	{
		auto const & result = getSchemaByName(filename).resolvePointer(URI.substr(posHash + 1));
		if (result.isNull())
			logMod->error("Error: missing schema %s", URI);
		return result;
	}
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
	std::swap(descendant, inheritedNode);
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

	for(const auto & file : files)
	{
		bool isValidFile = false;
		JsonNode section(JsonPath::builtinTODO(file), isValidFile);
		merge(result, section);
		isValid |= isValidFile;
	}
	return result;
}

JsonNode JsonUtils::assembleFromFiles(const std::string & filename)
{
	JsonNode result;
	JsonPath resID = JsonPath::builtinTODO(filename);

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

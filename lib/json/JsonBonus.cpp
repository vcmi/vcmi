/*
 * JsonUtils.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonBonus.h"

#include "JsonValidator.h"

#include "../texts/CGeneralTextHandler.h"
#include "../GameLibrary.h"
#include "../bonuses/BonusParams.h"
#include "../bonuses/Limiters.h"
#include "../bonuses/Propagators.h"
#include "../bonuses/Updaters.h"
#include "../constants/StringConstants.h"
#include "../modding/IdentifierStorage.h"

VCMI_LIB_USING_NAMESPACE

template <typename T>
const T parseByMap(const std::map<std::string, T> & map, const JsonNode * val, const std::string & err)
{
	if (!val->isNull())
	{
		const std::string & type = val->String();
		auto it = map.find(type);
		if (it == map.end())
		{
			logMod->error("Error: invalid %s%s.", err, type);
			return {};
		}
		else
		{
			return it->second;
		}
	}
	else
		return {};
}

template <typename T>
const T parseByMapN(const std::map<std::string, T> & map, const JsonNode * val, const std::string & err)
{
	if(val->isNumber())
		return static_cast<T>(val->Integer());
	else
		return parseByMap<T>(map, val, err);
}

static void loadBonusSubtype(BonusSubtypeID & subtype, BonusType type, const JsonNode & node)
{
	if (node.isNull())
	{
		subtype = BonusSubtypeID();
		return;
	}

	if (node.isNumber()) // Compatibility code for 1.3 or older
	{
		logMod->warn("Bonus subtype must be string! (%s)", node.getModScope());
		subtype = BonusCustomSubtype(node.Integer());
		return;
	}

	if (!node.isString())
	{
		logMod->warn("Bonus subtype must be string! (%s)", node.getModScope());
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
			LIBRARY->identifiers()->requestIdentifier( "spellSchool", node, [&subtype](int32_t identifier)
			{
				subtype = SpellSchool(identifier);
			});
			break;
		}
		case BonusType::NO_TERRAIN_PENALTY:
		{
			LIBRARY->identifiers()->requestIdentifier( "terrain", node, [&subtype](int32_t identifier)
			{
				subtype = TerrainId(identifier);
			});
			break;
		}
		case BonusType::PRIMARY_SKILL:
		{
			LIBRARY->identifiers()->requestIdentifier( "primarySkill", node, [&subtype](int32_t identifier)
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
			LIBRARY->identifiers()->requestIdentifier( "creature", node, [&subtype](int32_t identifier)
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
			LIBRARY->identifiers()->requestIdentifier( "spell", node, [&subtype](int32_t identifier)
			{
				subtype = SpellID(identifier);
			});
			break;
		}
		case BonusType::GENERATE_RESOURCE:
		case BonusType::RESOURCES_CONSTANT_BOOST:
		case BonusType::RESOURCES_TOWN_MULTIPLYING_BOOST:
		{
			LIBRARY->identifiers()->requestIdentifier( "resource", node, [&subtype](int32_t identifier)
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
		case BonusType::FIRST_STRIKE:
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
			LIBRARY->identifiers()->requestIdentifier( "bonusSubtype", node, [&subtype](int32_t identifier)
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
			LIBRARY->identifiers()->requestIdentifier( "artifact", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = ArtifactID(identifier);
			});
			break;
		}
		case BonusSource::OBJECT_TYPE:
		{
			LIBRARY->identifiers()->requestIdentifier( "object", node, [&sourceInstance](int32_t identifier)
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
			LIBRARY->identifiers()->requestIdentifier( "creature", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = CreatureID(identifier);
			});
			break;
		}
		case BonusSource::TERRAIN_OVERLAY:
		{
			LIBRARY->identifiers()->requestIdentifier( "spell", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = BattleField(identifier);
			});
			break;
		}
		case BonusSource::SPELL_EFFECT:
		{
			LIBRARY->identifiers()->requestIdentifier( "spell", node, [&sourceInstance](int32_t identifier)
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
			LIBRARY->identifiers()->requestIdentifier( "secondarySkill", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = SecondarySkill(identifier);
			});
			break;
		}
		case BonusSource::HERO_SPECIAL:
		{
			LIBRARY->identifiers()->requestIdentifier( "hero", node, [&sourceInstance](int32_t identifier)
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

static BonusParams convertDeprecatedBonus(const JsonNode &ability)
{
	if(vstd::contains(deprecatedBonusSet, ability["type"].String()))
	{
		logMod->warn("There is deprecated bonus found:\n%s\nTrying to convert...", ability.toString());
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

			logMod->warn("Please, use this bonus:\n%s\nConverted successfully!", params.toJson().toString());
			return params;
		}
		else
			logMod->error("Cannot convert bonus!\n%s", ability.toString());
	}
	BonusParams ret;
	ret.isConverted = false;
	return ret;
}

static TUpdaterPtr parseUpdater(const JsonNode & updaterJson)
{
	const std::map<std::string, std::function<TUpdaterPtr()>> bonusUpdaterMap =
	{
			{"TIMES_HERO_LEVEL", std::make_shared<TimesHeroLevelUpdater>},
			{"TIMES_HERO_LEVEL_DIVIDE_STACK_LEVEL", std::make_shared<TimesHeroLevelDivideStackLevelUpdater>},
			{"DIVIDE_STACK_LEVEL", std::make_shared<DivideStackLevelUpdater>},
			{"TIMES_STACK_LEVEL", std::make_shared<TimesStackLevelUpdater>},
			{"BONUS_OWNER_UPDATER", std::make_shared<OwnerUpdater>}
	};

	switch(updaterJson.getType())
	{
	case JsonNode::JsonType::DATA_STRING:
		{
			auto it = bonusUpdaterMap.find(updaterJson.String());
			if (it != bonusUpdaterMap.end())
				return it->second();

			logGlobal->error("Unknown bonus updater type '%s'", updaterJson.String());
			return nullptr;
		}
	case JsonNode::JsonType::DATA_STRUCT:
		if(updaterJson["type"].String() == "GROWS_WITH_LEVEL")
		{
			auto updater = std::make_shared<GrowsWithLevelUpdater>();
			const JsonVector param = updaterJson["parameters"].Vector();
			updater->valPer20 = static_cast<int>(param[0].Integer());
			if(param.size() > 1)
				updater->stepSize = static_cast<int>(param[1].Integer());
			return updater;
		}
		else
			logMod->warn("Unknown updater type \"%s\"", updaterJson["type"].String());
		break;
	}
	return nullptr;
}

VCMI_LIB_NAMESPACE_BEGIN

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
			LIBRARY->identifiers()->requestIdentifier(value, [&](si32 identifier)
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
							LIBRARY->identifiers()->requestIdentifier(vec[i], [&var,i](si32 identifier)
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
				auto creatureLimiter = std::make_shared<CCreatureTypeLimiter>();
				LIBRARY->identifiers()->requestIdentifier("creature", parameters[0], [=](si32 creature)
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
				auto bonusLimiter = std::make_shared<HasAnotherBonusLimiter>();

				if (!parameters[0].isNull())
				{
					std::string anotherBonusType = parameters[0].String();
					auto it = bonusNameMap.find(anotherBonusType);
					if(it != bonusNameMap.end())
					{
						bonusLimiter->type = it->second;
					}
					else
					{
						logMod->error("Error: invalid ability type %s.", anotherBonusType);
					}
				}

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
				auto factionLimiter = std::make_shared<FactionLimiter>();
				LIBRARY->identifiers()->requestIdentifier("faction", parameters[0], [=](si32 faction)
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
				auto terrainLimiter = std::make_shared<CreatureTerrainLimiter>();
				if(!parameters.empty())
				{
					LIBRARY->identifiers()->requestIdentifier("terrain", parameters[0], [terrainLimiter](si32 terrain)
					{
						terrainLimiter->terrainType = terrain;
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
		logGlobal->error("Failed to parse bonus! Json config was %S ", ability.toString());
		b->type = BonusType::NONE;
		return b;
	}
	return b;
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
			b->description.appendTextID(ability["description"].String());
		if (ability["description"].isNumber())
			b->description.appendTextID("core.arraytxt." + std::to_string(ability["description"].Integer()));
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

VCMI_LIB_NAMESPACE_END

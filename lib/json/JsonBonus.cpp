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
#include "../bonuses/Limiters.h"
#include "../bonuses/Propagators.h"
#include "../bonuses/Updaters.h"
#include "../CBonusTypeHandler.h"
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
		case BonusType::SKELETON_TRANSFORMER_TARGET:
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
		{
			logMod->warn("Bonus type %s does not supports subtypes!", LIBRARY->bth->bonusToString(type));
			subtype =  BonusSubtypeID();
		}
	}
}

static void loadBonusAddInfo(CAddInfo & var, BonusType type, const JsonNode & value)
{
	const auto & getFirstValue = [](const JsonNode & jsonNode) -> const JsonNode &
	{
		if (jsonNode.isVector())
			return jsonNode[0];
		else
			return jsonNode;
	};

	if (value.isNull())
		return;

	switch (type)
	{
		case BonusType::IMPROVED_NECROMANCY:
		case BonusType::SPECIAL_ADD_VALUE_ENCHANT:
		case BonusType::SPECIAL_FIXED_VALUE_ENCHANT:
		case BonusType::DESTRUCTION:
		case BonusType::LIMITED_SHOOTING_RANGE:
		case BonusType::ACID_BREATH:
		case BonusType::BIND_EFFECT:
		case BonusType::SPELLCASTER:
		case BonusType::FEROCITY:
		case BonusType::PRIMARY_SKILL:
		case BonusType::ENCHANTER:
		case BonusType::SPECIAL_PECULIAR_ENCHANT:
		case BonusType::SPELL_IMMUNITY:
		case BonusType::DARKNESS:
		case BonusType::FULL_MAP_SCOUTING:
		case BonusType::FULL_MAP_DARKNESS:
			// 1 number
			var = getFirstValue(value).Integer();
			break;
		case BonusType::SPECIAL_UPGRADE:
		case BonusType::TRANSMUTATION:
			// 1 creature ID
			LIBRARY->identifiers()->requestIdentifier("creature", getFirstValue(value), [&](si32 identifier) { var = identifier; });
			break;
		case BonusType::DEATH_STARE:
			// 1 spell ID
			LIBRARY->identifiers()->requestIdentifier("spell", getFirstValue(value), [&](si32 identifier) { var = identifier; });
			break;
		case BonusType::SPELL_BEFORE_ATTACK:
		case BonusType::SPELL_AFTER_ATTACK:
			// 3 numbers
			var.resize(3);
			var[0] = value[0].Integer();
			var[1] = value[1].Integer();
			var[2] = value[2].Integer();
			break;
		case BonusType::MULTIHEX_UNIT_ATTACK:
		case BonusType::MULTIHEX_ENEMY_ATTACK:
		case BonusType::MULTIHEX_ANIMATION:
			for (const auto & sequence : value.Vector())
			{
				static const std::map<char, int> charToDirection = {
					{ 'f', 1 }, { 'l', 6}, {'r', 2}, {'b', 4}
				};
				int converted = 0;
				for (const auto & ch : boost::adaptors::reverse(sequence.String()))
				{
					char chLower = std::tolower(ch);
					if (charToDirection.count(chLower))
						converted = 10 * converted + charToDirection.at(chLower);
				}
				var.push_back(converted);
			}
			break;
		default:
			logMod->warn("Bonus type %s does not supports addInfo!", LIBRARY->bth->bonusToString(type) );
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

static TUpdaterPtr parseUpdater(const JsonNode & updaterJson)
{
	const std::map<std::string, std::shared_ptr<IUpdater>> bonusUpdaterMap =
	{
			{"TIMES_HERO_LEVEL", std::make_shared<TimesHeroLevelUpdater>()},
			{"TIMES_HERO_LEVEL_DIVIDE_STACK_LEVEL", std::make_shared<TimesHeroLevelDivideStackLevelUpdater>()},
			{"DIVIDE_STACK_LEVEL", std::make_shared<DivideStackLevelUpdater>()},
			{"TIMES_STACK_LEVEL", std::make_shared<TimesStackLevelUpdater>()},
			{"BONUS_OWNER_UPDATER", std::make_shared<OwnerUpdater>()}
	};

	switch(updaterJson.getType())
	{
	case JsonNode::JsonType::DATA_STRING:
		{
			auto it = bonusUpdaterMap.find(updaterJson.String());
			if (it != bonusUpdaterMap.end())
				return it->second;

			logGlobal->error("Unknown bonus updater type '%s'", updaterJson.String());
			return nullptr;
		}
	case JsonNode::JsonType::DATA_STRUCT:
		if(updaterJson["type"].String() == "GROWS_WITH_LEVEL")
		{
			// MOD COMPATIBILITY - parameters is deprecated in 1.7
			const JsonVector param = updaterJson["parameters"].Vector();
			int valPer20 = updaterJson["valPer20"].isNull() ? param[0].Integer() : updaterJson["valPer20"].Integer();
			int stepSize = updaterJson["stepSize"].isNull() ? param[1].Integer() : updaterJson["stepSize"].Integer();

			return std::make_shared<GrowsWithLevelUpdater>(valPer20, std::max(1, stepSize));
		}
		if(updaterJson["type"].String() == "TIMES_HERO_LEVEL")
		{
			int stepSize = updaterJson["stepSize"].Integer();
			return std::make_shared<TimesHeroLevelUpdater>(std::max(1, stepSize));
		}
		if(updaterJson["type"].String() == "TIMES_STACK_SIZE")
		{
			int minimum = updaterJson["minimum"].isNull() ? std::numeric_limits<int>::min() : updaterJson["minimum"].Integer();
			int maximum = updaterJson["maximum"].isNull() ? std::numeric_limits<int>::max() : updaterJson["maximum"].Integer();
			int stepSize = updaterJson["stepSize"].Integer();
			if (minimum > maximum)
			{
				logMod->warn("TIMES_STACK_SIZE updater: minimum value (%d) is above maximum value(%d)!", minimum, maximum);
				return std::make_shared<TimesStackSizeUpdater>(maximum, minimum, std::max(1, stepSize));
			}
			return std::make_shared<TimesStackSizeUpdater>(minimum, maximum, std::max(1, stepSize));
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

	const JsonNode & typeNode = ability_vec[0];
	const JsonNode & subtypeNode = ability_vec[2];

	LIBRARY->identifiers()->requestIdentifier("bonus", typeNode, [b, subtypeNode](si32 bonusID)
	{
		b->type = static_cast<BonusType>(bonusID);
		loadBonusSubtype(b->subtype, b->type, subtypeNode);
	});
	b->val = static_cast<si32>(ability_vec[1].Float());
	b->additionalInfo = static_cast<si32>(ability_vec[3].Float());
	b->duration = BonusDuration::PERMANENT; //TODO: handle flags (as integer)
	b->turnsRemain = 0;
	return b;
}

std::shared_ptr<const ILimiter> JsonUtils::parseLimiter(const JsonNode & limiter)
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
					LIBRARY->identifiers()->requestIdentifier("bonus", parameters[0], [bonusLimiter](si32 bonusID)
					{
						bonusLimiter->type = static_cast<BonusType>(bonusID);
					});
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

std::shared_ptr<Bonus> JsonUtils::parseBonus(const JsonNode &ability, const TextIdentifier & descriptionID)
{
	auto b = std::make_shared<Bonus>();
	if (!parseBonus(ability, b.get(), descriptionID))
	{
		// caller code can not handle this case and presumes that returned bonus is always valid
		logGlobal->error("Failed to parse bonus! Json config was %S ", ability.toString());
		b->type = BonusType::NONE;
		return b;
	}
	return b;
}

bool JsonUtils::parseBonus(const JsonNode &ability, Bonus *b, const TextIdentifier & descriptionID)
{
	const JsonNode * value = nullptr;
	const JsonNode & subtypeNode = ability["subtype"];
	const JsonNode & addinfoNode = ability["addInfo"];

	LIBRARY->identifiers()->requestIdentifier("bonus", ability["type"], [b, subtypeNode, addinfoNode](si32 bonusID)
	{
		b->type = static_cast<BonusType>(bonusID);
		loadBonusSubtype(b->subtype, b->type, subtypeNode);
		loadBonusAddInfo(b->additionalInfo, b->type, addinfoNode);
	});

	b->val = static_cast<si32>(ability["val"].Float());

	value = &ability["valueType"];
	if (!value->isNull())
		b->valType = static_cast<BonusValueType>(parseByMapN(bonusValueMap, value, "value type "));

	b->stacking = ability["stacking"].String();
	b->turnsRemain = static_cast<si32>(ability["turns"].Float());

	if(!ability["description"].isNull())
	{
		if (ability["description"].isString() && !ability["description"].String().empty())
		{
			if (ability["description"].String()[0] == '@')
				b->description.appendTextID(ability["description"].String().substr(1));
			else if (!descriptionID.get().empty())
			{
				LIBRARY->generaltexth->registerString(ability.getModScope(), descriptionID, ability["description"]);
				b->description.appendTextID(descriptionID.get());
			}
		}
		if (ability["description"].isNumber())
			b->description.appendTextID("core.arraytxt." + std::to_string(ability["description"].Integer()));
	}

	if(!ability["icon"].isNull())
		b->customIconPath = ImagePath::fromJson(ability["icon"]);

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
		ret = ret.And(Selector::type()(static_cast<BonusType>(*LIBRARY->identifiers()->getIdentifier("bonus", value->String()))));
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
		loadBonusAddInfo(info, type, ability["addInfo"]);
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

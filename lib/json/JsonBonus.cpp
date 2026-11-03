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
#include "../bonuses/BonusMigration.h"
#include "../bonuses/BonusParameters.h"
#include "../constants/StringConstants.h"
#include "../modding/IdentifierResolver.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/LibraryLoadContext.h"
#include "../scripting/ScriptService.h"


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

static void loadBonusSubtype(BonusSubtypeID & subtype, BonusType type, const std::string & typeName, const JsonNode & node, IIdentifierResolver & resolver)
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

	const auto entityType = bonusSubtypeEntityType(type);

	if(entityType == EntityTypeEnum::NONE)
	{
		logMod->warn("Bonus type %s does not support subtypes! (%s)", typeName, node.getModScope());
		subtype = BonusSubtypeID();
		return;
	}

	// deferred, unlike the runtime decodeBonusSubtype: while content loads the identifier this
	// names may belong to a mod that has not been read yet
	resolver.requestIdentifier(entityTypeName(entityType), node, [&subtype, entityType](int32_t identifier)
	{
		subtype = bonusSubtypeOf(entityType, identifier);
	});
}

static std::shared_ptr<Bonus> parseBonusOrNone(const JsonNode & ability, IIdentifierResolver & resolver, LibraryLoadContext * context, const TextIdentifier & descriptionID);

static TBonusParametersPtr loadBonusAddInfo(BonusType type, const std::string & typeName, const JsonNode & value, IIdentifierResolver & resolver, LibraryLoadContext * context)
{
	const auto & getFirstValue = [](const JsonNode & jsonNode) -> const JsonNode &
	{
		if (jsonNode.isVector())
			return jsonNode[0];
		else
			return jsonNode;
	};

	if (value.isNull())
		return nullptr;

	auto result = std::make_shared<BonusParameters>();
	BonusParameters & var = *result;

	switch (type)
	{
		case BonusType::IMPROVED_NECROMANCY:
		case BonusType::SPECIAL_ADD_VALUE_ENCHANT:
		case BonusType::SPECIAL_FIXED_VALUE_ENCHANT:
		case BonusType::LIMITED_SHOOTING_RANGE:
		case BonusType::BIND_EFFECT:
		case BonusType::SPELLCASTER:
		case BonusType::FEROCITY:
		case BonusType::PRIMARY_SKILL:
		case BonusType::SLAYER:
		case BonusType::SPELL_IMMUNITY:
		case BonusType::DARKNESS:
		case BonusType::FULL_MAP_SCOUTING:
		case BonusType::FULL_MAP_DARKNESS:
		case BonusType::OPENING_BATTLE_SPELL:
			// 1 number
			var = static_cast<int32_t>(getFirstValue(value).Integer());
			break;
		case BonusType::ENCHANTER:
		{
			BonusParametersEnchanter loadedData;
			if (value.isStruct())
			{
				loadedData.cooldown = static_cast<int32_t>(value["cooldown"].Integer());
				if (!value["weight"].isNull())
					loadedData.weight = std::max<int32_t>(0, static_cast<int32_t>(value["weight"].Integer()));
			}
			else
				loadedData.cooldown = static_cast<int32_t>(getFirstValue(value).Integer());
			var = loadedData;
			break;
		}
		case BonusType::SPECIAL_UPGRADE:
			// 1 creature ID
			resolver.requestIdentifier("creature", getFirstValue(value), [&](si32 identifier) { var = CreatureID(identifier); });
			break;
		case BonusType::SPELL_BEFORE_ATTACK:
		case BonusType::SPELL_AFTER_ATTACK:
			// 3 numbers
			if (value.isNumber())
			{
				std::vector<int32_t> loadedData{
					static_cast<int32_t>(getFirstValue(value).Integer()),
					-1,
					-1
				};
				var = loadedData;
			}
			else
			{
				std::vector<int32_t> loadedData{
					static_cast<int32_t>(value[0].Integer()),
					static_cast<int32_t>(value[1].Integer()),
					static_cast<int32_t>(value[2].Integer())
				};
				var = loadedData;
			}
			break;
		case BonusType::MULTIHEX_UNIT_ATTACK:
		case BonusType::MULTIHEX_ENEMY_ATTACK:
		case BonusType::MULTIHEX_ANIMATION:
		{
			std::vector<int32_t> loadedData;
			for (const auto & sequence : value.Vector())
			{
				static const std::map<char, int> charToDirection = {
					{ 'f', 1 }, { 'l', 6}, {'r', 2}, {'b', 4}
				};
				int converted = 0;
				for (const auto & ch : std::views::reverse(sequence.String()))
				{
					char chLower = std::tolower(ch);
					if (charToDirection.count(chLower))
						converted = 10 * converted + charToDirection.at(chLower);
				}
				loadedData.push_back(converted);
			}
			var = loadedData;
			break;
		}
		case BonusType::SPECIAL_PECULIAR_ENCHANT:
		case BonusType::FORCE_NEUTRAL_ENCOUNTER_STACK_COUNT:
		{
			std::vector<int32_t> loadedData;
			if (value.isVector()) {
				for(const auto & sequence : value.Vector())
					loadedData.push_back(sequence.Integer());
			}
			var = loadedData;
			break;
		}
		case BonusType::ON_COMBAT_EVENT:
		{
			var = BonusParameters(BonusParametersOnCombatEvent());
			auto & loadedData = result->toCustom<BonusParametersOnCombatEvent>();

			for (const auto & effect : value["effect"].Vector())
			{
				int index = loadedData.effects.size();
				bool targetEnemy = effect["targetEnemy"].Bool();
				if (effect["action"].String() == "bonus")
				{
					const auto bonus = parseBonusOrNone(effect["bonus"], resolver, context, "");
					loadedData.effects.push_back(BonusParametersOnCombatEvent::CombatEffectBonus{
						bonus, targetEnemy
					});
				}
				if (effect["action"].String() == "spell")
				{
					int mastery = effect["mastery"].Integer();
					loadedData.effects.push_back(BonusParametersOnCombatEvent::CombatEffectSpell{
						SpellID(), mastery, targetEnemy
					});
					resolver.requestIdentifier( "spell", effect["spell"], [&loadedData, index](int32_t identifier)
					{
						auto * targetEffect = std::get_if<BonusParametersOnCombatEvent::CombatEffectSpell>(&loadedData.effects[index]);
						targetEffect->spell = SpellID(identifier);
					});
				}
			}
			break;
		}
		case BonusType::SPELL:
			// spell can be cast without spellbook
			var = BonusParameters(value.Bool());
			break;
		case BonusType::COMBAT_EVENT_TRIGGER:
		{
			// the whole addInfo is the script payload - which script runs is the bonus subtype
			var = BonusParameters(value);
			break;
		}
		default:
			logMod->warn("Bonus type %s does not support addInfo! (%s)", typeName, value.getModScope());
	}

	return result;
}

/// A combat script validates the parameters the bonus hands it and registers any translatable text
/// among them. Both need the script itself, which only resolves once every mod has been loaded -
/// scripts are a content type of their own and may well load after whoever refers to them.
/// `context` is null when the bonus is not part of library content
static void prepareCombatScriptParameters(Bonus * b, const JsonNode & scriptNode, IIdentifierResolver & resolver, LibraryLoadContext * context, const TextIdentifier & descriptionID)
{
	resolver.requestIdentifier("script", scriptNode, [b, descriptionID, &resolver, context](int32_t identifier)
	{
		ScriptID scriptID(identifier);

		const ScriptService & scripts = context ? *context->lib.scriptTypes() : *LIBRARY->scriptTypes();
		const ScriptTypeDescription & script = scripts.getById(scriptID);

		if (script.kind != ScriptKind::COMBAT_EVENT)
		{
			// the bonus stays, but no combat event script will ever be found for it, so it does nothing
			logMod->error("Bonus '%s' runs script '%s', which is not a combat event script!", descriptionID.get(), script.scriptId);
			return;
		}

		// a bonus may pass no parameters at all, while the script requires some. The schema only
		// applies to an object, so an absent payload is validated as an empty one rather than skipped
		JsonNode parameters;
		if (b->parameters)
			parameters = b->parameters->toCustom<JsonNode>();
		else
			parameters.setType(JsonNode::JsonType::DATA_STRUCT);

		if (context)
			scripts.prepareParameters(scriptID, parameters, *context, descriptionID);
		else
			scripts.prepareParameters(scriptID, parameters, resolver);

		// parameters are immutable once stored, so the prepared payload replaces them wholesale
		b->parameters = std::make_shared<BonusParameters>(parameters);
	});
}


static void loadBonusSourceInstance(BonusSourceID & sourceInstance, BonusSource sourceType, const JsonNode & node, const JsonNode & secondaryIdNode, IIdentifierResolver & resolver)
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
			resolver.requestIdentifier( "artifact", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = ArtifactID(identifier);
			});
			break;
		}
		case BonusSource::OBJECT_TYPE:
		{
			resolver.requestIdentifier( "object", node, [&sourceInstance](int32_t identifier)
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
			resolver.requestIdentifier( "creature", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = CreatureID(identifier);
			});
			break;
		}
		case BonusSource::TERRAIN_OVERLAY:
		{
			resolver.requestIdentifier( "battlefield", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = BattleField(identifier);
			});
			break;
		}
		case BonusSource::SPELL_EFFECT:
		{
			int8_t mastery = MasteryLevel::ANY;
			if (secondaryIdNode.isString())
			{
				mastery = MasteryLevel::decode(secondaryIdNode.String());
			}
			else if (secondaryIdNode.isNumber())
			{
				mastery = secondaryIdNode.Integer();
			}
			resolver.requestIdentifier( "spell", node, [&sourceInstance, mastery](int32_t identifier)
			{
				sourceInstance = SpellWithMasteryID(static_cast<MasteryLevel::Type>(mastery), identifier);
			});
			break;
		}
		case BonusSource::TOWN_STRUCTURE:
			assert(0); // TODO
			sourceInstance = BuildingTypeUniqueID();
			break;
		case BonusSource::SECONDARY_SKILL:
		{
			resolver.requestIdentifier( "secondarySkill", node, [&sourceInstance](int32_t identifier)
			{
				sourceInstance = SecondarySkill(identifier);
			});
			break;
		}
		case BonusSource::HERO_SPECIAL:
		{
			resolver.requestIdentifier( "hero", node, [&sourceInstance](int32_t identifier)
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

static TUpdaterPtr parseUpdater(const JsonNode & updaterJson, IIdentifierResolver & resolver)
{
	const std::map<std::string, std::shared_ptr<IUpdater>> bonusUpdaterMap =
	{
			{"TIMES_HERO_LEVEL", std::make_shared<TimesHeroLevelUpdater>()},
			{"TIMES_HERO_LEVEL_DIVIDE_STACK_LEVEL", std::make_shared<TimesHeroLevelDivideStackLevelUpdater>()},
			{"DIVIDE_STACK_LEVEL", std::make_shared<DivideStackLevelUpdater>()},
			{"TIMES_STACK_LEVEL", std::make_shared<TimesStackLevelUpdater>()},
			{"TIMES_STACK_SIZE", std::make_shared<TimesStackSizeUpdater>()},
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
	case JsonNode::JsonType::DATA_VECTOR:
		{
			auto result = std::make_shared<CompositeUpdater>();
			for (const auto & entry : updaterJson.Vector())
				result->updaters.push_back(parseUpdater(entry, resolver));
			return result;
		}
	case JsonNode::JsonType::DATA_STRUCT:
		if(updaterJson["type"].String() == "GROWS_WITH_LEVEL")
		{
			// MOD COMPATIBILITY - parameters is deprecated in 1.7
			const JsonNode & param = updaterJson["parameters"];
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
			int stepValue = updaterJson["stepValue"].Integer();
			if (minimum > maximum)
			{
				logMod->warn("TIMES_STACK_SIZE updater: minimum value (%d) is above maximum value(%d)!", minimum, maximum);
				return std::make_shared<TimesStackSizeUpdater>(maximum, minimum, std::max(1, stepSize), std::max(1, stepValue));
			}
			return std::make_shared<TimesStackSizeUpdater>(minimum, maximum, std::max(1, stepSize), std::max(1, stepValue));
		}
		if(updaterJson["type"].String() == "TIMES_ARMY_SIZE")
		{
			auto result = std::make_shared<TimesArmySizeUpdater>();

			result->minimum = updaterJson["minimum"].isNull() ? std::numeric_limits<int>::min() : updaterJson["minimum"].Integer();
			result->maximum = updaterJson["maximum"].isNull() ? std::numeric_limits<int>::max() : updaterJson["maximum"].Integer();
			result->stepSize = updaterJson["stepSize"].isNull() ? 1 : updaterJson["stepSize"].Integer();
			result->filteredLevel = updaterJson["filteredLevel"].isNull() ? -1 : updaterJson["filteredLevel"].Integer();
			if (result->minimum > result->maximum)
			{
				logMod->warn("TIMES_ARMY_SIZE updater: minimum value (%d) is above maximum value(%d)!", result->minimum, result->maximum);
				std::swap(result->minimum, result->maximum);
			}
			if (!updaterJson["filteredFaction"].isNull())
			{
				resolver.requestIdentifier( "faction", updaterJson["filteredFaction"], [result](int32_t identifier)
				{
					result->filteredFaction = FactionID(identifier);
				});
			}
			if (!updaterJson["filteredCreature"].isNull())
			{
				resolver.requestIdentifier( "creature", updaterJson["filteredCreature"], [result](int32_t identifier)
				{
					result->filteredCreature = CreatureID(identifier);
				});
			}
			return result;
		}
		else
			logMod->warn("Unknown updater type \"%s\"", updaterJson["type"].String());
		break;
	}
	return nullptr;
}

std::shared_ptr<Bonus> JsonUtils::parseBonus(const JsonVector & ability_vec, LibraryLoadContext & context)
{
	auto b = std::make_shared<Bonus>();

	const JsonNode & typeNode = ability_vec[0];
	const JsonNode & subtypeNode = ability_vec[2];

	context.resolver.requestIdentifier("bonus", typeNode, [b, typeName = typeNode.String(), subtypeNode, &context](si32 bonusID)
	{
		b->type = static_cast<BonusType>(bonusID);
		loadBonusSubtype(b->subtype, b->type, typeName, subtypeNode, context.resolver);
	});
	b->val = static_cast<si32>(ability_vec[1].Float());
	b->parameters = std::make_shared<BonusParameters>(static_cast<si32>(ability_vec[3].Integer()));
	b->duration = BonusDuration::PERMANENT; //TODO: handle flags (as integer)
	b->turnsRemain = 0;
	return b;
}

static std::shared_ptr<const ILimiter> parseAggregateLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
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
			return JsonUtils::parseLimiter(subLimiters[0], resolver);
		// implicit aggregator must be allOf
		result = std::make_shared<AllOfLimiter>();
		offset = 0;
	}
	if(subLimiters.size() == offset)
		logMod->warn("Warning: empty sub-limiter list");
	for(int sl = offset; sl < subLimiters.size(); ++sl)
		result->add(JsonUtils::parseLimiter(subLimiters[sl], resolver));
	return result;
}

static std::shared_ptr<const ILimiter> parseCreatureTypeLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	auto creatureLimiter = std::make_shared<CCreatureTypeLimiter>();

	const JsonNode & parameters = limiter["parameters"];
	const JsonNode & creatureNode = limiter.Struct().count("creature") ? limiter["creature"] : parameters[0];
	const JsonNode & upgradesNode = limiter.Struct().count("includeUpgrades") ? limiter["includeUpgrades"] : parameters[1];

	resolver.requestIdentifier( "creature", creatureNode, [creatureLimiter](si32 creature)
	{
		creatureLimiter->setCreature(CreatureID(creature));
	});

	if (upgradesNode.isString())
	{
		logGlobal->warn("CREATURE_TYPE_LIMITER: parameter 'includeUpgrades' is invalid! expected boolean, but string '%s' found!", upgradesNode.String());
		if (upgradesNode.String() == "true") // MOD COMPATIBILITY - broken mod, compensating
			creatureLimiter->includeUpgrades = true;
	}
	else
	{
		creatureLimiter->includeUpgrades = upgradesNode.Bool();
	}

	return creatureLimiter;
}

static std::shared_ptr<const ILimiter> parseHasAnotherBonusLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	auto bonusLimiter = std::make_shared<HasAnotherBonusLimiter>();

	const JsonNode & parameters = limiter["parameters"];
	const JsonNode & jsonType = limiter.Struct().count("bonusType") ? limiter["bonusType"] : parameters[0];
	const JsonNode & jsonSubtype = limiter.Struct().count("bonusSubtype") ? limiter["bonusSubtype"] : parameters[1];
	const JsonNode & jsonSourceType = limiter.Struct().count("bonusSourceType") ? limiter["bonusSourceType"] : parameters[2]["type"];
	const JsonNode & jsonSourceID = limiter.Struct().count("bonusSourceID") ? limiter["bonusSourceID"] : parameters[2]["id"];
	const JsonNode & jsonSourceSecondaryId = limiter["bonusSourceSecondaryID"];
	const JsonNode & jsonMinValue = limiter["bonusMinValue"];
	const JsonNode & jsonMaxValue = limiter["bonusMaxValue"];

	if (!jsonMinValue.isNull())
		bonusLimiter->minValue = jsonMinValue.Integer();

	if (!jsonMaxValue.isNull())
		bonusLimiter->maxValue = jsonMaxValue.Integer();

	if (!jsonType.isNull())
	{
		resolver.requestIdentifier("bonus", jsonType, [bonusLimiter, typeName = jsonType.String(), jsonSubtype, &resolver](si32 bonusID)
		{
			bonusLimiter->type = static_cast<BonusType>(bonusID);
			if (!jsonSubtype.isNull())
			{
				loadBonusSubtype(bonusLimiter->subtype, bonusLimiter->type, typeName, jsonSubtype, resolver);
				bonusLimiter->isSubtypeRelevant = true;
			}
		});
	}

	if(!jsonSourceType.isNull())
	{
		auto sourceIt = bonusSourceMap.find(jsonSourceType.String());
		if(sourceIt != bonusSourceMap.end())
		{
			bonusLimiter->source = sourceIt->second;
			bonusLimiter->isSourceRelevant = true;
			if(!jsonSourceID.isNull()) {
				loadBonusSourceInstance(bonusLimiter->sid, bonusLimiter->source, jsonSourceID, jsonSourceSecondaryId, resolver);
				bonusLimiter->isSourceIDRelevant = true;
			}
		}
		else
			logMod->warn("HAS_ANOTHER_BONUS_LIMITER: unknown bonus source type '%s'!", jsonSourceType.String());
	}

	return bonusLimiter;
}

static std::shared_ptr<const ILimiter> parseCreatureAlignmentLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	const JsonNode & parameters = limiter["parameters"];
	const JsonNode & alignmentNode = limiter.Struct().count("alignment") ? limiter["alignment"] : parameters[0];

	int alignment = vstd::find_pos(GameConstants::ALIGNMENT_NAMES, alignmentNode.String());
	if(alignment == -1)
	{
		logMod->error("Error: invalid alignment %s.", alignmentNode.String());
		return nullptr;
	}
	else
		return std::make_shared<CreatureAlignmentLimiter>(static_cast<EAlignment>(alignment));
}

static std::shared_ptr<const ILimiter> parseFactionLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	const JsonNode & parameters = limiter["parameters"];
	const JsonNode & factionNode = limiter.Struct().count("faction") ? limiter["faction"] : parameters[0];

	auto factionLimiter = std::make_shared<FactionLimiter>();
	resolver.requestIdentifier("faction", factionNode, [=](si32 faction)
	{
		factionLimiter->faction = FactionID(faction);
	});
	return factionLimiter;
}

static std::shared_ptr<const ILimiter> parseCreatureLevelLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	const JsonNode & parameters = limiter["parameters"];
	const JsonNode & minLevelNode = limiter.Struct().count("minLevel") ? limiter["minLevel"] : parameters[0];
	const JsonNode & maxLevelNode = limiter.Struct().count("maxlevel") ? limiter["maxlevel"] : parameters[1];

	//If parameters is empty, level limiter works as CREATURES_ONLY limiter
	auto levelLimiter = std::make_shared<CreatureLevelLimiter>();

	if (!minLevelNode.isNull())
		levelLimiter->minLevel = minLevelNode.Integer();

	if (!maxLevelNode.isNull())
		levelLimiter->maxLevel = maxLevelNode.Integer();

	return levelLimiter;
}

static std::shared_ptr<const ILimiter> parseExperienceRankLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	auto rankLimiter = std::make_shared<RankRangeLimiter>(0);

	if (!limiter["minRank"].isNull())
		rankLimiter->minRank = limiter["minRank"].Integer();

	if (!limiter["maxRank"].isNull())
		rankLimiter->maxRank = limiter["maxRank"].Integer();

	return rankLimiter;
}

static std::shared_ptr<const ILimiter> parseTerrainLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	const JsonNode & parameters = limiter["parameters"];
	const JsonNode & terrainNode = limiter.Struct().count("terrain") ? limiter["terrain"] : parameters[0];

	auto terrainLimiter = std::make_shared<TerrainLimiter>();
	if(!terrainNode.isNull())
	{
		resolver.requestIdentifier("terrain", terrainNode, [terrainLimiter](si32 terrain)
		{
			terrainLimiter->terrainType = terrain;
		});
	}
	return terrainLimiter;
}

static std::shared_ptr<const ILimiter> parseUnitOnHexLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	const JsonNode & parameters = limiter["parameters"];
	const JsonNode & hexesNode = limiter.Struct().count("hexes") ? limiter["hexes"] : parameters[0];

	auto hexLimiter = std::make_shared<UnitOnHexLimiter>();

	for (const auto & parameter : hexesNode.Vector())
		hexLimiter->applicableHexes.insert(BattleHex(parameter.Integer()));

	return hexLimiter;
}

static std::shared_ptr<const ILimiter> parseUnitAdjacentLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	const JsonNode & creatureNode = limiter["creature"];

	auto unitLimiter = std::make_shared<UnitAdjacentLimiter>();
	resolver.requestIdentifier("creature", creatureNode, [unitLimiter](si32 creatureID)
	{
		unitLimiter->targetUnit = creatureID;
	});

	return unitLimiter;
}

static std::shared_ptr<const ILimiter> parseHasChargesLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	const JsonNode & parameters = limiter["parameters"];
	const JsonNode & costNode = limiter.Struct().count("cost") ? limiter["cost"] : parameters[0];
	auto hasChargesLimiter = std::make_shared<HasChargesLimiter>();

	if (!costNode.isNull())
		hasChargesLimiter->chargeCost = costNode.Integer();

	return hasChargesLimiter;
}

std::shared_ptr<const ILimiter> JsonUtils::parseLimiter(const JsonNode & limiter, IIdentifierResolver & resolver)
{
	static const std::map<std::string, std::function<std::shared_ptr<const ILimiter>(const JsonNode & limiter, IIdentifierResolver & resolver)>> limiterParsers = {
		{"CREATURE_TYPE_LIMITER",      parseCreatureTypeLimiter     },
		{"HAS_ANOTHER_BONUS_LIMITER",  parseHasAnotherBonusLimiter  },
		{"CREATURE_ALIGNMENT_LIMITER", parseCreatureAlignmentLimiter},
		{"FACTION_LIMITER",            parseFactionLimiter          },
		{"CREATURE_FACTION_LIMITER",   parseFactionLimiter          },
		{"CREATURE_LEVEL_LIMITER",     parseCreatureLevelLimiter    },
		{"EXPERIENCE_RANK_LIMITER",    parseExperienceRankLimiter   },
		{"TERRAIN_LIMITER",            parseTerrainLimiter          },
		{"CREATURE_TERRAIN_LIMITER",   parseTerrainLimiter          },
		{"UNIT_ON_HEXES",              parseUnitOnHexLimiter        },
		{"UNIT_ADJACENT",              parseUnitAdjacentLimiter	    },
		{"HAS_CHARGES_LIMITER",        parseHasChargesLimiter       },
	};

	switch(limiter.getType())
	{
		case JsonNode::JsonType::DATA_VECTOR:
			return parseAggregateLimiter(limiter, resolver);
		case JsonNode::JsonType::DATA_STRING: //pre-defined limiters
			return parseByMap(bonusLimiterMap, &limiter, "limiter type ");
		case JsonNode::JsonType::DATA_STRUCT: //customizable limiters
		{
			std::string limiterType = limiter["type"].String();
			if (limiterParsers.count(limiterType))
				return limiterParsers.at(limiterType)(limiter, resolver);
			else
				logMod->error("Error: invalid customizable limiter type %s.", limiterType);
		}
	}
	return nullptr;
}

/// `context` is null when the bonus is not part of library content
static bool parseBonusImpl(const JsonNode & ability, Bonus * b, IIdentifierResolver & resolver, LibraryLoadContext * context, const TextIdentifier & descriptionID);

static std::shared_ptr<Bonus> parseBonusOrNone(const JsonNode & ability, IIdentifierResolver & resolver, LibraryLoadContext * context, const TextIdentifier & descriptionID)
{
	auto b = std::make_shared<Bonus>();
	if (!parseBonusImpl(ability, b.get(), resolver, context, descriptionID))
	{
		// caller code can not handle this case and presumes that returned bonus is always valid
		logGlobal->error("Failed to parse bonus! Json config was %S ", ability.toString());
		b->type = BonusType::NONE;
		return b;
	}
	return b;
}

std::shared_ptr<Bonus> JsonUtils::parseBonus(const JsonNode & ability, IIdentifierResolver & resolver)
{
	return parseBonusOrNone(ability, resolver, nullptr, "");
}

std::shared_ptr<Bonus> JsonUtils::parseBonus(const JsonNode & ability, LibraryLoadContext & context, const TextIdentifier & descriptionID)
{
	return parseBonusOrNone(ability, context.resolver, &context, descriptionID);
}

bool JsonUtils::parseBonus(const JsonNode & ability, Bonus * placement, LibraryLoadContext & context, const TextIdentifier & descriptionID)
{
	return parseBonusImpl(ability, placement, context.resolver, &context, descriptionID);
}

static bool parseBonusImpl(const JsonNode & ability, Bonus * b, IIdentifierResolver & resolver, LibraryLoadContext * context, const TextIdentifier & descriptionID)
{
	const JsonNode * value = nullptr;
	const JsonNode & subtypeNode = ability["subtype"];
	const JsonNode & addinfoNode = ability["addInfo"];

	if (ability["type"].isNull())
	{
		logMod->error("Failed to parse bonus. Description: '%s'. Config: '%s'", descriptionID.get(), ability.toCompactString());
		return false;
	}

	// rewrite before parsing, so that identifiers of the replacement resolve through the normal path
	JsonNode migrated;
	if (BonusMigration::migrateBonus(ability, migrated))
		return parseBonusImpl(migrated, b, resolver, context, descriptionID);

	BonusMigration::warnIfRetired(ability, descriptionID);

	resolver.requestIdentifier("bonus", ability["type"], [b, typeName = ability["type"].String(), subtypeNode, addinfoNode, descriptionID, &resolver, context](si32 bonusID)
	{
		b->type = static_cast<BonusType>(bonusID);
		loadBonusSubtype(b->subtype, b->type, typeName, subtypeNode, resolver);
		b->parameters = loadBonusAddInfo(b->type, typeName, addinfoNode, resolver, context);

		if (b->type == BonusType::COMBAT_EVENT_TRIGGER)
			prepareCombatScriptParameters(b, subtypeNode, resolver, context, descriptionID);
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
				assert(context);
				context->lib.generaltexth->registerString(ability.getModScope(), descriptionID, ability["description"]);
				b->description.appendTextID(descriptionID.get());
			}
		}
		if (ability["description"].isNumber())
			b->description.appendTextID("core.arraytxt", ability["description"].Integer());
	}

	if(!ability["icon"].isNull())
		b->customIconPath = ImagePath::fromJson(ability["icon"]);

	b->hidden = !ability["hidden"].isNull() && ability["hidden"].Bool();

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
		case JsonNode::JsonType::DATA_INTEGER:
		case JsonNode::JsonType::DATA_FLOAT:
			b->duration = static_cast<BonusDuration::Type>(value->Integer());
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
		b->source = static_cast<BonusSource>(parseByMapN(bonusSourceMap, value, "source type "));

	if (!ability["sourceID"].isNull())
		loadBonusSourceInstance(b->sid, b->source, ability["sourceID"], ability["sourceSecondaryID"], resolver);

	value = &ability["targetSourceType"];
	if (!value->isNull())
		b->targetSourceType = static_cast<BonusSource>(parseByMapN(bonusSourceMap, value, "target type "));

	value = &ability["targetSourceID"];
	if (!value->isNull())
	{
		if (b->targetSourceType == BonusSource::OTHER)
			logMod->warn("Bonus has 'targetSourceID' without 'targetSourceType' - target source ID will be ignored!");
		else
			loadBonusSourceInstance(b->targetSourceID, b->targetSourceType, *value, JsonNode(), resolver);
	}

	value = &ability["limiters"];
	if (!value->isNull())
		b->limiter = JsonUtils::parseLimiter(*value, resolver);

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
		b->addUpdater(parseUpdater(*value, resolver));
	value = &ability["propagationUpdater"];
	if(!value->isNull())
		b->propagationUpdater = parseUpdater(*value, resolver);
	return true;
}
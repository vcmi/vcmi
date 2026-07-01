/*
 * CSpellHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CSpellHandler.h"

#include "../constants/StringConstants.h"
#include "../json/JsonBonus.h"
#include "../json/JsonUtils.h"
#include "../GameLibrary.h"
#include "../modding/IdentifierStorage.h"
#include "../texts/CLegacyConfigParser.h"
#include "../texts/CGeneralTextHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

static constexpr std::array LEVEL_NAMES = {"none", "basic", "advanced", "expert"};

std::vector<JsonNode> CSpellHandler::loadLegacyData()
{
	std::vector<JsonNode> legacyData;

	CLegacyConfigParser parser(TextPath::builtin("DATA/SPTRAITS.TXT"));

	auto readSchool = [&](JsonMap & schools, const std::string & name)
	{
		if (parser.readString() == "x")
		{
			schools[name].Bool() = true;
		}
	};

	auto read = [&](bool combat, bool ability)
	{
		do
		{
			JsonNode lineNode;

			const auto id = legacyData.size();

			lineNode["index"].Integer() = id;
			lineNode["type"].String() = ability ? "ability" : (combat ? "combat" : "adventure");

			lineNode["name"].String() = parser.readString();

			parser.readString(); //ignored unused abbreviated name
			lineNode["level"].Integer() = static_cast<si64>(parser.readNumber());

			auto& schools = lineNode["school"].Struct();

			readSchool(schools, "earth");
			readSchool(schools, "water");
			readSchool(schools, "fire");
			readSchool(schools, "air");

			auto& levels = lineNode["levels"].Struct();

			auto getLevel = [&](const size_t idx)->JsonMap&
			{
				assert(idx < GameConstants::SPELL_SCHOOL_LEVELS);
				return levels[LEVEL_NAMES[idx]].Struct();
			};

			auto costs = parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);
			lineNode["power"].Integer() = static_cast<si64>(parser.readNumber());
			auto powers = parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);

			auto & chances = lineNode["gainChance"].Struct();

			for(const auto & name : NFaction::names)
				chances[name].Integer() = static_cast<si64>(parser.readNumber());

			// Unused, AI values
			parser.readNumArray<si32>(GameConstants::SPELL_SCHOOL_LEVELS);

			std::vector<std::string> descriptions;
			for(size_t i = 0; i < GameConstants::SPELL_SCHOOL_LEVELS; i++)
				descriptions.push_back(parser.readString());

			parser.readString(); //ignore attributes. All data present in JSON

			//save parsed level specific data
			for(size_t i = 0; i < GameConstants::SPELL_SCHOOL_LEVELS; i++)
			{
				auto& level = getLevel(i);
				level["description"].String() = descriptions[i];
				level["cost"].Integer() = costs[i];
				level["power"].Integer() = powers[i];
			}

			legacyData.push_back(lineNode);
		}
		while (parser.endLine() && !parser.isNextEntryEmpty());
	};

	auto skip = [&](int cnt)
	{
		for(int i=0; i<cnt; i++)
			parser.endLine();
	};

	skip(5);// header
	read(false,false); //read adventure map spells
	skip(3);
	read(true,false); //read battle spells
	skip(3);
	read(true,true);//read creature abilities

    //TODO: maybe move to config
	//clone Acid Breath attributes for Acid Breath damage effect
	if(legacyData.size() > SpellID::ACID_BREATH_DEFENSE) // not for RoE
	{
		JsonNode temp = legacyData[SpellID::ACID_BREATH_DEFENSE];
		temp["index"].Integer() = SpellID::ACID_BREATH_DAMAGE;
		legacyData.push_back(temp);
	}

	objects.resize(legacyData.size());

	return legacyData;
}

const std::vector<std::string> & CSpellHandler::getTypeNames() const
{
	static const std::vector<std::string> typeNames = { "spell" };
	return typeNames;
}

std::vector<int> CSpellHandler::spellRangeInHexes(std::string input) const
{
	BattleHexArray ret;
	std::string rng = input + ','; //copy + artificial comma for easier handling

	if(rng.size() >= 2 && std::tolower(rng[0]) != 'x') //there is at least one hex in range (+artificial comma)
	{
		std::string number1;
		std::string number2;
		int beg = 0;
		int end = 0;
		bool readingFirst = true;
		for(auto & elem : rng)
		{
			if(std::isdigit(elem) ) //reading number
			{
				if(readingFirst)
					number1 += elem;
				else
					number2 += elem;
			}
			else if(elem == ',') //comma
			{
				//calculating variables
				if(readingFirst)
				{
					beg = std::stoi(number1);
					number1 = "";
				}
				else
				{
					end = std::stoi(number2);
					number2 = "";
				}
				//obtaining new hexes
				std::set<ui16> curLayer;
				if(readingFirst)
				{
					ret.insert(beg);
				}
				else
				{
					for(int i = beg; i <= end; ++i)
						ret.insert(i);
				}
			}
			else if(elem == '-') //dash
			{
				beg = std::stoi(number1);
				number1 = "";
				readingFirst = false;
			}
		}
	}

	std::vector<int> result;
	result.reserve(ret.size());

	std::transform(ret.begin(), ret.end(), std::back_inserter(result),
		[](const BattleHex & hex) { return hex.toInt(); }
	);

	return result;
}

std::shared_ptr<CSpell> CSpellHandler::loadFromJson(const std::string & scope, const JsonNode & json, const std::string & identifier, size_t index)
{
	assert(identifier.find(':') == std::string::npos);
	assert(!scope.empty());

	SpellID id(static_cast<si32>(index));

	auto spell = std::make_shared<CSpell>();
	spell->id = id;
	spell->identifier = identifier;
	spell->modScope = scope;

	const auto type = json["type"].String();

	if(type == "ability")
	{
		spell->creatureAbility = true;
		spell->combat = true;
	}
	else
	{
		spell->creatureAbility = false;
		spell->combat = type == "combat";
	}

	LIBRARY->generaltexth->registerString(scope, spell->getNameTextID(), json["name"]);

	logMod->trace("%s: loading spell %s", __FUNCTION__, spell->getNameTranslated());

	for(const auto & schoolJson : json["school"].Struct())
	{
		if (schoolJson.second.Bool())
		{
			LIBRARY->identifiers()->requestIdentifier(schoolJson.second.getModScope(), "spellSchool", schoolJson.first, [spell](si32 schoolID)
			{
				spell->schools.emplace(schoolID);
			});
		}
	}

	spell->castOnSelf = json["canCastOnSelf"].Bool();
	spell->castOnlyOnSelf = json["canCastOnlyOnSelf"].Bool();
	spell->castWithoutSkip = json["canCastWithoutSkip"].Bool();
	spell->level = static_cast<si32>(json["level"].Integer());
	spell->power = static_cast<si32>(json["power"].Integer());

	spell->defaultProbability = static_cast<si32>(json["defaultGainChance"].Integer());

	for(const auto & node : json["gainChance"].Struct())
	{
		const int chance = static_cast<int>(node.second.Integer());

		LIBRARY->identifiers()->requestIdentifierIfFound(node.second.getModScope(), "faction", node.first, [=](si32 factionID)
		{
			spell->probabilities[FactionID(factionID)] = chance;
		});
	}

	auto targetType = json["targetType"].String();

	if(targetType == "NO_TARGET")
		spell->targetType = spells::AimType::NOTHING;
	else if(targetType == "CREATURE")
		spell->targetType = spells::AimType::CREATURE;
	else if(targetType == "OBSTACLE")
		spell->targetType = spells::AimType::OBSTACLE;
	else if(targetType == "LOCATION")
		spell->targetType = spells::AimType::LOCATION;
	else
		logMod->warn("Spell %s: target type %s - assumed NO_TARGET.", spell->getNameTranslated(), (targetType.empty() ? "empty" : "unknown ("+targetType+")"));

	for(const auto & counteredSpell: json["counters"].Struct())
	{
		if(counteredSpell.second.Bool())
		{
			LIBRARY->identifiers()->requestIdentifier(counteredSpell.second.getModScope(), "spell", counteredSpell.first, [=](si32 id)
			{
				spell->counteredSpells.emplace_back(id);
			});
		}
	}

	//TODO: more error checking - f.e. conflicting flags
	const auto flags = json["flags"];

	//by default all flags are set to false in constructor

	spell->damage = flags["damage"].Bool(); //do this before "offensive"
	spell->nonMagical = flags["nonMagical"].Bool();
	spell->persistent = flags["persistent"].Bool();


	if(flags["offensive"].Bool())
	{
		spell->setIsOffensive(true);
	}

	if(flags["rising"].Bool())
	{
		spell->setIsRising(true);
	}

	const bool implicitPositiveness = spell->offensive || spell->rising; //(!) "damage" does not mean NEGATIVE  --AVS

	if(flags["indifferent"].Bool())
	{
		spell->positiveness = CSpell::NEUTRAL;
	}
	else if(flags["negative"].Bool())
	{
		spell->positiveness = CSpell::NEGATIVE;
	}
	else if(flags["positive"].Bool())
	{
		spell->positiveness = CSpell::POSITIVE;
	}
	else if(!implicitPositiveness)
	{
		spell->positiveness = CSpell::NEUTRAL; //duplicates constructor but, just in case
		logMod->error("Spell %s: no positiveness specified, assumed NEUTRAL.", spell->getNameTranslated());
	}

	spell->special = flags["special"].Bool();

	spell->onlyOnWaterMap = json["onlyOnWaterMap"].Bool();

	auto readBonusStruct = [&](const std::string & name, std::vector<BonusType> & vec)
	{
		for(auto bonusData: json[name].Struct())
		{
			if(!bonusData.second.Bool())
				continue;

			LIBRARY->identifiers()->requestIdentifier(bonusData.second.getModScope(), "bonus", bonusData.first, [&vec](si32 bonusID)
			{
				vec.push_back(static_cast<BonusType>(bonusID));
			});
		}
	};

	if(json["targetCondition"].isNull())
	{
		CSpell::BTVector immunities;
		CSpell::BTVector absoluteImmunities;
		CSpell::BTVector limiters;
		CSpell::BTVector absoluteLimiters;

		readBonusStruct("immunity", immunities);
		readBonusStruct("absoluteImmunity", absoluteImmunities);
		readBonusStruct("limit", limiters);
		readBonusStruct("absoluteLimit", absoluteLimiters);

		if(!(immunities.empty() && absoluteImmunities.empty() && limiters.empty() && absoluteLimiters.empty()))
		{
			logMod->warn("Spell %s has old target condition format. Expected configuration: ", spell->getNameTranslated());
			spell->targetCondition = spell->convertTargetCondition(immunities, absoluteImmunities, limiters, absoluteLimiters);
			logMod->warn("\n\"targetCondition\" : %s", spell->targetCondition.toString());
		}
	}
	else
	{
		spell->targetCondition = json["targetCondition"];

		//TODO: could this be safely merged instead of discarding?
		if(!json["immunity"].isNull())
			logMod->warn("Spell %s 'immunity' field mixed with 'targetCondition' discarded", spell->getNameTranslated());
		if(!json["absoluteImmunity"].isNull())
			logMod->warn("Spell %s 'absoluteImmunity' field mixed with 'targetCondition' discarded", spell->getNameTranslated());
		if(!json["limit"].isNull())
			logMod->warn("Spell %s 'limit' field mixed with 'targetCondition' discarded", spell->getNameTranslated());
		if(!json["absoluteLimit"].isNull())
			logMod->warn("Spell %s 'absoluteLimit' field mixed with 'targetCondition' discarded", spell->getNameTranslated());
	}

	const JsonNode & graphicsNode = json["graphics"];

	spell->iconImmune = ImagePath::fromJson(graphicsNode["iconImmune"]);
	spell->iconBook = graphicsNode["iconBook"].String();
	spell->iconEffect = graphicsNode["iconEffect"].String();
	spell->iconScenarioBonus = graphicsNode["iconScenarioBonus"].String();
	spell->iconScroll = graphicsNode["iconScroll"].String();

	const JsonNode & animationNode = json["animation"];

	auto loadAnimationQueue = [&](const std::string & jsonName, SpellAnimationQueue & q)
	{
		auto queueNode = animationNode[jsonName].Vector();
		for(const JsonNode & item : queueNode)
		{
			SpellAnimationItem newItem;

			if(item.getType() == JsonNode::JsonType::DATA_STRING)
				newItem.resourceName = AnimationPath::fromJson(item);
			else if(item.getType() == JsonNode::JsonType::DATA_STRUCT)
			{
				newItem.resourceName = AnimationPath::fromJson(item["defName"]);
				newItem.effectName   = item["effectName"].String();

				auto vPosStr = item["verticalPosition"].String();
				if("bottom" == vPosStr)
					newItem.verticalPosition = VerticalPosition::BOTTOM;

				if (item["transparency"].isNumber())
					newItem.transparency = item["transparency"].Float();
				else
					newItem.transparency = 1.0;
			}
			else if(item.isNumber())
			{
				newItem.pause = item.Integer();
			}

			q.push_back(newItem);
		}
	};

	loadAnimationQueue("affect", spell->animationInfo.affect);
	loadAnimationQueue("cast", spell->animationInfo.cast);
	loadAnimationQueue("hit", spell->animationInfo.hit);

	const JsonVector & projectile = animationNode["projectile"].Vector();

	for(const JsonNode & item : projectile)
	{
		CSpell::ProjectileInfo info;
		info.resourceName = AnimationPath::fromJson(item["defName"]);
		info.minimumAngle = item["minimumAngle"].Float();

		spell->animationInfo.projectile.push_back(info);
	}

	const JsonNode & soundsNode = json["sounds"];
	spell->castSound = AudioPath::fromJson(soundsNode["cast"]);

	//load level attributes
	const int levelsCount = GameConstants::SPELL_SCHOOL_LEVELS;

	for(int levelIndex = 0; levelIndex < levelsCount; levelIndex++)
	{
		const JsonNode & levelNode = json["levels"][LEVEL_NAMES[levelIndex]];

		CSpell::LevelInfo & levelObject = spell->levels[levelIndex];

		if (!levelNode["description"].String().empty())
			LIBRARY->generaltexth->registerString(scope, spell->getDescriptionTextID(levelIndex), levelNode["description"]);

		levelObject.cost          = static_cast<si32>(levelNode["cost"].Integer());
		levelObject.smartTarget   = levelNode["targetModifier"]["smart"].Bool();
		levelObject.clearAffected = levelNode["targetModifier"]["clearAffected"].Bool();
		levelObject.range         = spellRangeInHexes(levelNode["range"].String());
		levelObject.power         = levelNode["power"].Integer();
		levelObject.effects = levelNode["effects"];
		levelObject.cumulativeEffects = levelNode["cumulativeEffects"];

		levelObject.adventureEffect = levelNode["adventureEffect"];

		if(levelObject.adventureEffect["type"].String() == "reinforcements")
		{
			auto registerField = [&](const std::string & field)
			{
				const std::string & value = levelObject.adventureEffect[field].String();
				if(!value.empty() && value.front() != '@')
					LIBRARY->generaltexth->registerString(scope, spell->getAdventureEffectTextID("reinforcements", field), levelObject.adventureEffect[field]);
			};

			registerField("casterInTown");
			registerField("selectTownTitle");
			registerField("selectTownDescription");
			registerField("garrisonTitle");
		}

		if(!levelNode["battleEffects"].Struct().empty())
		{
			levelObject.battleEffects = levelNode["battleEffects"];

			if(!levelObject.cumulativeEffects.Struct().empty() || !levelObject.effects.Struct().empty() || spell->isOffensive())
				logGlobal->error("Mixing %s special effects with old format effects gives unpredictable result", spell->getNameTranslated());
		}
	}
	return spell;
}

void CSpellHandler::afterLoadFinalization()
{
	for(auto & spell : objects)
	{
		spell->setupMechanics();
	}
}

void CSpellHandler::beforeValidate(JsonNode & object)
{
	//handle "base" level info

	JsonNode & levels = object["levels"];
	JsonNode & base = levels["base"];

	auto inheritNode = [&](const std::string & name)
	{
		JsonUtils::inherit(levels[name],base);
	};

	inheritNode("none");
	inheritNode("basic");
	inheritNode("advanced");
	inheritNode("expert");

	levels.Struct().erase("base");
}

std::set<SpellID> CSpellHandler::getDefaultAllowed() const
{
	std::set<SpellID> allowedSpells;

	for(auto const & s : objects)
		if (!s->isSpecial() && !s->isCreatureAbility())
			allowedSpells.insert(s->getId());

	return allowedSpells;
}

VCMI_LIB_NAMESPACE_END

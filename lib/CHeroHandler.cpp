#include "StdInc.h"
#include "CHeroHandler.h"

#include "CGeneralTextHandler.h"
#include "filesystem/CResourceLoader.h"
#include "VCMI_Lib.h"
#include "JsonNode.h"
#include "StringConstants.h"
#include "BattleHex.h"
#include "CModHandler.h"
#include "CTownHandler.h"
#include "CObjectHandler.h" //for hero specialty

/*
 * CHeroHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

SecondarySkill CHeroClass::chooseSecSkill(const std::set<SecondarySkill> & possibles) const //picks secondary skill out from given possibilities
{
	if(possibles.size()==1)
		return *possibles.begin();
	int totalProb = 0;
	for(auto i=possibles.begin(); i!=possibles.end(); i++)
	{
		totalProb += secSkillProbability[*i];
	}
	int ran = rand()%totalProb;
	for(auto i=possibles.begin(); i!=possibles.end(); i++)
	{
		ran -= secSkillProbability[*i];
		if(ran<0)
			return *i;
	}
	throw std::runtime_error("Cannot pick secondary skill!");
}

EAlignment::EAlignment CHeroClass::getAlignment() const
{
	return EAlignment::EAlignment(VLC->townh->factions[faction]->alignment);
}

std::vector<BattleHex> CObstacleInfo::getBlocked(BattleHex hex) const
{
	std::vector<BattleHex> ret;
	if(isAbsoluteObstacle)
	{
		assert(!hex.isValid());
		range::copy(blockedTiles, std::back_inserter(ret));
		return ret;
	}

	BOOST_FOREACH(int offset, blockedTiles)
	{
		BattleHex toBlock = hex + offset;
		if((hex.getY() & 1) && !(toBlock.getY() & 1))
			toBlock += BattleHex::LEFT;

		if(!toBlock.isValid())
            logGlobal->errorStream() << "Misplaced obstacle!";
		else
			ret.push_back(toBlock);
	}

	return ret;
}

bool CObstacleInfo::isAppropriate(ETerrainType terrainType, int specialBattlefield /*= -1*/) const
{
	if(specialBattlefield != -1)
		return vstd::contains(allowedSpecialBfields, specialBattlefield);

	return vstd::contains(allowedTerrains, terrainType);
}

CHeroClass *CHeroClassHandler::loadFromJson(const JsonNode & node)
{
	CHeroClass * heroClass = new CHeroClass();

	heroClass->imageBattleFemale = node["animation"]["battle"]["female"].String();
	heroClass->imageBattleMale   = node["animation"]["battle"]["male"].String();
	heroClass->imageMapFemale    = node["animation"]["map"]["female"].String();
	heroClass->imageMapMale      = node["animation"]["map"]["male"].String();

	heroClass->name = node["name"].String();

	BOOST_FOREACH(const std::string & pSkill, PrimarySkill::names)
	{
		heroClass->primarySkillInitial.push_back(node["primarySkills"][pSkill].Float());
		heroClass->primarySkillLowLevel.push_back(node["lowLevelChance"][pSkill].Float());
		heroClass->primarySkillHighLevel.push_back(node["highLevelChance"][pSkill].Float());
	}

	BOOST_FOREACH(const std::string & secSkill, NSecondarySkill::names)
	{
		heroClass->secSkillProbability.push_back(node["secondarySkills"][secSkill].Float());
	}

	BOOST_FOREACH(auto & tavern, node["tavern"].Struct())
	{
		int value = tavern.second.Float();

		VLC->modh->identifiers.requestIdentifier("faction." + tavern.first,
		[=](si32 factionID)
		{
			heroClass->selectionProbability[factionID] = value;
		});
	}

	VLC->modh->identifiers.requestIdentifier("faction." + node["faction"].String(),
	[=](si32 factionID)
	{
		heroClass->faction = factionID;
	});

	return heroClass;
}

std::vector<JsonNode> CHeroClassHandler::loadLegacyData(size_t dataSize)
{
	heroClasses.resize(GameConstants::F_NUMBER * 2);
	std::vector<JsonNode> h3Data;
	h3Data.reserve(dataSize);

	CLegacyConfigParser parser("DATA/HCTRAITS.TXT");

	parser.endLine(); // header
	parser.endLine();

	for (size_t i=0; i<dataSize; i++)
	{
		JsonNode entry;

		entry["name"].String() = parser.readString();

		parser.readNumber(); // unused aggression

		for (size_t i=0; i < GameConstants::PRIMARY_SKILLS; i++)
			entry["primarySkills"][PrimarySkill::names[i]].Float() = parser.readNumber();

		for (size_t i=0; i < GameConstants::PRIMARY_SKILLS; i++)
			entry["lowLevelChance"][PrimarySkill::names[i]].Float() = parser.readNumber();

		for (size_t i=0; i < GameConstants::PRIMARY_SKILLS; i++)
			entry["highLevelChance"][PrimarySkill::names[i]].Float() = parser.readNumber();

		for (size_t i=0; i < GameConstants::SKILL_QUANTITY; i++)
			entry["secondarySkills"][NSecondarySkill::names[i]].Float() = parser.readNumber();

		for(size_t i = 0; i < GameConstants::F_NUMBER; i++)
			entry["tavern"][ETownType::names[i]].Float() = parser.readNumber();

		parser.endLine();
		h3Data.push_back(entry);
	}
	return h3Data;
}

void CHeroClassHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(data);
	object->id = heroClasses.size();

	heroClasses.push_back(object);

	VLC->modh->identifiers.registerObject(scope, "heroClass", name, object->id);
}

void CHeroClassHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(data);
	object->id = index;

	assert(heroClasses[index] == nullptr); // ensure that this id was not loaded before
	heroClasses[index] = object;

	VLC->modh->identifiers.registerObject(scope, "heroClass", name, object->id);
}

std::vector<bool> CHeroClassHandler::getDefaultAllowed() const
{
	return std::vector<bool>(heroClasses.size(), true);
}

CHeroClassHandler::~CHeroClassHandler()
{
	BOOST_FOREACH(auto heroClass, heroClasses)
	{
		delete heroClass.get();
	}
}

CHeroHandler::~CHeroHandler()
{
	BOOST_FOREACH(auto hero, heroes)
		delete hero.get();
}

CHeroHandler::CHeroHandler()
{
	VLC->heroh = this;

	for (int i = 0; i < GameConstants::SKILL_QUANTITY; ++i)
	{
		VLC->modh->identifiers.registerObject("core", "skill", NSecondarySkill::names[i], i);
	}
	loadObstacles();
	loadTerrains();
	loadBallistics();
	loadExperience();
}

CHero * CHeroHandler::loadFromJson(const JsonNode & node)
{
	CHero * hero = new CHero;

	hero->sex = node["female"].Bool();
	hero->special = node["special"].Bool();

	hero->name        = node["texts"]["name"].String();
	hero->biography   = node["texts"]["biography"].String();
	hero->specName    = node["texts"]["specialty"]["name"].String();
	hero->specTooltip = node["texts"]["specialty"]["tooltip"].String();
	hero->specDescr   = node["texts"]["specialty"]["description"].String();

	hero->iconSpecSmall = node["images"]["specialtySmall"].String();
	hero->iconSpecLarge = node["images"]["specialtyLarge"].String();
	hero->portraitSmall = node["images"]["small"].String();
	hero->portraitLarge = node["images"]["large"].String();

	loadHeroArmy(hero, node);
	loadHeroSkills(hero, node);
	loadHeroSpecialty(hero, node);

	VLC->modh->identifiers.requestIdentifier("heroClass." + node["class"].String(),
	[=](si32 classID)
	{
		hero->heroClass = classes.heroClasses[classID];
	});

	return hero;
}

void CHeroHandler::loadHeroArmy(CHero * hero, const JsonNode & node)
{
	assert(node["army"].Vector().size() <= 3); // anything bigger is useless - army initialization uses up to 3 slots

	hero->initialArmy.resize(node["army"].Vector().size());

	for (size_t i=0; i< hero->initialArmy.size(); i++)
	{
		const JsonNode & source = node["army"].Vector()[i];

		hero->initialArmy[i].minAmount = source["min"].Float();
		hero->initialArmy[i].maxAmount = source["max"].Float();

		assert(hero->initialArmy[i].minAmount <= hero->initialArmy[i].maxAmount);

		VLC->modh->identifiers.requestIdentifier("creature." + source["creature"].String(), [=](si32 creature)
		{
			hero->initialArmy[i].creature = CreatureID(creature);
		});
	}
}

void CHeroHandler::loadHeroSkills(CHero * hero, const JsonNode & node)
{
	BOOST_FOREACH(const JsonNode &set, node["skills"].Vector())
	{
		int skillLevel = boost::range::find(NSecondarySkill::levels, set["level"].String()) - boost::begin(NSecondarySkill::levels);
		if (skillLevel < SecSkillLevel::LEVELS_SIZE)
		{
			size_t currentIndex = hero->secSkillsInit.size();
			hero->secSkillsInit.push_back(std::make_pair(-1, skillLevel));

			VLC->modh->identifiers.requestIdentifier("skill." + set["skill"].String(), [=](si32 id)
			{
				hero->secSkillsInit[currentIndex].first = SecondarySkill(id);
			});
		}
		else
		{
			logGlobal->errorStream() << "Unknown skill level: " <<set["level"].String();
		}
	}

	// spellbook is considered present if hero have "spellbook" entry even when this is an empty set (0 spells)
	hero->haveSpellBook = !node["spellbook"].isNull();

	BOOST_FOREACH(const JsonNode & spell, node["spellbook"].Vector())
	{
		if (spell.getType() == JsonNode::DATA_FLOAT) // for compatibility
		{
			hero->spells.insert(SpellID(spell.Float()));
		}
		else
		{
			VLC->modh->identifiers.requestIdentifier("spell." + spell.String(),
			[=](si32 spellID)
			{
				hero->spells.insert(SpellID(spellID));
			});
		}
	}
}

void CHeroHandler::loadHeroSpecialty(CHero * hero, const JsonNode & node)
{
	//deprecated, used only for original spciealties
	BOOST_FOREACH(const JsonNode &specialty, node["specialties"].Vector())
	{
		SSpecialtyInfo spec;

		spec.type = specialty["type"].Float();
		spec.val = specialty["val"].Float();
		spec.subtype = specialty["subtype"].Float();
		spec.additionalinfo = specialty["info"].Float();

		hero->spec.push_back(spec); //put a copy of dummy
	}
	//new format, using bonus system
	BOOST_FOREACH(const JsonNode &specialty, node["specialty"].Vector())
	{
		SSpecialtyBonus hs;
		hs.growsWithLevel = specialty["growsWithLevel"].Bool();
		BOOST_FOREACH (const JsonNode & bonus, specialty["bonuses"].Vector())
		{
			auto b = JsonUtils::parseBonus(bonus);
			hs.bonuses.push_back (b);
		}
		hero->specialty.push_back (hs); //now, how to get CGHeroInstance from it?
	}
}

void CHeroHandler::loadExperience()
{
	expPerLevel.push_back(0);
	expPerLevel.push_back(1000);
	expPerLevel.push_back(2000);
	expPerLevel.push_back(3200);
	expPerLevel.push_back(4600);
	expPerLevel.push_back(6200);
	expPerLevel.push_back(8000);
	expPerLevel.push_back(10000);
	expPerLevel.push_back(12200);
	expPerLevel.push_back(14700);
	expPerLevel.push_back(17500);
	expPerLevel.push_back(20600);
	expPerLevel.push_back(24320);
	expPerLevel.push_back(28784);
	expPerLevel.push_back(34140);
	while (expPerLevel[expPerLevel.size() - 1] > expPerLevel[expPerLevel.size() - 2])
	{
		int i = expPerLevel.size() - 1;
		expPerLevel.push_back (expPerLevel[i] + (expPerLevel[i] - expPerLevel[i-1]) * 1.2);
	}
	expPerLevel.pop_back();//last value is broken
}

void CHeroHandler::loadObstacles()
{
	auto loadObstacles = [](const JsonNode &node, bool absolute, std::map<int, CObstacleInfo> &out)
	{
		BOOST_FOREACH(const JsonNode &obs, node.Vector())
		{
			int ID = obs["id"].Float();
			CObstacleInfo & obi = out[ID];
			obi.ID = ID;
			obi.defName = obs["defname"].String();
			obi.width = obs["width"].Float();
			obi.height = obs["height"].Float();
			obi.allowedTerrains = obs["allowedTerrain"].convertTo<std::vector<ETerrainType> >();
			obi.allowedSpecialBfields = obs["specialBattlefields"].convertTo<std::vector<BFieldType> >();
			obi.blockedTiles = obs["blockedTiles"].convertTo<std::vector<si16> >();
			obi.isAbsoluteObstacle = absolute;
		}
	};

	const JsonNode config(ResourceID("config/obstacles.json"));
	loadObstacles(config["obstacles"], false, obstacles);
	loadObstacles(config["absoluteObstacles"], true, absoluteObstacles);
	//loadObstacles(config["moats"], true, moats);
}

/// convert h3-style ID (e.g. Gobin Wolf Rider) to vcmi (e.g. goblinWolfRider)
static std::string genRefName(std::string input)
{
	boost::algorithm::replace_all(input, " ", ""); //remove spaces
	input[0] = std::tolower(input[0]); // to camelCase
	return input;
}

void CHeroHandler::loadBallistics()
{
	CLegacyConfigParser ballParser("DATA/BALLIST.TXT");

	ballParser.endLine(); //header
	ballParser.endLine();

	do
	{
		ballParser.readString();
		ballParser.readString();

		CHeroHandler::SBallisticsLevelInfo bli;
		bli.keep   = ballParser.readNumber();
		bli.tower  = ballParser.readNumber();
		bli.gate   = ballParser.readNumber();
		bli.wall   = ballParser.readNumber();
		bli.shots  = ballParser.readNumber();
		bli.noDmg  = ballParser.readNumber();
		bli.oneDmg = ballParser.readNumber();
		bli.twoDmg = ballParser.readNumber();
		bli.sum    = ballParser.readNumber();
		ballistics.push_back(bli);
	}
	while (ballParser.endLine());
}

std::vector<JsonNode> CHeroHandler::loadLegacyData(size_t dataSize)
{
	heroes.resize(dataSize);
	std::vector<JsonNode> h3Data;
	h3Data.reserve(dataSize);

	CLegacyConfigParser specParser("DATA/HEROSPEC.TXT");
	CLegacyConfigParser bioParser("DATA/HEROBIOS.TXT");
	CLegacyConfigParser parser("DATA/HOTRAITS.TXT");

	parser.endLine(); //ignore header
	parser.endLine();

	specParser.endLine(); //ignore header
	specParser.endLine();

	for (int i=0; i<GameConstants::HEROES_QUANTITY; i++)
	{
		JsonNode heroData;

		heroData["texts"]["name"].String() = parser.readString();
		heroData["texts"]["biography"].String() = bioParser.readString();
		heroData["texts"]["specialty"]["name"].String() = specParser.readString();
		heroData["texts"]["specialty"]["tooltip"].String() = specParser.readString();
		heroData["texts"]["specialty"]["description"].String() = specParser.readString();

		for(int x=0;x<3;x++)
		{
			JsonNode armySlot;
			armySlot["min"].Float() = parser.readNumber();
			armySlot["max"].Float() = parser.readNumber();
			armySlot["creature"].String() = genRefName(parser.readString());

			heroData["army"].Vector().push_back(armySlot);
		}
		parser.endLine();
		specParser.endLine();
		bioParser.endLine();

		h3Data.push_back(heroData);
	}
	return h3Data;
}

void CHeroHandler::loadObject(std::string scope, std::string name, const JsonNode & data)
{
	auto object = loadFromJson(data);
	object->ID = heroes.size();
	object->imageIndex = heroes.size() + 8; // 2 special frames + some extra portraits

	heroes.push_back(object);

	VLC->modh->identifiers.registerObject(scope, "hero", name, object->ID);
}

void CHeroHandler::loadObject(std::string scope, std::string name, const JsonNode & data, size_t index)
{
	auto object = loadFromJson(data);
	object->ID = index;
	object->imageIndex = index;

	assert(heroes[index] == nullptr); // ensure that this id was not loaded before
	heroes[index] = object;

	VLC->modh->identifiers.registerObject(scope, "hero", name, object->ID);
}

ui32 CHeroHandler::level (ui64 experience) const
{
	return boost::range::upper_bound(expPerLevel, experience) - boost::begin(expPerLevel);
}

ui64 CHeroHandler::reqExp (ui32 level) const
{
	if(!level)
		return 0;

	if (level <= expPerLevel.size())
	{
		return expPerLevel[level-1];
	}
	else
	{
        logGlobal->warnStream() << "A hero has reached unsupported amount of experience";
		return expPerLevel[expPerLevel.size()-1];
	}
}

void CHeroHandler::loadTerrains()
{
	const JsonNode config(ResourceID("config/terrains.json"));

	terrCosts.reserve(GameConstants::TERRAIN_TYPES);
	BOOST_FOREACH(const std::string & name, GameConstants::TERRAIN_NAMES)
		terrCosts.push_back(config[name]["moveCost"].Float());
}

std::vector<bool> CHeroHandler::getDefaultAllowed() const
{
	// Look Data/HOTRAITS.txt for reference
	std::vector<bool> allowedHeroes;
	allowedHeroes.reserve(heroes.size());

	BOOST_FOREACH(const CHero * hero, heroes)
	{
		allowedHeroes.push_back(!hero->special);
	}

	return allowedHeroes;
}

std::vector<bool> CHeroHandler::getDefaultAllowedAbilities() const
{
	std::vector<bool> allowedAbilities;
	allowedAbilities.resize(GameConstants::SKILL_QUANTITY, true);
	return allowedAbilities;
}

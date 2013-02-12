#include "StdInc.h"
#include "CHeroHandler.h"

#include "CGeneralTextHandler.h"
#include "Filesystem/CResourceLoader.h"
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
	return EAlignment::EAlignment(VLC->townh->factions[faction].alignment);
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
			tlog1 << "Misplaced obstacle!\n";
		else
			ret.push_back(toBlock);
	}

	return ret;
}

bool CObstacleInfo::isAppropriate(ETerrainType::ETerrainType terrainType, int specialBattlefield /*= -1*/) const
{
	if(specialBattlefield != -1)
		return vstd::contains(allowedSpecialBfields, specialBattlefield);

	return vstd::contains(allowedTerrains, terrainType);
}

void CHeroClassHandler::load()
{
	CLegacyConfigParser parser("DATA/HCTRAITS.TXT");

	parser.endLine(); // header
	parser.endLine();

	do
	{
		CHeroClass * hc = new CHeroClass;

		hc->name             = parser.readString();
		hc->aggression       = parser.readNumber();
		hc->id = heroClasses.size();

		hc->primarySkillInitial   = parser.readNumArray<int>(GameConstants::PRIMARY_SKILLS);
		hc->primarySkillLowLevel  = parser.readNumArray<int>(GameConstants::PRIMARY_SKILLS);
		hc->primarySkillHighLevel = parser.readNumArray<int>(GameConstants::PRIMARY_SKILLS);

		hc->secSkillProbability   = parser.readNumArray<int>(GameConstants::SKILL_QUANTITY);

		for(int dd=0; dd<GameConstants::F_NUMBER; ++dd)
		{
			hc->selectionProbability[dd] = parser.readNumber();
		}

		VLC->modh->identifiers.requestIdentifier("faction." + ETownType::names[heroClasses.size()/2],
		[=](si32 faction)
		{
			hc->faction = faction;
		});

		heroClasses.push_back(hc);
		VLC->modh->identifiers.registerObject("heroClass." + GameConstants::HERO_CLASSES_NAMES[hc->id], hc->id);
	}
	while (parser.endLine() && !parser.isNextEntryEmpty());

	const JsonNode & heroGraphics = JsonNode(ResourceID("config/heroClasses.json"));

	for (size_t i=0; i<heroClasses.size(); i++)
	{
		const JsonNode & battle = heroGraphics["heroBattleAnim"].Vector()[i/2];

		heroClasses[i]->imageBattleFemale = battle["female"].String();
		heroClasses[i]->imageBattleMale = battle["male"].String();

		const JsonNode & map = heroGraphics["heroMapAnim"].Vector()[i];

		heroClasses[i]->imageMapMale = map.String();
		heroClasses[i]->imageMapFemale = map.String();
	}
}

void CHeroClassHandler::load(const JsonNode & classes)
{
	BOOST_FOREACH(auto & entry, classes.Struct())
	{
		if (!entry.second.isNull()) // may happens if mod removed creature by setting json entry to null
		{
			CHeroClass * heroClass = loadClass(entry.second);
			heroClass->identifier = entry.first;
			heroClass->id = heroClasses.size();

			heroClasses.push_back(heroClass);
			tlog5 << "Added hero class: " << entry.first << "\n";
			VLC->modh->identifiers.registerObject("heroClass." + heroClass->identifier, heroClass->id);
		}
	}
}

CHeroClass *CHeroClassHandler::loadClass(const JsonNode & node)
{
	CHeroClass * heroClass = new CHeroClass;

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
}

void CHeroHandler::load(const JsonNode & input)
{
	BOOST_FOREACH(auto & entry, input.Struct())
	{
		if (!entry.second.isNull()) // may happens if mod removed creature by setting json entry to null
		{
			CHero * hero = loadHero(entry.second);
			hero->ID = heroes.size();

			heroes.push_back(hero);
			tlog5 << "Added hero: " << entry.first << "\n";
			VLC->modh->identifiers.registerObject("hero." + entry.first, hero->ID);
		}
	}
}

CHero * CHeroHandler::loadHero(const JsonNode & node)
{
	CHero * hero = new CHero;

	hero->name        = node["texts"]["name"].String();
	hero->biography   = node["texts"]["biography"].String();
	hero->specName    = node["texts"]["specialty"]["name"].String();
	hero->specTooltip = node["texts"]["specialty"]["tooltip"].String();
	hero->specDescr   = node["texts"]["specialty"]["description"].String();

	hero->imageIndex = node["images"]["index"].Float();
	hero->iconSpecSmall = node["images"]["specialtySmall"].String();
	hero->iconSpecLarge = node["images"]["specialtyLarge"].String();
	hero->portraitSmall = node["images"]["small"].String();
	hero->portraitLarge = node["images"]["large"].String();

	assert(node["army"].Vector().size() <= 3); // anything bigger is useless - army initialization uses up to 3 slots
	hero->initialArmy.resize(node["army"].Vector().size());

	for (size_t i=0; i< hero->initialArmy.size(); i++)
	{
		const JsonNode & source = node["army"].Vector()[i];

		hero->initialArmy[i].minAmount = source["min"].Float();
		hero->initialArmy[i].maxAmount = source["max"].Float();

		assert(hero->initialArmy[i].minAmount <= hero->initialArmy[i].maxAmount);

		VLC->modh->identifiers.requestIdentifier(std::string("creature.") + source["creature"].String(), [=](si32 creature)
		{
			hero->initialArmy[i].creature = CreatureID(creature);
		});
	}

	loadHeroJson(hero, node);
	return hero;
}

void CHeroHandler::loadHeroJson(CHero * hero, const JsonNode & node)
{
	// sex: 0=male, 1=female
	hero->sex = node["female"].Bool();
	hero->special = node["special"].Bool();

	BOOST_FOREACH(const JsonNode &set, node["skills"].Vector())
	{
		SecondarySkill skillID = SecondarySkill(
			boost::range::find(NSecondarySkill::names,  set["skill"].String()) - boost::begin(NSecondarySkill::names));
		int skillLevel = boost::range::find(NSecondarySkill::levels, set["level"].String()) - boost::begin(NSecondarySkill::levels);

		hero->secSkillsInit.push_back(std::make_pair(skillID, skillLevel));
	}

	BOOST_FOREACH(const JsonNode & spell, node["spellbook"].Vector())
	{
		hero->spells.insert(SpellID(spell.Float()));
	}

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

	VLC->modh->identifiers.requestIdentifier("heroClass." + node["class"].String(),
	[=](si32 classID)
	{
		hero->heroClass = classes.heroClasses[classID];
	});
}

void CHeroHandler::load()
{
	for (int i = 0; i < GameConstants::SKILL_QUANTITY; ++i)
	{
		VLC->modh->identifiers.registerObject("skill." + NSecondarySkill::names[i], i);
	}
	classes.load();
	loadHeroes();
	loadHeroTexts();
	loadObstacles();
	loadTerrains();
	loadBallistics();
	loadExperience();
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
			obi.allowedTerrains = obs["allowedTerrain"].convertTo<std::vector<ETerrainType::ETerrainType> >();
			obi.allowedSpecialBfields = obs["specialBattlefields"].convertTo<std::vector<BFieldType::BFieldType> >();
			obi.blockedTiles = obs["blockedTiles"].convertTo<std::vector<si16> >();
			obi.isAbsoluteObstacle = absolute;
		}
	};

	const JsonNode config(ResourceID("config/obstacles.json"));
	loadObstacles(config["obstacles"], false, obstacles);
	loadObstacles(config["absoluteObstacles"], true, absoluteObstacles);
	//loadObstacles(config["moats"], true, moats);
}

void CHeroHandler::loadHeroes()
{
	VLC->heroh = this;
	CLegacyConfigParser parser("DATA/HOTRAITS.TXT");

	parser.endLine(); //ignore header
	parser.endLine();

	for (int i=0; i<GameConstants::HEROES_QUANTITY; i++)
	{
		CHero * hero = new CHero;
		hero->name = parser.readString();

		hero->initialArmy.resize(3);
		for(int x=0;x<3;x++)
		{
			hero->initialArmy[x].minAmount = parser.readNumber();
			hero->initialArmy[x].maxAmount = parser.readNumber();

			std::string refName = parser.readString();
			boost::algorithm::replace_all(refName, " ", ""); //remove spaces
			refName[0] = std::tolower(refName[0]); // to camelCase
			VLC->modh->identifiers.requestIdentifier(std::string("creature.") + refName, [=](si32 creature)
			{
				hero->initialArmy[x].creature = CreatureID(creature);
			});
		}
		parser.endLine();

		hero->ID = heroes.size();
		hero->imageIndex = hero->ID;
		heroes.push_back(hero);
	}

	// Load heroes information
	const JsonNode config(ResourceID("config/heroes.json"));
	BOOST_FOREACH(const JsonNode &hero, config["heroes"].Vector())
	{
		loadHeroJson(heroes[hero["id"].Float()], hero);
	}
}

void CHeroHandler::loadHeroTexts()
{
	CLegacyConfigParser parser("DATA/HEROSPEC.TXT");
	CLegacyConfigParser bioParser("DATA/HEROBIOS.TXT");

	//skip header
	parser.endLine();
	parser.endLine();

	int i=0;
	do
	{
		CHero * hero = heroes[i++];
		hero->specName    = parser.readString();
		hero->specTooltip = parser.readString();
		hero->specDescr   = parser.readString();
		hero->biography   = bioParser.readString();
	}
	while (parser.endLine() && bioParser.endLine() && heroes.size() > i);
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
		tlog3 << "A hero has reached unsupported amount of experience\n";
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

std::vector<bool> CHeroHandler::getDefaultAllowedHeroes() const
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

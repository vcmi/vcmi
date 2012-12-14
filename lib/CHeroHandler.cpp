#include "StdInc.h"
#include "CHeroHandler.h"

#include "CGeneralTextHandler.h"
#include "Filesystem/CResourceLoader.h"
#include "VCMI_Lib.h"
#include "JsonNode.h"
#include "StringConstants.h"
#include "BattleHex.h"
#include "CModHandler.h"

/*
 * CHeroHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CHeroClass::CHeroClass()
{
}
CHeroClass::~CHeroClass()
{
}
int CHeroClass::chooseSecSkill(const std::set<int> & possibles) const //picks secondary skill out from given possibilities
{
	if(possibles.size()==1)
		return *possibles.begin();
	int totalProb = 0;
	for(std::set<int>::const_iterator i=possibles.begin(); i!=possibles.end(); i++)
	{
		totalProb += secSkillProbability[*i];
	}
	int ran = rand()%totalProb;
	for(std::set<int>::const_iterator i=possibles.begin(); i!=possibles.end(); i++)
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

bool CObstacleInfo::isAppropriate(int terrainType, int specialBattlefield /*= -1*/) const
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
}

void CHeroClassHandler::load(const JsonNode & classes)
{
	//TODO
}

void CHeroClassHandler::loadClass(const JsonNode & heroClass)
{
	//TODO
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
{}

void CHeroHandler::load()
{
	classes.load();
	loadHeroes();
	loadObstacles();
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
			obi.allowedTerrains = obs["allowedTerrain"].convertTo<std::vector<ui8> >();
			obi.allowedSpecialBfields = obs["specialBattlefields"].convertTo<std::vector<ui8> >();
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

		for(int x=0;x<3;x++)
		{
			hero->initialArmy[x].minAmount = parser.readNumber();
			hero->initialArmy[x].maxAmount = parser.readNumber();

			std::string refName = parser.readString();
			boost::algorithm::replace_all(refName, " ", ""); //remove spaces
			VLC->modh->identifiers.requestIdentifier(std::string("creature.") + refName, [=](si32 creature)
			{
				hero->initialArmy[x].creature = creature;
			});
		}
		parser.endLine();

		hero->ID = heroes.size();
		heroes.push_back(hero);
	}

	// Load heroes information
	const JsonNode config(ResourceID("config/heroes.json"));
	BOOST_FOREACH(const JsonNode &hero, config["heroes"].Vector()) {

		CHero * currentHero = heroes[hero["id"].Float()];
		const JsonNode *value;

		// sex: 0=male, 1=female
		currentHero->sex = !!hero["female"].Bool();

		BOOST_FOREACH(const JsonNode &set, hero["skill_set"].Vector()) {
			currentHero->secSkillsInit.push_back(std::make_pair(set["skill"].Float(), set["level"].Float()));
		}

		value = &hero["spell"];
		if (!value->isNull()) {
			currentHero->startingSpell = value->Float();
		}

		BOOST_FOREACH(const JsonNode &specialty, hero["specialties"].Vector())
		{
			SSpecialtyInfo dummy;

			dummy.type = specialty["type"].Float();
			dummy.val = specialty["val"].Float();
			dummy.subtype = specialty["subtype"].Float();
			dummy.additionalinfo = specialty["info"].Float();

			currentHero->spec.push_back(dummy); //put a copy of dummy
		}

		VLC->modh->identifiers.requestIdentifier("heroClass." + hero["class"].String(),
		[=](si32 classID)
		{
			currentHero->heroClass = classes.heroClasses[classID];
		});
	}

	loadTerrains();
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
	int i;
	if (experience <= expPerLevel.back())
	{
		for (i = expPerLevel.size()-1; experience < expPerLevel[i]; i--);
		return i + 1;
	}
	else
	{
		i = expPerLevel.size() - 1;
		while (experience > reqExp (i))
			i++;
		return i;
	}
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

std::vector<ui8> CHeroHandler::getDefaultAllowedHeroes() const
{
	// Look Data/HOTRAITS.txt for reference
	std::vector<ui8> allowedHeroes;
	allowedHeroes.resize(156, 1);
	for(int i = 145; i < 156; ++i)
	{
		allowedHeroes[i] = 0;
	}
	allowedHeroes[4] = 0;
	allowedHeroes[25] = 0;
	return allowedHeroes;
}

CHero::CHero()
{
	startingSpell = -1;
	sex = 0xff;
}

CHero::~CHero()
{

}

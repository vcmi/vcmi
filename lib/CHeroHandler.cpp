#include "StdInc.h"
#include "CHeroHandler.h"

#include "CGeneralTextHandler.h"
#include "Filesystem/CResourceLoader.h"
#include "VCMI_Lib.h"
#include "JsonNode.h"
#include "GameConstants.h"
#include "BattleHex.h"

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
	skillLimit = 8;
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
		totalProb += proSec[*i];
	}
	int ran = rand()%totalProb;
	for(std::set<int>::const_iterator i=possibles.begin(); i!=possibles.end(); i++)
	{
		ran -= proSec[*i];
		if(ran<0)
			return *i;
	}
	throw std::runtime_error("Cannot pick secondary skill!");
}

EAlignment::EAlignment CHeroClass::getAlignment()
{
	return (EAlignment::EAlignment)alignment;
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

CHeroHandler::~CHeroHandler()
{
	for (int i = 0; i < heroes.size(); i++)
		heroes[i].dellNull();

	for (int i = 0; i < heroClasses.size(); i++)
		delete heroClasses[i];
}

CHeroHandler::CHeroHandler()
{}

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
			obi.allowedTerrains = obs["allowedTerrain"].StdVector<ui8>();
			obi.allowedSpecialBfields = obs["specialBattlefields"].StdVector<ui8>();
			obi.blockedTiles = obs["blockedTiles"].StdVector<si16>();
			obi.isAbsoluteObstacle = absolute;
		}
	};

	const JsonNode config(ResourceID("config/obstacles.json"));
	loadObstacles(config["obstacles"], false, obstacles);
	loadObstacles(config["absoluteObstacles"], true, absoluteObstacles);
	//loadObstacles(config["moats"], true, moats);
}

void CHeroHandler::loadPuzzleInfo()
{
	const JsonNode config(ResourceID("config/puzzle_map.json"));

	int faction = 0;

	BOOST_FOREACH(const JsonNode &puzzle, config["puzzles"].Vector()) 
	{
		int idx = 0;

		BOOST_FOREACH(const JsonNode &piece, puzzle.Vector()) 
		{
			SPuzzleInfo spi;

			spi.x = piece["x"].Float();
			spi.y = piece["y"].Float();
			spi.whenUncovered = piece["order"].Float();
			spi.number = idx;
				
			// filename calculation
			std::ostringstream suffix;
			suffix << std::setfill('0') << std::setw(2);
			suffix << idx << ".BMP";

			static const std::string factionToInfix[GameConstants::F_NUMBER] = {"CAS", "RAM", "TOW", "INF", "NEC", "DUN", "STR", "FOR", "ELE"};
			spi.filename = "PUZ" + factionToInfix[faction] + suffix.str();

			puzzleInfo[faction].push_back(spi);

			idx ++;
		}

		assert(idx == PUZZLES_PER_FACTION);

		faction ++;
	}

	assert(faction == GameConstants::F_NUMBER);
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
			hero->lowStack[x] = parser.readNumber();
			hero->highStack[x] = parser.readNumber();
			hero->refTypeStack[x] = parser.readString();
			boost::algorithm::replace_all(hero->refTypeStack[x], " ", ""); //remove spaces
		}
		parser.endLine();

		hero->ID = heroes.size();
		heroes.push_back(hero);
	}

	// Load heroes information
	const JsonNode config(ResourceID("config/heroes.json"));
	BOOST_FOREACH(const JsonNode &hero, config["heroes"].Vector()) {
		int hid = hero["id"].Float();
		const JsonNode *value;

		// sex: 0=male, 1=female
		heroes[hid]->sex = !!hero["female"].Bool();
		heroes[hid]->heroType = CHero::EHeroClasses((int)hero["class"].Float());

		BOOST_FOREACH(const JsonNode &set, hero["skill_set"].Vector()) {
			heroes[hid]->secSkillsInit.push_back(std::make_pair(set["skill"].Float(), set["level"].Float()));
		}

		value = &hero["spell"];
		if (!value->isNull()) {
			heroes[hid]->startingSpell = value->Float();
		}

		BOOST_FOREACH(const JsonNode &specialty, hero["specialties"].Vector())
		{
			SSpecialtyInfo dummy;

			dummy.type = specialty["type"].Float();
			dummy.val = specialty["val"].Float();
			dummy.subtype = specialty["subtype"].Float();
			dummy.additionalinfo = specialty["info"].Float();

			heroes[hid]->spec.push_back(dummy); //put a copy of dummy
		}
	}

	loadHeroClasses();
	initHeroClasses();
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

void CHeroHandler::loadHeroClasses()
{
	CLegacyConfigParser parser("DATA/HCTRAITS.TXT");

	parser.endLine(); // header
	parser.endLine();

	do
	{
		CHeroClass * hc = new CHeroClass;
		hc->alignment = heroClasses.size() / 6;

		hc->name             = parser.readString();
		hc->aggression       = parser.readNumber();
		hc->initialAttack    = parser.readNumber();
		hc->initialDefence   = parser.readNumber();
		hc->initialPower     = parser.readNumber();
		hc->initialKnowledge = parser.readNumber();

		hc->primChance.resize(GameConstants::PRIMARY_SKILLS);
		for(int x=0; x<GameConstants::PRIMARY_SKILLS; ++x)
		{
			hc->primChance[x].first = parser.readNumber();
		}
		for(int x=0; x<GameConstants::PRIMARY_SKILLS; ++x)
		{
			hc->primChance[x].second = parser.readNumber();
		}

		hc->proSec.resize(GameConstants::SKILL_QUANTITY);
		for(int dd=0; dd<GameConstants::SKILL_QUANTITY; ++dd)
		{
			hc->proSec[dd] = parser.readNumber();
		}

		for(int dd=0; dd<ARRAY_COUNT(hc->selectionProbability); ++dd)
		{
			hc->selectionProbability[dd] = parser.readNumber();
		}

		heroClasses.push_back(hc);
	}
	while (parser.endLine() && !parser.isNextEntryEmpty());
}

void CHeroHandler::initHeroClasses()
{
	for(int gg=0; gg<heroes.size(); ++gg)
	{
		heroes[gg]->heroClass = heroClasses[heroes[gg]->heroType];
	}

	loadTerrains();
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
	int faction = 0;
	const JsonNode config(ResourceID("config/terrains.json"));

	nativeTerrains.resize(GameConstants::F_NUMBER);

	BOOST_FOREACH(const JsonNode &terrain, config["terrains"].Vector()) {

		BOOST_FOREACH(const JsonNode &cost, terrain["costs"].Vector()) {
			int curCost = cost.Float();

			heroClasses[2*faction]->terrCosts.push_back(curCost);
			heroClasses[2*faction+1]->terrCosts.push_back(curCost);
		}

		nativeTerrains[faction] = terrain["native"].Float();

		faction ++;
	}

	assert(faction == GameConstants::F_NUMBER);
}

CHero::CHero()
{
	startingSpell = -1;
	sex = 0xff;
}

CHero::~CHero()
{

}

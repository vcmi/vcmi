#include "StdInc.h"
#include "CHeroHandler.h"

#include "CLodHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/JsonNode.h"
#include "GameConstants.h"
#include <boost/version.hpp>
#include "BattleHex.h"

extern CLodHandler * bitmaph;
void loadToIt(std::string &dest, const std::string &src, int &iter, int mode);
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


	const JsonNode config(GameConstants::DATA_DIR + "/config/obstacles.json");
	loadObstacles(config["obstacles"], false, obstacles);
	loadObstacles(config["absoluteObstacles"], true, absoluteObstacles);
}

void CHeroHandler::loadPuzzleInfo()
{
	const JsonNode config(GameConstants::DATA_DIR + "/config/puzzle_map.json");

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
	std::string buf = bitmaph->getTextFile("HOTRAITS.TXT");
	int it=0;
	std::string dump;
	for(int i=0; i<2; ++i)
	{
		loadToIt(dump,buf,it,3);
	}

	int numberOfCurrentClassHeroes = 0;
	int currentClass = 0;
	int additHero = 0;
	CHero::EHeroClasses addTab[12];
	addTab[0] = CHero::KNIGHT;
	addTab[1] = CHero::WITCH;
	addTab[2] = CHero::KNIGHT;
	addTab[3] = CHero::WIZARD;
	addTab[4] = CHero::RANGER;
	addTab[5] = CHero::BARBARIAN;
	addTab[6] = CHero::DEATHKNIGHT;
	addTab[7] = CHero::WARLOCK;
	addTab[8] = CHero::KNIGHT;
	addTab[9] = CHero::WARLOCK;
	addTab[10] = CHero::BARBARIAN;
	addTab[11] = CHero::DEMONIAC;

	
	for (int i=0; i<GameConstants::HEROES_QUANTITY; i++)
	{
		CHero * nher = new CHero;
		if(currentClass<18)
		{
			nher->heroType = static_cast<CHero::EHeroClasses>(currentClass);
			++numberOfCurrentClassHeroes;
			if(numberOfCurrentClassHeroes==8)
			{
				numberOfCurrentClassHeroes = 0;
				++currentClass;
			}
		}
		else
		{
			nher->heroType = addTab[additHero++];
		}

		std::string pom ;
		loadToIt(nher->name,buf,it,4);

		for(int x=0;x<3;x++)
		{
			loadToIt(pom,buf,it,4);
			nher->lowStack[x] = atoi(pom.c_str());
			loadToIt(pom,buf,it,4);
			nher->highStack[x] = atoi(pom.c_str());
			loadToIt(nher->refTypeStack[x],buf,it,(x==2) ? (3) : (4));
			int hlp = nher->refTypeStack[x].find_first_of(' ',0);
			if(hlp>=0)
				nher->refTypeStack[x].replace(hlp,1,"");
		}
	
		nher->ID = heroes.size();
		heroes.push_back(nher);
	}

	// Load heroes information
	const JsonNode config(GameConstants::DATA_DIR + "/config/heroes.json");
	BOOST_FOREACH(const JsonNode &hero, config["heroes"].Vector()) {
		int hid = hero["id"].Float();
		const JsonNode *value;

		// sex: 0=male, 1=female
		heroes[hid]->sex = !!hero["female"].Bool();

		BOOST_FOREACH(const JsonNode &set, hero["skill_set"].Vector()) {
			heroes[hid]->secSkillsInit.push_back(std::make_pair(set["skill"].Float(), set["level"].Float()));
		}

		value = &hero["spell"];
		if (!value->isNull()) {
			heroes[hid]->startingSpell = value->Float();
		}

		value = &hero["specialties"];
		if (!value->isNull()) {
			BOOST_FOREACH(const JsonNode &specialty, value->Vector()) {
				SSpecialtyInfo dummy;

				dummy.type = specialty["type"].Float();
				dummy.val = specialty["val"].Float();
				dummy.subtype = specialty["subtype"].Float();
				dummy.additionalinfo = specialty["info"].Float();

				heroes[hid]->spec.push_back(dummy); //put a copy of dummy
			}
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

	//ballistics info
	buf = bitmaph->getTextFile("BALLIST.TXT");
	it = 0;
	for(int i=0; i<22; ++i)
	{
		loadToIt(dump,buf,it,4);
	}
	for(int lvl=0; lvl<4; ++lvl)
	{
		CHeroHandler::SBallisticsLevelInfo bli;
		si32 tempNum;
		loadToIt(tempNum,buf,it,4);
		bli.keep = tempNum;
		loadToIt(tempNum,buf,it,4);
		bli.tower = tempNum;
		loadToIt(tempNum,buf,it,4);
		bli.gate = tempNum;
		loadToIt(tempNum,buf,it,4);
		bli.wall = tempNum;
		loadToIt(tempNum,buf,it,4);
		bli.shots = tempNum;
		loadToIt(tempNum,buf,it,4);
		bli.noDmg = tempNum;
		loadToIt(tempNum,buf,it,4);
		bli.oneDmg = tempNum;
		loadToIt(tempNum,buf,it,4);
		bli.twoDmg = tempNum;
		loadToIt(tempNum,buf,it,4);
		bli.sum = tempNum;
		if(lvl!=3)
		{
			loadToIt(dump,buf,it,4);
		}
		ballistics.push_back(bli);
	}
}

void CHeroHandler::loadHeroClasses()
{
	std::istringstream str(bitmaph->getTextFile("HCTRAITS.TXT")); //we'll be reading from it
	const int BUFFER_SIZE = 5000;
	char buffer[BUFFER_SIZE+1];

	for(int i=0; i<3; ++i) str.getline(buffer, BUFFER_SIZE); //omitting rubbish


	for(int ss=0; ss<18; ++ss) //18 classes of hero (including conflux)
	{
		CHeroClass * hc = new CHeroClass;
		hc->alignment = ss / 6;

		char name[BUFFER_SIZE+1];
		str.get(name, BUFFER_SIZE, '\t');
		hc->name = name;
		//workaround for locale issue (different localisations use different decimal separator)
		int intPart,fracPart;
		str >> intPart;
		str.ignore();//ignore decimal separator
		str >> fracPart;
		hc->aggression = intPart + fracPart/100.0;
		
		str >> hc->initialAttack;
		str >> hc->initialDefence;
		str >> hc->initialPower;
		str >> hc->initialKnowledge;

		hc->primChance.resize(GameConstants::PRIMARY_SKILLS);
		for(int x=0; x<GameConstants::PRIMARY_SKILLS; ++x)
		{
			str >> hc->primChance[x].first;
		}
		for(int x=0; x<GameConstants::PRIMARY_SKILLS; ++x)
		{
			str >> hc->primChance[x].second;
		}

		hc->proSec.resize(GameConstants::SKILL_QUANTITY);
		for(int dd=0; dd<GameConstants::SKILL_QUANTITY; ++dd)
		{
			str >> hc->proSec[dd];
		}

		for(int dd=0; dd<ARRAY_COUNT(hc->selectionProbability); ++dd)
		{
			str >> hc->selectionProbability[dd];
		}

		heroClasses.push_back(hc);
		str.getline(buffer, BUFFER_SIZE); //removing end of line characters
	}
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
	const JsonNode config(GameConstants::DATA_DIR + "/config/terrains.json");

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

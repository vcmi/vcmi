#define VCMI_DLL
#include "../stdafx.h"
#include "CHeroHandler.h"
#include "CLodHandler.h"
#include "../lib/VCMI_Lib.h"
#include <iomanip>
#include <sstream>
#include <fstream>
#include <boost/spirit.hpp>
using namespace boost::spirit;

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
	throw std::string("Cannot pick secondary skill!");
}

EAlignment CHeroClass::getAlignment()
{
	return (EAlignment)alignment;
}

int CObstacleInfo::getWidth() const
{
	int ret = 1;
	int line = 1;
	for(int h=0; h<blockmap.size(); ++h)
	{
		int cur = - line/2;
		switch(blockmap[h])
		{
		case 'X' : case 'N':
			++cur;
			break;
		case 'L':
			if(cur > ret)
				ret = cur;
			++line;
			break;
		}
	}
	return ret;
}

int CObstacleInfo::getHeight() const
{
	int ret = 1;
	for(int h=0; h<blockmap.size(); ++h)
	{
		if(blockmap[h] == 'L')
		{
			++ret;
		}
	}
	return ret;
}

std::vector<int> CObstacleInfo::getBlocked(int hex) const
{
	std::vector<int> ret;
	int cur = hex; //currently browsed hex
	int curBeg = hex; //beginning of current line
	for(int h=0; h<blockmap.size(); ++h)
	{
		switch(blockmap[h])
		{
		case 'X':
			ret.push_back(cur);
			++cur;
			break;
		case 'L':
			if((cur/17)%2 == 0)
			{
				cur = curBeg + 17;
				curBeg = cur;
			}
			else
			{
				cur = curBeg + 16;
				curBeg = cur;
			}
			break;
		case 'N':
			++cur;
			break;
		}
	}
	return ret;
}

int CObstacleInfo::getMaxBlocked(int hex) const
{
	std::vector<int> blocked = getBlocked(hex);
	return *std::max_element(blocked.begin(), blocked.end());
}

CHeroHandler::~CHeroHandler()
{
	for (int i = 0; i < heroes.size(); i++)
		delete heroes[i];

	for (int i = 0; i < heroClasses.size(); i++)
		delete heroClasses[i];
}

CHeroHandler::CHeroHandler()
{}

void CHeroHandler::loadWallPositions()
{
	std::ifstream inp;
	inp.open(DATA_DIR "/config/wall_pos.txt", std::ios_base::in|std::ios_base::binary);
	if(!inp.is_open())
	{
		tlog1<<"missing file: config/wall_pos.txt"<<std::endl;
	}
	else
	{
		const int MAX_BUF = 2000;
		char buf[MAX_BUF+1];

		inp.getline(buf, MAX_BUF);
		std::string dump;
		for(int g=0; g<ARRAY_COUNT(wallPositions); ++g)
		{
			inp >> dump;
			for(int b=0; b<12; ++b)
			{
				std::pair<int, int> pt;
				inp >> pt.first;
				inp >> pt.second;
				wallPositions[g].push_back(pt);
			}
		}
	}
	inp.close();
}

void CHeroHandler::loadObstacles()
{
	std::ifstream inp;
	inp.open(DATA_DIR "/config/obstacles.txt", std::ios_base::in|std::ios_base::binary);
	if(!inp.is_open())
	{
		tlog1<<"missing file: config/obstacles.txt"<<std::endl;
	}
	else
	{
		const int MAX_DUMP = 10000;
		char dump[MAX_DUMP+1];

		for(int i=0; i<8; ++i)
		{
			inp.getline(dump, MAX_DUMP);
		}
		while(true)
		{
			CObstacleInfo obi;
			inp>>obi.ID;
			if(obi.ID == -1) break;
			inp>>obi.defName;
			inp>>obi.blockmap;
			inp>>obi.allowedTerrains;
			inp>>obi.posShift.first;
			inp>>obi.posShift.second;
			obstacles[obi.ID] = obi;
		}
		inp.close();
	}
}

void CHeroHandler::loadPuzzleInfo()
{
	std::ifstream inp;
	inp.open(DATA_DIR "/config/puzzle_map.txt", std::ios_base::in|std::ios_base::binary);
	if(!inp.is_open())
	{
		tlog1<<"missing file: config/puzzle_map.txt"<<std::endl;
	}
	else
	{
		const int MAX_DUMP = 10000;
		char dump[MAX_DUMP+1];

		inp.getline(dump, MAX_DUMP);

		for(int fct = 0; fct < F_NUMBER; ++fct)
		{
			std::string dmp;
			inp >> dmp;

			for(int g=0; g<PUZZLES_PER_FACTION; ++g)
			{
				SPuzzleInfo spi;
				inp >> spi.x;
				inp >> spi.y;
				inp >> spi.whenUncovered;
				spi.number = g;
				
				//filename calculation
				std::ostringstream suffix;
				suffix << std::setfill('0') << std::setw(2);
				suffix << g << ".BMP";

				static const std::string factionToInfix[F_NUMBER] = {"CAS", "RAM", "TOW", "INF", "NEC", "DUN", "STR", "FOR", "ELE"};
				spi.filename = "PUZ" + factionToInfix[fct] + suffix.str();

				puzzleInfo[fct].push_back(spi);
			}
		}
		inp.close();
	}
}

void CHeroHandler::loadHeroes()
{
	VLC->heroh = this;
	int ID=0;
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

	
	for (int i=0; i<HEROES_QUANTITY; i++)
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
	//loading initial secondary skills
	{
		std::ifstream inp;
		inp.open(DATA_DIR "/config/heroes_sec_skills.txt", std::ios_base::in|std::ios_base::binary);
		if(!inp.is_open())
		{
			tlog1<<"missing file: config/heroes_sec_skills.txt"<<std::endl;
		}
		else
		{
			inp>>dump;
			int hid; //ID of currently read hero
			int secQ; //number of secondary abilities
			while(true)
			{
				inp>>hid;
				if(hid == -1)
					break;
				inp>>secQ;
				for(int g=0; g<secQ; ++g)
				{
					int a, b;
					inp>>a; inp>>b;
					heroes[hid]->secSkillsInit.push_back(std::make_pair(a, b));
				}
			}
			inp.close();
		}
	}
	//initial skills loaded

	{
		std::ifstream inp;
		std::istringstream iss;
		dump.clear();
		inp.open(DATA_DIR "/config/hero_spells.txt");
		while(inp)
		{
			getline(inp, dump);
			if(!dump.size()  ||  dump[0] == '-')
				continue;
			iss.clear();
			iss.str(dump);
			int hid, sid;
			iss >> hid >> sid;
			heroes[hid]->startingSpell = sid;
		}
		inp.close();
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
	{
		it = 0;
		std::ifstream inp;
		dump.clear();
		inp.open(DATA_DIR "/config/specials.txt"); //loading hero specials
		assert(inp);
		specialInfo dummy;
		si32 hid;
		inp.ignore(100, '\n');
		for (int i = 0; i < 175; ++i)
		{
			inp >> hid;
			inp >> dummy.type;
			inp >> dummy.val;
			inp >> dummy.subtype;
			inp >> dummy.additionalinfo;
			heroes[hid]->spec.push_back(dummy); //put a copy of dummy
		}
		inp.close();
	}
}
void CHeroHandler::loadHeroClasses()
{
	std::istringstream str(bitmaph->getTextFile("HCTRAITS.TXT")); //we'll be reading from it
	const int BUFFER_SIZE = 5000;
	char buffer[BUFFER_SIZE+1];

	for(int i=0; i<3; ++i) str.getline(buffer, BUFFER_SIZE); //omiting rubbish


	for(int ss=0; ss<18; ++ss) //18 classes of hero (including conflux)
	{
		CHeroClass * hc = new CHeroClass;
		hc->alignment = ss / 6;

		char name[BUFFER_SIZE+1];
		str.get(name, BUFFER_SIZE, '\t');
		hc->name = name;
		str >> hc->aggression;
		str >> hc->initialAttack;
		str >> hc->initialDefence;
		str >> hc->initialPower;
		str >> hc->initialKnowledge;

		hc->primChance.resize(PRIMARY_SKILLS);
		for(int x=0; x<PRIMARY_SKILLS; ++x)
		{
			str >> hc->primChance[x].first;
		}
		for(int x=0; x<PRIMARY_SKILLS; ++x)
		{
			str >> hc->primChance[x].second;
		}

		hc->proSec.resize(SKILL_QUANTITY);
		for(int dd=0; dd<SKILL_QUANTITY; ++dd)
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
	initTerrainCosts();
	loadNativeTerrains();
}

unsigned int CHeroHandler::level (ui64 experience)
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

ui64 CHeroHandler::reqExp (unsigned int level)
{
	if(!level)
		return 0;

	if (level <= expPerLevel.size())
	{
		return expPerLevel[level-1];
	}
	else
	{
		while (level > expPerLevel.size())
		{
			int i = expPerLevel.size() - 1;
			expPerLevel.push_back (expPerLevel[i] + (expPerLevel[i] - expPerLevel[i-1]) * 1.2);
		}
		return expPerLevel[level-1];
	}
}

void CHeroHandler::initTerrainCosts()
{
	std::ifstream inp;
	inp.open(DATA_DIR "/config/TERCOSTS.TXT", std::ios_base::in|std::ios_base::binary);

	if(!inp.is_open())
	{
		tlog1 << "Error while opening config/TERCOSTS.TXT file!" << std::endl;
	}

	int tynum;
	inp>>tynum;
	for(int i=0; i<2*tynum; i+=2)
	{
		int catNum;
		inp>>catNum;
		for(int k=0; k<catNum; ++k)
		{
			int curCost;
			inp>>curCost;
			heroClasses[i]->terrCosts.push_back(curCost);
			heroClasses[i+1]->terrCosts.push_back(curCost);
		}
	}
	inp.close();
}

void CHeroHandler::loadNativeTerrains()
{
	std::ifstream inp;
	inp.open(DATA_DIR "/config/native_terrains.txt", std::ios_base::in|std::ios_base::binary);

	if(!inp.is_open())
	{
		tlog1 << "Error while opening config/native_terrains.txt file!" << std::endl;
	}
	const int MAX_ELEM = 1000;
	char buf[MAX_ELEM+1];
	inp.getline(buf, MAX_ELEM);
	inp.getline(buf, MAX_ELEM);

	nativeTerrains.resize(F_NUMBER);
	for(int i=0; i<F_NUMBER; ++i)
	{
		int faction, terrain;
		inp >> faction;
		inp >> terrain;
		nativeTerrains[faction] = terrain;
	}
	inp.close();
}

CHero::CHero()
{
	startingSpell = -1;
}

CHero::~CHero()
{

}

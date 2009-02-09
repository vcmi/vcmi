#define VCMI_DLL
#include "../stdafx.h"
#include "CHeroHandler.h"
#include <sstream>
#include "CLodHandler.h"
#include "../lib/VCMI_Lib.h"
extern CLodHandler * bitmaph;
void loadToIt(std::string &dest, std::string &src, int &iter, int mode);

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

int CObstacleInfo::getWidth()
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

int CObstacleInfo::getHeight()
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

std::vector<int> CObstacleInfo::getBlocked(int hex)
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

CHeroHandler::~CHeroHandler()
{
	for (int i = 0; i < heroes.size(); i++)
		delete heroes[i];

	for (int i = 0; i < heroClasses.size(); i++)
		delete heroClasses[i];
}

CHeroHandler::CHeroHandler()
{}

void CHeroHandler::loadObstacles()
{
	std::ifstream inp;
	inp.open("config" PATHSEPARATOR "obstacles.txt", std::ios_base::in|std::ios_base::binary);
	if(!inp.is_open())
	{
		tlog1<<"missing file: config/heroes_sec_skills.txt"<<std::endl;
	}
	else
	{
		std::string dump;
		for(int i=0; i<99; ++i)
		{
			inp>>dump;
		}
		while(true)
		{
			CObstacleInfo obi;
			inp>>obi.ID;
			if(obi.ID == -1) break;
			inp>>obi.defName;
			inp>>obi.blockmap;
			inp>>obi.allowedTerrains;
			obstacles[obi.ID] = obi;
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
	EHeroClasses addTab[12];
	addTab[0] = HERO_KNIGHT;
	addTab[1] = HERO_WITCH;
	addTab[2] = HERO_KNIGHT;
	addTab[3] = HERO_WIZARD;
	addTab[4] = HERO_RANGER;
	addTab[5] = HERO_BARBARIAN;
	addTab[6] = HERO_DEATHKNIGHT;
	addTab[7] = HERO_WARLOCK;
	addTab[8] = HERO_KNIGHT;
	addTab[9] = HERO_WARLOCK;
	addTab[10] = HERO_BARBARIAN;
	addTab[11] = HERO_DEMONIAC;

	
	for (int i=0; i<HEROES_QUANTITY; i++)
	{
		CHero * nher = new CHero;
		if(currentClass<18)
		{
			nher->heroType = (EHeroClasses)currentClass;
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
	std::ifstream inp;
	inp.open("config" PATHSEPARATOR "heroes_sec_skills.txt", std::ios_base::in|std::ios_base::binary);
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
	//initial skills loaded

	loadHeroClasses();
	initHeroClasses();
	expPerLevel.push_back(0);
	expPerLevel.push_back(1000);
	expPerLevel.push_back(2000);
	expPerLevel.push_back(3200);
	expPerLevel.push_back(4500);
	expPerLevel.push_back(6000);
	expPerLevel.push_back(7700);
	expPerLevel.push_back(9000);
	expPerLevel.push_back(11000);
	expPerLevel.push_back(13200);
	expPerLevel.push_back(15500);
	expPerLevel.push_back(18500);
	expPerLevel.push_back(22100);
	expPerLevel.push_back(26420);
	expPerLevel.push_back(31604);

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
	std::string buf = bitmaph->getTextFile("HCTRAITS.TXT");
	int andame = buf.size();
	for(int y=0; y<andame; ++y)
		if(buf[y]==',')
			buf[y]='.';
	int i = 0; //buf iterator
	int hmcr = 0;
	for(i; i<andame; ++i) //omitting rubbish
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==2)
			break;
	}
	i+=2;
	for(int ss=0; ss<18; ++ss) //18 classes of hero (including conflux)
	{
		CHeroClass * hc = new CHeroClass;
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->name = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->aggression = atof(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->initialAttack = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->initialDefence = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->initialPower = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		hc->initialKnowledge = atoi(buf.substr(befi, i-befi).c_str());
		++i;

		hc->primChance.resize(PRIMARY_SKILLS);
		for(int x=0;x<PRIMARY_SKILLS;x++)
		{
			befi=i;
			for(i; i<andame; ++i)
			{
				if(buf[i]=='\t')
					break;
			}
			hc->primChance[x].first = atoi(buf.substr(befi, i-befi).c_str());
			++i;
		}
		for(int x=0;x<PRIMARY_SKILLS;x++)
		{
			befi=i;
			for(i; i<andame; ++i)
			{
				if(buf[i]=='\t')
					break;
			}
			hc->primChance[x].second = atoi(buf.substr(befi, i-befi).c_str());
			++i;
		}

		//CHero kkk = heroes[0];

		for(int dd=0; dd<SKILL_QUANTITY; ++dd)
		{
			befi=i;
			for(i; i<andame; ++i)
			{
				if(buf[i]=='\t')
					break;
			}
			int buff = atoi(buf.substr(befi, i-befi).c_str());
			++i;
			hc->proSec.push_back(buff);
		}

		for(int dd=0; dd<9; ++dd)
		{
			befi=i;
			for(i; i<andame; ++i)
			{
				if(buf[i]=='\t' || buf[i]=='\r')
					break;
			}
			hc->selectionProbability[dd] = atoi(buf.substr(befi, i-befi).c_str());
			++i;
		}
		++i;
		heroClasses.push_back(hc);
	}
}

void CHeroHandler::initHeroClasses()
{
	for(int gg=0; gg<heroes.size(); ++gg)
	{
		heroes[gg]->heroClass = heroClasses[heroes[gg]->heroType];
	}
	initTerrainCosts();
}

unsigned int CHeroHandler::level(unsigned int experience)
{
	int add=0;
	while(experience>=expPerLevel[expPerLevel.size()-1])
	{
		experience/=1.2;
		add+=1;
	}
	for(int i=expPerLevel.size()-1; i>=0; --i)
	{
		if(experience>=expPerLevel[i])
			return 1+i+add;
	}
	return -1;
}

unsigned int CHeroHandler::reqExp(unsigned int level)
{
	level-=1;
	if(level<expPerLevel.size())
		return expPerLevel[level];
	else
	{
		unsigned int exp = expPerLevel[expPerLevel.size()-1];
		level-=(expPerLevel.size()-1);
		while(level>0)
		{
			--level;
			exp*=1.2;
		}
		return exp;
	}
	return -1;
}

void CHeroHandler::initTerrainCosts()
{
	std::ifstream inp;
	inp.open("config" PATHSEPARATOR "TERCOSTS.TXT", std::ios_base::in|std::ios_base::binary);
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


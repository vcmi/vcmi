#define VCMI_DLL
#include "../stdafx.h"
#include "CCreatureHandler.h"
#include "CLodHandler.h"
#include <sstream>
#include <boost/assign/std/set.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign/std/list.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/find.hpp>
#include <boost/algorithm/string/replace.hpp>
#include "../lib/VCMI_Lib.h"
#include "../lib/CGameState.h"
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

using namespace boost::assign;
extern CLodHandler * bitmaph;

/*
 * CCreatureHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

static std::vector<int> getMindSpells()
{
	std::vector<int> ret;
	ret.push_back(50); //sorrow
	ret.push_back(59); //berserk
	ret.push_back(60); //hypnotize
	ret.push_back(61); //forgetfulness
	ret.push_back(62); //blind
	return ret;
}

CCreatureHandler::CCreatureHandler()
{
	VLC->creh = this;

	// Set the faction alignments to the defaults:
	// Good: Castle, Rampart, Tower	// Evil: Inferno, Necropolis, Dungeon
	// Neutral: Stronghold, Fortess, Conflux
	factionAlignments += 1, 1, 1, -1, -1, -1, 0, 0, 0;
	doubledCreatures +=  4, 14, 20, 28, 42, 44, 60, 70, 72, 85, 86, 100, 104; //according to Strategija

	allCreatures.description = "All creatures";
	creaturesOfLevel[0].description = "Creatures of unnormalized tier";
	for(int i = 1; i < ARRAY_COUNT(creaturesOfLevel); i++)
		creaturesOfLevel[i].description = "Creatures of tier " + boost::lexical_cast<std::string>(i);
}

int CCreature::getQuantityID(const int & quantity)
{
	if (quantity<5)
		return 0;
	if (quantity<10)
		return 1;
	if (quantity<20)
		return 2;
	if (quantity<50)
		return 3;
	if (quantity<100)
		return 4;
	if (quantity<250)
		return 5;
	if (quantity<500)
		return 5;
	if (quantity<1000)
		return 6;
	if (quantity<4000)
		return 7;
	return 8;
}

bool CCreature::isDoubleWide() const
{
	return doubleWide;
}

bool CCreature::isFlying() const
{
	return hasBonusOfType(Bonus::FLYING);
}

bool CCreature::isShooting() const
{
	return hasBonusOfType(Bonus::SHOOTER);
}

bool CCreature::isUndead() const
{
	return hasBonusOfType(Bonus::UNDEAD);
}

/**
 * Determines if the creature is of a good alignment.
 * @return true if the creture is good, false otherwise.
 */
bool CCreature::isGood () const
{
	return VLC->creh->isGood(faction);
}

/**
 * Determines if the creature is of an evil alignment.
 * @return true if the creature is evil, false otherwise.
 */
bool CCreature::isEvil () const
{
	return VLC->creh->isEvil(faction);
}

si32 CCreature::maxAmount(const std::vector<si32> &res) const //how many creatures can be bought
{
	int ret = 2147483645;
	int resAmnt = std::min(res.size(),cost.size());
	for(int i=0;i<resAmnt;i++)
		if(cost[i])
			ret = std::min(ret,(int)(res[i]/cost[i]));
	return ret;
}

CCreature::CCreature()
{
	doubleWide = false;
	nodeType = CBonusSystemNode::CREATURE;
}
void CCreature::addBonus(int val, int type, int subtype /*= -1*/)
{
	Bonus *added = new Bonus(Bonus::PERMANENT, type, Bonus::CREATURE_ABILITY, val, idNumber, subtype, Bonus::BASE_NUMBER);
	addNewBonus(added);
}
// void CCreature::getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const
// {
// 	out.insert (VLC->creh->globalEffects);
// }
bool CCreature::isMyUpgrade(const CCreature *anotherCre) const
{
	//TODO upgrade of upgrade?
	return vstd::contains(upgrades, anotherCre->idNumber);
}

bool CCreature::valid() const
{
	return this == VLC->creh->creatures[idNumber];
}

std::string CCreature::nodeName() const
{
	return "\"" + namePl + "\"";
}

int readNumber(int & befi, int & i, int andame, std::string & buf) //helper function for void CCreatureHandler::loadCreatures() and loadUnitAnimInfo()
{
	befi=i;
	for(; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	std::string tmp = buf.substr(befi, i-befi);
	int ret = atoi(buf.substr(befi, i-befi).c_str());
	++i;
	return ret;
}

float readFloat(int & befi, int & i, int andame, std::string & buf) //helper function for void CCreatureHandler::loadUnitAnimInfo()
{
	befi=i;
	for(; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	std::string tmp = buf.substr(befi, i-befi);
	float ret = atof(buf.substr(befi, i-befi).c_str());
	++i;
	return ret;
}

/**
 * Determines if a faction is good.
 * @param ID of the faction.
 * @return true if the faction is good, false otherwise.
 */
bool CCreatureHandler::isGood (si8 faction) const
{
	return faction != -1 && factionAlignments[faction] == 1;
}

/**
 * Determines if a faction is evil.
 * @param ID of the faction.
 * @return true if the faction is evil, false otherwise.
 */
bool CCreatureHandler::isEvil (si8 faction) const
{
	return faction != -1 && factionAlignments[faction] == -1;
}

void CCreatureHandler::loadCreatures()
{
	notUsedMonsters += 122,124,126,128,145,146,147,148,149,160,161,162,163,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191;
	tlog5 << "\t\tReading config/cr_abils.txt and ZCRTRAIT.TXT" << std::endl;

	bool useCreAbilsFromZCRTRAIT = true;

	////////////reading cr_abils.txt ///////////////////
	std::ifstream abils(DATA_DIR "/config/cr_abils.txt", std::ios::in | std::ios::binary); //this file is not in lod
	const int MAX_LINE_SIZE = 1000;
	char abilLine[MAX_LINE_SIZE+1];
	for(int i=0; i<5; ++i) //removing 5 comment lines
	{
		abils.getline(abilLine, MAX_LINE_SIZE);
	}
	//reading first line (determining if we should use creature abilities from ZCRTRAIT.TXT)
	abils.getline(abilLine, MAX_LINE_SIZE);
	useCreAbilsFromZCRTRAIT = atoi(abilLine);

	////////////reading ZCRTRAIT.TXT ///////////////////
	std::string buf = bitmaph->getTextFile("ZCRTRAIT.TXT");
	int andame = buf.size();
	int i=0; //buf iterator
	int hmcr=0;
	for(; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==2)
			break;
	}
	i+=2;

	while(i<buf.size())
	{
		CCreature &ncre = *new CCreature;
		ncre.idNumber = creatures.size();
		ncre.cost.resize(RESOURCE_QUANTITY);
		ncre.level=0;

		int befi=i;
		for(; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.nameSing = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.namePl = buf.substr(befi, i-befi);
		++i;

		for(int v=0; v<7; ++v)
		{
			ncre.cost[v] = readNumber(befi, i, andame, buf);
		}
		ncre.fightValue = readNumber(befi, i, andame, buf);
		ncre.AIValue = readNumber(befi, i, andame, buf);
		ncre.growth = readNumber(befi, i, andame, buf);
		ncre.hordeGrowth = readNumber(befi, i, andame, buf);

		ncre.hitPoints = readNumber(befi, i, andame, buf);
		ncre.addBonus(ncre.hitPoints, Bonus::STACK_HEALTH);
		ncre.speed = readNumber(befi, i, andame, buf);
		ncre.addBonus(ncre.speed, Bonus::STACKS_SPEED);
		ncre.attack = readNumber(befi, i, andame, buf);
		ncre.addBonus(ncre.attack, Bonus::PRIMARY_SKILL, PrimarySkill::ATTACK);
		ncre.defence = readNumber(befi, i, andame, buf);
		ncre.addBonus(ncre.defence, Bonus::PRIMARY_SKILL, PrimarySkill::DEFENSE);
		ncre.damageMin = readNumber(befi, i, andame, buf); //not used anymore?
		ncre.addBonus(ncre.damageMin, Bonus::CREATURE_DAMAGE, 1);
		ncre.damageMax = readNumber(befi, i, andame, buf);
		ncre.addBonus(ncre.damageMax, Bonus::CREATURE_DAMAGE, 2);

		ncre.shots = readNumber(befi, i, andame, buf);
		ncre.spells = readNumber(befi, i, andame, buf);
		ncre.ammMin = readNumber(befi, i, andame, buf);
		ncre.ammMax = readNumber(befi, i, andame, buf);

		befi=i;
		for(; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.abilityText = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		ncre.abilityRefs = buf.substr(befi, i-befi);
		i+=2;
		if(useCreAbilsFromZCRTRAIT)
		{ //adding abilities from ZCRTRAIT.TXT
			if(boost::algorithm::find_first(ncre.abilityRefs, "DOUBLE_WIDE"))
				ncre.doubleWide = true;
			if(boost::algorithm::find_first(ncre.abilityRefs, "FLYING_ARMY"))
				ncre.addBonus(0, Bonus::FLYING);
			if(boost::algorithm::find_first(ncre.abilityRefs, "SHOOTING_ARMY"))
				ncre.addBonus(0, Bonus::SHOOTER);
			if(boost::algorithm::find_first(ncre.abilityRefs, "SIEGE_WEAPON"))
				ncre.addBonus(0, Bonus::SIEGE_WEAPON);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_two_attacks"))
				ncre.addBonus(1, Bonus::ADDITIONAL_ATTACK);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_free_attack"))
				ncre.addBonus(0, Bonus::BLOCKS_RETALIATION);
			if(boost::algorithm::find_first(ncre.abilityRefs, "IS_UNDEAD"))
				ncre.addBonus(0, Bonus::UNDEAD);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_no_melee_penalty"))
				ncre.addBonus(0, Bonus::NO_MELEE_PENALTY);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_jousting"))
				ncre.addBonus(0, Bonus::JOUSTING);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_raises_morale"))
			{
				ncre.addBonus(+1, Bonus::MORALE);;
				ncre.bonuses.back()->effectRange = Bonus::ONLY_ALLIED_ARMY;
			}
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_lowers_morale"))
			{
				ncre.addBonus(-1, Bonus::MORALE);;
				ncre.bonuses.back()->effectRange = Bonus::ONLY_ENEMY_ARMY;
			}
			if(boost::algorithm::find_first(ncre.abilityRefs, "KING_1"))
				ncre.addBonus(0, Bonus::KING1);
			if(boost::algorithm::find_first(ncre.abilityRefs, "KING_2"))
				ncre.addBonus(0, Bonus::KING2);
			if(boost::algorithm::find_first(ncre.abilityRefs, "KING_3"))
				ncre.addBonus(0, Bonus::KING3);
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_no_wall_penalty"))
				ncre.addBonus(0, Bonus::NO_WALL_PENALTY);
			if(boost::algorithm::find_first(ncre.abilityRefs, "CATAPULT"))
				ncre.addBonus(0, Bonus::CATAPULT);
			if(boost::algorithm::find_first(ncre.abilityRefs, "MULTI_HEADED"))
				ncre.addBonus(0, Bonus::ATTACKS_ALL_ADJACENT);

			if(boost::algorithm::find_first(ncre.abilityRefs, "IMMUNE_TO_MIND_SPELLS"))
			{
				std::vector<int> mindSpells = getMindSpells();
				for(int g=0; g<mindSpells.size(); ++g)
					ncre.addBonus(0, Bonus::SPELL_IMMUNITY, mindSpells[g]); //giants are immune to mind spells
			}
			if(boost::algorithm::find_first(ncre.abilityRefs, "IMMUNE_TO_FIRE_SPELLS"))
				ncre.addBonus(0, Bonus::FIRE_IMMUNITY);
			if(boost::algorithm::find_first(ncre.abilityRefs, "HAS_EXTENDED_ATTACK"))
				ncre.addBonus(0, Bonus::TWO_HEX_ATTACK_BREATH);;
		}

		if(ncre.nameSing!=std::string("") && ncre.namePl!=std::string(""))
		{
			ncre.idNumber = creatures.size();
			creatures.push_back(&ncre);
		}
	}
	
	////second part of reading cr_abils.txt////
	bool contReading = true;
	while(contReading) //main reading loop
	{
		abils.getline(abilLine, MAX_LINE_SIZE);
		std::istringstream reader(abilLine);
		char command;
		reader >> command;
		switch(command)
		{
		case '+': //add new ability
			{
				int creatureID;
				Bonus *nsf = new Bonus();
				si32 buf;
				std::string type;

				reader >> creatureID;
				reader >> type;

				std::map<std::string, int>::const_iterator it = bonusNameMap.find(type);
				CCreature *cre = creatures[creatureID];

				if (it == bonusNameMap.end()) 
				{
					if(type == "DOUBLE_WIDE")
						cre->doubleWide = true;
					else if(type == "ENEMY_MORALE_DECREASING")
					{
						cre->addBonus(-1, Bonus::MORALE);
						cre->bonuses.back()->effectRange = Bonus::ONLY_ENEMY_ARMY;
					}
					else if(type == "ENEMY_LUCK_DECREASING")
					{
						cre->addBonus(-1, Bonus::LUCK);
						cre->bonuses.back()->effectRange = Bonus::ONLY_ENEMY_ARMY;
					}
					else
						tlog1 << "Error: invalid type " << type << " in cr_abils.txt" << std::endl;
					break;
				}
				nsf->type = it->second;

				reader >> buf; nsf->val = buf;
				reader >> buf; nsf->subtype = buf;
				reader >> buf; nsf->additionalInfo = buf;
				nsf->source = Bonus::CREATURE_ABILITY;
				nsf->id = cre->idNumber;
				nsf->duration = Bonus::ONE_BATTLE;
				nsf->turnsRemain = 0;

				cre->addNewBonus(nsf);
				break;
			}
		case '-': //remove ability
			{
				int creatureID;
				std::string type;
				reader >> creatureID;
				reader >> type;
				std::map<std::string, int>::const_iterator it = bonusNameMap.find(type);
				if (it == bonusNameMap.end())
				{
					if(type == "DOUBLE_WIDE")
						creatures[creatureID]->doubleWide = false;
					else
						tlog1 << "Error: invalid type " << type << " in cr_abils.txt" << std::endl;
					break;
				}
				int typeNo = it->second;

				Bonus::BonusType ecf = static_cast<Bonus::BonusType>(typeNo);

				Bonus *b = creatures[creatureID]->getBonus(Selector::type(ecf));
				creatures[creatureID]->removeBonus(b);
				break;
			}
		case '0': //end reading
			{
				contReading = false;
				break;
			}
		default: //invalid command
			{
				tlog1 << "Parse error in file config/cr_abils.txt" << std::endl;
				break;
			}
		}
	}

	abils.close();

	tlog5 << "\t\tReading config/crerefnam.txt" << std::endl;
	//loading reference names
	std::ifstream ifs(DATA_DIR "/config/crerefnam.txt");
	int tempi;
	std::string temps;
	for (;;)
	{
		ifs >> tempi >> temps;
		if (tempi>=creatures.size())
			break;
		boost::assign::insert(nameToID)(temps,tempi);
		creatures[tempi]->nameRef=temps;
	}
	ifs.close();
	ifs.clear();

	tlog5 << "\t\tReading config/monsters.txt" << std::endl;
	ifs.open(DATA_DIR "/config/monsters.txt");
	{
		while(!ifs.eof())
		{
			int id, lvl;
			ifs >> id >> lvl;
			if(!ifs.good())
				break;
			CCreature *c = creatures[id];
			c->level = lvl;
			if(isbetw(lvl, 0, ARRAY_COUNT(creaturesOfLevel)))
				c->attachTo(&creaturesOfLevel[lvl]);
			else
				c->attachTo(&creaturesOfLevel[0]);
		}
	}

	BOOST_FOREACH(CBonusSystemNode &b, creaturesOfLevel)
		b.attachTo(&allCreatures);

	ifs.close();
	ifs.clear();

	tlog5 << "\t\tReading config/cr_factions.txt" << std::endl;
	ifs.open(DATA_DIR "/config/cr_factions.txt");
	while(!ifs.eof())
	{
		int id, fact;
		ifs >> id >> fact;
		creatures[id]->faction = fact;
	}
	ifs.close();
	ifs.clear();

	tlog5 << "\t\tReading config/cr_upgrade_list.txt" << std::endl;
	ifs.open(DATA_DIR "/config/cr_upgrade_list.txt");
	while(!ifs.eof())
	{
		int id, up;
		ifs >> id >> up;
		creatures[id]->upgrades.insert(up);
	}
	ifs.close();
	ifs.clear();

	//loading unit animation def names
	tlog5 << "\t\tReading config/CREDEFS.TXT" << std::endl;
	std::ifstream inp(DATA_DIR "/config/CREDEFS.TXT", std::ios::in | std::ios::binary); //this file is not in lod
	inp.seekg(0,std::ios::end); // na koniec
	int andame2 = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame2+1]; // allocate memory
	inp.read((char*)bufor, andame2); // read map file to buffer
	inp.close();
	bufor[andame2] = 0;
	buf = std::string(bufor);
	delete [] bufor;

	i = 0; //buf iterator
	hmcr = 0;
	for(; i<andame2; ++i) //omitting rubbish
	{
		if(buf[i]=='\r')
			break;
	}
	i+=2;
	tlog5 << "We have "<<creatures.size() << " creatures\n";
	for(int s=0; s<creatures.size(); ++s)
	{
		//tlog5 <<"\t\t\t" << s <<". Reading defname. \n";
		int befi=i;
		std::string rub;
		for(; i<andame2; ++i)
		{
			if(buf[i]==' ')
				break;
		}
		rub = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(; i<andame2; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		std::string defName = buf.substr(befi, i-befi);
		creatures[s]->animDefName = defName;
	}
	tlog5 << "\t\tReading CRANIM.TXT.txt" << std::endl;
	loadAnimationInfo();

	//loading id to projectile mapping

	tlog5 << "\t\tReading config/cr_shots.txt" << std::endl;
	std::ifstream inp2(DATA_DIR "/config/cr_shots.txt", std::ios::in | std::ios::binary); //this file is not in lod
	char dump [200];
	inp2.getline(dump, 200);
	while(true)
	{
		int id;
		std::string name;
		bool spin;

		inp2>>id;
		if(id == -1)
			break;
		inp2>>name;
		idToProjectile[id] = name;
		inp2>>spin;
		idToProjectileSpin[id] = spin;
	}
	inp2.close();

	//reading factionToTurretCreature

	tlog5 << "\t\tReading config/cr_to_turret.txt" << std::endl;
	std::ifstream inp3(DATA_DIR "/config/cr_to_turret.txt", std::ios::in | std::ios::binary); //this file is not in lod
	std::string dump2;
	inp3 >> dump2 >> dump2;
	for(int g=0; g<F_NUMBER; ++g)
	{
		inp3 >> factionToTurretCreature[g];
	}
	inp3.close();

	//reading creature ability names
	ifs.open(DATA_DIR "/config/bonusnames.txt");
	{
		std::string buf2, buf3, line;
		int i;
		std::map<std::string,int>::const_iterator it;
		getline(ifs, line); //skip 1st line
		while(!ifs.eof())
		{
			getline(ifs, buf, '\t');
			getline(ifs, buf2, '\t');
			getline(ifs, buf3);
			it = bonusNameMap.find(buf);
			if (it != bonusNameMap.end())
				stackBonuses[it->second] = std::pair<std::string, std::string>(buf2,buf3);
			else
				tlog2 << "Bonus " << buf << " not recognized, ingoring\n";
		}
	}
	ifs.close();

	if (STACK_EXP) 	//reading default stack experience bonuses
	{
		buf = bitmaph->getTextFile("CREXPBON.TXT");
		int it = 0;
		si32 creid = -1;
		Bonus b; //prototype with some default properties
		b.source = Bonus::STACK_EXPERIENCE;
		b.duration = Bonus::PERMANENT;
		b.valType = Bonus::ADDITIVE_VALUE;
		b.effectRange = Bonus::NO_LIMIT;
		b.additionalInfo = 0;
		BonusList bl;

		loadToIt (dump2, buf, it, 3); //ignore first line
		loadToIt (dump2, buf, it, 4); //ignore index

		loadStackExp(b, bl, buf, it);
		BOOST_FOREACH(Bonus * b, bl)
			addBonusForAllCreatures(b); //health bonus is common for all

		loadToIt (dump2, buf, it, 3); //crop comment
		for (i = 1; i < 7; ++i)
		{
			for (int j = 0; j < 4; ++j) //four modifiers common for tiers
			{
				loadToIt (dump2, buf, it, 4); //ignore index
				bl.clear();
				loadStackExp(b, bl, buf, it);
				BOOST_FOREACH(Bonus * b, bl)
					addBonusForTier(i, b);
				loadToIt (dump2, buf, it, 3); //crop comment
			}
		}
		for (int j = 0; j < 4; ++j) //tier 7
		{
			loadToIt (dump2, buf, it, 4); //ignore index
			bl.clear();
			loadStackExp(b, bl, buf, it);
			BOOST_FOREACH(Bonus * b, bl)
			{
				addBonusForTier(7, b);
				creaturesOfLevel[0].addNewBonus(b); //bonuses from level 7 are given to high-level creatures
			}
			loadToIt (dump2, buf, it, 3); //crop comment
		}
		do //parse everything that's left
		{
			loadToIt(creid, buf, it, 4); //get index
			b.id = creid; //id = this particular creature ID
			loadStackExp(b, creatures[creid]->bonuses, buf, it); //add directly to CCreature Node
			loadToIt (dump2, buf, it, 3); //crop comment
		} while (it < buf.size());

		//Calculate rank exp values, formula appears complicated bu no parsing needed
		expRanks.resize(8);
		int dif = 0;
		it = 8000; //ignore name of this variable
		expRanks[0].push_back(it);
		for (int j = 1; j < 10; ++j) //used for tiers 8-10, and all other probably
		{
			expRanks[0].push_back(expRanks[0][j-1] + it + dif);
			dif += it/5;
		}
		for (i = 1; i < 8; ++i)
		{
			dif = 0;
			it = 1000 * i;
			expRanks[i].push_back(it);
			for (int j = 1; j < 10; ++j)
			{
				expRanks[i].push_back(expRanks[i][j-1] + it + dif);
				dif += it/5;
			}
		}

		buf = bitmaph->getTextFile("CREXPMOD.TXT"); //could be hardcoded though, lots of useless info
		it = 0;
		loadToIt (dump2, buf, it, 3); //ignore first line

		maxExpPerBattle.resize(8);
		si32 val;
		for (i = 1; i < 8; ++i)
		{
			loadToIt (dump2, buf, it, 4); //index
			loadToIt (dump2, buf, it, 4); //float multiplier -> hardcoded
			loadToIt (dump2, buf, it, 4); //ignore upgrade mod? ->hardcoded
			loadToIt (dump2, buf, it, 4); //already calculated
			loadToIt (val, buf, it, 4);
			maxExpPerBattle[i] = (ui32)val;
			loadToIt (val, buf, it, 4); //11th level
			val += (si32)expRanks[i].back();
			expRanks[i].push_back((ui32)val);
			loadToIt (dump2, buf, it, 3); //crop comment
		}
		//skeleton gets exp penalty
			creatures[56].get()->addBonus(-50, Bonus::EXP_MULTIPLIER, -1);
			creatures[57].get()->addBonus(-50, Bonus::EXP_MULTIPLIER, -1);
		//exp for tier >7, rank 11
			expRanks[0].push_back(147000);
			expAfterUpgrade = 75; //percent
			maxExpPerBattle[0] = maxExpPerBattle[7];

	}//end of Stack Experience
	//experiment - add 100 to attack for creatures of tier 1
// 	Bonus *b = new Bonus(Bonus::PERMANENT, Bonus::PRIMARY_SKILL, Bonus::OTHER, +100, 0, 0);
// 	addBonusForTier(1, b);
}

void CCreatureHandler::loadAnimationInfo()
{
	std::string buf = bitmaph->getTextFile("CRANIM.TXT");
	int andame = buf.size();
	int i=0; //buf iterator
	int hmcr=0;
	for(; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==2)
			break;
	}
	i+=2;
	for(int dd=0; dd<creatures.size(); ++dd)
	{
		//tlog5 << "\t\t\tReading animation info for creature " << dd << std::endl;
		loadUnitAnimInfo(*creatures[dd], buf, i);
	}
	return;
}

void CCreatureHandler::loadUnitAnimInfo(CCreature & unit, std::string & src, int & i)
{
	int befi=i;

	unit.timeBetweenFidgets = readFloat(befi, i, src.size(), src);

	while(unit.timeBetweenFidgets == 0.0)
	{
		for(; i<src.size(); ++i)
		{
			if(src[i]=='\r')
				break;
		}
		i+=2;

		unit.timeBetweenFidgets = readFloat(befi, i, src.size(), src);
	}

	unit.walkAnimationTime = readFloat(befi, i, src.size(), src);
	unit.attackAnimationTime = readFloat(befi, i, src.size(), src);
	unit.flightAnimationDistance = readFloat(befi, i, src.size(), src);
	///////////////////////

	unit.upperRightMissleOffsetX = readNumber(befi, i, src.size(), src);
	unit.upperRightMissleOffsetY = readNumber(befi, i, src.size(), src);
	unit.rightMissleOffsetX = readNumber(befi, i, src.size(), src);
	unit.rightMissleOffsetY = readNumber(befi, i, src.size(), src);
	unit.lowerRightMissleOffsetX = readNumber(befi, i, src.size(), src);
	unit.lowerRightMissleOffsetY = readNumber(befi, i, src.size(), src);

	///////////////////////

	for(int jjj=0; jjj<12; ++jjj)
	{
		unit.missleFrameAngles[jjj] = readFloat(befi, i, src.size(), src);
	}

	unit.troopCountLocationOffset= readNumber(befi, i, src.size(), src);
	unit.attackClimaxFrame = readNumber(befi, i, src.size(), src);

	for(; i<src.size(); ++i)
	{
		if(src[i]=='\r')
			break;
	}
	i+=2;
}

void CCreatureHandler::loadStackExp(Bonus & b, BonusList & bl, std::string & src, int & it) //help function for parsing CREXPBON.txt
{
	std::string buf, mod;
	bool enable = false; //some bonuses are activated with values 2 or 1
	loadToIt(buf, src, it, 4);
	loadToIt(mod, src, it, 4);

	switch (buf[0])
	{
	case 'H':
		b.type = Bonus::STACK_HEALTH;
		b.valType = Bonus::PERCENT_TO_BASE;
		break;
	case 'A':
		b.type = Bonus::PRIMARY_SKILL;
		b.subtype = PrimarySkill::ATTACK;
		break;
	case 'D':
		b.type = Bonus::PRIMARY_SKILL;
		b.subtype = PrimarySkill::DEFENSE;
		break;
	case 'M': //Max damage
		b.type = Bonus::CREATURE_DAMAGE;
		b.subtype = 2;
		break;
	case 'm': //Min damage
		b.type = Bonus::CREATURE_DAMAGE;
		b.subtype = 1;
		break;
	case 'S':
		b.type = Bonus::STACKS_SPEED; break;

	case 'b':
		b.type = Bonus::ENEMY_DEFENCE_REDUCTION; break;
	case 'C':
		b.type = Bonus::CHANGES_SPELL_COST_FOR_ALLY; break;
	case 'e':
		b.type = Bonus::DOUBLE_DAMAGE_CHANCE; break;
	case 'g':
		b.type = Bonus::SPELL_DAMAGE_REDUCTION; break;
	case 'R':
		b.type = Bonus::ADDITIONAL_RETALIATION; break;

	case 'f': //on-off skill
		enable = true; //sometimes format is: 2 -> 0, 1 -> 1
		switch (mod[0])
		{
			case 'A':
				b.type = Bonus::ATTACKS_ALL_ADJACENT; break;
			case 'b':
				b.type = Bonus::RETURN_AFTER_STRIKE; break;
			case 'B':
				b.type = Bonus::TWO_HEX_ATTACK_BREATH; break;
			case 'c':
				b.type = Bonus::JOUSTING; break;
			case 'D':
				b.type = Bonus::ADDITIONAL_ATTACK; break;
			case 'f':
				b.type = Bonus::FEARLESS; break;
			case 'F':
				b.type = Bonus::FLYING; break;
			case 'm':
				b.type = Bonus::SELF_MORALE; break;
			case 'M':
				b.type = Bonus::NO_MORALE; break;
			case 'p': //Mind spells
			case 'P':
				{
					loadMindImmunity(b, bl, src, it);
					return;
				}
				return;
			case 'r': //TODO: Rebirth on/off? makes sense?
				break;
			case 'R':
				b.type = Bonus::BLOCKS_RETALIATION; break;
			case 's':
				b.type = Bonus::FREE_SHOOTING; break;
			case 'u':
				b.type = Bonus::SPELL_RESISTANCE_AURA; break;
			case 'U':
				b.type = Bonus::UNDEAD; break;
			default:
			tlog3 << "Not parsed bonus " << buf << mod << "\n";
				break;
		}
		break;
	case 'w': //specific spell immunities, enabled/disabled
		enable = true;
		switch (mod[0])
		{
			case 'B': //Blind
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 74;
				break;
			case 'H': //Hypnotize
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 60;
				break;
			case 'I': //Implosion
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 18;
				break;
			case 'K': //Berserk
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 59;
				break;
			case 'M': //Meteor Shower
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 23;
				break;
			case 'R': //Armageddon
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 26;
				break;
			case 'S': //Slow
				b.type = Bonus::SPELL_IMMUNITY;
				b.subtype = 54;
				break;
			case '6':
			case '7':
			case '8':
			case '9':
				b.type = Bonus::LEVEL_SPELL_IMMUNITY; //TODO - value can't be read afterwards
				b.val = std::atoi(mod.c_str()) - 5;
				break;
			case ':':
				b.type = Bonus::LEVEL_SPELL_IMMUNITY;
				b.val = SPELL_LEVELS; //in case someone adds higher level spells?
				break;
			default:
				tlog3 << "Not parsed bonus " << buf << mod << "\n";
		}
		break;

	case 'i':
		enable = true;
		b.type = Bonus::NO_DISTANCE_PENALTY;
		break;
	case 'o':
		enable = true;
		b.type = Bonus::NO_OBSTACLES_PENALTY;
		break;

	case 'a':
	//case 'c': //some special abilities are threated as spells, will cause bugs
		b.type = Bonus::SPELL_AFTER_ATTACK;
		b.subtype = stringToNumber(mod); 
		break;
	case 'h':
		b.type= Bonus::HATE;
		b.subtype = stringToNumber(mod);
		break;
	default:
		tlog3 << "Not parsed bonus " << buf << mod << "\n";
		break;
	}
	switch (mod[0])
	{
		case '+':
		case '=': //should we allow percent values to stack or pick highest?
			b.valType = Bonus::ADDITIVE_VALUE;
			break;
	}

	//limiters, range
	si32 lastVal, curVal, lastLev = 0;

	if (enable) //0 and 2 means non-active, 1 - active
	{
		b.val = 0; //on-off ability, no value specified
		loadToIt (curVal, src, it, 4); // 0 level is never active
		for (int i = 1; i < 11; ++i)
		{
			loadToIt (curVal, src, it, 4);
			if (curVal == 1)
			{
				b.val = curVal;
				b.limiter.reset (new RankRangeLimiter(i));
				bl.push_back(new Bonus(b));
				break; //never turned off it seems
			}
		}
	}
	else
	{
		loadToIt (lastVal, src, it, 4); //basic value, not particularly useful but existent
		for (int i = 1; i < 11; ++i)
		{
			loadToIt (curVal, src, it, 4);
			if (curVal > lastVal) //threshold, add new bonus
			{
				b.val = curVal - lastVal;
				lastVal = curVal;
				b.limiter.reset (new RankRangeLimiter(i));
				bl.push_back(new Bonus(b));
				lastLev = i; //start new range from here, i = previous rank
			}
			else if (curVal < lastVal)
			{
				b.val = lastVal;
				b.limiter.reset (new RankRangeLimiter(lastLev, i));
			}
		}
	}
}

void CCreatureHandler::loadMindImmunity(Bonus & b, BonusList & bl, std::string & src, int & it)
{
	CCreature * cre = creatures[b.id]; //odd workaround

	b.type = Bonus::SPELL_IMMUNITY;
	b.val = Bonus::BASE_NUMBER;
	std::vector<si32> values;
	values.resize(10);
	si32 val;

	loadToIt (val, src, it, 4); //basic value
	for (int i = 0; i < 10; ++i)
	{
		loadToIt (values[i], src, it, 4);
		if (values[i] = 2)
			values[i] = 0;
	}
	std::vector<int> mindSpells = getMindSpells();
	for (int g=0; g < mindSpells.size(); ++g)
	{
		b.subtype = mindSpells[g];
		cre->bonuses.push_back(new Bonus(b));
	}
}

int CCreatureHandler::stringToNumber(std::string & s)
{
	boost::algorithm::replace_first(s,"#",""); //drop hash character
	return std::atoi(s.c_str());
}

CCreatureHandler::~CCreatureHandler()
{
}

static int retreiveRandNum(const boost::function<int()> &randGen)
{
	if(randGen)
		return randGen();
	else
		return rand();
}

template <typename T> const T & pickRandomElementOf(const std::vector<T> &v, const boost::function<int()> &randGen)
{
	return v[retreiveRandNum(randGen) % v.size()];
}

int CCreatureHandler::pickRandomMonster(const boost::function<int()> &randGen, int tier) const
{
	int r = 0;
	if(tier == -1) //pick any allowed creature
	{
		do 
		{
			pickRandomElementOf(creatures, randGen);
			//r = retreiveRandNum(randGen) % CREATURES_COUNT;
		} while (vstd::contains(VLC->creh->notUsedMonsters,r));
	}
	else
	{
		assert(iswith(tier, 1, 7));
		std::vector<int> allowed;
		BOOST_FOREACH(const CBonusSystemNode *b, creaturesOfLevel[tier].children)
		{
			assert(b->nodeType == CBonusSystemNode::CREATURE);
			int creid = static_cast<const CCreature*>(b)->idNumber;
			if(!vstd::contains(notUsedMonsters, creid))

				allowed.push_back(creid);
		}

		if(!allowed.size())
		{
			tlog2 << "Cannot pick a random creature of tier " << tier << "!\n";
			return 0;
		}

		return pickRandomElementOf(allowed, randGen);
	}
	return r;
}

void CCreatureHandler::addBonusForTier(int tier, Bonus *b)
{
	assert(iswith(tier, 1, 7));
	creaturesOfLevel[tier].addNewBonus(b);
}

void CCreatureHandler::addBonusForAllCreatures(Bonus *b)
{
	allCreatures.addNewBonus(b);
}
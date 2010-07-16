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
	return vstd::contains(bonuses, Bonus::FLYING);
}

bool CCreature::isShooting() const
{
	return vstd::contains(bonuses, Bonus::SHOOTER);
}

bool CCreature::isUndead() const
{
	return vstd::contains(bonuses, Bonus::UNDEAD);
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
}

ui32 CCreature::getMinDamage() const
{
	return damageMin + valOfBonuses(Bonus::CREATURE_DAMAGE, 0) + valOfBonuses(Bonus::CREATURE_DAMAGE, 1);
}
ui32 CCreature::getMaxDamage() const
{
	return damageMax + valOfBonuses(Bonus::CREATURE_DAMAGE, 0) + valOfBonuses(Bonus::CREATURE_DAMAGE, 2);
}

void CCreature::addBonus(int val, int type, int subtype /*= -1*/)
{
	Bonus added(Bonus::PERMANENT, type, Bonus::CREATURE_ABILITY, val, idNumber, subtype, Bonus::BASE_NUMBER);
	bonuses.push_back(added);
}

bool CCreature::isMyUpgrade(const CCreature *anotherCre) const
{
	//TODO upgrade of upgrade?
	return vstd::contains(upgrades, anotherCre->idNumber);
}

int readNumber(int & befi, int & i, int andame, std::string & buf) //helper function for void CCreatureHandler::loadCreatures() and loadUnitAnimInfo()
{
	befi=i;
	for(i; i<andame; ++i)
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
	for(i; i<andame; ++i)
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
	for(i; i<andame; ++i)
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
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.nameSing = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
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
		ncre.damageMin = readNumber(befi, i, andame, buf);
		ncre.damageMax = readNumber(befi, i, andame, buf);

		ncre.shots = readNumber(befi, i, andame, buf);
		ncre.spells = readNumber(befi, i, andame, buf);
		ncre.ammMin = readNumber(befi, i, andame, buf);
		ncre.ammMax = readNumber(befi, i, andame, buf);

		befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\t')
				break;
		}
		ncre.abilityText = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame; ++i)
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
				ncre.bonuses.back().effectRange = Bonus::ONLY_ALLIED_ARMY;
			}
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_lowers_morale"))
			{
				ncre.addBonus(-1, Bonus::MORALE);;
				ncre.bonuses.back().effectRange = Bonus::ONLY_ENEMY_ARMY;
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
				Bonus nsf;
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
						cre->addBonus(-1, Bonus::MORALE);;
						cre->bonuses.back().effectRange = Bonus::ONLY_ENEMY_ARMY;
					}
					else if(type == "ENEMY_LUCK_DECREASING")
					{
						cre->addBonus(-1, Bonus::LUCK);;
						cre->bonuses.back().effectRange = Bonus::ONLY_ENEMY_ARMY;
					}
					else
						tlog1 << "Error: invalid type " << type << " in cr_abils.txt" << std::endl;
					break;
				}
				nsf.type = it->second;

				reader >> buf; nsf.val = buf;
				reader >> buf; nsf.subtype = buf;
				reader >> buf; nsf.additionalInfo = buf;
				nsf.source = Bonus::CREATURE_ABILITY;
				nsf.id = cre->idNumber;
				nsf.duration = Bonus::ONE_BATTLE;
				nsf.turnsRemain = 0;

				cre->bonuses += nsf;
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

				creatures[creatureID]->bonuses -= ecf;
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
	for(int i=1;i<=10;i++)
		levelCreatures.insert(std::pair<int,std::vector<CCreature*> >(i,std::vector<CCreature*>()));

	tlog5 << "\t\tReading config/monsters.txt" << std::endl;
	ifs.open(DATA_DIR "/config/monsters.txt");
	{
		while(!ifs.eof())
		{
			int id, lvl;
			ifs >> id >> lvl;
			if(lvl>0)
			{
				creatures[id]->level = lvl;
				levelCreatures[lvl].push_back(creatures[id]);
			}
		}
	}
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
	for(i; i<andame2; ++i) //omitting rubbish
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
		for(i; i<andame2; ++i)
		{
			if(buf[i]==' ')
				break;
		}
		rub = buf.substr(befi, i-befi);
		++i;

		befi=i;
		for(i; i<andame2; ++i)
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
}

void CCreatureHandler::loadAnimationInfo()
{
	std::string buf = bitmaph->getTextFile("CRANIM.TXT");
	int andame = buf.size();
	int i=0; //buf iterator
	int hmcr=0;
	for(i; i<andame; ++i)
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
		for(i; i<src.size(); ++i)
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

	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\r')
			break;
	}
	i+=2;
}

CCreatureHandler::~CCreatureHandler()
{
}

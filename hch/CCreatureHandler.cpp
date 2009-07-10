#define VCMI_DLL
#include "../stdafx.h"
#include "CCreatureHandler.h"
#include "CLodHandler.h"
#include <sstream>
#include <boost/assign/std/set.hpp>
#include <boost/assign/std/vector.hpp>
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
	return ret;
}

CCreatureHandler::CCreatureHandler()
{
	VLC->creh = this;
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
	return vstd::contains(abilities, StackFeature::DOUBLE_WIDE);
}

bool CCreature::isFlying() const
{
	return vstd::contains(abilities, StackFeature::FLYING);
}

bool CCreature::isShooting() const
{
	return vstd::contains(abilities, StackFeature::SHOOTER);
}

bool CCreature::isUndead() const
{
	return vstd::contains(abilities, StackFeature::UNDEAD);
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

void CCreatureHandler::loadCreatures()
{
	notUsedMonsters += 122,124,126,128,145,146,147,148,149,160,161,162,163,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191;
	tlog5 << "\t\tReading config/cr_abils.txt and ZCRTRAIT.TXT" << std::endl;

	bool useCreAbilsFromZCRTRAIT = true;

	////////////reading cr_abils.txt ///////////////////
	std::ifstream abils("config" PATHSEPARATOR "cr_abils.txt", std::ios::in | std::ios::binary); //this file is not in lod
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
		CCreature ncre;
		ncre.cost.resize(RESOURCE_QUANTITY);
		ncre.level=0;
		ncre.sounds.attack = soundBase::invalid;
		ncre.sounds.defend = soundBase::invalid;
		ncre.sounds.killed = soundBase::invalid;
		ncre.sounds.move = soundBase::invalid;
		ncre.sounds.shoot = soundBase::invalid;
		ncre.sounds.wince = soundBase::invalid;
		ncre.sounds.ext1 = soundBase::invalid;
		ncre.sounds.ext2 = soundBase::invalid;
		ncre.sounds.startMoving = soundBase::invalid;
		ncre.sounds.endMoving = soundBase::invalid;

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
		ncre.speed = readNumber(befi, i, andame, buf);
		ncre.attack = readNumber(befi, i, andame, buf);
		ncre.defence = readNumber(befi, i, andame, buf);
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
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::DOUBLE_WIDE, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "FLYING_ARMY"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::FLYING, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "SHOOTING_ARMY"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::SHOOTER, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "SIEGE_WEAPON"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::SIEGE_WEAPON, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_two_attacks"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::ADDITIONAL_ATTACK, 1));
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_free_attack"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::BLOCKS_RETALIATION, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "IS_UNDEAD"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::UNDEAD, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_no_melee_penalty"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::NO_MELEE_PENALTY, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_jousting"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::JOUSTING, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_raises_morale"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::RAISING_MORALE, 1));
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_lowers_morale"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::ENEMY_MORALE_DECREASING, 1));
			if(boost::algorithm::find_first(ncre.abilityRefs, "KING_1"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::KING1, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "KING_2"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::KING2, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "KING_3"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::KING3, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "const_no_wall_penalty"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::NO_WALL_PENALTY, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "CATAPULT"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::CATAPULT, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "MULTI_HEADED"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::ATTACKS_ALL_ADJACENT, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "IMMUNE_TO_MIND_SPELLS"))
			{
				std::vector<int> mindSpells = getMindSpells();
				for(int g=0; g<mindSpells.size(); ++g)
				{
					creatures[40].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, mindSpells[g]); //giants are immune to mind spells
				}
			}
			if(boost::algorithm::find_first(ncre.abilityRefs, "IMMUNE_TO_FIRE_SPELLS"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::FIRE_IMMUNITY, 0));
			if(boost::algorithm::find_first(ncre.abilityRefs, "HAS_EXTENDED_ATTACK"))
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::TWO_HEX_ATTACK_BREATH, 0));
		}

		if(ncre.nameSing!=std::string("") && ncre.namePl!=std::string(""))
		{
			ncre.idNumber = creatures.size();
			creatures.push_back(ncre);
		}
	}

	// Map types names
#define VCMI_CREATURE_ABILITY_NAME(x) ( #x, StackFeature::x )
	std::map<std::string, int> type_list = map_list_of VCMI_CREATURE_ABILITY_LIST;
#undef VCMI_CREATURE_ABILITY_NAME

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
				StackFeature nsf;
				si32 buf;
				std::string type;

				reader >> creatureID;

				reader >> type;
				std::map<std::string, int>::iterator it = type_list.find(type);
				if (it == type_list.end()) {
					tlog1 << "Error: invalid type " << type << " in cr_abils.txt" << std::endl;
					break;
				}
				nsf.type = it->second;

				reader >> buf; nsf.value = buf;
				reader >> buf; nsf.subtype = buf;
				reader >> buf; nsf.additionalInfo = buf;
				nsf.source = StackFeature::CREATURE_ABILITY;
				nsf.duration = StackFeature::WHOLE_BATTLE;
				nsf.turnsRemain = 0;

				creatures[creatureID].abilities += nsf;
				break;
			}
		case '-': //remove ability
			{
				int creatureID;
				ui32 type;
				reader >> creatureID;
				reader >> type;
				StackFeature::ECombatFeatures ecf = static_cast<StackFeature::ECombatFeatures>(type);

				creatures[creatureID].abilities -= ecf;
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
	std::ifstream ifs("config/crerefnam.txt");
	int tempi;
	std::string temps;
	for (;;)
	{
		ifs >> tempi >> temps;
		if (tempi>=creatures.size())
			break;
		boost::assign::insert(nameToID)(temps,tempi);
		creatures[tempi].nameRef=temps;
	}
	ifs.close();
	ifs.clear();
	for(int i=1;i<=10;i++)
		levelCreatures.insert(std::pair<int,std::vector<CCreature*> >(i,std::vector<CCreature*>()));

	tlog5 << "\t\tReading config/monsters.txt" << std::endl;
	ifs.open("config/monsters.txt");
	{
		while(!ifs.eof())
		{
			int id, lvl;
			ifs >> id >> lvl;
			if(lvl>0)
			{
				creatures[id].level = lvl;
				levelCreatures[lvl].push_back(&(creatures[id]));
			}
		}
	}
	ifs.close();
	ifs.clear();

	tlog5 << "\t\tReading config/cr_factions.txt" << std::endl;
	ifs.open("config/cr_factions.txt");
	while(!ifs.eof())
	{
		int id, fact;
		ifs >> id >> fact;
		creatures[id].faction = fact;
	}
	ifs.close();
	ifs.clear();

	tlog5 << "\t\tReading config/cr_upgrade_list.txt" << std::endl;
	ifs.open("config/cr_upgrade_list.txt");
	while(!ifs.eof())
	{
		int id, up;
		ifs >> id >> up;
		creatures[id].upgrades.insert(up);
	}
	ifs.close();
	ifs.clear();

	//loading unit animation def names
	tlog5 << "\t\tReading config/CREDEFS.TXT" << std::endl;
	std::ifstream inp("config/CREDEFS.TXT", std::ios::in | std::ios::binary); //this file is not in lod
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
		creatures[s].animDefName = defName;
	}
	tlog5 << "\t\tReading CRANIM.TXT.txt" << std::endl;
	loadAnimationInfo();

	//loading id to projectile mapping

	tlog5 << "\t\tReading config/cr_shots.txt" << std::endl;
	std::ifstream inp2("config" PATHSEPARATOR "cr_shots.txt", std::ios::in | std::ios::binary); //this file is not in lod
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
		loadUnitAnimInfo(creatures[dd], buf, i);
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

#define VCMI_DLL
#include "../stdafx.h"
#include "CCreatureHandler.h"
#include "CLodHandler.h"
#include <sstream>
#include <boost/assign/std/set.hpp>
#include <boost/assign/std/vector.hpp>
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

int readNumber(int & befi, int & i, int andame, std::string & buf) //helper function for void CCreatureHandler::loadCreatures()
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
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0));
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
				ncre.abilities.push_back(makeCreatureAbility(StackFeature::ATTACKS_ALL_ADAJCENT, 0));
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
				reader >> creatureID;
				reader >> buf; nsf.type = buf; //it reads ui8 as byte, in file it has different format
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

	//TODO: create a tidy configuration file to control fixing unit abilities
/*	creatures[115].abilities.push_back(makeCreatureAbility(StackFeature::DOUBLE_WIDE, 0));//water elemental should be treated as double-wide
	creatures[123].abilities.push_back(makeCreatureAbility(StackFeature::DOUBLE_WIDE, 0));//ice elemental should be treated as double-wide
	creatures[140].abilities.push_back(makeCreatureAbility(StackFeature::DOUBLE_WIDE, 0));//boar should be treated as double-wide
	creatures[142].abilities.push_back(makeCreatureAbility(StackFeature::DOUBLE_WIDE, 0));//nomads should be treated as double-wide

	creatures[46].abilities -= StackFeature::FLYING; //hell hound
	creatures[47].abilities -= StackFeature::FLYING; //cerberus
*/
	creatures[52].abilities += makeCreatureAbility(StackFeature::FLYING, 0); //Efreeti
	creatures[53].abilities += makeCreatureAbility(StackFeature::FLYING, 0); //Efreet Sultan

	creatures[47].abilities += makeCreatureAbility(StackFeature::THREE_HEADED_ATTACK, 0); //cerberus

	creatures[87].abilities += makeCreatureAbility(StackFeature::ADDITIONAL_ATTACK, 1); //wolf raider

	creatures[147].abilities += makeCreatureAbility(StackFeature::NOT_ACTIVE, 0); //First Aid Tent //TODO: remove when support is added
	creatures[148].abilities += makeCreatureAbility(StackFeature::NOT_ACTIVE, 0); //Ammo Cart

	creatures[120].abilities -= StackFeature::DOUBLE_WIDE; //psychic elemental
	creatures[121].abilities -= StackFeature::DOUBLE_WIDE; //magic elemental

	creatures[0].abilities += makeCreatureAbility(StackFeature::CHARGE_IMMUNITY, 0); //pikeman immunity to Champion charge bonus
	creatures[1].abilities += makeCreatureAbility(StackFeature::CHARGE_IMMUNITY, 0); //halberdier immunity to Champion charge bonus
	creatures[4].abilities += makeCreatureAbility(StackFeature::ADDITIONAL_RETAILATION, 1); //griffins retailate twice
	creatures[5].abilities += makeCreatureAbility(StackFeature::UNLIMITED_RETAILATIONS, 0); //royal griffins retailate always
	creatures[12].abilities += makeCreatureAbility(StackFeature::HATE, 0, 54); //angels hate devils
	creatures[12].abilities += makeCreatureAbility(StackFeature::HATE, 0, 55); //angels hate archdevils
	creatures[13].abilities += makeCreatureAbility(StackFeature::HATE, 0, 54); //archangels hate devils
	creatures[13].abilities += makeCreatureAbility(StackFeature::HATE, 0, 55); //archangels hate arch
	creatures[13].abilities += makeCreatureAbility(StackFeature::SPELLCASTER, 0, 38); //archangels cast resurrection
	creatures[16].abilities += makeCreatureAbility(StackFeature::MAGIC_RESISTANCE, 20); //dwarf's magic resistance 20%
	creatures[17].abilities += makeCreatureAbility(StackFeature::MAGIC_RESISTANCE, 40); //battle dwarf's magic resistance 40%
	creatures[20].abilities += makeCreatureAbility(StackFeature::CHANGES_SPELL_COST_FOR_ENEMY, 2); //pegasus makes spell cost higher for enemy mage
	creatures[21].abilities += makeCreatureAbility(StackFeature::CHANGES_SPELL_COST_FOR_ENEMY, 2); //silver pegasus makes spell cost higher for enemy mage
	creatures[22].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 72, 100); //dendroids cast bind
	creatures[23].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 72, 100); //dendroid guards cast bind
	creatures[24].abilities += makeCreatureAbility(StackFeature::SPELL_RESISTANCE_AURA, 0, 55); //unicorn
	creatures[25].abilities += makeCreatureAbility(StackFeature::SPELL_RESISTANCE_AURA, 0, 55); //war unicorn
	creatures[24].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 62, 100); //unicorns cast blind with 20% probability
	creatures[25].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 62, 100); //war unicorns cast blind with 20% probability
	creatures[26].abilities += makeCreatureAbility(StackFeature::LEVEL_SPELL_IMMUNITY, 3); //green dragon's spell immunity
	creatures[27].abilities += makeCreatureAbility(StackFeature::LEVEL_SPELL_IMMUNITY, 4); //gold dragon's spell immunity
	creatures[26].abilities += makeCreatureAbility(StackFeature::TWO_HEX_ATTACK_BREATH, 0); //green dragon's breath
	creatures[27].abilities += makeCreatureAbility(StackFeature::TWO_HEX_ATTACK_BREATH, 0); //gold dragon's breath
	creatures[30].abilities += makeCreatureAbility(StackFeature::NON_LIVING, 0); //stone gargoyles are non-living
	creatures[31].abilities += makeCreatureAbility(StackFeature::NON_LIVING, 0); //obsidian gargoyles are non-living
	creatures[32].abilities += makeCreatureAbility(StackFeature::NON_LIVING, 0); //stone golems are non-living
	creatures[33].abilities += makeCreatureAbility(StackFeature::NON_LIVING, 0); //iron golems are non-living
	creatures[32].abilities += makeCreatureAbility(StackFeature::SPELL_DAMAGE_REDUCTION, 50, -1); //stone golems reduce dmg from spells
	creatures[33].abilities += makeCreatureAbility(StackFeature::SPELL_DAMAGE_REDUCTION, 75, -1); //iron golems reduce dmg from spells
	creatures[34].abilities += makeCreatureAbility(StackFeature::CHANGES_SPELL_COST_FOR_ALLY, -2); //mages reduce spell cost
	creatures[35].abilities += makeCreatureAbility(StackFeature::CHANGES_SPELL_COST_FOR_ALLY, -2); //archmages reduce spell cost
	creatures[36].abilities += makeCreatureAbility(StackFeature::HATE, 0, 52); //genies hate efreets
	creatures[36].abilities += makeCreatureAbility(StackFeature::HATE, 0, 53); //genies hate efreet sultans
	creatures[37].abilities += makeCreatureAbility(StackFeature::HATE, 0, 52); //master genies hate efreets
	creatures[37].abilities += makeCreatureAbility(StackFeature::HATE, 0, 53); //master genies hate efreet sultans
	creatures[37].abilities += makeCreatureAbility(StackFeature::RANDOM_GENIE_SPELLCASTER, 0); //master genies cast spells
	creatures[38].abilities += makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0); //nagas block retailation
	creatures[39].abilities += makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0); //naga queens block retailation
	std::vector<int> mindSpells = getMindSpells();
	for(int g=0; g<mindSpells.size(); ++g)
	{
		creatures[40].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, mindSpells[g]); //giants are immune to mind spells
	}
	for(int g=0; g<mindSpells.size(); ++g)
	{
		creatures[41].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, mindSpells[g]); //titans are immune to mind spells
	}
	creatures[41].abilities += makeCreatureAbility(StackFeature::HATE, 0, 83); //titans hate black dragons
	creatures[43].abilities += makeCreatureAbility(StackFeature::MANA_CHANNELING, 20); //familiars
	creatures[45].abilities += makeCreatureAbility(StackFeature::SPELL_LIKE_ATTACK, 21); //magogs fire with fireballs
	creatures[47].abilities += makeCreatureAbility(StackFeature::THREE_HEADED_ATTACK, 0); //creberus
	creatures[47].abilities += makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0); //cerberus
	creatures[51].abilities += makeCreatureAbility(StackFeature::DEAMON_SUMMONING, 0, 52); //pit lord
	creatures[52].abilities += makeCreatureAbility(StackFeature::FIRE_IMMUNITY, 0); //efreeti
	creatures[52].abilities += makeCreatureAbility(StackFeature::HATE, 0, 36); //efreeti hate genies
	creatures[52].abilities += makeCreatureAbility(StackFeature::HATE, 0, 37); //efreeti hate master genies
	creatures[53].abilities += makeCreatureAbility(StackFeature::FIRE_IMMUNITY, 0); //efreet sultan
	creatures[53].abilities += makeCreatureAbility(StackFeature::HATE, 0, 36); //efreet sultans hate genies
	creatures[53].abilities += makeCreatureAbility(StackFeature::HATE, 0, 37); //efreet sultans hate master genies
	creatures[53].abilities += makeCreatureAbility(StackFeature::FIRE_SHIELD, 0, 36); //efreet sultans
	creatures[54].abilities += makeCreatureAbility(StackFeature::ENEMY_LUCK_DECREASING, -1); //devils
	creatures[54].abilities += makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0); //devils
	creatures[54].abilities += makeCreatureAbility(StackFeature::HATE, 0, 12); //devils hate angels
	creatures[54].abilities += makeCreatureAbility(StackFeature::HATE, 0, 13); //devils hate archangles
	creatures[55].abilities += makeCreatureAbility(StackFeature::ENEMY_LUCK_DECREASING, -1); //archdevils
	creatures[55].abilities += makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0); //archdevils
	creatures[55].abilities += makeCreatureAbility(StackFeature::HATE, 0, 12); //archdevils hate angels
	creatures[55].abilities += makeCreatureAbility(StackFeature::HATE, 0, 13); //archdevils hate archangles
	creatures[60].abilities += makeCreatureAbility(StackFeature::REGENERATION, 0); //wight
	creatures[61].abilities += makeCreatureAbility(StackFeature::REGENERATION, 0); //wraith
	creatures[61].abilities += makeCreatureAbility(StackFeature::MANA_DRAIN, 2); //wraith
	creatures[62].abilities += makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0); //vampires
	creatures[63].abilities += makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0); //vampire lords
	creatures[63].abilities += makeCreatureAbility(StackFeature::LIFE_DRAIN, 0); //vampire lords
	creatures[64].abilities += makeCreatureAbility(StackFeature::SPELL_LIKE_ATTACK, 76); //liches
	creatures[65].abilities += makeCreatureAbility(StackFeature::SPELL_LIKE_ATTACK, 76); //power liches
	creatures[66].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 42, 20); //black knights
	creatures[67].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 42, 20); //dread knights
	creatures[67].abilities += makeCreatureAbility(StackFeature::DOUBLE_DAMAGE_CHANCE, 20); //vampire lords
	creatures[68].abilities += makeCreatureAbility(StackFeature::ENEMY_MORALE_DECREASING, -1); //bone dragon
	creatures[69].abilities += makeCreatureAbility(StackFeature::ENEMY_MORALE_DECREASING, -1); //ghost dragon
	creatures[69].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 75, 20); //ghost dragon
	creatures[70].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, 62); //troglodytes are immune to blind
	creatures[71].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, 62); //infernal troglodytes are immune to blind
	creatures[72].abilities += makeCreatureAbility(StackFeature::RETURN_AFTER_STRIKE, 0); //Harpies return after attack
	creatures[73].abilities += makeCreatureAbility(StackFeature::RETURN_AFTER_STRIKE, 0); //Harpy Hags return after attack
	creatures[73].abilities += makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0); //Harpy Hags
	creatures[76].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 70, 20 + 200); //medusas
	creatures[77].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 70, 20 + 200); //medusa queens
	creatures[78].abilities += makeCreatureAbility(StackFeature::SELF_MORALE, 0); //minotaurs
	creatures[79].abilities += makeCreatureAbility(StackFeature::SELF_MORALE, 0); //minotaur kings
	creatures[81].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 74, 20); //scorpicore
	creatures[82].abilities += makeCreatureAbility(StackFeature::LEVEL_SPELL_IMMUNITY, 3); //red dragon's spell immunity
	creatures[83].abilities += makeCreatureAbility(StackFeature::LEVEL_SPELL_IMMUNITY, 5); //black dragon's spell immunity
	creatures[91].abilities += makeCreatureAbility(StackFeature::SPELLCASTER, 2, 43, 6); //ogre magi cast bloodlust
	creatures[93].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 77, 20); //thunderbirds
	creatures[96].abilities += makeCreatureAbility(StackFeature::ENEMY_DEFENCE_REDUCTION, 40); //behemots
	creatures[97].abilities += makeCreatureAbility(StackFeature::ENEMY_DEFENCE_REDUCTION, 80); //ancient behemots
	creatures[103].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 79, 10); //mighty gorgons
	creatures[104].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 78, 100); //serpent fly
	creatures[105].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 78, 100); //dragon fly
	creatures[105].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 45, 100); //mighty gorgons
	creatures[106].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 70, 20); //basilisks
	creatures[107].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 70, 20); //greater basilisks
	creatures[109].abilities += makeCreatureAbility(StackFeature::SPELL_AFTER_ATTACK, 0, 71, 20); //Wyvern Monarch
	creatures[110].abilities += makeCreatureAbility(StackFeature::ATTACKS_ALL_ADAJCENT, 0); //hydras
	creatures[110].abilities += makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0); //hydras
	creatures[111].abilities += makeCreatureAbility(StackFeature::ATTACKS_ALL_ADAJCENT, 0); //chaos hydras
	creatures[111].abilities += makeCreatureAbility(StackFeature::BLOCKS_RETAILATION, 0); //chaos hydras
	for(int g=0; g<mindSpells.size(); ++g)
	{
		creatures[112].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, mindSpells[g]); //air elementals are immune to mind spells
	}
	creatures[112].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, 23); //air elementals are immune to meteor shower
	creatures[112].abilities += makeCreatureAbility(StackFeature::MORE_DAMEGE_FROM_SPELL, 100, 17); //air elementals are vulnerable to lightning bolt
	creatures[112].abilities += makeCreatureAbility(StackFeature::MORE_DAMEGE_FROM_SPELL, 100, 19); //air elementals are vulnerable to chain lightning
	creatures[112].abilities += makeCreatureAbility(StackFeature::NON_LIVING, 0); //air elementals are non-living
	for(int g=0; g<mindSpells.size(); ++g)
	{
		creatures[113].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, mindSpells[g]); //earth elementals are immune to mind spells
	}
	creatures[113].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, 17); //earth elementals are immune to lightning bolt
	creatures[113].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, 19); //earth elementals are immune to chain lightning
	creatures[113].abilities += makeCreatureAbility(StackFeature::MORE_DAMEGE_FROM_SPELL, 100, 23); //earth elementals are vulnerable to meteor shower
	creatures[113].abilities += makeCreatureAbility(StackFeature::NON_LIVING, 0); //earth elementals are non-living
	for(int g=0; g<mindSpells.size(); ++g)
	{
		creatures[114].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, mindSpells[g]); //fire elementals are immune to mind spells
	}
	creatures[114].abilities += makeCreatureAbility(StackFeature::FIRE_IMMUNITY, 0); //fire elementals are immune to fire spells
	creatures[114].abilities += makeCreatureAbility(StackFeature::MORE_DAMEGE_FROM_SPELL, 100, 16); //fire elementals are vulnerable to ice bolt
	creatures[114].abilities += makeCreatureAbility(StackFeature::MORE_DAMEGE_FROM_SPELL, 100, 20); //fire elementals are vulnerable to frost ring
	creatures[114].abilities += makeCreatureAbility(StackFeature::NON_LIVING, 0); //fire elementals are non-living
	for(int g=0; g<mindSpells.size(); ++g)
	{
		creatures[115].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, mindSpells[g]); //water elementals are immune to mind spells
	}
	creatures[115].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, 16); //water elementals are immune to ice bolt
	creatures[115].abilities += makeCreatureAbility(StackFeature::SPELL_IMMUNITY, 0, 20); //water elementals are immune to frost ring
	creatures[115].abilities += makeCreatureAbility(StackFeature::MORE_DAMEGE_FROM_SPELL, 100, 13); //water elementals are vulnerable to fire wall
	creatures[115].abilities += makeCreatureAbility(StackFeature::MORE_DAMEGE_FROM_SPELL, 100, 22); //water elementals are vulnerable to inferno
	creatures[115].abilities += makeCreatureAbility(StackFeature::MORE_DAMEGE_FROM_SPELL, 100, 21); //water elementals are vulnerable to fireball
	creatures[115].abilities += makeCreatureAbility(StackFeature::MORE_DAMEGE_FROM_SPELL, 100, 29); //water elementals are vulnerable to fire shield
	creatures[115].abilities += makeCreatureAbility(StackFeature::NON_LIVING, 0); //water elementals are non-living
	creatures[116].abilities += makeCreatureAbility(StackFeature::SPELL_DAMAGE_REDUCTION, 85, -1); //gold golems reduce dmg from spells
	creatures[116].abilities += makeCreatureAbility(StackFeature::NON_LIVING, 0); //gold golems are non-living
	creatures[117].abilities += makeCreatureAbility(StackFeature::SPELL_DAMAGE_REDUCTION, 95, -1); //diamond golems reduce dmg from spells
	creatures[117].abilities += makeCreatureAbility(StackFeature::NON_LIVING, 0); //diamond golems are non-living
	creatures[121].abilities += makeCreatureAbility(StackFeature::LEVEL_SPELL_IMMUNITY, 5); //magic elementals are immune to all spells

	//TODO: the rest of creatures
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
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.timeBetweenFidgets = atof(src.substr(befi, i-befi).c_str());
	++i;

	while(unit.timeBetweenFidgets == 0.0)
	{
		for(i; i<src.size(); ++i)
		{
			if(src[i]=='\r')
				break;
		}
		i+=2;
		befi=i;
		for(i; i<src.size(); ++i)
		{
			if(src[i]=='\t')
				break;
		}
		unit.timeBetweenFidgets = atof(src.substr(befi, i-befi).c_str());
		++i;
	}

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.walkAnimationTime = atof(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.attackAnimationTime = atof(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.flightAnimationDistance = atof(src.substr(befi, i-befi).c_str());
	++i;

	///////////////////////

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.upperRightMissleOffsetX = atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.upperRightMissleOffsetY = atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.rightMissleOffsetX = atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.rightMissleOffsetY = atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.lowerRightMissleOffsetX = atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.lowerRightMissleOffsetY = atoi(src.substr(befi, i-befi).c_str());
	++i;

	///////////////////////

	for(int jjj=0; jjj<12; ++jjj)
	{
		befi=i;
		for(i; i<src.size(); ++i)
		{
			if(src[i]=='\t')
				break;
		}
		unit.missleFrameAngles[jjj] = atof(src.substr(befi, i-befi).c_str());
		++i;
	}

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.troopCountLocationOffset= atoi(src.substr(befi, i-befi).c_str());
	++i;

	befi=i;
	for(i; i<src.size(); ++i)
	{
		if(src[i]=='\t')
			break;
	}
	unit.attackClimaxFrame = atoi(src.substr(befi, i-befi).c_str());
	++i;

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

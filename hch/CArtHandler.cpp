#define VCMI_DLL
#include "../stdafx.h"
#include "CArtHandler.h"
#include "CLodHandler.h"
#include "CGeneralTextHandler.h"
#include <boost/assign/std/vector.hpp>
#include <boost/assign/list_of.hpp>
#include "../lib/VCMI_Lib.h"
extern CLodHandler *bitmaph;
using namespace boost::assign;

/*
 * CArtHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

const std::string & CArtifact::Name() const
{
	if(name.size())
		return name;
	else
		return VLC->generaltexth->artifNames[id];
}

const std::string & CArtifact::Description() const
{
	if(description.size())
		return description;
	else
		return VLC->generaltexth->artifDescriptions[id];
}
CArtHandler::CArtHandler()
{
	VLC->arth = this;
}
void CArtHandler::loadArtifacts(bool onlyTxt)
{
	std::vector<ui16> slots;
	slots += 17, 16, 15,14,13, 18, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0;
	std::map<char,EartClass> classes = 
	  map_list_of('S',ART_SPECIAL)('T',ART_TREASURE)('N',ART_MINOR)('J',ART_MAJOR)('R',ART_RELIC);
	std::string buf = bitmaph->getTextFile("ARTRAITS.TXT"), dump, pom;
	int it=0;
	for(int i=0; i<2; ++i)
	{
		loadToIt(dump,buf,it,3);
	}
	VLC->generaltexth->artifNames.resize(ARTIFACTS_QUANTITY);
	VLC->generaltexth->artifDescriptions.resize(ARTIFACTS_QUANTITY);
	for (int i=0; i<ARTIFACTS_QUANTITY; i++)
	{
		CArtifact nart;
		loadToIt(VLC->generaltexth->artifNames[i],buf,it,4);
		loadToIt(pom,buf,it,4);
		nart.price=atoi(pom.c_str());
		for(int j=0;j<slots.size();j++)
		{
			loadToIt(pom,buf,it,4);
			if(pom[0]=='x')
				nart.possibleSlots.push_back(slots[j]);
		}
		loadToIt(pom,buf,it,4);
		nart.aClass = classes[pom[0]];
		loadToIt(VLC->generaltexth->artifDescriptions[i],buf,it,3);
		nart.id=i;
		if(!onlyTxt)
			artifacts.push_back(nart);
	}
	sortArts();
	addBonuses();
}

int CArtHandler::convertMachineID(int id, bool creToArt )
{
	int dif = 142;
	if(creToArt)
	{
		switch (id)
		{
		case 147:
			dif--;
			break;
		case 148:
			dif++;
			break;
		}
		dif = -dif;
	}
	else
	{
		switch (id)
		{
		case 6:
			dif--;
			break;
		case 5:
			dif++;
			break;
		}
	}
	return id + dif;
}

void CArtHandler::sortArts()
{
	for(int i=0;i<144;i++) //do 144, bo nie chcemy bzdurek
	{
		switch (artifacts[i].aClass)
		{
		case ART_TREASURE:
			treasures.push_back(&(artifacts[i]));
			break;
		case ART_MINOR:
			minors.push_back(&(artifacts[i]));
			break;
		case ART_MAJOR:
			majors.push_back(&(artifacts[i]));
			break;
		case ART_RELIC:
			relics.push_back(&(artifacts[i]));
			break;
		}
	}
}

void CArtHandler::giveArtBonus( int aid, HeroBonus::BonusType type, int val, int subtype )
{
	artifacts[aid].bonuses.push_back(HeroBonus(HeroBonus::PERMANENT,type,HeroBonus::ARTIFACT,val,aid,subtype));
}

void CArtHandler::addBonuses()
{
	#define ART_PRIM_SKILL(ID, whichSkill, val) giveArtBonus(ID,HeroBonus::PRIMARY_SKILL,val,whichSkill)
	#define ART_MORALE(ID, val) giveArtBonus(ID,HeroBonus::MORALE,val)
	#define ART_LUCK(ID, val) giveArtBonus(ID,HeroBonus::LUCK,val)
	#define ART_MORALE_AND_LUCK(ID, val) giveArtBonus(ID,HeroBonus::MORALE_AND_LUCK,val)
	#define ART_ALL_PRIM_SKILLS(ID, val) ART_PRIM_SKILL(ID,0,val); ART_PRIM_SKILL(ID,1,val); ART_PRIM_SKILL(ID,2,val); ART_PRIM_SKILL(ID,3,val)
	#define ART_ATTACK_AND_DEFENSE(ID, val) ART_PRIM_SKILL(ID,0,val); ART_PRIM_SKILL(ID,1,val)
	#define ART_POWER_AND_KNOWLEDGE(ID, val) ART_PRIM_SKILL(ID,2,val); ART_PRIM_SKILL(ID,3,val)

	//Attack bonus artifacts (Weapons)
	ART_PRIM_SKILL(7,0,+2); //Centaur Axe
	ART_PRIM_SKILL(8,0,+3);
	ART_PRIM_SKILL(9,0,+4);
	ART_PRIM_SKILL(10,0,+5);
	ART_PRIM_SKILL(11,0,+6);
	ART_PRIM_SKILL(12,0,+12);
	ART_PRIM_SKILL(12,1,-3);

	//Defense bonus artifacts (Shields)
	ART_PRIM_SKILL(13,1,+2); //Shield of the Dwarven Lords
	ART_PRIM_SKILL(14,1,+3); 
	ART_PRIM_SKILL(15,1,+4); 
	ART_PRIM_SKILL(16,1,+5); 
	ART_PRIM_SKILL(17,1,+6); 
	ART_PRIM_SKILL(18,1,+12); 
	ART_PRIM_SKILL(18,0,-3); 

	//Knowledge bonus artifacts (Helmets)
	ART_PRIM_SKILL(19,3,+1); //Helm of the Alabaster Unicorn 
	ART_PRIM_SKILL(20,3,+2); 
	ART_PRIM_SKILL(21,3,+3); 
	ART_PRIM_SKILL(22,3,+4); 
	ART_PRIM_SKILL(23,3,+5); 
	ART_PRIM_SKILL(24,3,+10); 
	ART_PRIM_SKILL(24,2,-2); 

	//Spell power bonus artifacts (Armours)
	ART_PRIM_SKILL(25,2,+1); //Breastplate of Petrified Wood
	ART_PRIM_SKILL(26,2,+2); 
	ART_PRIM_SKILL(27,2,+3); 
	ART_PRIM_SKILL(28,2,+4); 
	ART_PRIM_SKILL(29,2,+5); 
	ART_PRIM_SKILL(30,2,+10); 
	ART_PRIM_SKILL(30,3,-2); 

	//All primary skills (various)
	ART_ALL_PRIM_SKILLS(31,+1); //Armor of Wonder
	ART_ALL_PRIM_SKILLS(32,+2); 
	ART_ALL_PRIM_SKILLS(33,+3); 
	ART_ALL_PRIM_SKILLS(34,+4); 
	ART_ALL_PRIM_SKILLS(35,+5); 
	ART_ALL_PRIM_SKILLS(36,+6); //Helm of Heavenly Enlightenment

	//Attack and Defense (various)
	ART_ATTACK_AND_DEFENSE(37,+1); //Quiet Eye of the Dragon
	ART_ATTACK_AND_DEFENSE(38,+2); 
	ART_ATTACK_AND_DEFENSE(39,+3); 
	ART_ATTACK_AND_DEFENSE(40,+4); 

	//Spell power and Knowledge (various)
	ART_POWER_AND_KNOWLEDGE(41,+1); //Dragonbone Greaves
	ART_POWER_AND_KNOWLEDGE(42,+2); 
	ART_POWER_AND_KNOWLEDGE(43,+3); 
	ART_POWER_AND_KNOWLEDGE(44,+4); 

	//Luck and morale 
	ART_MORALE(45,+1); //Still Eye of the Dragon
	ART_LUCK(46,+1); //Clover of Fortune
	ART_LUCK(47,+1); //Cards of Prophecy
	ART_LUCK(48,+1); //Ladybird of Luck
	ART_MORALE(49,+1); //Badge of Courage
	ART_MORALE(50,+1); //Crest of Valor
	ART_MORALE(51,+1); //Glyph of Gallantry

	giveArtBonus(52,HeroBonus::SIGHT_RADIOUS,+1);//Speculum
	giveArtBonus(53,HeroBonus::SIGHT_RADIOUS,+1);//Spyglass

	//necromancy bonus
	giveArtBonus(54,HeroBonus::SECONDARY_SKILL_PREMY,+5,12);//Amulet of the Undertaker
	giveArtBonus(55,HeroBonus::SECONDARY_SKILL_PREMY,+10,12);//Vampire's Cowl
	giveArtBonus(56,HeroBonus::SECONDARY_SKILL_PREMY,+15,12);//Dead Man's Boots

	giveArtBonus(57,HeroBonus::MAGIC_RESISTANCE,+5);//Garniture of Interference
	giveArtBonus(58,HeroBonus::MAGIC_RESISTANCE,+10);//Surcoat of Counterpoise
	giveArtBonus(59,HeroBonus::MAGIC_RESISTANCE,+15);//Boots of Polarity

	//archery bonus
	giveArtBonus(60,HeroBonus::SECONDARY_SKILL_PREMY,+5,1);//Bow of Elven Cherrywood
	giveArtBonus(61,HeroBonus::SECONDARY_SKILL_PREMY,+10,1);//Bowstring of the Unicorn's Mane
	giveArtBonus(62,HeroBonus::SECONDARY_SKILL_PREMY,+15,1);//Angel Feather Arrows

	//eagle eye bonus
	giveArtBonus(63,HeroBonus::SECONDARY_SKILL_PREMY,+5,11);//Bird of Perception
	giveArtBonus(64,HeroBonus::SECONDARY_SKILL_PREMY,+10,11);//Stoic Watchman
	giveArtBonus(65,HeroBonus::SECONDARY_SKILL_PREMY,+15,11);//Emblem of Cognizance

	//reducing cost of surrendering
	giveArtBonus(66,HeroBonus::SURRENDER_DISCOUNT,+10);//Statesman's Medal
	giveArtBonus(67,HeroBonus::SURRENDER_DISCOUNT,+10);//Diplomat's Ring
	giveArtBonus(68,HeroBonus::SURRENDER_DISCOUNT,+10);//Ambassador's Sash

	giveArtBonus(69,HeroBonus::STACKS_SPEED,+1);//Ring of the Wayfarer

	giveArtBonus(70,HeroBonus::LAND_MOVEMENT,+300);//Equestrian's Gloves
	giveArtBonus(71,HeroBonus::SEA_MOVEMENT,+1000);//Necklace of Ocean Guidance
	giveArtBonus(72,HeroBonus::FLYING_MOVEMENT,+1);//Angel Wings

	giveArtBonus(73,HeroBonus::MANA_REGENERATION,+1);//Charm of Mana
	giveArtBonus(74,HeroBonus::MANA_REGENERATION,+2);//Talisman of Mana
	giveArtBonus(75,HeroBonus::MANA_REGENERATION,+3);//Mystic Orb of Mana

	giveArtBonus(76,HeroBonus::SPELL_DURATION,+1);//Collar of Conjuring
	giveArtBonus(77,HeroBonus::SPELL_DURATION,+2);//Ring of Conjuring
	giveArtBonus(78,HeroBonus::SPELL_DURATION,+3);//Cape of Conjuring

	giveArtBonus(79,HeroBonus::AIR_SPELL_DMG_PREMY,+50);//Orb of the Firmament
	giveArtBonus(80,HeroBonus::EARTH_SPELL_DMG_PREMY,+50);//Orb of Silt
	giveArtBonus(81,HeroBonus::FIRE_SPELL_DMG_PREMY,+50);//Orb of Tempestuous Fire
	giveArtBonus(82,HeroBonus::WATER_SPELL_DMG_PREMY,+50);//Orb of Driving Rain

	giveArtBonus(83,HeroBonus::BLOCK_SPELLS_ABOVE_LEVEL,3);//Recanter's Cloak
	giveArtBonus(84,HeroBonus::BLOCK_MORALE,0);//Spirit of Oppression
	giveArtBonus(85,HeroBonus::BLOCK_LUCK,0);//Hourglass of the Evil Hour

	giveArtBonus(86,HeroBonus::FIRE_SPELLS,0);//Tome of Fire Magic
	giveArtBonus(87,HeroBonus::AIR_SPELLS,0);//Tome of Air Magic
	giveArtBonus(88,HeroBonus::WATER_SPELLS,0);//Tome of Water Magic
	giveArtBonus(89,HeroBonus::EARTH_SPELLS,0);//Tome of Earth Magic

	giveArtBonus(90,HeroBonus::WATER_WALKING,0);//Boots of Levitation
	giveArtBonus(91,HeroBonus::NO_SHOTING_PENALTY,0);//Golden Bow
	giveArtBonus(92,HeroBonus::DISPEL_IMMUNITY,0);//Sphere of Permanence
	giveArtBonus(93,HeroBonus::NEGATE_ALL_NATURAL_IMMUNITIES,0);//Orb of Vulnerability

	giveArtBonus(94,HeroBonus::STACK_HEALTH,+1);//Ring of Vitality
	giveArtBonus(95,HeroBonus::STACK_HEALTH,+1);//Ring of Life
	giveArtBonus(96,HeroBonus::STACK_HEALTH,+2);//Vial of Lifeblood

	giveArtBonus(97,HeroBonus::STACKS_SPEED,+1);//Necklace of Swiftness
	giveArtBonus(98,HeroBonus::LAND_MOVEMENT,+600);//Boots of Speed
	giveArtBonus(99,HeroBonus::STACKS_SPEED,+1);//Cape of Velocity

	giveArtBonus(100,HeroBonus::SPELL_IMMUNITY,59);//Pendant of Dispassion
	giveArtBonus(101,HeroBonus::SPELL_IMMUNITY,62);//Pendant of Second Sight
	giveArtBonus(102,HeroBonus::SPELL_IMMUNITY,42);//Pendant of Holiness
	giveArtBonus(103,HeroBonus::SPELL_IMMUNITY,24);//Pendant of Life
	giveArtBonus(104,HeroBonus::SPELL_IMMUNITY,25);//Pendant of Death
	giveArtBonus(105,HeroBonus::SPELL_IMMUNITY,60);//Pendant of Free Will
	giveArtBonus(106,HeroBonus::SPELL_IMMUNITY,17);//Pendant of Negativity
	giveArtBonus(107,HeroBonus::SPELL_IMMUNITY,61);//Pendant of Total Recall
	giveArtBonus(108,HeroBonus::MORALE_AND_LUCK,+3);//Pendant of Courage

	giveArtBonus(109,HeroBonus::GENERATE_RESOURCE,+1,4); //Everflowing Crystal Cloak
	giveArtBonus(110,HeroBonus::GENERATE_RESOURCE,+1,5); //Ring of Infinite Gems
	giveArtBonus(111,HeroBonus::GENERATE_RESOURCE,+1,1); //Everpouring Vial of Mercury
	giveArtBonus(112,HeroBonus::GENERATE_RESOURCE,+1,2); //Inexhaustible Cart of Ore
	giveArtBonus(113,HeroBonus::GENERATE_RESOURCE,+1,3); //Eversmoking Ring of Sulfur
	giveArtBonus(114,HeroBonus::GENERATE_RESOURCE,+1,0); //Inexhaustible Cart of Lumber
	giveArtBonus(115,HeroBonus::GENERATE_RESOURCE,+1000,6); //Endless Sack of Gold
	giveArtBonus(116,HeroBonus::GENERATE_RESOURCE,+750,6); //Endless Bag of Gold
	giveArtBonus(117,HeroBonus::GENERATE_RESOURCE,+500,6); //Endless Purse of Gold

	giveArtBonus(118,HeroBonus::CREATURE_GROWTH,+5,2); //Legs of Legion
	giveArtBonus(119,HeroBonus::CREATURE_GROWTH,+4,3); //Loins of Legion
	giveArtBonus(120,HeroBonus::CREATURE_GROWTH,+3,4); //Torso of Legion
	giveArtBonus(121,HeroBonus::CREATURE_GROWTH,+2,5); //Arms of Legion
	giveArtBonus(122,HeroBonus::CREATURE_GROWTH,+1,6); //Head of Legion

	//Sea Captain's Hat 
	giveArtBonus(123,HeroBonus::WHIRLPOOL_PROTECTION,0); 
	giveArtBonus(123,HeroBonus::SEA_MOVEMENT,+500); 
	giveArtBonus(123,HeroBonus::SPELL,3,0); 
	giveArtBonus(123,HeroBonus::SPELL,3,1); 

	giveArtBonus(124,HeroBonus::SPELLS_OF_LEVEL,3,1); //Spellbinder's Hat
	giveArtBonus(125,HeroBonus::ENEMY_CANT_ESCAPE,0); //Shackles of War
	giveArtBonus(126,HeroBonus::BLOCK_SPELLS_ABOVE_LEVEL,0);//Orb of Inhibition
}

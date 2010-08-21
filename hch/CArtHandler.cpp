#define VCMI_DLL
#include "../stdafx.h"
#include "CArtHandler.h"
#include "CLodHandler.h"
#include "CGeneralTextHandler.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/assign/std/vector.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/random/linear_congruential.hpp>
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

extern boost::rand48 ran;

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

bool CArtifact::isBig () const
{
	return VLC->arth->isBigArtifact(id);
}

/**
 * Checks whether the artifact fits at a given slot.
 * @param artifWorn A hero's set of worn artifacts.
 */
bool CArtifact::fitsAt (const std::map<ui16, ui32> &artifWorn, ui16 slotID) const
{
	if (!vstd::contains(possibleSlots, slotID))
		return false;

	// Can't put an artifact in a locked slot.
	std::map<ui16, ui32>::const_iterator it = artifWorn.find(slotID);
	if (it != artifWorn.end() && it->second == 145)
		return false;

	// Check if a combination artifact fits.
	// TODO: Might want a more general algorithm?
	//       Assumes that misc & rings fits only in their slots, and others in only one slot and no duplicates.
	if (constituents != NULL) {
		std::map<ui16, ui32> tempArtifWorn = artifWorn;
		const ui16 ringSlots[] = {6, 7};
		const ui16 miscSlots[] = {9, 10, 11, 12, 18};
		int rings = 0;
		int misc = 0;

		VLC->arth->unequipArtifact(tempArtifWorn, slotID);

		BOOST_FOREACH(ui32 constituentID, *constituents) 
		{
			const CArtifact& constituent = *VLC->arth->artifacts[constituentID];
			const int slot = constituent.possibleSlots[0];

			if (slot == 6 || slot == 7)
				rings++;
			else if ((slot >= 9 && slot <= 12) || slot == 18)
				misc++;
			else if (tempArtifWorn.find(slot) != tempArtifWorn.end())
				return false;
		}

		// Ensure enough ring slots are free
		for (int i = 0; i < sizeof(ringSlots)/sizeof(*ringSlots); i++) 
		{
			if (tempArtifWorn.find(ringSlots[i]) == tempArtifWorn.end() || ringSlots[i] == slotID)
				rings--;
		}
		if (rings > 0)
			return false;

		// Ensure enough misc slots are free.
		for (int i = 0; i < sizeof(miscSlots)/sizeof(*miscSlots); i++) 
		{
			if (tempArtifWorn.find(miscSlots[i]) == tempArtifWorn.end() || miscSlots[i] == slotID)
				misc--;
		}
		if (misc > 0)
			return false;
	}

	return true;
}

bool CArtifact::canBeAssembledTo (const std::map<ui16, ui32> &artifWorn, ui32 artifactID) const
{
	if (constituentOf == NULL || !vstd::contains(*constituentOf, artifactID))
		return false;

	const CArtifact &artifact = *VLC->arth->artifacts[artifactID];
	assert(artifact.constituents);

	BOOST_FOREACH(ui32 constituentID, *artifact.constituents) 
	{
		bool found = false;
		for (std::map<ui16, ui32>::const_iterator it = artifWorn.begin(); it != artifWorn.end(); ++it) 
		{
			if (it->second == constituentID) 
			{
				found = true;
				break;
			}
		}
		if (!found)
			return false;
	}

	return true;
}

CArtifact::CArtifact()
{
	nodeType = ARTIFACT;
}

CArtifact::~CArtifact()
{
}

int CArtifact::getArtClassSerial() const
{
	if(id == 1)
		return 4;
	switch(aClass)
	{
	case ART_TREASURE:
		return 0;
	case ART_MINOR:
		return 1;
	case ART_MAJOR:
		return 2;
	case ART_RELIC:
		return 3;
	case ART_SPECIAL:
		return 5;
	}

	return -1;
}

void CArtifact::getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const
{
	//combined artifact carries bonuses from its parts
	if(constituents)
	{
		BOOST_FOREACH(ui32 id, *constituents)
			out.insert(VLC->arth->artifacts[id]);
	}
}

CArtHandler::CArtHandler()
{
	VLC->arth = this;

	// War machines are the default big artifacts.
	for (ui32 i = 3; i <= 6; i++)
		bigArtifacts.insert(i);
}

CArtHandler::~CArtHandler()
{
	for (std::vector<CArtifact*>::iterator it = artifacts.begin(); it != artifacts.end(); ++it) {
		delete (*it)->constituents;
		delete (*it)->constituentOf;
	}
}

void CArtHandler::loadArtifacts(bool onlyTxt)
{
	std::vector<ui16> slots;
	slots += 17, 16, 15, 14, 13, 18, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0;
	static std::map<char, CArtifact::EartClass> classes = 
	  map_list_of('S',CArtifact::ART_SPECIAL)('T',CArtifact::ART_TREASURE)('N',CArtifact::ART_MINOR)('J',CArtifact::ART_MAJOR)('R',CArtifact::ART_RELIC);
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
		CArtifact *art = new CArtifact;
		CArtifact &nart = *art;
		nart.id=i;
		loadToIt(VLC->generaltexth->artifNames[i],buf,it,4);
		loadToIt(pom,buf,it,4);
		nart.price=atoi(pom.c_str());
		for(int j=0;j<slots.size();j++)
		{
			loadToIt(pom,buf,it,4);
			if(pom.size() && pom[0]=='x')
				nart.possibleSlots.push_back(slots[j]);
		}
		loadToIt(pom,buf,it,4);
		nart.aClass = classes[pom[0]];

		//load description and remove quotation marks
		std::string &desc = VLC->generaltexth->artifDescriptions[i];
		loadToIt(desc,buf,it,3);
		if(desc[0] == '\"' && desc[desc.size()-1] == '\"')
			desc = desc.substr(1,desc.size()-2);

		if(onlyTxt)
			continue;

		// Fill in information about combined artifacts. Should perhaps be moved to a config file?
		nart.constituentOf = NULL;
		switch (nart.id) 
		{
			case 129: // Angelic Alliance
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 31, 32, 33, 34, 35, 36;
				break;

			case 130: // Cloak of the Undead King
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 54, 55, 56;
				break;

			case 131: // Elixir of Life
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 94, 95, 96;
				break;

			case 132: // Armor of the Damned
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 8, 14, 20, 26;
				break;

			case 133: // Statue of Legion
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 118, 119, 120, 121, 122;
				break;

			case 134: // Power of the Dragon Father
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 37, 38, 39, 40, 41, 42, 43, 44, 45;
				break;

			case 135: // Titan's Thunder
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 12, 18, 24, 30;
				break;

			case 136: // Admiral's Hat
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 71, 123;
				break;

			case 137: // Bow of the Sharpshooter
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 60, 61, 62;
				break;

			case 138: // Wizards' Well
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 73, 74, 75;
				break;

			case 139: // Ring of the Magi
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 76, 77, 78;
				break;

			case 140: // Cornucopia
				nart.constituents = new std::vector<ui32>();
				*nart.constituents += 109, 110, 111, 113;
				break;

			// TODO: WoG combinationals

			default:
				nart.constituents = NULL;
				break;
		}

		artifacts.push_back(&nart);
	}
	sortArts();
	if(onlyTxt)
		return;

	addBonuses();

	// Populate reverse mappings of combinational artifacts.
	BOOST_FOREACH(CArtifact *artifact, artifacts) 
	{
		if (artifact->constituents != NULL) 
		{
			BOOST_FOREACH(ui32 constituentID, *artifact->constituents) 
			{
				if (artifacts[constituentID]->constituentOf == NULL)
					artifacts[constituentID]->constituentOf = new std::vector<ui32>();
				artifacts[constituentID]->constituentOf->push_back(artifact->id);
			}
		}
	}
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
// 	for (int i=0; i<allowedArtifacts.size(); ++i) //do 144, bo nie chcemy bzdurek
// 	{
// 		switch (allowedArtifacts[i]->aClass)
// 		{
// 		case CArtifact::ART_TREASURE:
// 			treasures.push_back(allowedArtifacts[i]);
// 			break;
// 		case CArtifact::ART_MINOR:
// 			minors.push_back(allowedArtifacts[i]);
// 			break;
// 		case CArtifact::ART_MAJOR:
// 			majors.push_back(allowedArtifacts[i]);
// 			break;
// 		case CArtifact::ART_RELIC:
// 			relics.push_back(allowedArtifacts[i]);
// 			break;
// 		}
// 	}
}
void CArtHandler::erasePickedArt (si32 id)
{
	std::vector<CArtifact*>* ptr;
	CArtifact *art = artifacts[id];
	switch (art->aClass)
	{
		case CArtifact::ART_TREASURE:
			ptr = &treasures;
			break;
		case CArtifact::ART_MINOR:
			ptr = &minors;
			break;
		case CArtifact::ART_MAJOR:
			ptr = &majors;
			break;
		case CArtifact::ART_RELIC:
			ptr = &relics;
			break;
		default: //special artifacts should not be erased
			return;
	}
	ptr->erase (std::find(ptr->begin(), ptr->end(), art)); //remove the artifact from avaliable list
}
ui16 CArtHandler::getRandomArt(int flags)
{
	std::vector<CArtifact*> out;
	getAllowed(out, flags);
	ui16 id = out[ran() % out.size()]->id;
	erasePickedArt (id);
	return id;
}
ui16 CArtHandler::getArtSync (ui32 rand, int flags)
{
	std::vector<CArtifact*> out;
	getAllowed(out, flags);
	CArtifact *art = out[rand % out.size()];
	return art->id;	
}
void CArtHandler::getAllowed(std::vector<CArtifact*> &out, int flags)
{
	if (flags & CArtifact::ART_TREASURE)
		getAllowedArts (out, &treasures, CArtifact::ART_TREASURE);
	if (flags & CArtifact::ART_MINOR)
		getAllowedArts (out, &minors, CArtifact::ART_MINOR);
	if (flags & CArtifact::ART_MAJOR)
		getAllowedArts (out, &majors, CArtifact::ART_MAJOR);
	if (flags & CArtifact::ART_RELIC)
		getAllowedArts (out, &relics, CArtifact::ART_RELIC);
	if (!out.size()) //no arts are avaliable
	{
		out.resize (64);
		std::fill_n (out.begin(), 64, artifacts[2]); //magic
	}
}
void CArtHandler::getAllowedArts(std::vector<CArtifact*> &out, std::vector<CArtifact*> *arts, int flag)
{
	if (arts->empty()) //restock avaliable arts
	{
		for (int i = 0; i < allowedArtifacts.size(); ++i)
		{
			if (allowedArtifacts[i]->aClass == flag)
				arts->push_back(allowedArtifacts[i]);
		}
	}

	for (int i = 0; i < arts->size(); ++i)
	{
		CArtifact *art = (*arts)[i];
		out.push_back(art);
	}
}
void CArtHandler::giveArtBonus( int aid, Bonus::BonusType type, int val, int subtype, int valType, ILimiter * limiter )
{
	Bonus added(Bonus::PERMANENT,type,Bonus::ARTIFACT,val,aid,subtype);
	added.valType = valType;
	added.limiter = limiter;
	if(type == Bonus::MORALE || Bonus::LUCK)
		added.description = "\n" + artifacts[aid]->Name()  + (val > 0 ? " +" : " ") + boost::lexical_cast<std::string>(val);
	artifacts[aid]->bonuses.push_back(added);
}

void CArtHandler::addBonuses()
{
	#define ART_PRIM_SKILL(ID, whichSkill, val) giveArtBonus(ID,Bonus::PRIMARY_SKILL,val,whichSkill)
	#define ART_MORALE(ID, val) giveArtBonus(ID,Bonus::MORALE,val)
	#define ART_LUCK(ID, val) giveArtBonus(ID,Bonus::LUCK,val)
	#define ART_MORALE_AND_LUCK(ID, val) giveArtBonus(ID,Bonus::MORALE_AND_LUCK,val)
	#define ART_ALL_PRIM_SKILLS(ID, val) ART_PRIM_SKILL(ID,0,val); ART_PRIM_SKILL(ID,1,val); ART_PRIM_SKILL(ID,2,val); ART_PRIM_SKILL(ID,3,val)
	#define ART_ATTACK_AND_DEFENSE(ID, val) ART_PRIM_SKILL(ID,0,val); ART_PRIM_SKILL(ID,1,val)
	#define ART_POWER_AND_KNOWLEDGE(ID, val) ART_PRIM_SKILL(ID,2,val); ART_PRIM_SKILL(ID,3,val)

	//Attack bonus artifacts (Weapons)
	ART_PRIM_SKILL(7,0,+2); //Centaur Axe
	ART_PRIM_SKILL(8,0,+3); //Blackshard of the Dead Knight
	ART_PRIM_SKILL(9,0,+4); //Greater Gnoll's Flail
	ART_PRIM_SKILL(10,0,+5); //Ogre's Club of Havoc
	ART_PRIM_SKILL(11,0,+6); //Sword of Hellfire
	ART_PRIM_SKILL(12,0,+12); //Titan's Gladius
	ART_PRIM_SKILL(12,1,-3);  //Titan's Gladius

	//Defense bonus artifacts (Shields)
	ART_PRIM_SKILL(13,1,+2); //Shield of the Dwarven Lords
	ART_PRIM_SKILL(14,1,+3); //Shield of the Yawning Dead
	ART_PRIM_SKILL(15,1,+4); //Buckler of the Gnoll King
	ART_PRIM_SKILL(16,1,+5); //Targ of the Rampaging Ogre
	ART_PRIM_SKILL(17,1,+6); //Shield of the Damned
	ART_PRIM_SKILL(18,1,+12); //Sentinel's Shield
	ART_PRIM_SKILL(18,0,-3);  //Sentinel's Shield

	//Knowledge bonus artifacts (Helmets)
	ART_PRIM_SKILL(19,3,+1); //Helm of the Alabaster Unicorn 
	ART_PRIM_SKILL(20,3,+2); //Skull Helmet
	ART_PRIM_SKILL(21,3,+3); //Helm of Chaos
	ART_PRIM_SKILL(22,3,+4); //Crown of the Supreme Magi
	ART_PRIM_SKILL(23,3,+5); //Hellstorm Helmet
	ART_PRIM_SKILL(24,3,+10); //Thunder Helmet
	ART_PRIM_SKILL(24,2,-2);  //Thunder Helmet

	//Spell power bonus artifacts (Armours)
	ART_PRIM_SKILL(25,2,+1); //Breastplate of Petrified Wood
	ART_PRIM_SKILL(26,2,+2); //Rib Cage
	ART_PRIM_SKILL(27,2,+3); //Scales of the Greater Basilisk
	ART_PRIM_SKILL(28,2,+4); //Tunic of the Cyclops King
	ART_PRIM_SKILL(29,2,+5); //Breastplate of Brimstone
	ART_PRIM_SKILL(30,2,+10); //Titan's Cuirass
	ART_PRIM_SKILL(30,3,-2);  //Titan's Cuirass

	//All primary skills (various)
	ART_ALL_PRIM_SKILLS(31,+1); //Armor of Wonder
	ART_ALL_PRIM_SKILLS(32,+2); //Sandals of the Saint
	ART_ALL_PRIM_SKILLS(33,+3); //Celestial Necklace of Bliss
	ART_ALL_PRIM_SKILLS(34,+4); //Lion's Shield of Courage
	ART_ALL_PRIM_SKILLS(35,+5); //Sword of Judgement
	ART_ALL_PRIM_SKILLS(36,+6); //Helm of Heavenly Enlightenment

	//Attack and Defense (various)
	ART_ATTACK_AND_DEFENSE(37,+1); //Quiet Eye of the Dragon
	ART_ATTACK_AND_DEFENSE(38,+2); //Red Dragon Flame Tongue
	ART_ATTACK_AND_DEFENSE(39,+3); //Dragon Scale Shield
	ART_ATTACK_AND_DEFENSE(40,+4); //Dragon Scale Armor

	//Spell power and Knowledge (various)
	ART_POWER_AND_KNOWLEDGE(41,+1); //Dragonbone Greaves
	ART_POWER_AND_KNOWLEDGE(42,+2); //Dragon Wing Tabard
	ART_POWER_AND_KNOWLEDGE(43,+3); //Necklace of Dragonteeth
	ART_POWER_AND_KNOWLEDGE(44,+4); //Crown of Dragontooth

	//Luck and morale 
	ART_MORALE(45,+1); //Still Eye of the Dragon
	ART_LUCK(45,+1); //Still Eye of the Dragon
	ART_LUCK(46,+1); //Clover of Fortune
	ART_LUCK(47,+1); //Cards of Prophecy
	ART_LUCK(48,+1); //Ladybird of Luck
	ART_MORALE(49,+1); //Badge of Courage -> +1 morale and immunity to hostile mind spells:
	giveArtBonus(49,Bonus::SPELL_IMMUNITY,0,50);//sorrow
	giveArtBonus(49,Bonus::SPELL_IMMUNITY,0,59);//berserk
	giveArtBonus(49,Bonus::SPELL_IMMUNITY,0,60);//hypnotize
	giveArtBonus(49,Bonus::SPELL_IMMUNITY,0,61);//forgetfulness
	giveArtBonus(49,Bonus::SPELL_IMMUNITY,0,62);//blind
	ART_MORALE(50,+1); //Crest of Valor
	ART_MORALE(51,+1); //Glyph of Gallantry

	giveArtBonus(52,Bonus::SIGHT_RADIOUS,+1);//Speculum
	giveArtBonus(53,Bonus::SIGHT_RADIOUS,+1);//Spyglass

	//necromancy bonus
	giveArtBonus(54,Bonus::SECONDARY_SKILL_PREMY,+5,12, Bonus::ADDITIVE_VALUE);//Amulet of the Undertaker
	giveArtBonus(55,Bonus::SECONDARY_SKILL_PREMY,+10,12, Bonus::ADDITIVE_VALUE);//Vampire's Cowl
	giveArtBonus(56,Bonus::SECONDARY_SKILL_PREMY,+15,12, Bonus::ADDITIVE_VALUE);//Dead Man's Boots

	giveArtBonus(57,Bonus::MAGIC_RESISTANCE,+5);//Garniture of Interference
	giveArtBonus(58,Bonus::MAGIC_RESISTANCE,+10);//Surcoat of Counterpoise
	giveArtBonus(59,Bonus::MAGIC_RESISTANCE,+15);//Boots of Polarity

	//archery bonus
	giveArtBonus(60,Bonus::SECONDARY_SKILL_PREMY,+5,1, Bonus::ADDITIVE_VALUE);//Bow of Elven Cherrywood
	giveArtBonus(61,Bonus::SECONDARY_SKILL_PREMY,+10,1, Bonus::ADDITIVE_VALUE);//Bowstring of the Unicorn's Mane
	giveArtBonus(62,Bonus::SECONDARY_SKILL_PREMY,+15,1, Bonus::ADDITIVE_VALUE);//Angel Feather Arrows

	//eagle eye bonus
	giveArtBonus(63,Bonus::SECONDARY_SKILL_PREMY,+5,11, Bonus::ADDITIVE_VALUE);//Bird of Perception
	giveArtBonus(64,Bonus::SECONDARY_SKILL_PREMY,+10,11, Bonus::ADDITIVE_VALUE);//Stoic Watchman
	giveArtBonus(65,Bonus::SECONDARY_SKILL_PREMY,+15,11, Bonus::ADDITIVE_VALUE);//Emblem of Cognizance

	//reducing cost of surrendering
	giveArtBonus(66,Bonus::SURRENDER_DISCOUNT,+10);//Statesman's Medal
	giveArtBonus(67,Bonus::SURRENDER_DISCOUNT,+10);//Diplomat's Ring
	giveArtBonus(68,Bonus::SURRENDER_DISCOUNT,+10);//Ambassador's Sash

	giveArtBonus(69,Bonus::STACKS_SPEED,+1);//Ring of the Wayfarer

	giveArtBonus(70,Bonus::LAND_MOVEMENT,+300);//Equestrian's Gloves
	giveArtBonus(71,Bonus::SEA_MOVEMENT,+1000);//Necklace of Ocean Guidance
	giveArtBonus(72,Bonus::FLYING_MOVEMENT, 0, 1);//Angel Wings

	giveArtBonus(73,Bonus::MANA_REGENERATION,+1);//Charm of Mana
	giveArtBonus(74,Bonus::MANA_REGENERATION,+2);//Talisman of Mana
	giveArtBonus(75,Bonus::MANA_REGENERATION,+3);//Mystic Orb of Mana

	giveArtBonus(76,Bonus::SPELL_DURATION,+1);//Collar of Conjuring
	giveArtBonus(77,Bonus::SPELL_DURATION,+2);//Ring of Conjuring
	giveArtBonus(78,Bonus::SPELL_DURATION,+3);//Cape of Conjuring

	giveArtBonus(79,Bonus::AIR_SPELL_DMG_PREMY,+50);//Orb of the Firmament
	giveArtBonus(80,Bonus::EARTH_SPELL_DMG_PREMY,+50);//Orb of Silt
	giveArtBonus(81,Bonus::FIRE_SPELL_DMG_PREMY,+50);//Orb of Tempestuous Fire
	giveArtBonus(82,Bonus::WATER_SPELL_DMG_PREMY,+50);//Orb of Driving Rain

	giveArtBonus(83,Bonus::BLOCK_SPELLS_ABOVE_LEVEL,3);//Recanter's Cloak
	giveArtBonus(84,Bonus::BLOCK_MORALE,0);//Spirit of Oppression
	giveArtBonus(85,Bonus::BLOCK_LUCK,0);//Hourglass of the Evil Hour

	giveArtBonus(86,Bonus::FIRE_SPELLS,0);//Tome of Fire Magic
	giveArtBonus(87,Bonus::AIR_SPELLS,0);//Tome of Air Magic
	giveArtBonus(88,Bonus::WATER_SPELLS,0);//Tome of Water Magic
	giveArtBonus(89,Bonus::EARTH_SPELLS,0);//Tome of Earth Magic

	giveArtBonus(90,Bonus::WATER_WALKING, 0, 1);//Boots of Levitation
	giveArtBonus(91,Bonus::NO_SHOTING_PENALTY,0);//Golden Bow
	giveArtBonus(92,Bonus::SPELL_IMMUNITY,0,35);//Sphere of Permanence
	giveArtBonus(93,Bonus::NEGATE_ALL_NATURAL_IMMUNITIES,0);//Orb of Vulnerability

	giveArtBonus(94,Bonus::STACK_HEALTH,+1);//Ring of Vitality
	giveArtBonus(95,Bonus::STACK_HEALTH,+1);//Ring of Life
	giveArtBonus(96,Bonus::STACK_HEALTH,+2);//Vial of Lifeblood

	giveArtBonus(97,Bonus::STACKS_SPEED,+1);//Necklace of Swiftness
	giveArtBonus(98,Bonus::LAND_MOVEMENT,+600);//Boots of Speed
	giveArtBonus(99,Bonus::STACKS_SPEED,+2);//Cape of Velocity

	giveArtBonus(100,Bonus::SPELL_IMMUNITY,0,59);//Pendant of Dispassion
	giveArtBonus(101,Bonus::SPELL_IMMUNITY,0,62);//Pendant of Second Sight
	giveArtBonus(102,Bonus::SPELL_IMMUNITY,0,42);//Pendant of Holiness
	giveArtBonus(103,Bonus::SPELL_IMMUNITY,0,24);//Pendant of Life
	giveArtBonus(104,Bonus::SPELL_IMMUNITY,0,25);//Pendant of Death
	giveArtBonus(105,Bonus::SPELL_IMMUNITY,0,60);//Pendant of Free Will
	giveArtBonus(106,Bonus::SPELL_IMMUNITY,0,17);//Pendant of Negativity
	giveArtBonus(107,Bonus::SPELL_IMMUNITY,0,61);//Pendant of Total Recall
	giveArtBonus(108,Bonus::MORALE,+3);//Pendant of Courage
	giveArtBonus(108,Bonus::LUCK,+3);//Pendant of Courage

	giveArtBonus(109,Bonus::GENERATE_RESOURCE,+1,4); //Everflowing Crystal Cloak
	giveArtBonus(110,Bonus::GENERATE_RESOURCE,+1,5); //Ring of Infinite Gems
	giveArtBonus(111,Bonus::GENERATE_RESOURCE,+1,1); //Everpouring Vial of Mercury
	giveArtBonus(112,Bonus::GENERATE_RESOURCE,+1,2); //Inexhaustible Cart of Ore
	giveArtBonus(113,Bonus::GENERATE_RESOURCE,+1,3); //Eversmoking Ring of Sulfur
	giveArtBonus(114,Bonus::GENERATE_RESOURCE,+1,0); //Inexhaustible Cart of Lumber
	giveArtBonus(115,Bonus::GENERATE_RESOURCE,+1000,6); //Endless Sack of Gold
	giveArtBonus(116,Bonus::GENERATE_RESOURCE,+750,6); //Endless Bag of Gold
	giveArtBonus(117,Bonus::GENERATE_RESOURCE,+500,6); //Endless Purse of Gold

	giveArtBonus(118,Bonus::CREATURE_GROWTH,+5,1); //Legs of Legion
	giveArtBonus(119,Bonus::CREATURE_GROWTH,+4,2); //Loins of Legion
	giveArtBonus(120,Bonus::CREATURE_GROWTH,+3,3); //Torso of Legion
	giveArtBonus(121,Bonus::CREATURE_GROWTH,+2,4); //Arms of Legion
	giveArtBonus(122,Bonus::CREATURE_GROWTH,+1,5); //Head of Legion

	//Sea Captain's Hat 
	giveArtBonus(123,Bonus::WHIRLPOOL_PROTECTION,0); 
	giveArtBonus(123,Bonus::SEA_MOVEMENT,+500); 
	giveArtBonus(123,Bonus::SPELL,3,0, Bonus::INDEPENDENT_MAX); 
	giveArtBonus(123,Bonus::SPELL,3,1, Bonus::INDEPENDENT_MAX); 

	giveArtBonus(124,Bonus::SPELLS_OF_LEVEL,3,1); //Spellbinder's Hat
	giveArtBonus(125,Bonus::ENEMY_CANT_ESCAPE,0); //Shackles of War
	giveArtBonus(126,Bonus::BLOCK_SPELLS_ABOVE_LEVEL,0);//Orb of Inhibition

	//vial of dragon blood
	giveArtBonus(127, Bonus::PRIMARY_SKILL, +5, PrimarySkill::ATTACK, Bonus::BASE_NUMBER, new HasAnotherBonusLimiter(Bonus::DRAGON_NATURE));
	giveArtBonus(127, Bonus::PRIMARY_SKILL, +5, PrimarySkill::DEFENSE, Bonus::BASE_NUMBER, new HasAnotherBonusLimiter(Bonus::DRAGON_NATURE));

	//Armageddon's Blade
	giveArtBonus(128, Bonus::SPELL, 3, 26, Bonus::INDEPENDENT_MAX);
	giveArtBonus(128, Bonus::SPELL_IMMUNITY,0, 26);
	ART_ATTACK_AND_DEFENSE(128, +3);
	ART_PRIM_SKILL(128, 2, +3);
	ART_PRIM_SKILL(128, 3, +6);

	//Angelic Alliance
	giveArtBonus(129, Bonus::NONEVIL_ALIGNMENT_MIX, 0);
	giveArtBonus(129, Bonus::OPENING_BATTLE_SPELL, 10, 29); // Prayer

	//Cloak of the Undead King
	giveArtBonus(130, Bonus::IMPROVED_NECROMANCY, 0);

	//Elixir of Life
	giveArtBonus(131, Bonus::STACK_HEALTH, +25, -1, Bonus::PERCENT_TO_BASE);
	giveArtBonus(131, Bonus::HP_REGENERATION, +50);

	//Armor of the Damned
	giveArtBonus(132, Bonus::OPENING_BATTLE_SPELL, 50, 54); // Slow
	giveArtBonus(132, Bonus::OPENING_BATTLE_SPELL, 50, 47); // Disrupting Ray
	giveArtBonus(132, Bonus::OPENING_BATTLE_SPELL, 50, 45); // Weakness
	giveArtBonus(132, Bonus::OPENING_BATTLE_SPELL, 50, 52); // Misfortune

	// Statue of Legion - gives only 50% growth
	giveArtBonus(133, Bonus::CREATURE_GROWTH_PERCENT, 50);

	//Power of the Dragon Father
	giveArtBonus(134, Bonus::LEVEL_SPELL_IMMUNITY, 4);

	//Titan's Thunder
	// FIXME: should also add a permanent spell book, somehow.
	giveArtBonus(135, Bonus::SPELL, 3, 57);

	//Admiral's Hat
	giveArtBonus(136, Bonus::FREE_SHIP_BOARDING, 0);

	//Bow of the Sharpshooter
	giveArtBonus(137, Bonus::NO_SHOTING_PENALTY, 0);
	giveArtBonus(137, Bonus::FREE_SHOOTING, 0);

	//Wizard's Well
	giveArtBonus(138, Bonus::FULL_MANA_REGENERATION, 0);

	//Ring of the Magi
	giveArtBonus(139, Bonus::SPELL_DURATION, +50);

	//Cornucopia
	giveArtBonus(140, Bonus::GENERATE_RESOURCE, +4, 1);
	giveArtBonus(140, Bonus::GENERATE_RESOURCE, +4, 3);
	giveArtBonus(140, Bonus::GENERATE_RESOURCE, +4, 4);
	giveArtBonus(140, Bonus::GENERATE_RESOURCE, +4, 5);
}

void CArtHandler::clear()
{
	BOOST_FOREACH(CArtifact *art, artifacts)
		delete art;
	artifacts.clear();

	clearHlpLists();

}

/**
 * Locally equips an artifact to a hero's worn slots. Unequips an already present artifact.
 * Does not test if the operation is legal.
 * @param artifWorn A hero's set of worn artifacts.
 * @param bonuses Optional list of bonuses to update.
 */
void CArtHandler::equipArtifact(std::map<ui16, ui32> &artifWorn, ui16 slotID, ui32 artifactID)
{
	unequipArtifact(artifWorn, slotID);

	const CArtifact &artifact = *artifacts[artifactID];

	// Add artifact.
	artifWorn[slotID] = artifactID;

	// Add locks, in reverse order of being removed.
	if (artifact.constituents != NULL) 
	{
		bool destConsumed = false; // Determines which constituent that will be counted for together with the artifact.

		BOOST_FOREACH(ui32 constituentID, *artifact.constituents) 
		{
			const CArtifact &constituent = *artifacts[constituentID];

			if (!destConsumed && vstd::contains(constituent.possibleSlots, slotID)) 
			{
				destConsumed = true;
			} 
			else 
			{
				BOOST_FOREACH(ui16 slot, constituent.possibleSlots) 
				{
					if (!vstd::contains(artifWorn, slot)) 
					{
						artifWorn[slot] = 145;
						break;
					}
				}
			}
		}
	}
}

/**
 * Locally unequips an artifact from a hero's worn slots.
 * Does not test if the operation is legal.
 * @param artifWorn A hero's set of worn artifacts.
 * @param bonuses Optional list of bonuses to update.
 */
void CArtHandler::unequipArtifact(std::map<ui16, ui32> &artifWorn, ui16 slotID)
{
	if (!vstd::contains(artifWorn, slotID))
		return;

	const CArtifact &artifact = *artifacts[artifWorn[slotID]];

	// Remove artifact, if it's not already removed.
	artifWorn.erase(slotID);

	// Remove locks, in reverse order of being added.
	if (artifact.constituents != NULL) 
	{
		bool destConsumed = false;

		BOOST_FOREACH(ui32 constituentID, *artifact.constituents) 
		{
			const CArtifact &constituent = *artifacts[constituentID];

			if (!destConsumed && vstd::contains(constituent.possibleSlots, slotID)) 
			{
				destConsumed = true;
			} 
			else 
			{
				BOOST_REVERSE_FOREACH(ui16 slot, constituent.possibleSlots) 
				{
					if (vstd::contains(artifWorn, slot) && artifWorn[slot] == 145) 
					{
						artifWorn.erase(slot);
						break;
					}
				}
			}
		}
	}
}

void CArtHandler::clearHlpLists()
{
	treasures.clear();
	minors.clear();
	majors.clear();
	relics.clear();
}

void CArtHandler::initAllowedArtifactsList(const std::vector<ui8> &allowed)
{
	allowedArtifacts.clear();
	clearHlpLists();
	for (int i=0; i<144; ++i) //yes, 144
	{
		if (allowed[i])
			allowedArtifacts.push_back(artifacts[i]);
	}
}

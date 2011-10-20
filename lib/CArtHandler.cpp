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
#include <boost/algorithm/string/replace.hpp>
#include "../lib/VCMI_Lib.h"
#include "CSpellHandler.h"
#include "CObjectHandler.h"
#include "NetPacks.h"

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
// 
// bool CArtifact::isModable () const
// {
// 	return (bool)dynamic_cast<const IModableArt *>(this);
// }

// /**
//  * Checks whether the artifact fits at a given slot.
//  * @param artifWorn A hero's set of worn artifacts.
//  */
// bool CArtifact::fitsAt (const std::map<ui16, const CArtifact*> &artifWorn, ui16 slotID) const
// {
// 	if (!vstd::contains(possibleSlots, slotID))
// 		return false;
// 
// 	// Can't put an artifact in a locked slot.
// 	std::map<ui16, const CArtifact*>::const_iterator it = artifWorn.find(slotID);
// 	if (it != artifWorn.end() && it->second->id == 145)
// 		return false;
// 
// 	// Check if a combination artifact fits.
// 	// TODO: Might want a more general algorithm?
// 	//       Assumes that misc & rings fits only in their slots, and others in only one slot and no duplicates.
// 	if (constituents != NULL)
// 	{
// 		std::map<ui16, const CArtifact*> tempArtifWorn = artifWorn;
// 		const ui16 ringSlots[] = {6, 7};
// 		const ui16 miscSlots[] = {9, 10, 11, 12, 18};
// 		int rings = 0;
// 		int misc = 0;
// 
// 		VLC->arth->unequipArtifact(tempArtifWorn, slotID);
// 
// 		BOOST_FOREACH(ui32 constituentID, *constituents) 
// 		{
// 			const CArtifact& constituent = *VLC->arth->artifacts[constituentID];
// 			const int slot = constituent.possibleSlots[0];
// 
// 			if (slot == 6 || slot == 7)
// 				rings++;
// 			else if ((slot >= 9 && slot <= 12) || slot == 18)
// 				misc++;
// 			else if (tempArtifWorn.find(slot) != tempArtifWorn.end())
// 				return false;
// 		}
// 
// 		// Ensure enough ring slots are free
// 		for (int i = 0; i < sizeof(ringSlots)/sizeof(*ringSlots); i++) 
// 		{
// 			if (tempArtifWorn.find(ringSlots[i]) == tempArtifWorn.end() || ringSlots[i] == slotID)
// 				rings--;
// 		}
// 		if (rings > 0)
// 			return false;
// 
// 		// Ensure enough misc slots are free.
// 		for (int i = 0; i < sizeof(miscSlots)/sizeof(*miscSlots); i++) 
// 		{
// 			if (tempArtifWorn.find(miscSlots[i]) == tempArtifWorn.end() || miscSlots[i] == slotID)
// 				misc--;
// 		}
// 		if (misc > 0)
// 			return false;
// 	}
// 
// 	return true;
// }

// bool CArtifact::canBeAssembledTo (const std::map<ui16, const CArtifact*> &artifWorn, ui32 artifactID) const
// {
// 	if (constituentOf == NULL || !vstd::contains(*constituentOf, artifactID))
// 		return false;
// 
// 	const CArtifact &artifact = *VLC->arth->artifacts[artifactID];
// 	assert(artifact.constituents);
// 
// 	BOOST_FOREACH(ui32 constituentID, *artifact.constituents) 
// 	{
// 		bool found = false;
// 		for (std::map<ui16, const CArtifact*>::const_iterator it = artifWorn.begin(); it != artifWorn.end(); ++it) 
// 		{
// 			if (it->second->id == constituentID) 
// 			{
// 				found = true;
// 				break;
// 			}
// 		}
// 		if (!found)
// 			return false;
// 	}
// 
// 	return true;
// }

CArtifact::CArtifact()
{
	setNodeType(ARTIFACT);
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

std::string CArtifact::nodeName() const
{
	return "Artifact: " + Name();
}
// void CArtifact::getParents(TCNodes &out, const CBonusSystemNode *root /*= NULL*/) const
// {
// 	//combined artifact carries bonuses from its parts
// 	if(constituents)
// 	{
// 		BOOST_FOREACH(ui32 id, *constituents)
// 			out.insert(VLC->arth->artifacts[id]);
// 	}
// }

// void CScroll::Init()
// {
// // 	addNewBonus (Bonus (Bonus::PERMANENT, Bonus::SPELL, Bonus::ARTIFACT, 1, id, spellid, Bonus::INDEPENDENT_MAX));
// // 	//boost::algorithm::replace_first(description, "[spell name]", VLC->spellh->spells[spellid].name);
// }

CArtHandler::CArtHandler()
{
	VLC->arth = this;

	// War machines are the default big artifacts.
	for (ui32 i = 3; i <= 6; i++)
		bigArtifacts.insert(i);
	//modableArtifacts = boost::assign::map_list_of(1, 1)(146,3)(147,3)(148,3)(150,3)(151,3)(152,3)(154,3)(156,2);
}

CArtHandler::~CArtHandler()
{
	for (std::vector< ConstTransitivePtr<CArtifact> >::iterator it = artifacts.begin(); it != artifacts.end(); ++it)
	{
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
	std::map<ui32,ui8>::iterator itr;
	for (int i=0; i<ARTIFACTS_QUANTITY; i++)
	{
		CArtifact *art = new CArtifact();
// 		if ((itr = modableArtifacts.find(i)) != modableArtifacts.end())
// 		{
// 			switch (itr->second)
// 			{
// 			case 1:
// 				art = new CScroll;
// 				break;
// 			case 2:
// 				art = new CCustomizableArt;
// 				break;
// 			case 3:
// 				art = new CCommanderArt;
// 				break;
// 			};
// 		}
// 		else
//			art = new CArtifact;

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
 	//for (int i=0; i<allowedArtifacts.size(); ++i) //do 144, bo nie chcemy bzdurek
 	//{
 	//	switch (allowedArtifacts[i]->aClass)
 	//	{
 	//	case CArtifact::ART_TREASURE:
 	//		treasures.push_back(allowedArtifacts[i]);
 	//		break;
 	//	case CArtifact::ART_MINOR:
 	//		minors.push_back(allowedArtifacts[i]);
 	//		break;
 	//	case CArtifact::ART_MAJOR:
 	//		majors.push_back(allowedArtifacts[i]);
 	//		break;
 	//	case CArtifact::ART_RELIC:
 	//		relics.push_back(allowedArtifacts[i]);
 	//		break;
 	//	}
 	//}
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
	ptr->erase (std::find(ptr->begin(), ptr->end(), art)); //remove the artifact from available list
}
ui16 CArtHandler::getRandomArt(int flags)
{
	std::vector<ConstTransitivePtr<CArtifact> > out;
	getAllowed(out, flags);
	ui16 id = out[ran() % out.size()]->id;
	erasePickedArt (id);
	return id;
}
ui16 CArtHandler::getArtSync (ui32 rand, int flags)
{
	std::vector<ConstTransitivePtr<CArtifact> > out;
	getAllowed(out, flags);
	CArtifact *art = out[rand % out.size()];
	return art->id;	
}
void CArtHandler::getAllowed(std::vector<ConstTransitivePtr<CArtifact> > &out, int flags)
{
	if (flags & CArtifact::ART_TREASURE)
		getAllowedArts (out, &treasures, CArtifact::ART_TREASURE);
	if (flags & CArtifact::ART_MINOR)
		getAllowedArts (out, &minors, CArtifact::ART_MINOR);
	if (flags & CArtifact::ART_MAJOR)
		getAllowedArts (out, &majors, CArtifact::ART_MAJOR);
	if (flags & CArtifact::ART_RELIC)
		getAllowedArts (out, &relics, CArtifact::ART_RELIC);
	if (!out.size()) //no artifact of specified rarity, we need to take another one
	{
		getAllowedArts (out, &treasures, CArtifact::ART_TREASURE);
		getAllowedArts (out, &minors, CArtifact::ART_MINOR);
		getAllowedArts (out, &majors, CArtifact::ART_MAJOR);
		getAllowedArts (out, &relics, CArtifact::ART_RELIC);
	}
	if (!out.size()) //no arts are available at all
	{
		out.resize (64);
		std::fill_n (out.begin(), 64, artifacts[2]); //Give Grail - this can't be banned (hopefully)
	}
}
void CArtHandler::getAllowedArts(std::vector<ConstTransitivePtr<CArtifact> > &out, std::vector<CArtifact*> *arts, int flag)
{
	if (arts->empty()) //restock available arts
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
	Bonus *added = new Bonus(Bonus::PERMANENT,type,Bonus::ARTIFACT,val,aid,subtype);
	added->valType = valType;
	added->limiter.reset(limiter);
	if(type == Bonus::MORALE || type == Bonus::LUCK)
		added->description = artifacts[aid]->Name()  + (val > 0 ? " +" : " ") + boost::lexical_cast<std::string>(val);
	else
		added->description = artifacts[aid]->Name();
	artifacts[aid]->addNewBonus(added);
}

void CArtHandler::giveArtBonus(int aid, Bonus::BonusType type, int val, int subtype, IPropagator* propagator /*= NULL*/)
{
	Bonus *added = new Bonus(Bonus::PERMANENT,type,Bonus::ARTIFACT,val,aid,subtype);
	added->valType = Bonus::BASE_NUMBER;
	added->propagator.reset(propagator);
	if(type == Bonus::MORALE || type == Bonus::LUCK)
		added->description = artifacts[aid]->Name()  + (val > 0 ? " +" : " ") + boost::lexical_cast<std::string>(val);
	else
		added->description = artifacts[aid]->Name();
	artifacts[aid]->addNewBonus(added);
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
	giveArtBonus(54,Bonus::SECONDARY_SKILL_PREMY,+5, CGHeroInstance::NECROMANCY, Bonus::ADDITIVE_VALUE);//Amulet of the Undertaker
	giveArtBonus(55,Bonus::SECONDARY_SKILL_PREMY,+10, CGHeroInstance::NECROMANCY, Bonus::ADDITIVE_VALUE);//Vampire's Cowl
	giveArtBonus(56,Bonus::SECONDARY_SKILL_PREMY,+15, CGHeroInstance::NECROMANCY, Bonus::ADDITIVE_VALUE);//Dead Man's Boots

	giveArtBonus(57,Bonus::MAGIC_RESISTANCE,+5, 0);//Garniture of Interference
	giveArtBonus(58,Bonus::MAGIC_RESISTANCE,+10, 0);//Surcoat of Counterpoise
	giveArtBonus(59,Bonus::MAGIC_RESISTANCE,+15, 0);//Boots of Polarity

	//archery bonus
	giveArtBonus(60,Bonus::SECONDARY_SKILL_PREMY,+5, CGHeroInstance::ARCHERY, Bonus::ADDITIVE_VALUE);//Bow of Elven Cherrywood
	giveArtBonus(61,Bonus::SECONDARY_SKILL_PREMY,+10,CGHeroInstance::ARCHERY, Bonus::ADDITIVE_VALUE);//Bowstring of the Unicorn's Mane
	giveArtBonus(62,Bonus::SECONDARY_SKILL_PREMY,+15,CGHeroInstance::ARCHERY, Bonus::ADDITIVE_VALUE);//Angel Feather Arrows

	//eagle eye bonus
	giveArtBonus(63,Bonus::SECONDARY_SKILL_PREMY,+5, CGHeroInstance::EAGLE_EYE, Bonus::ADDITIVE_VALUE);//Bird of Perception
	giveArtBonus(64,Bonus::SECONDARY_SKILL_PREMY,+10, CGHeroInstance::EAGLE_EYE, Bonus::ADDITIVE_VALUE);//Stoic Watchman
	giveArtBonus(65,Bonus::SECONDARY_SKILL_PREMY,+15, CGHeroInstance::EAGLE_EYE, Bonus::ADDITIVE_VALUE);//Emblem of Cognizance

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

	giveArtBonus(83,Bonus::LEVEL_SPELL_IMMUNITY,3,-1,Bonus::INDEPENDENT_MAX);//Recanter's Cloak
	giveArtBonus(84,Bonus::BLOCK_MORALE,0);//Spirit of Oppression
	giveArtBonus(85,Bonus::BLOCK_LUCK,0);//Hourglass of the Evil Hour

	giveArtBonus(86,Bonus::FIRE_SPELLS,0);//Tome of Fire Magic
	giveArtBonus(87,Bonus::AIR_SPELLS,0);//Tome of Air Magic
	giveArtBonus(88,Bonus::WATER_SPELLS,0);//Tome of Water Magic
	giveArtBonus(89,Bonus::EARTH_SPELLS,0);//Tome of Earth Magic

	giveArtBonus(90,Bonus::WATER_WALKING, 0, 1);//Boots of Levitation
	giveArtBonus(91,Bonus::NO_DISTANCE_PENALTY,0, 0, 0, new HasAnotherBonusLimiter(Bonus::SHOOTER));//Golden Bow
	giveArtBonus(91,Bonus::NO_WALL_PENALTY, 0, 0, 0, new HasAnotherBonusLimiter(Bonus::SHOOTER));
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
	giveArtBonus(104,Bonus::SPELL_IMMUNITY,0,25, 1, new HasAnotherBonusLimiter(Bonus::UNDEAD));//Pendant of Death does not display info for living stacks
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
	giveArtBonus(115,Bonus::GENERATE_RESOURCE,+1000, Res::GOLD); //Endless Sack of Gold
	giveArtBonus(116,Bonus::GENERATE_RESOURCE,+750, Res::GOLD); //Endless Bag of Gold
	giveArtBonus(117,Bonus::GENERATE_RESOURCE,+500, Res::GOLD); //Endless Purse of Gold

	giveArtBonus(118,Bonus::CREATURE_GROWTH,+5,1, new CPropagatorNodeType(CBonusSystemNode::TOWN_AND_VISITOR)); //Legs of Legion
	giveArtBonus(119,Bonus::CREATURE_GROWTH,+4,2, new CPropagatorNodeType(CBonusSystemNode::TOWN_AND_VISITOR)); //Loins of Legion
	giveArtBonus(120,Bonus::CREATURE_GROWTH,+3,3, new CPropagatorNodeType(CBonusSystemNode::TOWN_AND_VISITOR)); //Torso of Legion
	giveArtBonus(121,Bonus::CREATURE_GROWTH,+2,4, new CPropagatorNodeType(CBonusSystemNode::TOWN_AND_VISITOR)); //Arms of Legion
	giveArtBonus(122,Bonus::CREATURE_GROWTH,+1,5, new CPropagatorNodeType(CBonusSystemNode::TOWN_AND_VISITOR)); //Head of Legion

	//Sea Captain's Hat 
	giveArtBonus(123,Bonus::WHIRLPOOL_PROTECTION,0); 
	giveArtBonus(123,Bonus::SEA_MOVEMENT,+500); 
	giveArtBonus(123,Bonus::SPELL,3,0, Bonus::INDEPENDENT_MAX); 
	giveArtBonus(123,Bonus::SPELL,3,1, Bonus::INDEPENDENT_MAX); 

	giveArtBonus(124,Bonus::SPELLS_OF_LEVEL,3,1); //Spellbinder's Hat
	giveArtBonus(125,Bonus::ENEMY_CANT_ESCAPE,0); //Shackles of War
	giveArtBonus(126,Bonus::LEVEL_SPELL_IMMUNITY,SPELL_LEVELS,-1,Bonus::INDEPENDENT_MAX);//Orb of Inhibition

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
	giveArtBonus(129, Bonus::OPENING_BATTLE_SPELL, 10, 48); // Prayer

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
	giveArtBonus(133, Bonus::CREATURE_GROWTH_PERCENT, 50, -1, new CPropagatorNodeType(CBonusSystemNode::PLAYER));

	//Power of the Dragon Father
	giveArtBonus(134, Bonus::LEVEL_SPELL_IMMUNITY, 4, -1, Bonus::INDEPENDENT_MAX);

	//Titan's Thunder
	giveArtBonus(135, Bonus::SPELL, 3, 57);

	//Admiral's Hat
	giveArtBonus(136, Bonus::FREE_SHIP_BOARDING, 0);

	//Bow of the Sharpshooter
	giveArtBonus(137, Bonus::NO_DISTANCE_PENALTY, 0, 0, 0, new HasAnotherBonusLimiter(Bonus::SHOOTER));
	giveArtBonus(137, Bonus::NO_WALL_PENALTY, 0, 0, 0, new HasAnotherBonusLimiter(Bonus::SHOOTER));
	giveArtBonus(137, Bonus::FREE_SHOOTING, 0, 0, 0, new HasAnotherBonusLimiter(Bonus::SHOOTER));

	//Wizard's Well
	giveArtBonus(138, Bonus::FULL_MANA_REGENERATION, 0);

	//Ring of the Magi
	giveArtBonus(139, Bonus::SPELL_DURATION, +50);

	//Cornucopia
	giveArtBonus(140, Bonus::GENERATE_RESOURCE, +4, Res::MERCURY);
	giveArtBonus(140, Bonus::GENERATE_RESOURCE, +4, Res::SULFUR);
	giveArtBonus(140, Bonus::GENERATE_RESOURCE, +4, Res::CRYSTAL);
	giveArtBonus(140, Bonus::GENERATE_RESOURCE, +4, Res::GEMS);
}

void CArtHandler::clear()
{
	BOOST_FOREACH(CArtifact *art, artifacts)
		delete art;
	artifacts.clear();

	clearHlpLists();

}

// /**
//  * Locally equips an artifact to a hero's worn slots. Unequips an already present artifact.
//  * Does not test if the operation is legal.
//  * @param artifWorn A hero's set of worn artifacts.
//  * @param bonuses Optional list of bonuses to update.
//  */
// void CArtHandler::equipArtifact( std::map<ui16, const CArtifact*> &artifWorn, ui16 slotID, const CArtifact* art ) const
// {
// 	unequipArtifact(artifWorn, slotID);
// 
// 	if (art) //false when artifact is NULL -> slot set to empty
// 	{
// 		const CArtifact &artifact = *art;
// 
// 		// Add artifact.
// 		artifWorn[slotID] = art;
// 
// 		// Add locks, in reverse order of being removed.
// 		if (artifact.constituents != NULL) 
// 		{
// 			bool destConsumed = false; // Determines which constituent that will be counted for together with the artifact.
// 
// 			BOOST_FOREACH(ui32 constituentID, *artifact.constituents) 
// 			{
// 				const CArtifact &constituent = *artifacts[constituentID];
// 
// 				if (!destConsumed && vstd::contains(constituent.possibleSlots, slotID)) 
// 				{
// 					destConsumed = true;
// 				} 
// 				else 
// 				{
// 					BOOST_FOREACH(ui16 slot, constituent.possibleSlots) 
// 					{
// 						if (!vstd::contains(artifWorn, slot)) 
// 						{
// 							artifWorn[slot] = VLC->arth->artifacts[145]; //lock
// 							break;
// 						}
// 					}
// 				}
// 			}
// 		}
// 	}
// }
// 
// /**
//  * Locally unequips an artifact from a hero's worn slots.
//  * Does not test if the operation is legal.
//  * @param artifWorn A hero's set of worn artifacts.
//  * @param bonuses Optional list of bonuses to update.
//  */
// void CArtHandler::unequipArtifact(std::map<ui16, const CArtifact*> &artifWorn, ui16 slotID) const
// {
// 	if (!vstd::contains(artifWorn, slotID))
// 		return;
// 
// 	const CArtifact &artifact = *artifWorn[slotID];
// 
// 	// Remove artifact, if it's not already removed.
// 	artifWorn.erase(slotID);
// 
// 	// Remove locks, in reverse order of being added.
// 	if (artifact.constituents != NULL) 
// 	{
// 		bool destConsumed = false;
// 
// 		BOOST_FOREACH(ui32 constituentID, *artifact.constituents) 
// 		{
// 			const CArtifact &constituent = *artifacts[constituentID];
// 
// 			if (!destConsumed && vstd::contains(constituent.possibleSlots, slotID)) 
// 			{
// 				destConsumed = true;
// 			} 
// 			else 
// 			{
// 				BOOST_REVERSE_FOREACH(ui16 slot, constituent.possibleSlots) 
// 				{
// 					if (vstd::contains(artifWorn, slot) && artifWorn[slot]->id == 145) 
// 					{
// 						artifWorn.erase(slot);
// 						break;
// 					}
// 				}
// 			}
// 		}
// 	}
// }

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

CArtifactInstance::CArtifactInstance()
{
	init();
}

CArtifactInstance::CArtifactInstance( CArtifact *Art)
{
	init();
	setType(Art);
}

// CArtifactInstance::CArtifactInstance(int aid)
// {
// 	init();
// 	setType(VLC->arth->artifacts[aid]);
// }

void CArtifactInstance::setType( CArtifact *Art )
{
	artType = Art;
	attachTo(Art);
}

std::string CArtifactInstance::nodeName() const
{
	return "Artifact instance of " + (artType ? artType->Name() : std::string("uninitialized")) + " type";
}

CArtifactInstance * CArtifactInstance::createScroll( const CSpell *s)
{
	CArtifactInstance *ret = new CArtifactInstance(VLC->arth->artifacts[1]);
	Bonus *b = new Bonus(Bonus::PERMANENT, Bonus::SPELL, Bonus::ARTIFACT_INSTANCE, -1, 1, s->id);
	ret->addNewBonus(b);
	return ret;
}

void CArtifactInstance::init()
{
	id = -1;
	setNodeType(ARTIFACT_INSTANCE);
}

int CArtifactInstance::firstAvailableSlot(const CGHeroInstance *h) const
{
	BOOST_FOREACH(ui16 slot, artType->possibleSlots)
	{
		if(canBePutAt(ArtifactLocation(h, slot))) //if(artType->fitsAt(h->artifWorn, slot))
		{
			//we've found a free suitable slot.
			return slot;
		}
	}

	//if haven't find proper slot, use backpack
	return firstBackpackSlot(h);
}

int CArtifactInstance::firstBackpackSlot(const CGHeroInstance *h) const
{
	if(!artType->isBig()) //discard big artifact
		return Arts::BACKPACK_START + h->artifactsInBackpack.size();

	return -1;
}

bool CArtifactInstance::canBePutAt(const ArtifactLocation &al, bool assumeDestRemoved /*= false*/) const
{
	if(al.slot >= Arts::BACKPACK_START)
	{
		if(artType->isBig())
			return false;
		
		//TODO backpack limit
		return true;
	}

	if(!vstd::contains(artType->possibleSlots, al.slot))
		return false;

	return al.hero->isPositionFree(al.slot, assumeDestRemoved);
}

void CArtifactInstance::putAt(CGHeroInstance *h, ui16 slot)
{
	assert(canBePutAt(ArtifactLocation(h, slot)));

	h->setNewArtSlot(slot, this, false);
	if(slot < Arts::BACKPACK_START)
		h->attachTo(this);
}

void CArtifactInstance::removeFrom(CGHeroInstance *h, ui16 slot)
{
	assert(h->CArtifactSet::getArt(slot) == this);
	h->eraseArtSlot(slot);
	if(slot < Arts::BACKPACK_START)
		h->detachFrom(this);

	//TODO delete me?
}

bool CArtifactInstance::canBeDisassembled() const
{
	return artType->constituents && artType->constituentOf->size();
}

std::vector<const CArtifact *> CArtifactInstance::assemblyPossibilities(const CGHeroInstance *h) const
{
	std::vector<const CArtifact *> ret;
	if(!artType->constituentOf //not a part of combined artifact
		|| artType->constituents) //combined artifact already: no combining of combined artifacts... for now.
		return ret;

	BOOST_FOREACH(ui32 possibleCombinedArt, *artType->constituentOf) 
	{
		const CArtifact * const artifact = VLC->arth->artifacts[possibleCombinedArt];
		assert(artifact->constituents);
		bool possible = true;

		BOOST_FOREACH(ui32 constituentID, *artifact->constituents) //check if all constituents are available
		{
			if(!h->hasArt(constituentID, true)) //constituent must be equipped
			{
				possible = false;
				break;
			}
		}

		if(possible)
			ret.push_back(artifact);
	}

	return ret;
}

void CArtifactInstance::move(ArtifactLocation &src, ArtifactLocation &dst)
{
	removeFrom(src.hero, src.slot);
	putAt(dst.hero, dst.slot);
	if (artType->id == 135 && dst.slot == Arts::RIGHT_HAND && !dst.hero->hasSpellbook()) //Titan's Thunder creates new spellbook on equip
		dst.hero->giveArtifact(0);
}

CArtifactInstance * CArtifactInstance::createNewArtifactInstance(CArtifact *Art)
{
	if(!Art->constituents)
		return new CArtifactInstance(Art);
	else
	{
		CCombinedArtifactInstance * ret = new CCombinedArtifactInstance(Art);
		ret->createConstituents();
		return ret;
	}
}

CArtifactInstance * CArtifactInstance::createNewArtifactInstance(int aid)
{
	return createNewArtifactInstance(VLC->arth->artifacts[aid]);
}

void CArtifactInstance::deserializationFix()
{
	setType(artType);
}

int CArtifactInstance::getGivenSpellID() const
{
	const Bonus * b = getBonus(Selector::type(Bonus::SPELL));
	if(!b)
	{
		tlog3 << "Warning: " << nodeName() << " doesn't bear any spell!\n";
		return -1;
	}
	return b->subtype;
}

bool CArtifactInstance::isPart(const CArtifactInstance *supposedPart) const
{
	return supposedPart == this;
}

bool CCombinedArtifactInstance::canBePutAt(const ArtifactLocation &al, bool assumeDestRemoved /*= false*/) const
{
	bool canMainArtifactBePlaced = CArtifactInstance::canBePutAt(al, assumeDestRemoved);
	if(!canMainArtifactBePlaced)
		return false; //no is no...
	if(al.slot >= Arts::BACKPACK_START)
		return true; //we can always remove combined art to the backapck


	assert(artType->constituents);
	std::vector<ConstituentInfo> constituentsToBePlaced = constituentsInfo; //we'll remove constituents from that list, as we find a suitable slot for them

	//it may be that we picked a combined artifact in hero screen (though technically it's still there) to move it
	//so we remove from the list all constituents that are already present on dst hero in the form of locks
	BOOST_FOREACH(const ConstituentInfo &constituent, constituentsInfo)
	{
		if(constituent.art == al.hero->getArt(constituent.slot, false)) //no need to worry about locked constituent
			constituentsToBePlaced -= constituent;
	}
	
	//we iterate over all active slots and check if constituents fits them
	for (int i = 0; i < Arts::BACKPACK_START; i++)
	{
		for(std::vector<ConstituentInfo>::iterator art = constituentsToBePlaced.begin(); art != constituentsToBePlaced.end(); art++)
		{
			if(art->art->canBePutAt(ArtifactLocation(al.hero, i), i == al.slot)) // i == al.slot because we can remove already worn artifact only from that slot  that is our main destination
			{
				constituentsToBePlaced.erase(art);
				break;
			}
		}
	}

	return constituentsToBePlaced.empty();
}

bool CCombinedArtifactInstance::canBeDisassembled() const
{
	return true;
}

CCombinedArtifactInstance::CCombinedArtifactInstance(CArtifact *Art)
	: CArtifactInstance(Art) //TODO: seems unued, but need to be written
{
}

CCombinedArtifactInstance::CCombinedArtifactInstance()
{
}

void CCombinedArtifactInstance::createConstituents()
{
	assert(artType);
	assert(artType->constituents);

	BOOST_FOREACH(ui32 a, *artType->constituents)
	{
		addAsConstituent(CArtifactInstance::createNewArtifactInstance(a), -1);
	}
}

void CCombinedArtifactInstance::addAsConstituent(CArtifactInstance *art, int slot)
{
	assert(vstd::contains(*artType->constituents, art->artType->id));
	assert(art->getParentNodes().size() == 1  &&  art->getParentNodes().front() == art->artType);
	constituentsInfo.push_back(ConstituentInfo(art, slot));
	attachTo(art);
}

void CCombinedArtifactInstance::putAt(CGHeroInstance *h, ui16 slot)
{
	if(slot >= Arts::BACKPACK_START)
	{
		CArtifactInstance::putAt(h, slot);
		BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
			ci.slot = -1;
	}
	else
	{
		CArtifactInstance *mainConstituent = figureMainConstituent(slot); //it'll be replaced with combined artifact, not a lock
		CArtifactInstance::putAt(h, slot); //puts combined art (this)

		BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
		{
			if(ci.art != mainConstituent)
			{
				int pos = -1;
				if(isbetw(ci.slot, 0, Arts::BACKPACK_START)  &&  ci.art->canBePutAt(ArtifactLocation(h, ci.slot))) //there is a valid suggestion where to place lock 
					pos = ci.slot;
				else
					ci.slot = pos = ci.art->firstAvailableSlot(h);

				assert(pos < Arts::BACKPACK_START);
				h->setNewArtSlot(pos, ci.art, true); //sets as lock
			}
			else
			{
				ci.slot = -1;
			}
		}
	}
}

void CCombinedArtifactInstance::removeFrom(CGHeroInstance *h, ui16 slot)
{
	if(slot >= Arts::BACKPACK_START)
	{
		CArtifactInstance::removeFrom(h, slot);
	}
	else
	{
		BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
		{
			if(ci.slot >= 0)
			{
				h->eraseArtSlot(ci.slot);
				ci.slot = -1;
			}
			else
			{
				//main constituent
				CArtifactInstance::removeFrom(h, slot);
			}
		}
	}
}

CArtifactInstance * CCombinedArtifactInstance::figureMainConstituent(ui16 slot)
{
	CArtifactInstance *mainConstituent = NULL; //it'll be replaced with combined artifact, not a lock
	BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
		if(ci.slot == slot)
			mainConstituent = ci.art;

	if(!mainConstituent)
	{
		BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
		{
			if(vstd::contains(ci.art->artType->possibleSlots, slot))
			{
				mainConstituent = ci.art;
			}
		}
	}

	return mainConstituent;
}

void CCombinedArtifactInstance::deserializationFix()
{
	BOOST_FOREACH(ConstituentInfo &ci, constituentsInfo)
		attachTo(ci.art);
}

bool CCombinedArtifactInstance::isPart(const CArtifactInstance *supposedPart) const
{
	bool me = CArtifactInstance::isPart(supposedPart);
	if(me)
		return true;

	//check for constituents
	BOOST_FOREACH(const ConstituentInfo &constituent, constituentsInfo)
		if(constituent.art == supposedPart)
			return true;

	return false;
}

CCombinedArtifactInstance::ConstituentInfo::ConstituentInfo(CArtifactInstance *Art /*= NULL*/, ui16 Slot /*= -1*/)
{
	art = Art;
	slot = Slot;
}

bool CCombinedArtifactInstance::ConstituentInfo::operator==(const ConstituentInfo &rhs) const
{
	return art == rhs.art && slot == rhs.slot;
}

const CArtifactInstance* IArtifactSetBase::getArt(ui16 pos, bool excludeLocked) const
{
	if(const ArtSlotInfo *si = getSlot(pos))
	{
		if(si->artifact && (!excludeLocked || !si->locked))
			return si->artifact;
	}

	return NULL;
}

bool IArtifactSetBase::hasArt(ui32 aid, bool onlyWorn) const
{
	return getArtPos(aid, onlyWorn) != -1;
}

bool IArtifactSetBase::isPositionFree(ui16 pos, bool onlyLockCheck) const
{
	if(const ArtSlotInfo *s = getSlot(pos))
		return (onlyLockCheck || !s->artifact) && !s->locked;

	return true; //no slot means not used
}

void IArtifactSetBase::setNewArtSlot(ui16 slot, CArtifactInstance *art, bool locked)
{
	ArtSlotInfo &asi = retreiveNewArtSlot(slot);
	asi.artifact = art;
	asi.locked = locked;
}

CArtifactInstance* IArtifactSetBase::getArt(ui16 pos, bool excludeLocked /*= true*/)
{
	return const_cast<CArtifactInstance*>((const_cast<const IArtifactSetBase*>(this))->getArt(pos, excludeLocked));
}

si32 CArtifactSet::getArtPos(int aid, bool onlyWorn /*= true*/) const
{
	for(std::map<ui16, ArtSlotInfo>::const_iterator i = artifactsWorn.begin(); i != artifactsWorn.end(); i++)
		if(i->second.artifact->artType->id == aid)
			return i->first;

	if(onlyWorn)
		return -1;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact->artType->id == aid)
			return Arts::BACKPACK_START + i;

	return -1;
}

si32 CArtifactSet::getArtPos(const CArtifactInstance *art) const
{
	for(std::map<ui16, ArtSlotInfo>::const_iterator i = artifactsWorn.begin(); i != artifactsWorn.end(); i++)
		if(i->second.artifact == art)
			return i->first;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact == art)
			return Arts::BACKPACK_START + i;

	return -1;
}

const CArtifactInstance * CArtifactSet::getArtByInstanceId(int artInstId) const
{
	for(std::map<ui16, ArtSlotInfo>::const_iterator i = artifactsWorn.begin(); i != artifactsWorn.end(); i++)
		if(i->second.artifact->id == artInstId)
			return i->second.artifact;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact->id == artInstId)
			return artifactsInBackpack[i].artifact;

	return NULL;
}

const ArtSlotInfo * CArtifactSet::getSlot(ui16 pos) const
{
	if(vstd::contains(artifactsWorn, pos))
		return &artifactsWorn[pos];
	if(pos >= Arts::AFTER_LAST )
	{
		int backpackPos = (int)pos - Arts::BACKPACK_START;
		if(backpackPos < 0 || backpackPos >= artifactsInBackpack.size())
			return NULL;
		else
			return &artifactsInBackpack[backpackPos];
	}

	return NULL;
}

si32 CArtifactSet::getArtTypeId(ui16 pos) const
{
	const CArtifactInstance * const a = getArt(pos);
	if(!a)
	{
		tlog2 << (dynamic_cast<const CGHeroInstance*>(this))->name << " has no artifact at " << pos << " (getArtTypeId)\n";
		return -1;
	}
	return a->artType->id;
}

CArtifactSet::~CArtifactSet()
{

}

ArtSlotInfo & CArtifactSet::retreiveNewArtSlot(ui16 slot)
{
	assert(!vstd::contains(artifactsWorn, slot));
	ArtSlotInfo &ret = slot < Arts::BACKPACK_START 
		? artifactsWorn[slot]
		: *artifactsInBackpack.insert(artifactsInBackpack.begin() + (slot - Arts::BACKPACK_START), ArtSlotInfo());

	return ret;
}

void CArtifactSet::eraseArtSlot(ui16 slot)
{
	if(slot < Arts::BACKPACK_START)
	{
		artifactsWorn.erase(slot);
	}
	else
	{
		slot -= Arts::BACKPACK_START;
		artifactsInBackpack.erase(artifactsInBackpack.begin() + slot);
	}
}

ArtSlotInfo & CCreatureArtifactSet::retreiveNewArtSlot(ui16 slot)
{
	assert(slot); //ke?
	ArtSlotInfo &ret = slot <= Arts::CREATURE_ART
		? activeArtifact
		: *artifactsInBackpack.insert(artifactsInBackpack.begin() + (slot - 1), ArtSlotInfo());

	return ret;
}

void CCreatureArtifactSet::eraseArtSlot(ui16 slot)
{
	if(slot == Arts::CREATURE_ART)
	{
		activeArtifact.artifact = NULL; //hmm?
	}
	else
	{
		slot -= 1;
		artifactsInBackpack.erase(artifactsInBackpack.begin() + slot);
	}
}

const ArtSlotInfo * CCreatureArtifactSet::getSlot(ui16 pos) const
{
	if (pos == Arts::CREATURE_ART)
		return &activeArtifact;
	else if(pos > Arts::CREATURE_ART)
	{
		int backpackPos = (int)pos - 1;
		if(backpackPos < 0 || backpackPos >= artifactsInBackpack.size())
			return NULL;
		else
			return &artifactsInBackpack[backpackPos];
	}
	return NULL;
}

si32 CCreatureArtifactSet::getArtPos(int aid, bool onlyWorn) const
{
	if (aid == activeArtifact.artifact->artType->id )
		return Arts::CREATURE_ART;

	if(onlyWorn)
		return -1;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
	{
		if(artifactsInBackpack[i].artifact->artType->id == aid)
			return i + 1;
	}

	return -1;
}

si32 CCreatureArtifactSet::getArtPos(const CArtifactInstance *art) const
{
	if (activeArtifact.artifact == art)
		return Arts::CREATURE_ART;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact == art)
			return Arts::BACKPACK_START + i;

	return -1;
}

const CArtifactInstance * CCreatureArtifactSet::getArtByInstanceId(int artInstId) const
{
	if (activeArtifact.artifact->id == artInstId)
		return activeArtifact.artifact;

	for(int i = 0; i < artifactsInBackpack.size(); i++)
		if(artifactsInBackpack[i].artifact->id == artInstId)
			return artifactsInBackpack[i].artifact;

	return NULL;
}

si32 CCreatureArtifactSet::getArtTypeId(ui16 pos) const
{
	const CArtifactInstance * const a = getArt(pos);
	if(!a)
	{
		tlog2 << "Stack has no artifact at " << pos << " (getArtTypeId)\n";
		return -1;
	}
	return a->artType->id;
}

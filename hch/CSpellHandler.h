#ifndef __CSPELLHANDLER_H__
#define __CSPELLHANDLER_H__

#include <string>
#include <vector>
#include <set>

/*
 * CSpellHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class DLL_EXPORT CSpell
{
public:
	ui32 id;
	std::string name;
	std::string abbName; //abbreviated name
	std::vector<std::string> descriptions; //descriptions of spell for skill levels: 0 - none, 1 - basic, etc
	si32 level;
	bool earth;
	bool water;
	bool fire;
	bool air;
	si32 power; //spell's power
	std::vector<si32> costs; //per skill level: 0 - none, 1 - basic, etc
	std::vector<si32> powers; //[er skill level: 0 - none, 1 - basic, etc
	std::vector<si32> probabilities; //% chance to gain for castles 
	std::vector<si32> AIVals; //AI values: per skill level: 0 - none, 1 - basic, etc
	std::string attributes; //reference only attributes
	bool combatSpell; //is this spell combat (true) or adventure (false)
	bool creatureAbility; //if true, only creatures can use this spell
	si8 positiveness; //1 if spell is positive for influenced stacks, 0 if it is indifferent, -1 if it's negative
	std::vector<std::string> range; //description of spell's range in SRSL by magic school level
	std::set<ui16> rangeInHexes(unsigned int centralHex, ui8 schoolLvl ) const; //convert range to specific hexes


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & name & abbName & descriptions & level & earth & water & fire & air & power & costs 
			& powers & probabilities & AIVals & attributes & combatSpell & creatureAbility & positiveness & range;
	}
};

class DLL_EXPORT CSpellHandler
{
public:
	std::vector<CSpell> spells;
	void loadSpells();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & spells;
	}
};


#endif // __CSPELLHANDLER_H__

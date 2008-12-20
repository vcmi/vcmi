#ifndef CSPELLHANDLER_H
#define CSPELLHANDLER_H

#include <string>
#include <vector>

class CSpell
{
public:
	ui32 id;
	std::string name;
	std::string abbName; //abbreviated name
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
	std::vector<std::string> descriptions; //descriptions of spell for skill levels: 0 - none, 1 - basic, etc
	std::string attributes; //reference only attributes
	bool combatSpell; //is this spell combat (true) or adventure (false)
	bool creatureAbility; //if true, only creatures can use this spell
	si8 positiveness; //1 if spell is positive for influenced stacks, 0 if it is indifferent, -1 if it's negative
};

class DLL_EXPORT CSpellHandler
{
public:
	std::vector<CSpell> spells;
	void loadSpells();
};

#endif //CSPELLHANDLER_H

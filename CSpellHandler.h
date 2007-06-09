#ifndef CSPELLHANDLER_H
#define CSPELLHANDLER_H

#include <string>
#include <vector>

class CSpell
{
public:
	std::string name;
	std::string abbName; //abbreviated name
	int level;
	bool earth;
	bool water;
	bool fire;
	bool air;
	int costNone;
	int costBas;
	int costAdv;
	int costExp;
	int power; //spell's power
	int powerNone; //effect without magic ability
	int powerBas; //efect with basic magic ability
	int powerAdv; //efect with advanced magic ability
	int powerExp; //efect with expert magic ability
	int castle, rampart, tower, inferno, necropolis, dungeon, stronghold, fortress, conflux; //% chance to gain
	int none2, bas2, adv2, exp2; //AI values
	std::string noneTip, basicTip, advTip, expTip; //descriptions of spell
	std::string attributes; //reference only attributes
	bool combatSpell; //is this spell combat (true) or adventure (false)
};

class CSpellHandler
{
public:
	std::vector<CSpell> spells;
	void loadSpells();
};

#endif //CSPELLHANDLER_H
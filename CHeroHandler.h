#ifndef CHEROHANDLER_H
#define CHEROHANDLER_H

#include <string>
#include <vector>

class CHero
{
public:
	std::string name;
	int low1stack, high1stack, low2stack, high2stack, low3stack, high3stack; //amount of units; described below
	std::string refType1stack, refType2stack, refType3stack; //reference names of units appearing in hero's army if he is recruited in tavern
	std::string bonusName, shortBonus, longBonus; //for special abilities
};

class CHeroInstance
{
public:
	CHero type;
	int x, y; //position
	bool under; //is underground?
	//TODO: armia, artefakty, itd.
};

class CHeroHandler
{
public:
	std::vector<CHero> heroes;
	void loadHeroes();
	void loadSpecialAbilities();
};


#endif //CHEROHANDLER_H
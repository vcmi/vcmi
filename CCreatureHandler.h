#ifndef CCREATUREHANDLER_H
#define CCREATUREHANDLER_H

#include <string>
#include <vector>

class CCreature
{
public:
	std::string namePl, nameSing; //name in singular and plural form
	int wood, mercury, ore, sulfur, crystal, gems, gold, fightValue, AIValue, growth, hordeGrowth, hitPoints, speed, attack, defence, shots, spells;
	int low1, low2, high1, high2; //TODO - co to w ogóle jest???
	std::string abilityText; //description of abilities
	std::string abilityRefs; //references to abilities, in textformat
	int idNumber;
	//TODO - zdolnoœci - na typie wyliczeniowym czy czymœ
};

class CCreatureHandler
{
public:
	std::vector<CCreature> creatures;
	void loadCreatures();
};

#endif //CCREATUREHANDLER_H
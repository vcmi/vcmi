#ifndef CARTHANDLER_H
#define CARTHANDLER_H

#include <string>
#include <vector>

enum EartClass {SartClass=0, TartClass, NartClass, JartClass, RartClass}; //artifact class (relict, treasure, strong, weak etc.)

class CArtifact //container for artifacts
{
public:
	std::string name;
	std::string description;
	std::string desc2;
	unsigned int price;
	bool spellBook, warMachine1, warMachine2, warMachine3, warMachine4, misc1, misc2, misc3, misc4, misc5, feet, lRing, rRing, torso, lHand, rHand, neck, shoulders, head;
	EartClass aClass;
	int number;
};

class CArtHandler //handles artifacts
{
public:
	std::vector<CArtifact> artifacts;
	void loadArtifacts();
};

#endif // CARTHANDLER_H
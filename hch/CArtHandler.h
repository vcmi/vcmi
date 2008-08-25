#ifndef CARTHANDLER_H
#define CARTHANDLER_H
#include "../global.h"
#include <string>
#include <vector>

enum EartClass {SartClass=0, TartClass, NartClass, JartClass, RartClass}; //artifact class (relict, treasure, strong, weak etc.)
class CDefHandler;

class DLL_EXPORT CArtifact //container for artifacts
{
public:
	bool isAllowed; //true if we can use this artifact (map information)
	std::string name;
	std::string description;
	//std::string desc2;
	unsigned int price;
	std::vector<ui16> possibleSlots; //ids of slots where artifact can be placed
	//bool spellBook, warMachine1, warMachine2, warMachine3, warMachine4, misc1, misc2, misc3, misc4, misc5, feet, lRing, rRing, torso, lHand, rHand, neck, shoulders, head;
	EartClass aClass;
	int id;


};

class DLL_EXPORT CArtHandler //handles artifacts
{
public:
	std::vector<CArtifact*> treasures, minors, majors, relics;
	std::vector<CArtifact> artifacts;
	void loadArtifacts();
	CArtHandler();
};

#endif // CARTHANDLER_H

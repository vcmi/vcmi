#ifndef COBJECTHANDLER_H
#define COBJECTHANDLER_H

#include <string>
#include <vector>
#include "CCreatureHandler.h"
#include "CArtHandler.h"
#include "CAbilityHandler.h"
#include "CSpellHandler.h"

class CSpecObjInfo //class with object - specific info (eg. different information for creatures and heroes); use inheritance to make object - specific classes
{
};

class CEventObjInfo : public CSpecObjInfo
{
public:
	bool areGuarders; //true if there are
	CCreatureSet guarders;
	bool isMessage; //true if there is a message
	std::string message;
	unsigned int gainedExp;
	int manaDiff; //amount of gained / lost mana
	int moraleDiff; //morale modifier
	int luckDiff; //luck modifier
	int wood, mercury, ore, sulfur, crystal, gems, gold; //gained / lost resources
	unsigned int attack; //added attack points
	unsigned int defence; //added defence points
	unsigned int power; //added power points
	unsigned int knowledge; //added knowledge points
	std::vector<CAbility> abilities; //gained abilities
	std::vector<int> abilityLevels; //levels of gained abilities
	std::vector<CArtifact> artifacts; //gained artifacts
	std::vector<CSpell> spells; //gained spells
	CCreatureSet creatures; //gained creatures
	unsigned char availableFor; //players whom this event is available for
	bool computerActivate; //true if computre player can activate this event
	bool humanActivate; //true if human player can activate this event
};

class CObject //typical object that can be encountered on a map
{
public:
	std::string name; //object's name
};

class CObjectInstance //instance of object
{
public:
	int defNumber; //specifies number of def file with animation of this object
	int id; //number of object in CObjectHandler's vector
	int x, y, z; // position
	CSpecObjInfo * info; //pointer to something with additional information
};

class CObjectHandler
{
public:
	std::vector<CObject> objects; //vector of objects; i-th object in vector has subnumber i
	std::vector<CObjectInstance> objInstances; //vector with objects on map
	void loadObjects();
};


#endif //COBJECTHANDLER_H
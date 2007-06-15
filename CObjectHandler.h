#ifndef COBJECTHANDLER_H
#define COBJECTHANDLER_H

#include <string>
#include <vector>
#include "CCreatureHandler.h"
#include "CArtHandler.h"
#include "CAbilityHandler.h"
#include "CSpellHandler.h"
#include "CHeroHandler.h"

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
	std::vector<CAbility *> abilities; //gained abilities
	std::vector<int> abilityLevels; //levels of gained abilities
	std::vector<CArtifact *> artifacts; //gained artifacts
	std::vector<CSpell *> spells; //gained spells
	CCreatureSet creatures; //gained creatures
	unsigned char availableFor; //players whom this event is available for
	bool computerActivate; //true if computre player can activate this event
	bool humanActivate; //true if human player can activate this event
};

class CHeroObjInfo : public CSpecObjInfo
{
public:
	unsigned char bytes[4]; //mysterius bytes identifying hero in a strange way
	int player;
	CHero * type;
	std::string name; //if nonstandard
	bool standardGarrison; //true if hero has standard garrison
	CCreatureSet garrison; //hero's army
	std::vector<CArtifact *> artifacts; //hero's artifacts from bag
	CArtifact * artHead, * artLRing, * artRRing, * artLHand, * artRhand, * artFeet, * artSpellBook, * artMach1, * artMach2, * artMach3, * artMach4, * artMisc1, * artMisc2, * artMisc3, * artMisc4, * artMisc5, * artTorso, * artNeck, * artShoulders; //working artifacts
	bool isGuarding;
	int guardRange; //range of hero's guard
	std::string biography; //if nonstandard
	bool sex; //if true, reverse hero's sex
	std::vector<CSpell *> spells; //hero's spells
	int attack, defence, power, knowledge; //main hero's attributes
	unsigned int experience; //hero's experience points
	std::vector<CAbility *> abilities; //hero's abilities
	std::vector<int> abilityLevels; //hero ability levels
	bool defaultMianStats; //if true attack, defence, power and knowledge are typical
};

class CCreatureObjInfo : public CSpecObjInfo
{
public:
	unsigned char bytes[4]; //mysterious bytes identifying creature
	unsigned int number; //number of units (0 - random)
	unsigned char character; //chracter of this set of creatures (0 - the most friendly, 4 - the most hostile)
	std::string message; //message printed for attacking hero
	int wood, mercury, ore, sulfur, crytal, gems, gold; //resources gained to hero that has won with monsters
	CArtifact * gainedArtifact; //artifact gained to hero
	bool neverFlees; //if true, the troops will never flee
	bool notGrowingTeam; //if true, number of units won't grow
};

class CSignObjInfo : public CSpecObjInfo
{
public:
	std::string message; //message
};

class CSeerHutObjInfo : public CSpecObjInfo
{
public:
	char missionType; //type of mission: 0 - no mission; 1 - reach level; 2 - reach main statistics values; 3 - win with a certain hero; 4 - win with a certain creature; 5 - collect some atifacts; 6 - have certain troops in army; 7 - collect resources; 8 - be a certain hero; 9 - be a certain player
	bool isDayLimit; //if true, there is a day limit
	int lastDay; //after this day (first day is 0) mission cannot be completed
	//for mission 1
	int m1level;
	//for mission 2
	int m2attack, m2defence, m2power, m2knowledge;
	//for mission 3
	unsigned char m3bytes[4];
	//for mission 4
	unsigned char m4bytes[4];
	//for mission 5
	std::vector<CArtifact *> m5arts;
	//for mission 6
	std::vector<CCreature *> m6cre;
	std::vector<int> m6number;
	//for mission 7
	int m7wood, m7mercury, m7ore, m7sulfur, m7crystal, m7gems, m7gold;
	//for mission 8
	CHero * m8hero;
	//for mission 9
	int m9player; //number; from 0 to 7

	std::string firstVisitText, nextVisitText, completedText;
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
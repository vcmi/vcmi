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

	char rewardType; //type of reward: 0 - no reward; 1 - experience; 2 - mana points; 3 - morale bonus; 4 - luck bonus; 5 - resources; 6 - main ability bonus (attak, defence etd.); 7 - secondary ability gain; 8 - artifact; 9 - spell; 10 - creature
	//for reward 1
	int r1exp;
	//for reward 2
	int r2mana;
	//for reward 3
	int r3morale;
	//for reward 4
	int r4luck;
	//for reward 5
	unsigned char r5type; //0 - wood, 1 - mercury, 2 - ore, 3 - sulfur, 4 - crystal, 5 - gems, 6 - gold
	int r5amount;
	//for reward 6
	unsigned char r6type; //0 - attack, 1 - defence, 2 - power, 3 - knowledge
	int r6amount;
	//for reward 7
	CAbility * r7ability;
	unsigned char r7level; //1 - basic, 2 - advanced, 3 - expert
	//for reward 8
	CArtifact * r8art;
	//for reward 9
	CSpell * r9spell;
	//for reward 10
	CCreature * r10creature;
	int r10amount;
};

class CWitchHutObjInfo : public CSpecObjInfo
{
public:
	std::vector<CAbility *> allowedAbilities;
};

class CScholarObjInfo : public CSpecObjInfo
{
public:
	unsigned char bonusType; //255 - random, 0 - primary skill, 1 - secondary skill, 2 - spell

	unsigned char r0type;
	CAbility * r1;
	CSpell * r2;
};

class CGarrisonObjInfo : public CSpecObjInfo
{
public:
	unsigned char player; //255 - nobody; 0 - 7 - players
	CCreatureSet units;
	bool movableUnits; //if true, units can be moved
};

class CArtifactObjInfo : public CSpecObjInfo
{
public:
	bool areGuards;
	std::string message;
	CCreatureSet guards;
};

class CResourceObjInfo : public CSpecObjInfo
{
public:
	bool randomAmount;
	int amount; //if not random
	bool areGuards;
	CCreatureSet guards;
	std::string message;
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
#ifndef CCREATUREHANDLER_H
#define CCREATUREHANDLER_H

#include <string>
#include <vector>
#include <map>
#include "CDefHandler.h"

class CDefHandler;
struct SDL_Surface;
//#include "CDefHandler.h"

class CCreature
{
public:
	std::string namePl, nameSing, nameRef; //name in singular and plural form; and reference name
	int wood, mercury, ore, sulfur, crystal, gems, gold, fightValue, AIValue, growth, hordeGrowth, hitPoints, speed, attack, defence, shots, spells;
	int low1, low2, high1, high2; //TODO - co to w ogóle jest???
	std::string abilityText; //description of abilities
	std::string abilityRefs; //references to abilities, in textformat
	int idNumber;

	///animation info
	float timeBetweenFidgets, walkAnimationTime, attackAnimationTime, flightAnimationDistance;
	int upperRightMissleOffsetX, rightMissleOffsetX, lowerRightMissleOffsetX, upperRightMissleOffsetY, rightMissleOffsetY, lowerRightMissleOffsetY;
	float missleFrameAngles[12];
	int troopCountLocationOffset, attackClimaxFrame;
	///end of anim info

	//for some types of towns
	bool isDefinite; //if the creature type is wotn dependent, it should be true
	int indefLevel; //only if indefinite
	bool indefUpgraded; //onlu if inddefinite
	//end
	CDefHandler * battleAnimation;
	//TODO - zdolnoœci - na typie wyliczeniowym czy czymœ
};

class CCreatureSet //seven combined creatures
{
public:
	std::map<int,std::pair<CCreature*,int> > slots;
	//CCreature * slot1, * slot2, * slot3, * slot4, * slot5, * slot6, * slot7; //types of creatures on each slot
	//unsigned int s1, s2, s3, s4, s5, s6, s7; //amounts of units in slots
	bool formation; //false - wide, true - tight
};

class CCreatureHandler
{
public:
	std::map<int,SDL_Surface*> smallImgs; //creature ID -> small 32x32 img of creature; //ID=-2 is for blank (black) img; -1 for the border
	std::vector<CCreature> creatures;
	std::map<std::string,int> nameToID;
	void loadCreatures();
	void loadAnimationInfo();
	void loadUnitAnimInfo(CCreature & unit, std::string & src, int & i);
	void loadUnitAnimations();
};

class CCreatureAnimation
{
private:
	int totalEntries, DEFType, totalBlocks, fullWidth, fullHeight;
	unsigned char fbuffer[800];
	bool allowRepaint;
	int length;
	BMPPalette palette[256];
	unsigned int * RWEntries;
	int * RLEntries;
	struct SEntry
	{
		std::string name;
		int offset;
		int group;
	} ;
	std::vector<SEntry> SEntries ;
	char id[2];
	std::string defName, curDir;
	int readNormalNr (int pos, int bytCon, unsigned char * str=NULL, bool cyclic=false);

	////////////

	unsigned char * FDef; //animation raw data
	unsigned int curFrame; //number of currently displayed frame
	unsigned int frames; //number of frames
	int type; //type of animation being displayed (-1 - whole animation, >0 - specified part [default: -1])
public:
	CCreatureAnimation(std::string name); //c-tor
	//~CCreatureAnimation(); //d-tor //not necessery ATM

	void setType(int type); //sets type of animation and cleares framecount
	int getType() const; //returns type of animation

	int nextFrame(SDL_Surface * dest, int x, int y); //0 - success, any other - error //print next 
};

#endif //CCREATUREHANDLER_H
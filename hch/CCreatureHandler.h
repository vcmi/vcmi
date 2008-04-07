#ifndef CCREATUREHANDLER_H
#define CCREATUREHANDLER_H

#include "CPlayerInterface.h"
#include <string>
#include <vector>
#include <map>
#include "CDefHandler.h"

class CDefHandler;
struct SDL_Surface;

class CCreature
{
public:
	std::string namePl, nameSing, nameRef; //name in singular and plural form; and reference name
	std::vector<int> cost; //cost[res_id] - amount of that resource
	int fightValue, AIValue, growth, hordeGrowth, hitPoints, speed, attack, defence, shots, spells;
	int low1, low2, high1, high2; //TODO - co to w ogóle jest???
	int level; // 0 - unknown
	std::string abilityText; //description of abilities
	std::string abilityRefs; //references to abilities, in textformat
	std::string animDefName;
	int idNumber;
	int faction; //-1 = neutral

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

	static int getQuantityID(int quantity); //0 - a few, 1 - several, 2 - pack, 3 - lots, 4 - horde, 5 - throng, 6 - swarm, 7 - zounds, 8 - legion
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
	std::map<int,SDL_Surface*> bigImgs; //creature ID -> big 58x64 img of creature; //ID=-2 is for blank (black) img; -1 for the border
	std::map<int,SDL_Surface*> backgrounds; //castle ID -> 100x130 background creature image // -1 is for neutral
	std::vector<CCreature> creatures;
	std::map<int,std::vector<CCreature*> > levelCreatures; //level -> list of creatures
	std::map<std::string,int> nameToID;
	void loadCreatures();
	void loadAnimationInfo();
	void loadUnitAnimInfo(CCreature & unit, std::string & src, int & i);
	void loadUnitAnimations();
};

class CCreatureAnimation : public CIntObject
{
private:
	int totalEntries, DEFType, totalBlocks;
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
	void putPixel(SDL_Surface * dest, const int & ftcp, const BMPPalette & color, const unsigned char & palc, const bool & yellowBorder) const;

	////////////

	unsigned char * FDef; //animation raw data
	unsigned int curFrame; //number of currently displayed frame
	unsigned int frames; //number of frames
	int type; //type of animation being displayed (-1 - whole animation, >0 - specified part [default: -1])
public:
	int fullWidth, fullHeight; //read-only, please!
	CCreatureAnimation(std::string name); //c-tor
	~CCreatureAnimation(); //d-tor //not necessery ATM

	void setType(int type); //sets type of animation and cleares framecount
	int getType() const; //returns type of animation

	int nextFrame(SDL_Surface * dest, int x, int y, bool attacker, bool incrementFrame = true, bool yellowBorder = false); //0 - success, any other - error //print next 
	int nextFrameMiddle(SDL_Surface * dest, int x, int y, bool attacker, bool incrementFrame = true, bool yellowBorder = false); //0 - success, any other - error //print next 
};

#endif //CCREATUREHANDLER_H
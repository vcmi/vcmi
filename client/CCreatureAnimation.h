#pragma once
#include "../global.h"
#include "../CPlayerInterface.h"
#include "../hch/CDefHandler.h"

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
	~CCreatureAnimation(); //d-tor

	void setType(int type); //sets type of animation and cleares framecount
	int getType() const; //returns type of animation

	int nextFrame(SDL_Surface * dest, int x, int y, bool attacker, bool incrementFrame = true, bool yellowBorder = false, SDL_Rect * destRect = NULL); //0 - success, any other - error //print next 
	int nextFrameMiddle(SDL_Surface * dest, int x, int y, bool attacker, bool IncrementFrame = true, bool yellowBorder = false, SDL_Rect * destRect = NULL); //0 - success, any other - error //print next 
	void incrementFrame();

	int framesInGroup(int group) const; //retirns number of fromes in given group
};

#ifndef CCREATUREANIMATION_H
#define CCREATUREANIMATION_H


#include "../global.h"
#include "../CPlayerInterface.h"
#include "../hch/CDefHandler.h"

class CCreatureAnimation : public CIntObject
{
private:
	int totalEntries, DEFType, totalBlocks;
	int length;
	BMPPalette palette[256];
	int * RLEntries;
	struct SEntry
	{
		std::string name;
		int offset;
		int group;
	} ;
	std::vector<SEntry> SEntries ;
	std::string defName, curDir;
	int readNormalNr (int pos, int bytCon, unsigned char * str=NULL) const;
	void putPixel(
                SDL_Surface * dest,
                const int & ftcp,
                const BMPPalette & color,
                const unsigned char & palc,
                const bool & yellowBorder
        ) const;

	////////////

	unsigned char * FDef; //animation raw data
	int curFrame; //number of currently displayed frame
	unsigned int frames; //number of frames
public:
	int type; //type of animation being displayed (-1 - whole animation, >0 - specified part [default: -1])
	int fullWidth, fullHeight; //read-only, please!
	CCreatureAnimation(std::string name); //c-tor
	~CCreatureAnimation(); //d-tor

	void setType(int type); //sets type of animation and cleares framecount
	int getType() const; //returns type of animation

	int nextFrame(SDL_Surface * dest, int x, int y, bool attacker, bool incrementFrame = true, bool yellowBorder = false, SDL_Rect * destRect = NULL); //0 - success, any other - error //print next 
	int nextFrameMiddle(SDL_Surface * dest, int x, int y, bool attacker, bool IncrementFrame = true, bool yellowBorder = false, SDL_Rect * destRect = NULL); //0 - success, any other - error //print next 
	void incrementFrame();
	int getFrame() const;

	int framesInGroup(int group) const; //retirns number of fromes in given group
};
#endif //CCREATUREANIMATION_H
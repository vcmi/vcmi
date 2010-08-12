#ifndef __CCREATUREANIMATION_H__
#define __CCREATUREANIMATION_H__


#include "../global.h"
#include "../hch/CDefHandler.h"
#include "GUIBase.h"

/*
 * CCreatureAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CCreatureAnimation : public CIntObject
{
private:
	int totalEntries, DEFType, totalBlocks;
	int length;
	BMPPalette palette[256];
	struct SEntry
	{
		int offset;
		int group;
	} ;
	std::vector<SEntry> SEntries ;
	std::string defName, curDir;
	template< int bytCon > static int readNormalNr (int pos, unsigned char * str)
	{
		int ret=0;
		int amp=1;

		for (int i=0; i<bytCon; i++)
		{
			ret+=str[pos+i]*amp;
			amp<<=8; //amp*=256;
		}

		return ret;
	}

	template<int bpp>
	void putPixel(
                SDL_Surface * dest,
                const int & ftcp,
                const BMPPalette & color,
                const unsigned char & palc,
                const bool & yellowBorder,
				const bool & blueBorder,
				const unsigned char & animCount
        ) const;

	////////////

	unsigned char * FDef; //animation raw data
	int curFrame, internalFrame; //number of currently displayed frame
	unsigned int frames; //number of frames
public:
	std::map<int, std::vector<int> > frameGroups; //groups of frames; [groupID] -> vector of frame IDs in group
	int type; //type of animation being displayed (-1 - whole animation, >0 - specified part [default: -1])
	int fullWidth, fullHeight; //read-only, please!
	CCreatureAnimation(std::string name); //c-tor
	~CCreatureAnimation(); //d-tor

	void setType(int type); //sets type of animation and cleares framecount
	int getType() const; //returns type of animation


	template<int bpp>
	int nextFrameT(SDL_Surface * dest, int x, int y, bool attacker, unsigned char animCount, bool incrementFrame = true, bool yellowBorder = false, bool blueBorder = false, SDL_Rect * destRect = NULL); //0 - success, any other - error //print next 
	int nextFrame(SDL_Surface * dest, int x, int y, bool attacker, unsigned char animCount, bool incrementFrame = true, bool yellowBorder = false, bool blueBorder = false, SDL_Rect * destRect = NULL); //0 - success, any other - error //print next 
	int nextFrameMiddle(SDL_Surface * dest, int x, int y, bool attacker, unsigned char animCount, bool IncrementFrame = true, bool yellowBorder = false, bool blueBorder = false, SDL_Rect * destRect = NULL); //0 - success, any other - error //print next 
	void incrementFrame();
	int getFrame() const;
	bool onFirstFrameInGroup();
	bool onLastFrameInGroup();

	bool once;
	void playOnce(int type); //plays once given stage of animation, then resets to 2

	int framesInGroup(int group) const; //retirns number of fromes in given group
};

#endif // __CCREATUREANIMATION_H__

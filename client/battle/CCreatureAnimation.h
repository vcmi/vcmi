#pragma once


#include "../CDefHandler.h"
#include "../../client/CBitmapHandler.h"
#include "../CAnimation.h"

/*
 * CCreatureAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

struct BMPPalette;
class CIntObject;

/// Class which manages animations of creatures/units inside battles
class CCreatureAnimation : public CIntObject
{
private:
	int totalEntries, DEFType, totalBlocks;
	BMPPalette palette[256];
	struct SEntry
	{
		int offset;
		int group;
	} ;
	std::vector<SEntry> SEntries ;
	std::string defName, curDir;

	template<int bpp>
	void putPixel(
                SDL_Surface * dest,
                const int & ftcpX,
                const int & ftcpY,
                const BMPPalette & color,
                const ui8 & palc,
                const bool & yellowBorder,
				const bool & blueBorder,
				const ui8 & animCount
        ) const;

	////////////

	ui8 * FDef; //animation raw data
	int curFrame, internalFrame; //number of currently displayed frame
	ui32 frames; //number of frames
	CCreatureAnim::EAnimType type; //type of animation being displayed (-1 - whole animation, >0 - specified part [default: -1])
	
	template<int bpp>
	int nextFrameT(SDL_Surface * dest, int x, int y, bool attacker, ui8 animCount, bool incrementFrame = true, bool yellowBorder = false, bool blueBorder = false, SDL_Rect * destRect = nullptr); //0 - success, any other - error //print next 
	int nextFrameMiddle(SDL_Surface * dest, int x, int y, bool attacker, ui8 animCount, bool IncrementFrame = true, bool yellowBorder = false, bool blueBorder = false, SDL_Rect * destRect = nullptr); //0 - success, any other - error //print next 
	
	std::map<int, std::vector<int> > frameGroups; //groups of frames; [groupID] -> vector of frame IDs in group
	bool once;

public:
	int fullWidth, fullHeight; //read-only, please!
	CCreatureAnimation(std::string name); //c-tor
	~CCreatureAnimation(); //d-tor

	void setType(CCreatureAnim::EAnimType type); //sets type of animation and cleares framecount
	CCreatureAnim::EAnimType getType() const; //returns type of animation

	int nextFrame(SDL_Surface * dest, int x, int y, bool attacker, ui8 animCount, bool incrementFrame = true, bool yellowBorder = false, bool blueBorder = false, SDL_Rect * destRect = nullptr); //0 - success, any other - error //print next 
	void incrementFrame();
	int getFrame() const; // Gets the current frame ID relative to DEF file.
	int getAnimationFrame() const; // Gets the current frame ID relative to frame group.
	bool onFirstFrameInGroup();
	bool onLastFrameInGroup();

	void playOnce(CCreatureAnim::EAnimType type); //plays once given stage of animation, then resets to 2

	int framesInGroup(CCreatureAnim::EAnimType group) const; //retirns number of fromes in given group
};

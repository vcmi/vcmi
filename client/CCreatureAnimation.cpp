#include "CCreatureAnimation.h"
#include "../lib/CLodHandler.h"
#include "../lib/VCMI_Lib.h"
#include "../lib/vcmi_endian.h"
#include <assert.h>
#include "SDL_Extensions.h"

/*
 * CCreatureAnimation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CCreatureAnim::EAnimType CCreatureAnimation::getType() const
{
	return type;
}

void CCreatureAnimation::setType(CCreatureAnim::EAnimType type)
{
	assert(framesInGroup(type) > 0 && "Bad type for void CCreatureAnimation::setType(int type)!");
	this->type = type;
	internalFrame = 0;
	if(type!=-1)
	{
		curFrame = frameGroups[type][0];
	}
	else
	{
		if(curFrame>=frames)
		{
			curFrame = 0;
		}
	}
}

CCreatureAnimation::CCreatureAnimation(std::string name) : internalFrame(0), once(false)
{
	FDef = spriteh->giveFile(name, FILE_ANIMATION); //load main file

	//init anim data
	int i,j, totalInBlock;

	defName=name;
	i = 0;
	DEFType = read_le_u32(FDef + i); i+=4;
	fullWidth = read_le_u32(FDef + i); i+=4;
	fullHeight = read_le_u32(FDef + i); i+=4;
	i=0xc;
	totalBlocks = read_le_u32(FDef + i); i+=4;

	i=0x10;
	for (int it=0;it<256;it++)
	{
		palette[it].R = FDef[i++];
		palette[it].G = FDef[i++];
		palette[it].B = FDef[i++];
		palette[it].F = 0;
	}
	i=0x310;
	totalEntries=0;
	for (int z=0; z<totalBlocks; z++)
	{
		std::vector<int> frameIDs;
		int group = read_le_u32(FDef + i); i+=4; //block ID
		totalInBlock = read_le_u32(FDef + i); i+=4;
		for (j=SEntries.size(); j<totalEntries+totalInBlock; j++)
		{
			SEntries.push_back(SEntry());
			SEntries[j].group = group;
			frameIDs.push_back(j);
		}
		/*int unknown2 = read_le_u32(FDef + i);*/ i+=4; //TODO use me
		/*int unknown3 = read_le_u32(FDef + i);*/ i+=4; //TODO use me
		i+=13*totalInBlock; //omitting names
		for (j=0; j<totalInBlock; j++)
		{ 
			SEntries[totalEntries+j].offset = read_le_u32(FDef + i); i+=4;
		}
		//totalEntries+=totalInBlock;
		for(int hh=0; hh<totalInBlock; ++hh)
		{
			++totalEntries;
		}
		frameGroups[group] = frameIDs;
	}

	//init vars
	curFrame = 0;
	type = CCreatureAnim::WHOLE_ANIM;
	frames = totalEntries;
}

int CCreatureAnimation::nextFrameMiddle(SDL_Surface *dest, int x, int y, bool attacker, unsigned char animCount, bool incrementFrame, bool yellowBorder, bool blueBorder, SDL_Rect * destRect)
{
	return nextFrame(dest, x-fullWidth/2, y-fullHeight/2, attacker, animCount, incrementFrame, yellowBorder, blueBorder, destRect);
}

void CCreatureAnimation::incrementFrame()
{
	if(type!=-1) //when a specific part of animation is played
	{
		++internalFrame;
		if(internalFrame == frameGroups[type].size()) //rewind
		{
			internalFrame = 0;
			if(once) //playing animation once - return to standing animation
			{
				type = CCreatureAnim::HOLDING;
				once = false;
				curFrame = frameGroups[2][0];
			}
			else //
			{
				curFrame = frameGroups[type][0];
			}
		}
		curFrame = frameGroups[type][internalFrame];
	}
	else //when whole animation is played
	{
		++curFrame;
		if(curFrame>=frames)
			curFrame = 0;
	}
}

int CCreatureAnimation::getFrame() const
{
	return curFrame;
}

int CCreatureAnimation::getAnimationFrame() const
{
	return internalFrame;
}

bool CCreatureAnimation::onFirstFrameInGroup()
{
	return internalFrame == 0;
}

bool CCreatureAnimation::onLastFrameInGroup()
{
	if(internalFrame == frameGroups[type].size() - 1)
		return true;
	return false;
}

void CCreatureAnimation::playOnce( CCreatureAnim::EAnimType type )
{
	setType(type);
	once = true;
}


template<int bpp>
int CCreatureAnimation::nextFrameT(SDL_Surface * dest, int x, int y, bool attacker, unsigned char animCount, bool IncrementFrame /*= true*/, bool yellowBorder /*= false*/, bool blueBorder /*= false*/, SDL_Rect * destRect /*= NULL*/)
{
	//increasing frame number
	int SIndex = curFrame;
	if (IncrementFrame)
		incrementFrame();

#if 0
	long SpriteWidth, SpriteHeight, //sprite format
		LeftMargin, RightMargin, TopMargin,BottomMargin,
		i, FullHeight,

#endif
	unsigned char SegmentType, SegmentLength;
	unsigned int i;

	i = SEntries[SIndex].offset;

	/*int prSize = read_le_u32(FDef + i);*/ i += 4; //TODO use me
	const unsigned int defType2 = read_le_u32(FDef + i); i += 4;
	const unsigned int FullWidth = read_le_u32(FDef + i); i += 4;
	const unsigned int FullHeight = read_le_u32(FDef + i); i += 4;
	const unsigned int SpriteWidth = read_le_u32(FDef + i); i += 4;
	const unsigned int SpriteHeight = read_le_u32(FDef + i); i += 4;
	const int LeftMargin = read_le_u32(FDef + i); i += 4;
	const int TopMargin = read_le_u32(FDef + i); i += 4;
	const int RightMargin = FullWidth - SpriteWidth - LeftMargin;
	const int BottomMargin = FullHeight - SpriteHeight - TopMargin;

	if (defType2 == 1) //as it should be always in creature animations
	{
		const int BaseOffsetor = i;
		int ftcp = 0;

		if (TopMargin > 0)
		{
			ftcp += FullWidth * TopMargin;
		}
		ui32 *RLEntries = (ui32 *)(FDef + BaseOffsetor);

		for (int i = 0; i < SpriteHeight; i++)
		{
			int BaseOffset = BaseOffsetor + read_le_u32(RLEntries + i);
			int TotalRowLength; // length of read segment

			if (LeftMargin > 0)
			{
				ftcp += LeftMargin;
			}
			
			TotalRowLength = 0;

			// Note: Bug fixed (Rev 2115): The implementation of omitting lines was false. 
			// We've to calculate several things so not showing/putting pixels should suffice.

			int yB = ftcp / FullWidth + y;

			do
			{
				SegmentType = FDef[BaseOffset++];
				SegmentLength = FDef[BaseOffset++];

				const int remainder = ftcp % FullWidth;
				int xB = (attacker ? remainder : FullWidth - remainder - 1) + x;

				const unsigned char aCountMod = (animCount & 0x20) ? ((animCount & 0x1e) >> 1) << 4 : (0x0f - ((animCount & 0x1e) >> 1)) << 4;

				for (int k = 0; k <= SegmentLength; k++)
				{
					if(xB >= 0 && xB < dest->w && yB >= 0 && yB < dest->h)
					{
						if(!destRect || (destRect->x <= xB && destRect->x + destRect->w > xB && destRect->y <= yB && destRect->y + destRect->h > yB))
						{
							const ui8 colorNr = SegmentType == 0xff ? FDef[BaseOffset+k] : SegmentType;
							putPixel<bpp>(dest, xB, yB, palette[colorNr], colorNr, yellowBorder, blueBorder, aCountMod);
						}
					}
					ftcp++; //increment pos
					if(attacker)
						xB++;
					else
						xB--;
					if ( SegmentType == 0xFF && TotalRowLength+k+1 >= SpriteWidth )
						break;
				}
				if (SegmentType == 0xFF)
				{
					BaseOffset += SegmentLength+1;
				}

				TotalRowLength+=SegmentLength+1;
			} while(TotalRowLength < SpriteWidth);
			if (RightMargin > 0)
			{
				ftcp += RightMargin;
			}
		}
		if (BottomMargin > 0)
		{
			ftcp += BottomMargin * FullWidth;
		}
	}

	return 0;
}

int CCreatureAnimation::nextFrame(SDL_Surface *dest, int x, int y, bool attacker, unsigned char animCount, bool IncrementFrame, bool yellowBorder, bool blueBorder, SDL_Rect * destRect)
{
	switch(dest->format->BytesPerPixel)
	{
	case 2: return nextFrameT<2>(dest, x, y, attacker, animCount, IncrementFrame, yellowBorder, blueBorder, destRect);
	case 3: return nextFrameT<3>(dest, x, y, attacker, animCount, IncrementFrame, yellowBorder, blueBorder, destRect);
	case 4: return nextFrameT<4>(dest, x, y, attacker, animCount, IncrementFrame, yellowBorder, blueBorder, destRect);
	default:
		tlog1 << (int)dest->format->BitsPerPixel << " bpp is not supported!!!\n";
		return -1;
	}
}

int CCreatureAnimation::framesInGroup(CCreatureAnim::EAnimType group) const
{
	if(frameGroups.find(group) == frameGroups.end())
		return 0;
	return frameGroups.find(group)->second.size();
}

CCreatureAnimation::~CCreatureAnimation()
{
	delete [] FDef;
}

template<int bpp>
inline void CCreatureAnimation::putPixel(
	SDL_Surface * dest,
	const int & ftcpX,
	const int & ftcpY,
	const BMPPalette & color,
	const unsigned char & palc,
	const bool & yellowBorder,
	const bool & blueBorder,
	const unsigned char & animCount
) const
{	
	if(palc!=0)
	{
		Uint8 * p = (Uint8*)dest->pixels + ftcpX*dest->format->BytesPerPixel + ftcpY*dest->pitch;
		if(palc > 7) //normal color
		{
			ColorPutter<bpp, 0>::PutColor(p, color.R, color.G, color.B);
		}
		else if((yellowBorder || blueBorder) && (palc == 6 || palc == 7)) //selection highlight
		{
			if(blueBorder)
				ColorPutter<bpp, 0>::PutColor(p, 0, 0x0f + animCount, 0x0f + animCount);
			else
				ColorPutter<bpp, 0>::PutColor(p, 0x0f + animCount, 0x0f + animCount, 0);
		}
		else if (palc == 5) //selection highlight or transparent
		{
			if(blueBorder)
				ColorPutter<bpp, 0>::PutColor(p, color.B, color.G - 0xf0 + animCount, color.R - 0xf0 + animCount); //shouldn't it be reversed? its bgr instead of rgb
			else if (yellowBorder)
				ColorPutter<bpp, 0>::PutColor(p, color.R - 0xf0 + animCount, color.G - 0xf0 + animCount, color.B);
		}
		else //shadow
		{
			//determining transparency value, 255 or 0 should be already filtered
			static Uint16 colToAlpha[8] = {255,192,128,128,128,255,128,192};
			Uint16 alpha = colToAlpha[palc];

			if(bpp != 3 && bpp != 4)
			{
				ColorPutter<bpp, 0>::PutColor(p, 0, 0, 0, alpha);
			}
			else
			{
				p[0] = (p[0] * alpha)>>8;
				p[1] = (p[1] * alpha)>>8;
				p[2] = (p[2] * alpha)>>8;
			}
		}
	}
}

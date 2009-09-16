#include "CCreatureAnimation.h"
#include "../hch/CLodHandler.h"
#include "../lib/VCMI_Lib.h"
#include <assert.h>

/*
 * CCreatureAnimation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

int CCreatureAnimation::getType() const
{
	return type;
}

void CCreatureAnimation::setType(int type)
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

CCreatureAnimation::CCreatureAnimation(std::string name) : RLEntries(NULL), internalFrame(0), once(false)
{
	FDef = spriteh->giveFile(name); //load main file

	//init anim data
	int i,j, totalInBlock;
	char Buffer[13];
	defName=name;
	i = 0;
	DEFType = readNormalNr(i,4); i+=4;
	fullWidth = readNormalNr(i,4); i+=4;
	fullHeight = readNormalNr(i,4); i+=4;
	i=0xc;
	totalBlocks = readNormalNr(i,4); i+=4;

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
		int group = readNormalNr(i,4); i+=4; //block ID
		totalInBlock = readNormalNr(i,4); i+=4;
		for (j=SEntries.size(); j<totalEntries+totalInBlock; j++)
		{
			SEntries.push_back(SEntry());
			SEntries[j].group = group;
			frameIDs.push_back(j);
		}
		int unknown2 = readNormalNr(i,4); i+=4; //TODO use me
		int unknown3 = readNormalNr(i,4); i+=4; //TODO use me
		i+=13*totalInBlock; //ommiting names
		for (j=0; j<totalInBlock; j++)
		{ 
			SEntries[totalEntries+j].offset = readNormalNr(i,4); i+=4;
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
	type = -1;
	frames = totalEntries;
	RLEntries = new int[fullHeight];
}

int CCreatureAnimation::readNormalNr (int pos, int bytCon, unsigned char * str) const
{
	int ret=0;
	int amp=1;
	if (str)
	{
		for (int i=0; i<bytCon; i++)
		{
			ret+=str[pos+i]*amp;
			amp<<=8; //amp*=256;
		}
	}
	else 
	{
		for (int i=0; i<bytCon; i++)
		{
			ret+=FDef[pos+i]*amp;
			amp<<=8; //amp*=256;
		}
	}
	return ret;
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
				type = 2;
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

bool CCreatureAnimation::onLastFrameInGroup()
{
	if(internalFrame == frameGroups[type].size() - 1)
		return true;
	return false;
}

void CCreatureAnimation::playOnce(int type)
{
	setType(type);
	once = true;
}

int CCreatureAnimation::nextFrame(SDL_Surface *dest, int x, int y, bool attacker, unsigned char animCount, bool IncrementFrame, bool yellowBorder, bool blueBorder, SDL_Rect * destRect)
{
	if(dest->format->BytesPerPixel<3)
		return -1; //not enough depth

	//increasing frame numer
	int SIndex = curFrame;
	if(IncrementFrame)
		incrementFrame();
	//frame number increased

	long BaseOffset, 
		SpriteWidth, SpriteHeight, //sprite format
		LeftMargin, RightMargin, TopMargin,BottomMargin,
		i, FullHeight,FullWidth,
		TotalRowLength; // length of read segment
	unsigned char SegmentType, SegmentLength;
	
	i=BaseOffset=SEntries[SIndex].offset;
	int prSize=readNormalNr(i,4,FDef);i+=4;//TODO use me
	int defType2 = readNormalNr(i,4,FDef);i+=4;
	FullWidth = readNormalNr(i,4,FDef);i+=4;
	FullHeight = readNormalNr(i,4,FDef);i+=4;
	SpriteWidth = readNormalNr(i,4,FDef);i+=4;
	SpriteHeight = readNormalNr(i,4,FDef);i+=4;
	LeftMargin = readNormalNr(i,4,FDef);i+=4;
	TopMargin = readNormalNr(i,4,FDef);i+=4;
	RightMargin = FullWidth - SpriteWidth - LeftMargin;
	BottomMargin = FullHeight - SpriteHeight - TopMargin;

	int BaseOffsetor = BaseOffset = i;

	int ftcp = 0;

	if (defType2==1) //as it should be allways in creature animations
	{
		if (TopMargin>0)
		{
			ftcp+=FullWidth * TopMargin;
		}
		memcpy(RLEntries, FDef+BaseOffset, SpriteHeight*sizeof(int));
		BaseOffset += sizeof(int) * SpriteHeight;
		for (int i=0;i<SpriteHeight;i++)
		{
			BaseOffset=BaseOffsetor+RLEntries[i];
			if (LeftMargin>0)
			{
				ftcp+=LeftMargin;
			}
			TotalRowLength=0;
			do
			{
				SegmentType=FDef[BaseOffset++];
				SegmentLength=FDef[BaseOffset++];
				unsigned char aCountMod = (animCount & 0x20) ? ((animCount & 0x1e)>>1)<<4 : 0x0f - ((animCount & 0x1e)>>1)<<4;
				if (SegmentType==0xFF)
				{
					for (int k=0;k<=SegmentLength;k++)
					{
						int xB = (attacker ? ftcp%FullWidth : FullWidth - ftcp%FullWidth - 1) + x;
						int yB = ftcp/FullWidth + y;
						if(xB>=0 && yB>=0 && xB<dest->w && yB<dest->h)
						{
							if(!destRect || (destRect->x <= xB && destRect->x + destRect->w > xB && destRect->y <= yB && destRect->y + destRect->h > yB))
								putPixel(dest, xB + yB*dest->w, palette[FDef[BaseOffset+k]], FDef[BaseOffset+k], yellowBorder, blueBorder, aCountMod);
						}
						ftcp++; //increment pos
						if ((TotalRowLength+k+1)>=SpriteWidth)
							break;
					}
					BaseOffset+=SegmentLength+1;////
					TotalRowLength+=SegmentLength+1;
				}
				else
				{
					for (int k=0;k<SegmentLength+1;k++)
					{
						int xB = (attacker ? ftcp%FullWidth : FullWidth - ftcp%FullWidth - 1) + x;
						int yB = ftcp/FullWidth + y;
						if(xB>=0 && yB>=0 && xB<dest->w && yB<dest->h)
						{
							if(!destRect || (destRect->x <= xB && destRect->x + destRect->w > xB && destRect->y <= yB && destRect->y + destRect->h > yB))
								putPixel(dest, xB + yB*dest->w, palette[SegmentType], SegmentType, yellowBorder, blueBorder, aCountMod);
						}
						ftcp++; //increment pos
					}
					TotalRowLength+=SegmentLength+1;
				}
			}while(TotalRowLength<SpriteWidth);
			if (RightMargin>0)
			{
				ftcp+=RightMargin;
			}
		}
		if (BottomMargin>0)
		{
			ftcp += BottomMargin * FullWidth;
		}
	}

	return 0;
}

int CCreatureAnimation::framesInGroup(int group) const
{
	if(frameGroups.find(group) == frameGroups.end())
		return 0;
	return frameGroups.find(group)->second.size();
}

CCreatureAnimation::~CCreatureAnimation()
{
	delete [] FDef;
	if (RLEntries)
		delete [] RLEntries;
}

inline void CCreatureAnimation::putPixel(
	SDL_Surface * dest,
	const int & ftcp,
	const BMPPalette & color,
	const unsigned char & palc,
	const bool & yellowBorder,
	const bool & blueBorder,
	const unsigned char & animCount
) const
{	
    if(palc!=0)
	{
		Uint8 * p = (Uint8*)dest->pixels + ftcp*dest->format->BytesPerPixel;
		if(palc > 7) //normal color
		{
			p[0] = color.B;
			p[1] = color.G;
			p[2] = color.R;
		}
		else if((yellowBorder || blueBorder) && (palc == 6 || palc == 7)) //dark yellow border
		{
			if(blueBorder)
			{
				p[0] = 0x0f + animCount;
				p[1] = 0x0f + animCount;
				p[2] = 0;
			}
			else
			{
				p[0] = 0;
				p[1] = 0x0f + animCount;
				p[2] = 0x0f + animCount;
			}
		}
		else if((yellowBorder || blueBorder) && (palc == 5)) //yellow border
		{
			if(blueBorder)
			{
				p[0] = color.R - 0xf0 + animCount;
				p[1] = color.G - 0xf0 + animCount;
				p[2] = color.B;
			}
			else
			{
				p[0] = color.B;
				p[1] = color.G - 0xf0 + animCount;
				p[2] = color.R - 0xf0 + animCount;
			}
		}
		else if(palc < 5) //shadow
		{ 
			Uint16 alpha;
			switch(color.G)
			{
			case 0:
				alpha = 128;
				break;
			case 50:
				alpha = 50+32;
				break;
			case 100:
				alpha = 100+64;
				break;
			case 125:
				alpha = 125+64;
				break;
			case 128:
				alpha = 128+64;
				break;
			case 150:
				alpha = 150+64;
				break;
			default:
				alpha = 255;
				break;
			}
			//alpha counted
			p[0] = (p[0] * alpha)>>8;
			p[1] = (p[1] * alpha)>>8;
			p[2] = (p[2] * alpha)>>8;
		}
	}
}

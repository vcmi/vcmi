#include "CCreatureAnimation.h"
#include "../hch/CLodHandler.h"
int CCreatureAnimation::getType() const
{
	return type;
}

void CCreatureAnimation::setType(int type)
{
	this->type = type;
	curFrame = 0;
	if(type!=-1)
	{
		if(SEntries[curFrame].group!=type) //rewind
		{
			int j=-1; //first frame in displayed group
			for(size_t g=0; g<SEntries.size(); ++g)
			{
				if(SEntries[g].group==type && j==-1)
				{
					j = g;
					break;
				}
			}
			if(curFrame != -1) {
				curFrame = j;
                        }
		}
	}
	else
	{
		if(curFrame>=frames) {
			curFrame = 0;
                }
	}
}

CCreatureAnimation::CCreatureAnimation(std::string name) : RLEntries(NULL)
{
	FDef = CDefHandler::Spriteh->giveFile(name); //load main file

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
		int group = readNormalNr(i,4); i+=4; //block ID
		totalInBlock = readNormalNr(i,4); i+=4;
		for (j=SEntries.size(); j<totalEntries+totalInBlock; j++)
		{
			SEntries.push_back(SEntry());
			SEntries[j].group = group;
		}
		int unknown2 = readNormalNr(i,4); i+=4; //TODO use me
		int unknown3 = readNormalNr(i,4); i+=4; //TODO use me
		for (j=0; j<totalInBlock; j++)
		{
			for (int k=0;k<13;k++) Buffer[k]=FDef[i+k]; 
			i+=13;
			SEntries[totalEntries+j].name=Buffer;
		}
		for (j=0; j<totalInBlock; j++)
		{ 
			SEntries[totalEntries+j].offset = readNormalNr(i,4);
			int unknown4 = readNormalNr(i,4); i+=4; //TODO use me
		}
		//totalEntries+=totalInBlock;
		for(int hh=0; hh<totalInBlock; ++hh)
		{
			++totalEntries;
		}
	}
	for(j=0; j<SEntries.size(); ++j)
	{
		SEntries[j].name = SEntries[j].name.substr(0, SEntries[j].name.find('.')+4);
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
int CCreatureAnimation::nextFrameMiddle(SDL_Surface *dest, int x, int y, bool attacker, bool incrementFrame, bool yellowBorder, SDL_Rect * destRect)
{
	return nextFrame(dest,x-fullWidth/2,y-fullHeight/2,attacker,incrementFrame,yellowBorder,destRect);
}
void CCreatureAnimation::incrementFrame()
{
	curFrame++;
	if(type!=-1)
	{
		if(curFrame==SEntries.size() || SEntries[curFrame].group!=type) //rewind
		{
			int j=-1; //first frame in displayed group
			for(size_t g=0; g<SEntries.size(); ++g)
			{
				if(SEntries[g].group==type)
				{
					j = g;
					break;
				}
			}
			if(curFrame!=-1)
				curFrame = j;
		}
	}
	else
	{
		if(curFrame>=frames)
			curFrame = 0;
	}
}

int CCreatureAnimation::getFrame() const
{
	return curFrame;
}

int CCreatureAnimation::nextFrame(SDL_Surface *dest, int x, int y, bool attacker, bool IncrementFrame, bool yellowBorder, SDL_Rect * destRect)
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
				if (SegmentType==0xFF)
				{
					for (int k=0;k<=SegmentLength;k++)
					{
						int xB = (attacker ? ftcp%FullWidth : FullWidth - ftcp%FullWidth - 1) + x;
						int yB = ftcp/FullWidth + y;
						if(xB>=0 && yB>=0 && xB<dest->w && yB<dest->h)
						{
							if(!destRect || (destRect->x <= xB && destRect->x + destRect->w > xB && destRect->y <= yB && destRect->y + destRect->h > yB))
								putPixel(dest, xB + yB*dest->w, palette[FDef[BaseOffset+k]], FDef[BaseOffset+k], yellowBorder);
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
								putPixel(dest, xB + yB*dest->w, palette[SegmentType], SegmentType, yellowBorder);
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
	int ret = 0; //number of frames in given group
	for(size_t g=0; g<SEntries.size(); ++g)
	{
		if(SEntries[g].group == group)
			++ret;
	}
	return ret;
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
        const bool & yellowBorder
) const {
	
    if(palc!=0) {
		Uint8 * p = (Uint8*)dest->pixels + ftcp*3;
		if(palc > 7) //normal color
		{
			p[0] = color.B;
			p[1] = color.G;
			p[2] = color.R;
		}
		else if(yellowBorder && (palc == 6 || palc == 7)) //dark yellow border
		{
			p[0] = 0;
			p[1] = 0xff;
			p[2] = 0xff;
		}
		else if(yellowBorder && (palc == 5)) //yellow border
		{
			p[0] = color.B;
			p[1] = color.G;
			p[2] = color.R;
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

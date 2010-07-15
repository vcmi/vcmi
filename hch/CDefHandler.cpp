#include "../stdafx.h"
#include "SDL.h"
#include "CDefHandler.h"
#include <sstream>
#include "CLodHandler.h"
#include "../lib/VCMI_Lib.h"

/*
 * CDefHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

static long long pow(long long a, int b)
{
	if (!b) return 1;
	long c = a;
	while (--b)
		a*=c;
	return a;
}
CDefHandler::CDefHandler()
{
	//FDef = NULL;
	notFreeImgs = false;
}
CDefHandler::~CDefHandler()
{
	//if (FDef)
		//delete [] FDef;
	if (notFreeImgs)
		return;
	for (size_t i=0; i<ourImages.size(); ++i)
	{
		if (ourImages[i].bitmap)
		{
			SDL_FreeSurface(ourImages[i].bitmap);
			ourImages[i].bitmap=NULL;
		}
	}
}
CDefEssential::~CDefEssential()
{
	for(size_t i=0; i < ourImages.size(); ++i)
		SDL_FreeSurface(ourImages[i].bitmap);
}

void CDefHandler::openFromMemory(unsigned char *table, const std::string & name)
{
	BMPPalette palette[256];
	SDefEntry &de = * reinterpret_cast<SDefEntry *>(table);
	unsigned char *p;

	defName = name;
	DEFType = SDL_SwapLE32(de.DEFType);
	width = SDL_SwapLE32(de.width);
	height = SDL_SwapLE32(de.height);
	unsigned int totalBlocks = SDL_SwapLE32(de.totalBlocks);

	for (unsigned int it=0;it<256;it++)
	{
		palette[it].R = de.palette[it].R;
		palette[it].G = de.palette[it].G;
		palette[it].B = de.palette[it].B;
		palette[it].F = 0;
	}

	// The SDefEntryBlock starts just after the SDefEntry
	p = reinterpret_cast<unsigned char *>(&de);
	p += sizeof(de);

	int totalEntries=0;
	for (unsigned int z=0; z<totalBlocks; z++)
	{
		SDefEntryBlock &block = * reinterpret_cast<SDefEntryBlock *>(p);
		unsigned int totalInBlock;

		totalInBlock = SDL_SwapLE32(read_unaligned_u32(&block.totalInBlock));

		for (unsigned int j=SEntries.size(); j<totalEntries+totalInBlock; j++)
			SEntries.push_back(SEntry());

		p = block.data;
		for (unsigned int j=0; j<totalInBlock; j++)
		{
			char Buffer[13];
			memcpy(Buffer, p, 12);
			Buffer[12] = 0;
			SEntries[totalEntries+j].name=Buffer;
			p += 13;
		}
		for (unsigned int j=0; j<totalInBlock; j++)
		{ 
			SEntries[totalEntries+j].offset = SDL_SwapLE32(read_unaligned_u32(p));
			p += 4;
		}
		//totalEntries+=totalInBlock;
		for(unsigned int hh=0; hh<totalInBlock; ++hh)
		{
			SEntries[totalEntries].group = z;
			++totalEntries;
		}
	}

	for(unsigned int j=0; j<SEntries.size(); ++j)
	{
		SEntries[j].name = SEntries[j].name.substr(0, SEntries[j].name.find('.')+4);
	}
	//RWEntries = new unsigned int[height];
	for(unsigned int i=0; i < SEntries.size(); ++i)
	{
		Cimage nimg;
		nimg.bitmap = getSprite(i, table, palette);
		nimg.imName = SEntries[i].name;
		nimg.groupNumber = SEntries[i].group;
		ourImages.push_back(nimg);
	}
}

void CDefHandler::expand(unsigned char N,unsigned char & BL, unsigned char & BR)
{
	BL = (N & 0xE0) >> 5;
	BR = N & 0x1F;
}

SDL_Surface * CDefHandler::getSprite (int SIndex, const unsigned char * FDef, const BMPPalette * palette) const
{
	SDL_Surface * ret=NULL;

	unsigned int BaseOffset, 
		SpriteWidth, SpriteHeight, //format of sprite
		TotalRowLength,			// length of read segment
		add, FullHeight,FullWidth,
		RowAdd,					//, NextSpriteOffset; //TODO use me
		prSize,
		defType2;
	int LeftMargin, RightMargin, TopMargin, BottomMargin;


	unsigned char SegmentType;//, BL, BR; //TODO use me

	BaseOffset = SEntries[SIndex].offset;
	SSpriteDef sd = * reinterpret_cast<const SSpriteDef *>(FDef + BaseOffset);

	prSize = SDL_SwapLE32(sd.prSize); //TODO use me
	defType2 = SDL_SwapLE32(sd.defType2);
	FullWidth = SDL_SwapLE32(sd.FullWidth);
	FullHeight = SDL_SwapLE32(sd.FullHeight);
	SpriteWidth = SDL_SwapLE32(sd.SpriteWidth);
	SpriteHeight = SDL_SwapLE32(sd.SpriteHeight);
	LeftMargin = SDL_SwapLE32(sd.LeftMargin);
	TopMargin = SDL_SwapLE32(sd.TopMargin);
	RightMargin = FullWidth - SpriteWidth - LeftMargin;
	BottomMargin = FullHeight - SpriteHeight - TopMargin;

	//if(LeftMargin + RightMargin < 0)
	//	SpriteWidth += LeftMargin + RightMargin; //ugly construction... TODO: check how to do it nicer
	if(LeftMargin<0)
		SpriteWidth+=LeftMargin;
	if(RightMargin<0)
		SpriteWidth+=RightMargin;
	
	// Note: this looks bogus because we allocate only FullWidth, not FullWidth+add
	add = 4 - FullWidth%4;
	if (add==4)
		add=0;

	ret = SDL_CreateRGBSurface(SDL_SWSURFACE, FullWidth, FullHeight, 8, 0, 0, 0, 0);
	//int tempee2 = readNormalNr(0,4,((unsigned char *)tempee.c_str()));

	BaseOffset += sizeof(SSpriteDef);
	int BaseOffsetor = BaseOffset;

	for(int i=0; i<256; ++i)
	{
		SDL_Color pr;
		pr.r = palette[i].R;
		pr.g = palette[i].G;
		pr.b = palette[i].B;
		pr.unused = palette[i].F;
		(*(ret->format->palette->colors+i))=pr;
	}

	int ftcp=0;

	// If there's a margin anywhere, just blank out the whole surface.
	if (TopMargin > 0 || BottomMargin > 0 || LeftMargin > 0 || RightMargin > 0) {
		memset( reinterpret_cast<char*>(ret->pixels), 0, FullHeight*FullWidth);
	}

	// Skip top margin
	if (TopMargin > 0)
		ftcp += TopMargin*(FullWidth+add);

	switch(defType2)
	{
	case 0:
	{
		for (unsigned int i=0;i<SpriteHeight;i++)
		{
			if (LeftMargin>0)
				ftcp += LeftMargin;

			memcpy(reinterpret_cast<char*>(ret->pixels)+ftcp, &FDef[BaseOffset], SpriteWidth);
			ftcp += SpriteWidth;
			BaseOffset += SpriteWidth;
			
			if (RightMargin>0)
				ftcp += RightMargin;
		}
	}
	break;

	case 1:
	{
		const unsigned int * RWEntriesLoc = reinterpret_cast<const unsigned int *>(FDef+BaseOffset);
		BaseOffset += sizeof(int) * SpriteHeight;
		for (unsigned int i=0;i<SpriteHeight;i++)
		{
			BaseOffset=BaseOffsetor + SDL_SwapLE32(read_unaligned_u32(RWEntriesLoc + i));
			if (LeftMargin>0)
				ftcp += LeftMargin;

			TotalRowLength=0;
			do
			{
				unsigned int SegmentLength;

				SegmentType=FDef[BaseOffset++];
				SegmentLength=FDef[BaseOffset++] + 1;

				if (SegmentType==0xFF)
				{
					memcpy(reinterpret_cast<char*>(ret->pixels)+ftcp, FDef + BaseOffset, SegmentLength);
					BaseOffset+=SegmentLength;
				}
				else
				{
					memset(reinterpret_cast<char*>(ret->pixels)+ftcp, SegmentType, SegmentLength);
				}
				ftcp += SegmentLength;
				TotalRowLength += SegmentLength;
			}while(TotalRowLength<SpriteWidth);

			RowAdd=SpriteWidth-TotalRowLength;

			if (RightMargin>0)
				ftcp += RightMargin;

			if (add>0)
				ftcp += add+RowAdd;
		}
	}
	break;

	case 2:
	{
		BaseOffset = BaseOffsetor + SDL_SwapLE16(read_unaligned_u16(FDef + BaseOffsetor));

		for (unsigned int i=0;i<SpriteHeight;i++)
		{
			//BaseOffset = BaseOffsetor+RWEntries[i];
			if (LeftMargin>0)
				ftcp += LeftMargin;

			TotalRowLength=0;

			do
			{
				SegmentType=FDef[BaseOffset++];
				unsigned char code = SegmentType / 32;
				unsigned char value = (SegmentType & 31) + 1;
				if(code==7)
				{
					memcpy(reinterpret_cast<char*>(ret->pixels)+ftcp, &FDef[BaseOffset], value);
					ftcp += value;
					BaseOffset += value;
				}
				else
				{
					memset(reinterpret_cast<char*>(ret->pixels)+ftcp, code, value);
					ftcp += value;
				}
				TotalRowLength+=value;
			} while(TotalRowLength<SpriteWidth);

			if (RightMargin>0)
				ftcp += RightMargin;

			RowAdd=SpriteWidth-TotalRowLength;

			if (add>0)
				ftcp += add+RowAdd;
		}
	}
	break;

	case 3:
	{
		for (unsigned int i=0;i<SpriteHeight;i++)
		{
			BaseOffset = BaseOffsetor + SDL_SwapLE16(read_unaligned_u16(FDef + BaseOffsetor+i*2*(SpriteWidth/32)));
			if (LeftMargin>0)
				ftcp += LeftMargin;

			TotalRowLength=0;

			do
			{
				SegmentType=FDef[BaseOffset++];
				unsigned char code = SegmentType / 32;
				unsigned char value = (SegmentType & 31) + 1;

				int len = std::min<unsigned int>(value, SpriteWidth - TotalRowLength) - std::max(0, -LeftMargin);
				amax(len, 0);

				if(code==7)
				{
					memcpy((ui8*)ret->pixels + ftcp, FDef + BaseOffset, len);
					ftcp += len;
					BaseOffset += len;
				}
				else
				{
					memset((ui8*)ret->pixels + ftcp, code, len);
					ftcp += len;
				}
				TotalRowLength+=( LeftMargin>=0 ? value : value+LeftMargin );
			}while(TotalRowLength<SpriteWidth);

			if (RightMargin>0)
				ftcp += RightMargin;

			RowAdd=SpriteWidth-TotalRowLength;

			if (add>0)
				ftcp += add+RowAdd;
		}
	}
	break;

	default:
		throw std::string("Unknown sprite format.");
		break;
	}

	SDL_Color ttcol = ret->format->palette->colors[0];
	Uint32 keycol = SDL_MapRGBA(ret->format, ttcol.r, ttcol.b, ttcol.g, ttcol.unused);
	SDL_SetColorKey(ret, SDL_SRCCOLORKEY, keycol);
	return ret;
};

CDefEssential * CDefHandler::essentialize()
{
	CDefEssential * ret = new CDefEssential;
	ret->ourImages = ourImages;
	notFreeImgs = true;
	return ret;
}

CDefHandler * CDefHandler::giveDef(const std::string & defName)
{
	unsigned char * data = spriteh->giveFile(defName);
	if(!data)
		throw "bad def name!";
	CDefHandler * nh = new CDefHandler();
	nh->openFromMemory(data, defName);
	nh->alphaTransformed = false;
	delete [] data;
	return nh;
}
CDefEssential * CDefHandler::giveDefEss(const std::string & defName)
{
	CDefEssential * ret;
	CDefHandler * temp = giveDef(defName);
	ret = temp->essentialize();
	delete temp;
	return ret;
}


#include "StdInc.h"
#include "SDL.h"
#include "CDefHandler.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/VCMI_Lib.h"
#include "CBitmapHandler.h"
#include "gui/SDL_Extensions.h"
/*
 * CDefHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#ifdef unused
static long long pow(long long a, int b)
{
	if (!b) return 1;
	long c = a;
	while (--b)
		a*=c;
	return a;
}
#endif

CDefHandler::CDefHandler()
{
	notFreeImgs = false;
}
CDefHandler::~CDefHandler()
{
	if (notFreeImgs)
		return;
	for (auto & elem : ourImages)
	{
		if (elem.bitmap)
		{
			SDL_FreeSurface(elem.bitmap);
			elem.bitmap=nullptr;
		}
	}
}
CDefEssential::~CDefEssential()
{
	for(auto & elem : ourImages)
		SDL_FreeSurface(elem.bitmap);
}

void CDefHandler::openFromMemory(ui8 *table, const std::string & name)
{
	SDL_Color palette[256];
	SDefEntry &de = * reinterpret_cast<SDefEntry *>(table);
	ui8 *p;

	defName = name;
	DEFType = read_le_u32(&de.DEFType);
	width = read_le_u32(&de.width);
	height = read_le_u32(&de.height);
	ui32 totalBlocks = read_le_u32(&de.totalBlocks);

	for (ui32 it=0;it<256;it++)
	{
		palette[it].r = de.palette[it].R;
		palette[it].g = de.palette[it].G;
		palette[it].b = de.palette[it].B;
		CSDL_Ext::colorSetAlpha(palette[it],SDL_ALPHA_OPAQUE);	
	}

	// The SDefEntryBlock starts just after the SDefEntry
	p = reinterpret_cast<ui8 *>(&de);
	p += sizeof(de);

	int totalEntries=0;
	for (ui32 z=0; z<totalBlocks; z++)
	{
		SDefEntryBlock &block = * reinterpret_cast<SDefEntryBlock *>(p);
		ui32 totalInBlock;

		totalInBlock = read_le_u32(&block.totalInBlock);

		for (ui32 j=SEntries.size(); j<totalEntries+totalInBlock; j++)
			SEntries.push_back(SEntry());

		p = block.data;
		for (ui32 j=0; j<totalInBlock; j++)
		{
			char Buffer[13];
			memcpy(Buffer, p, 12);
			Buffer[12] = 0;
			SEntries[totalEntries+j].name=Buffer;
			p += 13;
		}
		for (ui32 j=0; j<totalInBlock; j++)
		{
			SEntries[totalEntries+j].offset = read_le_u32(p);
			p += 4;
		}
		//totalEntries+=totalInBlock;
		for(ui32 hh=0; hh<totalInBlock; ++hh)
		{
			SEntries[totalEntries].group = z;
			++totalEntries;
		}
	}

	for(auto & elem : SEntries)
	{
		elem.name = elem.name.substr(0, elem.name.find('.')+4);
	}
	//RWEntries = new ui32[height];
	for(ui32 i=0; i < SEntries.size(); ++i)
	{
		Cimage nimg;
		nimg.bitmap = getSprite(i, table, palette);
		nimg.imName = SEntries[i].name;
		nimg.groupNumber = SEntries[i].group;
		ourImages.push_back(nimg);
	}
}

void CDefHandler::expand(ui8 N,ui8 & BL, ui8 & BR)
{
	BL = (N & 0xE0) >> 5;
	BR = N & 0x1F;
}

SDL_Surface * CDefHandler::getSprite (int SIndex, const ui8 * FDef, const SDL_Color * palette) const
{
	SDL_Surface * ret=nullptr;

	ui32 BaseOffset,
		SpriteWidth, SpriteHeight, //format of sprite
		TotalRowLength,			// length of read segment
		add, FullHeight,FullWidth,
		RowAdd,
		defType2;
	int LeftMargin, RightMargin, TopMargin, BottomMargin;


	ui8 SegmentType;

	BaseOffset = SEntries[SIndex].offset;
	SSpriteDef sd = * reinterpret_cast<const SSpriteDef *>(FDef + BaseOffset);

	defType2 = read_le_u32(&sd.defType2);
	FullWidth = read_le_u32(&sd.FullWidth);
	FullHeight = read_le_u32(&sd.FullHeight);
	SpriteWidth = read_le_u32(&sd.SpriteWidth);
	SpriteHeight = read_le_u32(&sd.SpriteHeight);
	LeftMargin = read_le_u32(&sd.LeftMargin);
	TopMargin = read_le_u32(&sd.TopMargin);
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
	
	if(nullptr == ret)
	{
		logGlobal->errorStream() << __FUNCTION__ <<": Unable to create surface";
		logGlobal->errorStream() << FullWidth << "X" << FullHeight;
		logGlobal->errorStream() << SDL_GetError();
		throw std::runtime_error("Unable to create surface");		
	}

	BaseOffset += sizeof(SSpriteDef);
	int BaseOffsetor = BaseOffset;

	#ifdef VCMI_SDL1
	for(int i=0; i<256; ++i)
	{		
		SDL_Color pr;
		pr.r = palette[i].r;
		pr.g = palette[i].g;
		pr.b = palette[i].b;
		pr.unused = palette[i].unused;
		(*(ret->format->palette->colors+i))=pr;		
	}
	#else
	if(SDL_SetPaletteColors(ret->format->palette,palette,0,256) != 0)
	{
		throw std::runtime_error("Unable to set palette");	
	}
	
	#endif

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
		for (ui32 i=0;i<SpriteHeight;i++)
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
		const ui32 * RWEntriesLoc = reinterpret_cast<const ui32 *>(FDef+BaseOffset);
		BaseOffset += sizeof(int) * SpriteHeight;
		for (ui32 i=0;i<SpriteHeight;i++)
		{
			BaseOffset=BaseOffsetor + read_le_u32(RWEntriesLoc + i);
			if (LeftMargin>0)
				ftcp += LeftMargin;

			TotalRowLength=0;
			do
			{
				ui32 SegmentLength;

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
		BaseOffset = BaseOffsetor + read_le_u16(FDef + BaseOffsetor);

		for (ui32 i=0;i<SpriteHeight;i++)
		{
			//BaseOffset = BaseOffsetor+RWEntries[i];
			if (LeftMargin>0)
				ftcp += LeftMargin;

			TotalRowLength=0;

			do
			{
				SegmentType=FDef[BaseOffset++];
				ui8 code = SegmentType / 32;
				ui8 value = (SegmentType & 31) + 1;
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
		for (ui32 i=0;i<SpriteHeight;i++)
		{
			BaseOffset = BaseOffsetor + read_le_u16(FDef + BaseOffsetor+i*2*(SpriteWidth/32));
			if (LeftMargin>0)
				ftcp += LeftMargin;

			TotalRowLength=0;

			do
			{
				SegmentType=FDef[BaseOffset++];
				ui8 code = SegmentType / 32;
				ui8 value = (SegmentType & 31) + 1;

				int len = std::min<ui32>(value, SpriteWidth - TotalRowLength) - std::max(0, -LeftMargin);
				vstd::amax(len, 0);

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
		throw std::runtime_error("Unknown sprite format.");
		break;
	}

	SDL_Color ttcol = ret->format->palette->colors[0];
	#ifdef VCMI_SDL1
	Uint32 keycol = SDL_MapRGBA(ret->format, ttcol.r, ttcol.b, ttcol.g, ttcol.unused);	
	SDL_SetColorKey(ret, SDL_SRCCOLORKEY, keycol);	
	#else
	Uint32 keycol = SDL_MapRGBA(ret->format, ttcol.r, ttcol.b, ttcol.g, ttcol.a);	
	SDL_SetColorKey(ret, SDL_TRUE, keycol);	
	#endif // 0

	return ret;
}

CDefEssential * CDefHandler::essentialize()
{
	auto   ret = new CDefEssential;
	ret->ourImages = ourImages;
	notFreeImgs = true;
	return ret;
}

CDefHandler * CDefHandler::giveDef(const std::string & defName)
{
	ResourceID resID(std::string("SPRITES/") + defName, EResType::ANIMATION);

	auto data = CResourceHandler::get()->load(resID)->readAll().first;
	if(!data)
		throw std::runtime_error("bad def name!");
	auto   nh = new CDefHandler();
	nh->openFromMemory(data.get(), defName);
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


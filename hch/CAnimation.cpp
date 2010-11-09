#include <iostream>
#include <sstream>
#include <boost/foreach.hpp>

#include "SDL.h"
#include "SDL_image.h"

#include "../client/CBitmapHandler.h"
#include "CAnimation.h"
#include "CLodHandler.h"

/*
 * CAnimation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


extern DLL_EXPORT CLodHandler *spriteh;

/*************************************************************************
 *  DefFile, class used for def loading                                  *
 *************************************************************************/

bool CDefFile::haveFrame(size_t frame, size_t group) const
{
	if (offset.size() > group)
		if (offset[group].size() > frame)
			return true;

	return false;
}

SDL_Surface *  CDefFile::loadFrame(size_t frame, size_t group) const
{
	if (haveFrame(frame, group))
		return loadFrame(( unsigned char * )data+offset[group][frame], colors);
	return NULL;
}

SDL_Surface * CDefFile::loadFrame (const unsigned char * FDef, const BMPPalette * palette)
{
	SDL_Surface * ret=NULL;

	unsigned int BaseOffset,
	         SpriteWidth, SpriteHeight, //format of sprite
	         TotalRowLength,			// length of read segment
	         add, FullHeight,FullWidth,
	         RowAdd,
	         prSize,
	         defType2;
	int LeftMargin, RightMargin, TopMargin, BottomMargin;


	unsigned char SegmentType;

	BaseOffset = 0;
	SSpriteDef sd = * reinterpret_cast<const SSpriteDef *>(FDef + BaseOffset);

	prSize = SDL_SwapLE32(sd.prSize);
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
	if (LeftMargin<0)
		SpriteWidth+=LeftMargin;
	if (RightMargin<0)
		SpriteWidth+=RightMargin;

	// Note: this looks bogus because we allocate only FullWidth, not FullWidth+add
	add = 4 - FullWidth%4;
	if (add==4)
		add=0;

	ret = SDL_CreateRGBSurface(SDL_SWSURFACE, FullWidth, FullHeight, 8, 0, 0, 0, 0);
	//int tempee2 = readNormalNr(0,4,((unsigned char *)tempee.c_str()));

	BaseOffset += sizeof(SSpriteDef);
	int BaseOffsetor = BaseOffset;

	for (int i=0; i<256; ++i)
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
	if (TopMargin > 0 || BottomMargin > 0 || LeftMargin > 0 || RightMargin > 0)
	{
		memset( reinterpret_cast<char*>(ret->pixels), 0, FullHeight*FullWidth);
	}

	// Skip top margin
	if (TopMargin > 0)
		ftcp += TopMargin*(FullWidth+add);

	switch (defType2)
	{
	case 0:
		{
			for (unsigned int i=0; i<SpriteHeight; i++)
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
			for (unsigned int i=0; i<SpriteHeight; i++)
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
				}
				while (TotalRowLength<SpriteWidth);

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

			for (unsigned int i=0; i<SpriteHeight; i++)
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
					if (code==7)
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
				}
				while (TotalRowLength<SpriteWidth);

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
			for (unsigned int i=0; i<SpriteHeight; i++)
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

					if (code==7)
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
				}
				while (TotalRowLength<SpriteWidth);

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

BMPPalette * CDefFile::getPalette()
{
	BMPPalette * ret = new BMPPalette[256];
	memcpy(ret, colors, sizeof(BMPPalette)*256);
	return ret;
}

CDefFile::CDefFile(std::string Name):data(NULL),colors(NULL)
{
	data = spriteh->giveFile(Name, FILE_ANIMATION, &datasize);
	if (!data)
	{
		tlog0<<"Error: file "<< Name <<" not found\n";
		return;
	}

	colors = new BMPPalette[256];
	int it = 0;

	//int type   = readNormalNr(data, it); it+=4;
	//int width  = readNormalNr(data, it); it+=4;//not used
	//int height = readNormalNr(data, it); it+=4;
	it+=12;
	unsigned int totalBlocks = readNormalNr(data, it);
	it+=4;

	for (unsigned int i=0; i<256; i++)
	{
		colors[i].R = data[it++];
		colors[i].G = data[it++];
		colors[i].B = data[it++];
		colors[i].F = 0;
	}

	offset.resize(totalBlocks);
	offList.insert(datasize);

	for (unsigned int i=0; i<totalBlocks; i++)
	{
		it+=4;
		unsigned int totalEntries = readNormalNr(data, it);
		it+=12;

		//13 bytes for name of every frame in this block - not used, skipping
		it+= 13 * totalEntries;

		for (unsigned int j=0; j<totalEntries; j++)
		{
			size_t currOffset = readNormalNr(data, it);
			offset[i].push_back(currOffset);
			offList.insert(currOffset);
			it += 4;
		}
	}
}
unsigned char * CDefFile::getFrame(size_t frame, size_t group) const
{
	if (offset.size() > group)
	{
		if (offset[group].size() > frame)
		{
			size_t offs = offset[group][frame];

			std::set<size_t>::iterator it = offList.find(offs);

			if (it == offList.end() || ++it == offList.end())
				tlog0<<"Error: offset not found!\n";

			size_t size = *it - offs;

			unsigned char * ret = new unsigned char[size];
			memcpy(ret, data+offs, size);
			return ret;
		}
	}
	return NULL;
}

CDefFile::~CDefFile()
{
	delete[] data;
	delete[] colors;
}

bool CDefFile::loaded() const
{
	return data != NULL;
}

/*************************************************************************
 *  CAnimation for animations handling, can load part of file if needed  *
 *************************************************************************/

CAnimation::AnimEntry::AnimEntry():
	surf(NULL),
	source(0),
	refCount(0),
	data(NULL),
	dataSize(0)
{

}

bool CAnimation::loadFrame(CDefFile * file, size_t frame, size_t group)
{
	if (groupSize(group) <= frame)
		return false;
	AnimEntry &e = entries[group][frame];

	if (e.surf || e.data)
	{
		e.refCount++;
		return true;
	}

	if (e.source & 6)//load frame with SDL_Image
	{
		int size;
		unsigned char * pic = NULL;

		std::ostringstream str;
		if ( e.source & 2 )
			str << name << '#' << (group+1) << '#' << (frame+1); // file#12#34.*
		else
			str << name << '#' << (frame+1);//file#34.*

		pic = spriteh->giveFile(str.str(), FILE_GRAPHICS, &size);
		if (pic)
		{
			if (compressed)
			{
				e.data = pic;
				e.dataSize = size;
			}
			else
			{
				e.surf = IMG_Load_RW( SDL_RWFromMem((void*)pic, size), 1);
				delete pic;
			}
		}
	}
	else if (file && e.source & 1)//try to get image from def
	{
		if (compressed)
			e.data = file->getFrame(frame, group);
		else
			e.surf = file->loadFrame(frame, group);
	}

	if (!(e.surf || e.data))
		return false;//failed to load

	e.refCount++;
	return true;
}

bool CAnimation::unloadFrame(size_t frame, size_t group)
{
	if (groupSize(group) > frame && entries[group][frame].refCount)
	{
		AnimEntry &e = entries[group][frame];
		if (--e.refCount)//not last ref
			return true;

		SDL_FreeSurface(e.surf);
		delete e.data;

		e.surf = NULL;
		e.data = NULL;
		return true;
	}
	return false;
}

void CAnimation::decompress(AnimEntry &entry)
{
	if (entry.source & 6)//load frame with SDL_Image
		entry.surf = IMG_Load_RW( SDL_RWFromMem((void*)entry.data, entry.dataSize), 1);

	else if (entry.source & 1)
		entry.surf = CDefFile::loadFrame(entry.data, defPalette);
}

void CAnimation::init(CDefFile * file)
{
	if (compressed)
		defPalette = file->getPalette();

	for (size_t group = 0; ; group++)
	{
		std::vector<AnimEntry> toAdd;

		for (size_t frame = 0; ; frame++)
		{
			unsigned char res=0;

			{
				std::ostringstream str;
				str << name << '#' << (group+1) << '#' << (frame+1); // format: file#12#34.*
				if (spriteh->haveFile(str.str()))
					res |= 2;
			}

			if (group == 0)
			{
				std::ostringstream str;
				str << name << '#' << (frame+1);// format: file#34.*
				if ( spriteh->haveFile(str.str()))
					res |=4;
			}

			if (file)//we have def too. try to get image from def
			{
				if (file->haveFrame(frame, group))
					res |=1;
			}

			if (res)
			{
				toAdd.push_back(AnimEntry());
				toAdd.back().source = res;
			}
			else
				break;
		}
		if (!toAdd.empty())
		{
			entries.push_back(toAdd);
			break;
		}
	}
}

CDefFile * CAnimation::getFile() const
{
	CDefFile * file = new CDefFile(name);
	if (!file->loaded())
	{
		delete file;
		return NULL;
	}
	return file;
}

void CAnimation::printError(size_t frame, size_t group, std::string type) const
{
	tlog0 << type <<" error: Request for frame not present in CAnimation!\n"
	      <<"\tFile name: "<<name<<" Group: "<<group<<" Frame: "<<frame<<"\n";
}

CAnimation::CAnimation(std::string Name, bool Compressed):
	name(Name),
	compressed(Compressed),
	defPalette(NULL)
{
	std::transform(name.begin(), name.end(), name.begin(), (int(*)(int))toupper);
	int dotPos = name.find_last_of('.');
	if ( dotPos != -1 )
		name.erase(dotPos);

	CDefFile * file = getFile();
	init(file);
	delete file;
}

CAnimation::CAnimation():
	name(""),
	compressed(false),
	defPalette(NULL)
{

}

CAnimation::~CAnimation()
{
	delete defPalette;

	for (size_t i = 0; i < entries.size(); i++)
		for (size_t j = 0; j < entries.at(i).size(); j++)
		{
			delete entries[i][j].data;
			if (entries[i][j].surf)
				SDL_FreeSurface(entries[i][j].surf);
		}
}

void CAnimation::add(SDL_Surface * surf, bool shared, size_t group)
{
	if (!surf)
		return;

	if (entries.size() <= group)
		entries.resize(group+1);

	if (shared)
		surf->refcount++;

	entries[group].push_back(AnimEntry());
	entries[group].back().refCount = 1;
	entries[group].back().surf = surf;
}

void CAnimation::purgeCompressed()
{
	for (size_t group; group < entries.size(); group++)
		for (size_t frame; frame < entries[group].size(); frame++)
			if (entries[group][frame].surf)
				SDL_FreeSurface(entries[group][frame].surf);
}

SDL_Surface * CAnimation::image(size_t frame)
{
	size_t group=0;
	for (; group<entries.size() && frame > entries[group].size(); group++)
		frame -= entries[group].size();

	return image(frame, group);
}

SDL_Surface * CAnimation::image(size_t frame, size_t group)
{
	if ( groupSize(group) > frame )
	{
		AnimEntry &e = entries[group][frame];
		if (!e.surf && e.data)
			decompress(e);
		return e.surf;
	}

	printError(frame, group, "GetImage");
	return NULL;
}

void CAnimation::clear()
{
	unload();
	entries.clear();
}

void CAnimation::load()
{
	CDefFile * file = getFile();

	for (size_t group = 0; group<entries.size(); group++)
		for (size_t frame = 0; frame<entries[group].size(); frame++)
			loadFrame(file, frame, group);

	delete file;
}

void CAnimation::unload()
{
	for (size_t group = 0; group<entries.size(); group++)
		for (size_t frame = 0; frame<entries[group].size(); frame++)
			unloadFrame(frame, group);
}

void CAnimation::loadGroup(size_t group)
{
	CDefFile * file = getFile();

	for (size_t frame = 0; frame<entries[group].size(); frame++)
		loadFrame(file, frame, group);
}

void CAnimation::unloadGroup(size_t group)
{
	for (size_t frame = 0; frame<entries[group].size(); frame++)
		unloadFrame(frame, group);
}

void CAnimation::load(size_t frame, size_t group)
{
	CDefFile * file = getFile();
	loadFrame(file, frame, group);
	delete file;
}

void CAnimation::unload(size_t frame, size_t group)
{
	unloadFrame(frame, group);
}

void CAnimation::load(std::vector <std::pair <size_t, size_t> > frames)
{
	CDefFile * file = getFile();
	for (size_t i=0; i<frames.size(); i++)
		loadFrame(file, frames[i].second, frames[i].first);
	delete file;
}

void CAnimation::unload(std::vector <std::pair <size_t, size_t> > frames)
{
	for (size_t i=0; i<frames.size(); i++)
		unloadFrame(frames[i].second, frames[i].first);
}

void CAnimation::fixButtonPos()
{
	if ( groupSize(0) > 1 )
		std::swap(entries[0][1].surf, entries[0][0].surf);
}

size_t CAnimation::groupSize(size_t group) const
{
	if (entries.size() > group)
		return entries[group].size();

	return 0;
}

size_t CAnimation::size() const
{
	size_t ret=0;
	for (size_t i=0; i<entries.size(); i++)
	{
		ret += entries[i].size();
	}
	return ret;
}

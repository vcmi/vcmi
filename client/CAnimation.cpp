#include <iostream>
#include <sstream>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>

#include "SDL.h"
#include "SDL_image.h"

#include "CBitmapHandler.h"
#include "CAnimation.h"
#include "SDL_Extensions.h"
#include "../lib/CLodHandler.h"

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

	unsigned int DefFormat, // format in which pixel data of sprite is encoded
	             FullHeight,FullWidth, // full width and height of sprite including borders
	             SpriteWidth, SpriteHeight, // width and height of encoded sprite

	             TotalRowLength,
	             RowAdd, add,
	             BaseOffset;

	int LeftMargin, RightMargin, // position of 1st stored in sprite pixel on surface
	    TopMargin, BottomMargin;

	SSpriteDef sd = * reinterpret_cast<const SSpriteDef *>(FDef);

	DefFormat = SDL_SwapLE32(sd.defType2);
	FullWidth = SDL_SwapLE32(sd.FullWidth);
	FullHeight = SDL_SwapLE32(sd.FullHeight);
	SpriteWidth = SDL_SwapLE32(sd.SpriteWidth);
	SpriteHeight = SDL_SwapLE32(sd.SpriteHeight);
	LeftMargin = SDL_SwapLE32(sd.LeftMargin);
	TopMargin = SDL_SwapLE32(sd.TopMargin);
	RightMargin = FullWidth - SpriteWidth - LeftMargin;
	BottomMargin = FullHeight - SpriteHeight - TopMargin;

	if (LeftMargin<0)
		SpriteWidth+=LeftMargin;
	if (RightMargin<0)
		SpriteWidth+=RightMargin;

	// Note: this looks bogus because we allocate only FullWidth, not FullWidth+add
	add = 4 - FullWidth%4;
	if (add==4)
		add=0;

	ret = SDL_CreateRGBSurface(SDL_SWSURFACE, FullWidth, FullHeight, 8, 0, 0, 0, 0);

	BaseOffset = sizeof(SSpriteDef);
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

	// If there's a margin anywhere, just blank out the whole surface.
	if (TopMargin > 0 || BottomMargin > 0 || LeftMargin > 0 || RightMargin > 0)
	{
		memset( reinterpret_cast<char*>(ret->pixels), 0, FullHeight*FullWidth);
	}

	int current=0;//current pixel on output surface

	// Skip top margin
	if (TopMargin > 0)
		current += TopMargin*(FullWidth+add);

	switch (DefFormat)
	{
	//pixel data is not compressed, copy each line to surface
	case 0:
		{
			for (unsigned int i=0; i<SpriteHeight; i++)
			{
				if (LeftMargin>0)
					current += LeftMargin;

				memcpy(reinterpret_cast<char*>(ret->pixels)+current, &FDef[BaseOffset], SpriteWidth);
				current += SpriteWidth;
				BaseOffset += SpriteWidth;

				if (RightMargin>0)
					current += RightMargin;
			}
		}
		break;

	// RLE encoding:
	// read offset of pixel data of each line
	// for each line
	//     read type and size
	//     if type is 0xff
	//         no encoding, copy to output
	//     else
	//        RLE: set size pixels to type
	//   do this until all line is parsed
	case 1:
		{
			//for each line we have offset of pixel data
			const unsigned int * RWEntriesLoc = reinterpret_cast<const unsigned int *>(FDef+BaseOffset);
			BaseOffset += sizeof(int) * SpriteHeight;

			for (unsigned int i=0; i<SpriteHeight; i++)
			{
				BaseOffset=BaseOffsetor + SDL_SwapLE32(read_unaligned_u32(RWEntriesLoc + i));
				if (LeftMargin>0)
					current += LeftMargin;

				TotalRowLength=0;
				do
				{
					unsigned char SegmentType=FDef[BaseOffset++];
					unsigned int  SegmentLength=FDef[BaseOffset++] + 1;

					if (SegmentType==0xFF)
					{
						memcpy(reinterpret_cast<char*>(ret->pixels)+current, FDef + BaseOffset, SegmentLength);
						BaseOffset+=SegmentLength;
					}
					else
					{
						memset(reinterpret_cast<char*>(ret->pixels)+current, SegmentType, SegmentLength);
					}
					current += SegmentLength;
					TotalRowLength += SegmentLength;
				}
				while (TotalRowLength<SpriteWidth);

				RowAdd=SpriteWidth-TotalRowLength;

				if (RightMargin>0)
					current += RightMargin;

				if (add>0)
					current += add+RowAdd;
			}
		}
		break;

	// Something like RLE
	// read base offset
	// for each line
	//     read type, set code and value
	//     if code is 7
	//       copy value pixels
	//     else
	//       set value pixels to code
	case 2:
		{
			BaseOffset = BaseOffsetor + SDL_SwapLE16(read_unaligned_u16(FDef + BaseOffsetor));

			for (unsigned int i=0; i<SpriteHeight; i++)
			{
				if (LeftMargin>0)
					current += LeftMargin;

				TotalRowLength=0;

				do
				{
					unsigned char SegmentType=FDef[BaseOffset++];
					unsigned char code = SegmentType / 32;
					unsigned char value = (SegmentType & 31) + 1;
					if (code==7)
					{
						memcpy(reinterpret_cast<char*>(ret->pixels)+current, &FDef[BaseOffset], value);
						current += value;
						BaseOffset += value;
					}
					else
					{
						memset(reinterpret_cast<char*>(ret->pixels)+current, code, value);
						current += value;
					}
					TotalRowLength+=value;
				}
				while (TotalRowLength<SpriteWidth);

				if (RightMargin>0)
					current += RightMargin;

				RowAdd=SpriteWidth-TotalRowLength;

				if (add>0)
					current += add+RowAdd;
			}
		}
		break;

	//combo of 1st and 2nd:
	// offset for each line
	// code and value combined in 1 byte
	case 3:
		{
			for (unsigned int i=0; i<SpriteHeight; i++)
			{
				BaseOffset = BaseOffsetor + SDL_SwapLE16(read_unaligned_u16(FDef + BaseOffsetor+i*2*(SpriteWidth/32)));
				if (LeftMargin>0)
					current += LeftMargin;

				TotalRowLength=0;

				do
				{
					unsigned char SegmentType=FDef[BaseOffset++];
					unsigned char code = SegmentType / 32;
					unsigned char value = (SegmentType & 31) + 1;

					int len = std::min<unsigned int>(value, SpriteWidth - TotalRowLength) - std::max(0, -LeftMargin);
					amax(len, 0);

					if (code==7)
					{
						memcpy((ui8*)ret->pixels + current, FDef + BaseOffset, len);
						current += len;
						BaseOffset += len;
					}
					else
					{
						memset((ui8*)ret->pixels + current, code, len);
						current += len;
					}
					TotalRowLength+=( LeftMargin>=0 ? value : value+LeftMargin );
				}
				while (TotalRowLength<SpriteWidth);

				if (RightMargin>0)
					current += RightMargin;

				RowAdd=SpriteWidth-TotalRowLength;

				if (add>0)
					current += add+RowAdd;
			}
		}
		break;

	default:
		throw std::string("Unknown sprite format.");
		break;
	}

	SDL_Color *col = ret->format->palette->colors;
	
	Uint32 keycol = SDL_MapRGBA(ret->format, col[0].r, col[0].b, col[0].g, col[0].unused);
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
	//First 8 colors in def palette used for transparency
	static SDL_Color H3Palette[8] = {
	{  0,   0,   0, 255},// 100% - transparency
	{  0,   0,   0, 192},//  75% - shadow border, 
	{  0,   0,   0, 128},// TODO: find exact value
	{  0,   0,   0, 128},// TODO: for transparency
	{  0,   0,   0, 128},//  50% - shadow body
	{  0,   0,   0, 255},// 100% - selection highlight
	{  0,   0,   0, 128},//  50% - shadow body   below selection
	{  0,   0,   0, 192}};// 75% - shadow border below selection
	
	data = spriteh->giveFile(Name, FILE_ANIMATION, &datasize);
	if (!data)
	{
		tlog0<<"Error: file "<< Name <<" not found\n";
		return;
	}

	colors = new BMPPalette[256];
	int it = 0;

	type = readNormalNr(data, it); it+=4;
	//int width  = readNormalNr(data, it); it+=4;//not used
	//int height = readNormalNr(data, it); it+=4;
	it+=8;
	unsigned int totalBlocks = readNormalNr(data, it);
	it+=4;

	for (unsigned int i= 0; i<256; i++)
	{
		colors[i].R = data[it++];
		colors[i].G = data[it++];
		colors[i].B = data[it++];
		colors[i].F = 0;
	}
	memcpy(colors, H3Palette, (type == 66)? 32:20);//initialize shadow\selection colors

	offList.insert(datasize);

	for (unsigned int i=0; i<totalBlocks; i++)
	{
		size_t blockID = readNormalNr(data, it);
		it+=4;
		size_t totalEntries = readNormalNr(data, it);
		it+=12;
		//8 unknown bytes - skipping

		//13 bytes for name of every frame in this block - not used, skipping
		it+= 13 * totalEntries;

		offset.resize(std::max(blockID+1, offset.size()));

		for (unsigned int j=0; j<totalEntries; j++)
		{
			size_t currOffset = readNormalNr(data, it);
			offset[blockID].push_back(currOffset);
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
	offset.clear();
	offList.clear();
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
	{
		printError(frame, group, "LoadFrame");
		return false;
	}
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
				delete [] pic;
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
		delete [] e.data;

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
		}
		else
		{
			entries.resize(entries.size()+1);
			if (group > 21)
				break;//FIXME: crude workaround: if some groups are not present
				      //(common for creatures) parser will exit before reaching them
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
	delete [] defPalette;
	for (size_t i = 0; i < entries.size(); i++)
		for (size_t j = 0; j < entries.at(i).size(); j++)
		{
			delete [] entries[i][j].data;
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

void CAnimation::removeDecompressed(size_t frame, size_t group)
{
	AnimEntry &e = entries[group][frame];
	if (e.surf && e.data)
	{
		SDL_FreeSurface(e.surf);
		e.surf = NULL;
	}
}

SDL_Surface * CAnimation::image(size_t frame)
{
	size_t group=0;
	while (group<entries.size() && frame > entries[group].size())
		frame -= entries[group].size();

	if (group <entries.size() && frame < entries[group].size())
		return image(frame, group);
	return NULL;
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

	for (size_t frame = 0; frame<groupSize(group); frame++)
		loadFrame(file, frame, group);
	delete file;
}

void CAnimation::unloadGroup(size_t group)
{
	for (size_t frame = 0; frame<groupSize(group); frame++)
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
/*
CAnimImage::CAnimImage(int x, int y, std::string name, size_t Frame, size_t Group):
	anim(name),
	frame(Frame),
	group(Group)
{
	anim.load(frame, group);
	pos.w = anim.image(frame, group)->w;
	pos.h = anim.image(frame, group)->h;
}

CAnimImage::~CAnimImage()
{

}

void CAnimImage::show(SDL_Surface *to)
{
	blitAtLoc(anim.image(frame, group), 0,0, to);
}

void CAnimImage::setFrame(size_t Frame, size_t Group)
{
	if (frame == Frame && group==Group)
		return;
	if (anim.groupSize(Group) > Frame)
	{
		anim.unload(frame, group);
		anim.load(Frame, Group);
		frame = Frame;
		group = Group;
	}
}
*/
CShowableAnim::CShowableAnim(int x, int y, std::string name, unsigned char Flags, unsigned int Delay, size_t Group):
	anim(name, Flags),
	group(Group),
	frame(0),
	first(0),
	flags(Flags),
	frameDelay(Delay),
	value(0),
	xOffset(0),
	yOffset(0)
{
	pos.x+=x;
	pos.y+=y;

	anim.loadGroup(group);
	last = anim.groupSize(group);
	pos.w = anim.image(0, group)->w;
	pos.h = anim.image(0, group)->h;
}

CShowableAnim::~CShowableAnim()
{

}

bool CShowableAnim::set(size_t Group, size_t from, size_t to)
{
	size_t max = anim.groupSize(Group);

	if (max>to)
		max = to;

	if (max < from || max == 0)
		return false;

	anim.load(Group);
	anim.unload(group);
	group = Group;
	frame = first = from;
	last = max;
	value = 0;
	return true;
}

bool CShowableAnim::set(size_t Group)
{
	if (anim.groupSize(Group)== 0)
		return false;
	if (group != Group)
	{
		anim.loadGroup(Group);
		anim.unloadGroup(group);
		first = 0;
		group = Group;
		last = anim.groupSize(Group);
	}
	frame = value = 0;
	return true;
}

void CShowableAnim::reset()
{
	value = 0;
	frame = first;
	if (callback)
		callback();
}

void CShowableAnim::movePic( int byX, int byY)
{
	xOffset += byX;
	yOffset += byY;
}

void CShowableAnim::show(SDL_Surface *to)
{
	if ( flags & FLAG_BASE && frame != first)
		blitImage(anim.image(first, group), to);
	blitImage(anim.image(frame, group), to);
	
	if ( ++value == frameDelay )
	{
		value = 0;
		if (flags & FLAG_COMPRESSED)
			anim.removeDecompressed(frame, group);
		if ( ++frame == last)
			reset();
	}

}

void CShowableAnim::showAll(SDL_Surface *to)
{
	show(to);
}

void CShowableAnim::blitImage(SDL_Surface *what, SDL_Surface *to)
{
	assert(what);
	//TODO: SDL RLE?
	SDL_Rect dstRect=genRect(pos.h, pos.w, pos.x, pos.y);
	SDL_Rect srcRect;
	
	srcRect.x = xOffset;
	srcRect.y = yOffset;
	srcRect.w = pos.w;
	srcRect.h = pos.h;
	
	/*if ( flags & FLAG_ROTATED )
	{} //TODO: rotate surface
	else */
	if (flags & FLAG_ALPHA && what->format->BytesPerPixel == 1) //alpha on 8-bit surf - use custom blitter
		CSDL_Ext::blit8bppAlphaTo24bpp(what, &srcRect, to, &dstRect);
	else
		CSDL_Ext::blitSurface(what, &srcRect, to, &dstRect);
}

void CShowableAnim::rotate(bool on)
{
	if (on)
		flags |= FLAG_ROTATED;
	else
		flags &= ~FLAG_ROTATED;
}

CCreatureAnim::CCreatureAnim(int x, int y, std::string name, unsigned char flags, EAnimType type):
	CShowableAnim(x,y,name,flags,3,type)
{
	if (flags & FLAG_PREVIEW)
		callback = boost::bind(&CCreatureAnim::loopPreview,this);
};

void CCreatureAnim::loopPreview()
{
	std::vector<EAnimType> available;
	if (anim.groupSize(ANIM_HOLDING))
		available.push_back(ANIM_HOLDING);

	if (anim.groupSize(ANIM_HITTED))
		available.push_back(ANIM_HITTED);

	if (anim.groupSize(ANIM_DEFENCE))
		available.push_back(ANIM_DEFENCE);

	if (anim.groupSize(ANIM_ATTACK_FRONT))
		available.push_back(ANIM_ATTACK_FRONT);

	if (anim.groupSize(ANIM_CAST_FRONT))
		available.push_back(ANIM_CAST_FRONT);

	size_t rnd = rand()%(available.size()*2);

	if (rnd >= available.size())
	{
		if ( anim.groupSize(ANIM_MOVING) == 0 )//no moving animation present
			addLast( ANIM_HOLDING );
		else
			addLast( ANIM_MOVING ) ;
	}
	else
		addLast(available[rnd]);
}

void CCreatureAnim::addLast(EAnimType newType)
{
	if (type != ANIM_MOVING && newType == ANIM_MOVING)//starting moving - play init sequence
	{
		queue.push( ANIM_MOVE_START );
	}
	else if (type == ANIM_MOVING && newType != ANIM_MOVING )//previous anim was moving - finish it
	{
		queue.push( ANIM_MOVE_END);
	}
	if (newType == ANIM_TURN_L || newType == ANIM_TURN_R)
		queue.push(newType);

	queue.push(newType);
}

void CCreatureAnim::reset()
{
	//if we are in the middle of rotation - set flag
	if (type == ANIM_TURN_L && !queue.empty() && queue.front() == ANIM_TURN_L)
		flags |= FLAG_ROTATED;
	if (type == ANIM_TURN_R && !queue.empty() && queue.front() == ANIM_TURN_R)
		flags &= ~FLAG_ROTATED;

	while (!queue.empty())//FIXME: remove dublication
	{
		EAnimType at = queue.front();
		queue.pop();
		if (set(at))
			return;
	}
	if  (callback)
		callback();
	while (!queue.empty())
	{
		EAnimType at = queue.front();
		queue.pop();
		if (set(at))
			return;
	}
	tlog0<<"Warning: next sequence is not found for animation!\n";
}

void CCreatureAnim::clearAndSet(EAnimType type)
{
	while (!queue.empty())
		queue.pop();
	set(type);
}

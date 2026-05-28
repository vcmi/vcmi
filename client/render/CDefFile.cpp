/*
 * CDefFile.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CDefFile.h"

#include "IImageLoader.h"

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/Point.h"

#include <SDL_pixels.h>

enum class DefType : uint32_t
{
	SPELL = 0x40,
	SPRITE = 0x41,
	CREATURE = 0x42,
	MAP = 0x43,
	MAP_HERO = 0x44,
	TERRAIN = 0x45,
	CURSOR = 0x46,
	INTERFACE = 0x47,
	SPRITE_FRAME = 0x48,
	BATTLE_HERO = 0x49
};

/*************************************************************************
 *  DefFile, class used for def loading                                  *
 *************************************************************************/

CDefFile::CDefFile(const AnimationPath & Name):
	data(nullptr),
	palette(nullptr)
{
	data = CResourceHandler::get()->load(Name)->readAll().first;

	palette = std::unique_ptr<SDL_Color[]>(new SDL_Color[256]);
	int it = 0;

	//ui32 type = read_le_u32(data.get() + it);
	it+=4;
	//int width  = read_le_u32(data + it); it+=4;//not used
	//int height = read_le_u32(data + it); it+=4;
	it+=8;
	ui32 totalBlocks = read_le_u32(data.get() + it);
	it+=4;

	for (ui32 i= 0; i<256; i++)
	{
		palette[i].r = data[it++];
		palette[i].g = data[it++];
		palette[i].b = data[it++];
		palette[i].a = SDL_ALPHA_OPAQUE;
	}

	for (ui32 i=0; i<totalBlocks; i++)
	{
		size_t blockID = read_le_u32(data.get() + it);
		it+=4;
		size_t totalEntries = read_le_u32(data.get() + it);
		it+=12;
		//8 unknown bytes - skipping

		std::vector<std::string> names;
		names.reserve(totalEntries);
		for (ui32 j = 0; j < totalEntries; j++)
		{
			std::string n(reinterpret_cast<const char*>(data.get() + it), 13);
			if (auto pos = n.find('\0'); pos != std::string::npos)
				n.erase(pos);
			names.push_back(std::move(n));
			it += 13;
		}
		name[blockID] = std::move(names);

		for (ui32 j=0; j<totalEntries; j++)
		{
			size_t currOffset = read_le_u32(data.get() + it);
			offset[blockID].push_back(currOffset);
			it += 4;
		}
	}
}

void CDefFile::loadFrame(size_t frame, size_t group, IImageLoader &loader) const
{
	assert(hasFrame(frame, group));		// hasFrame() should be called before calling loadFrame()

	const ui8 * FDef = data.get() + offset.at(group)[frame];

	SSpriteDef sprite = getFrameInfo(frame, group);

	ui32 currentOffset = sizeof(SSpriteDef);

	//special case for some "old" format defs (SGTWMTA.DEF and SGTWMTB.DEF)

	if(sprite.format == 1 && sprite.width > sprite.fullWidth && sprite.height > sprite.fullHeight)
	{
		sprite.leftMargin = 0;
		sprite.topMargin = 0;
		sprite.width = sprite.fullWidth;
		sprite.height = sprite.fullHeight;

		currentOffset -= 16;
	}

	const ui32 BaseOffset = currentOffset;

	loader.init(Point(sprite.width, sprite.height),
				Point(sprite.leftMargin, sprite.topMargin),
				Point(sprite.fullWidth, sprite.fullHeight), palette.get());

	switch(sprite.format)
	{
	case 0:
		{
			//pixel data is not compressed, copy data to surface
			for(ui32 i=0; i<sprite.height; i++)
			{
				loader.load(sprite.width, FDef + currentOffset);
				currentOffset += sprite.width;
				loader.endLine();
			}
			break;
		}
	case 1:
		{
			//for each line we have offset of pixel data
			const ui32 * RWEntriesLoc = reinterpret_cast<const ui32 *>(FDef+currentOffset);
			currentOffset += sizeof(ui32) * sprite.height;

			for(ui32 i=0; i<sprite.height; i++)
			{
				//get position of the line
				currentOffset=BaseOffset + read_le_u32(RWEntriesLoc + i);
				ui32 TotalRowLength = 0;

				while(TotalRowLength<sprite.width)
				{
					ui8 segmentType = FDef[currentOffset++];
					ui32 length = FDef[currentOffset++] + 1;

					if(segmentType==0xFF)//Raw data
					{
						loader.load(length, FDef + currentOffset);
						currentOffset+=length;
					}
					else// RLE
					{
						loader.load(length, segmentType);
					}
					TotalRowLength += length;
				}

				loader.endLine();
			}
			break;
		}
	case 2:
		{
			currentOffset = BaseOffset + read_le_u16(FDef + BaseOffset);

			for(ui32 i=0; i<sprite.height; i++)
			{
				ui32 TotalRowLength=0;

				while(TotalRowLength<sprite.width)
				{
					ui8 segment=FDef[currentOffset++];
					ui8 code = segment / 32;
					ui8 length = (segment & 31) + 1;

					if(code==7)//Raw data
					{
						loader.load(length, FDef + currentOffset);
						currentOffset += length;
					}
					else//RLE
					{
						loader.load(length, code);
					}
					TotalRowLength+=length;
				}
				loader.endLine();
			}
			break;
		}
	case 3:
		{
			for(ui32 i=0; i<sprite.height; i++)
			{
				currentOffset = BaseOffset + read_le_u16(FDef + BaseOffset+i*2*(sprite.width/32));
				ui32 TotalRowLength=0;

				while(TotalRowLength<sprite.width)
				{
					ui8 segment = FDef[currentOffset++];
					ui8 code = segment / 32;
					ui8 length = (segment & 31) + 1;

					if(code==7)//Raw data
					{
						loader.load(length, FDef + currentOffset);
						currentOffset += length;
					}
					else//RLE
					{
						loader.load(length, code);
					}
					TotalRowLength += length;
				}
				loader.endLine();
			}
			break;
		}
	default:
	logGlobal->error("Error: unsupported format of def file: %d", sprite.format);
		break;
	}
}

bool CDefFile::hasFrame(size_t frame, size_t group) const
{
	std::map<size_t, std::vector <size_t> >::const_iterator it;
	it = offset.find(group);
	if(it == offset.end())
	{
		return false;
	}

	if(frame >= it->second.size())
	{
		return false;
	}

	return true;
}

std::string CDefFile::getName(size_t frame, size_t group) const
{
	std::map<size_t, std::vector <std::string> >::const_iterator it;
	it = name.find(group);
	if(it == name.end())
	{
		return "";
	}

	if(frame >= it->second.size())
	{
		return "";
	}

	return name.at(group)[frame];
}

CDefFile::SSpriteDef CDefFile::getFrameInfo(size_t frame, size_t group) const
{
	if(!hasFrame(frame, group))
		return SSpriteDef();

	const ui8 * FDef = data.get() + offset.at(group)[frame];
	const SSpriteDef sd = *reinterpret_cast<const SSpriteDef *>(FDef);

	SSpriteDef sprite;

	sprite.format = read_le_u32(&sd.format);
	sprite.fullWidth = read_le_u32(&sd.fullWidth);
	sprite.fullHeight = read_le_u32(&sd.fullHeight);
	sprite.width = read_le_u32(&sd.width);
	sprite.height = read_le_u32(&sd.height);
	sprite.leftMargin = read_le_u32(&sd.leftMargin);
	sprite.topMargin = read_le_u32(&sd.topMargin);

	return sprite;
}

CDefFile::~CDefFile() = default;

const std::map<size_t, size_t > CDefFile::getEntries() const
{
	std::map<size_t, size_t > ret;

	for (auto & elem : offset)
		ret[elem.first] =  elem.second.size();
	return ret;
}


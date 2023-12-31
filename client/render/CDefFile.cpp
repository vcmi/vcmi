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

// Extremely simple file cache. TODO: smarter, more general solution
class CFileCache
{
	static const int cacheSize = 50; //Max number of cached files
	struct FileData
	{
		AnimationPath             name;
		size_t                 size;
		std::unique_ptr<ui8[]> data;

		std::unique_ptr<ui8[]> getCopy()
		{
			auto ret = std::unique_ptr<ui8[]>(new ui8[size]);
			std::copy(data.get(), data.get() + size, ret.get());
			return ret;
		}
		FileData(AnimationPath name_, size_t size_, std::unique_ptr<ui8[]> data_):
			name{std::move(name_)},
			size{size_},
			data{std::move(data_)}
		{}
	};

	std::deque<FileData> cache;
public:
	std::unique_ptr<ui8[]> getCachedFile(AnimationPath rid)
	{
		for(auto & file : cache)
		{
			if (file.name == rid)
				return file.getCopy();
		}
		// Still here? Cache miss
		if (cache.size() > cacheSize)
			cache.pop_front();

		auto data =  CResourceHandler::get()->load(rid)->readAll();

		cache.emplace_back(std::move(rid), data.second, std::move(data.first));

		return cache.back().getCopy();
	}
};

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

static CFileCache animationCache;

/*************************************************************************
 *  DefFile, class used for def loading                                  *
 *************************************************************************/

static bool colorsSimilar (const SDL_Color & lhs, const SDL_Color & rhs)
{
	// it seems that H3 does not requires exact match to replace colors -> (255, 103, 255) gets interpreted as shadow
	// exact logic is not clear and requires extensive testing with image editing
	// potential reason is that H3 uses 16-bit color format (565 RGB bits), meaning that 3 least significant bits are lost in red and blue component
	static const int threshold = 8;

	int diffR = static_cast<int>(lhs.r) - rhs.r;
	int diffG = static_cast<int>(lhs.g) - rhs.g;
	int diffB = static_cast<int>(lhs.b) - rhs.b;
	int diffA = static_cast<int>(lhs.a) - rhs.a;

	return std::abs(diffR) < threshold && std::abs(diffG) < threshold && std::abs(diffB) < threshold && std::abs(diffA) < threshold;
}

CDefFile::CDefFile(const AnimationPath & Name):
	data(nullptr),
	palette(nullptr)
{
	//First 8 colors in def palette used for transparency
	static const SDL_Color sourcePalette[8] = {
		{0,   255, 255, SDL_ALPHA_OPAQUE},
		{255, 150, 255, SDL_ALPHA_OPAQUE},
		{255, 100, 255, SDL_ALPHA_OPAQUE},
		{255, 50,  255, SDL_ALPHA_OPAQUE},
		{255, 0,   255, SDL_ALPHA_OPAQUE},
		{255, 255, 0,   SDL_ALPHA_OPAQUE},
		{180, 0,   255, SDL_ALPHA_OPAQUE},
		{0,   255, 0,   SDL_ALPHA_OPAQUE}
	};

	static const SDL_Color targetPalette[8] = {
		{0, 0, 0, 0  }, // transparency                  ( used in most images )
		{0, 0, 0, 64 }, // shadow border                 ( used in battle, adventure map def's )
		{0, 0, 0, 64 }, // shadow border                 ( used in fog-of-war def's )
		{0, 0, 0, 128}, // shadow body                   ( used in fog-of-war def's )
		{0, 0, 0, 128}, // shadow body                   ( used in battle, adventure map def's )
		{0, 0, 0, 0  }, // selection / owner flag        ( used in battle, adventure map def's )
		{0, 0, 0, 128}, // shadow body   below selection ( used in battle def's )
		{0, 0, 0, 64 }  // shadow border below selection ( used in battle def's )
	};

	data = animationCache.getCachedFile(Name);

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

	// these colors seems to be used unconditionally
	palette[0] = targetPalette[0];
	palette[1] = targetPalette[1];
	palette[4] = targetPalette[4];

	// rest of special colors are used only if their RGB values are close to H3
	for (uint32_t i = 0; i < 8; ++i)
	{
		if (colorsSimilar(sourcePalette[i], palette[i]))
			palette[i] = targetPalette[i];
	}

	for (ui32 i=0; i<totalBlocks; i++)
	{
		size_t blockID = read_le_u32(data.get() + it);
		it+=4;
		size_t totalEntries = read_le_u32(data.get() + it);
		it+=12;
		//8 unknown bytes - skipping

		//13 bytes for name of every frame in this block - not used, skipping
		it+= 13 * (int)totalEntries;

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
	std::map<size_t, std::vector <size_t> >::const_iterator it;
	it = offset.find(group);
	assert (it != offset.end());

	const ui8 * FDef = data.get()+it->second[frame];

	const SSpriteDef sd = * reinterpret_cast<const SSpriteDef *>(FDef);
	SSpriteDef sprite;

	sprite.format = read_le_u32(&sd.format);
	sprite.fullWidth = read_le_u32(&sd.fullWidth);
	sprite.fullHeight = read_le_u32(&sd.fullHeight);
	sprite.width = read_le_u32(&sd.width);
	sprite.height = read_le_u32(&sd.height);
	sprite.leftMargin = read_le_u32(&sd.leftMargin);
	sprite.topMargin = read_le_u32(&sd.topMargin);

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

CDefFile::~CDefFile() = default;

const std::map<size_t, size_t > CDefFile::getEntries() const
{
	std::map<size_t, size_t > ret;

	for (auto & elem : offset)
		ret[elem.first] =  elem.second.size();
	return ret;
}


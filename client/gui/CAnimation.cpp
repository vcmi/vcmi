#include "StdInc.h"
#include "CAnimation.h"

#include <SDL_image.h>

#include "../CBitmapHandler.h"
#include "../Graphics.h"
#include "../gui/SDL_Extensions.h"
#include "../gui/SDL_Pixels.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/ISimpleResourceLoader.h"
#include "../lib/JsonNode.h"
#include "../lib/CRandomGenerator.h"

/*
 * CAnimation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

typedef std::map <size_t, std::vector <JsonNode> > source_map;
typedef std::map<size_t, IImage* > image_map;
typedef std::map<size_t, image_map > group_map;

class SDLImageLoader
{
	SDLImage * image;
	ui8 * lineStart;
	ui8 * position;
public:
	//load size raw pixels from data
	inline void Load(size_t size, const ui8 * data);
	//set size pixels to color
	inline void Load(size_t size, ui8 color=0);
	inline void EndLine();
	//init image with these sizes and palette
	inline void init(Point SpriteSize, Point Margins, Point FullSize, SDL_Color *pal);

	SDLImageLoader(SDLImage * Img);
	~SDLImageLoader();
};

class CompImageLoader
{
	CompImage * image;
	ui8 *position;
	ui8 *entry;
	ui32 currentLine;

	inline ui8 typeOf(ui8 color);
	inline void NewEntry(ui8 color, size_t size);
	inline void NewEntry(const ui8 * &data, size_t size);

public:
	//load size raw pixels from data
	inline void Load(size_t size, const ui8 * data);
	//set size pixels to color
	inline void Load(size_t size, ui8 color=0);
	inline void EndLine();
	//init image with these sizes and palette
	inline void init(Point SpriteSize, Point Margins, Point FullSize, SDL_Color *pal);

	CompImageLoader(CompImage * Img);
	~CompImageLoader();
};

// Extremely simple file cache. TODO: smarter, more general solution
class CFileCache
{
	static const int cacheSize = 50; //Max number of cached files
	struct FileData
	{
		ResourceID             name;
		size_t                 size;
		std::unique_ptr<ui8[]> data;

		std::unique_ptr<ui8[]> getCopy()
		{
			auto ret = std::unique_ptr<ui8[]>(new ui8[size]);
			std::copy(data.get(), data.get() + size, ret.get());
			return ret;
		}
		FileData(ResourceID name_, size_t size_, std::unique_ptr<ui8[]> data_):
			name{std::move(name_)},
			size{size_},
			data{std::move(data_)}
		{}
	};

	std::deque<FileData> cache;
public:
	std::unique_ptr<ui8[]> getCachedFile(ResourceID rid)
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

static CFileCache animationCache;

/*************************************************************************
 *  DefFile, class used for def loading                                  *
 *************************************************************************/

CDefFile::CDefFile(std::string Name):
	data(nullptr),
	palette(nullptr)
{
	//First 8 colors in def palette used for transparency
	static SDL_Color H3Palette[8] =
	{
		{   0,   0,   0,   0},// 100% - transparency
		{   0,   0,   0,  64},//  75% - shadow border,
		{   0,   0,   0, 128},// TODO: find exact value
		{   0,   0,   0, 128},// TODO: for transparency
		{   0,   0,   0, 128},//  50% - shadow body
		{   0,   0,   0,   0},// 100% - selection highlight
		{   0,   0,   0, 128},//  50% - shadow body   below selection
		{   0,   0,   0,  64} // 75% - shadow border below selection
	};
	data = animationCache.getCachedFile(ResourceID(std::string("SPRITES/") + Name, EResType::ANIMATION));

	palette = std::unique_ptr<SDL_Color[]>(new SDL_Color[256]);
	int it = 0;

	ui32 type = read_le_u32(data.get() + it);
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
	if (type == 71 || type == 64)//Buttons/buildings don't have shadows\semi-transparency
		memset(palette.get(), 0, sizeof(SDL_Color)*2);
	else
		memcpy(palette.get(), H3Palette, sizeof(SDL_Color)*8);//initialize shadow\selection colors

	for (ui32 i=0; i<totalBlocks; i++)
	{
		size_t blockID = read_le_u32(data.get() + it);
		it+=4;
		size_t totalEntries = read_le_u32(data.get() + it);
		it+=12;
		//8 unknown bytes - skipping

		//13 bytes for name of every frame in this block - not used, skipping
		it+= 13 * totalEntries;

		for (ui32 j=0; j<totalEntries; j++)
		{
			size_t currOffset = read_le_u32(data.get() + it);
			offset[blockID].push_back(currOffset);
			it += 4;
		}
	}
}

template<class ImageLoader>
void CDefFile::loadFrame(size_t frame, size_t group, ImageLoader &loader) const
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
	ui32 BaseOffset =    sizeof(SSpriteDef);

	loader.init(Point(sprite.width, sprite.height),
	            Point(sprite.leftMargin, sprite.topMargin),
	            Point(sprite.fullWidth, sprite.fullHeight), palette.get());

	switch (sprite.format)
	{
	case 0:
		{
			//pixel data is not compressed, copy data to surface
			for (ui32 i=0; i<sprite.height; i++)
			{
				loader.Load(sprite.width, FDef[currentOffset]);
				currentOffset += sprite.width;
				loader.EndLine();
			}
			break;
		}
	case 1:
		{
			//for each line we have offset of pixel data
			const ui32 * RWEntriesLoc = reinterpret_cast<const ui32 *>(FDef+currentOffset);
			currentOffset += sizeof(ui32) * sprite.height;

			for (ui32 i=0; i<sprite.height; i++)
			{
				//get position of the line
				currentOffset=BaseOffset + read_le_u32(RWEntriesLoc + i);
				ui32 TotalRowLength = 0;

				while (TotalRowLength<sprite.width)
				{
					ui8 type=FDef[currentOffset++];
					ui32 length=FDef[currentOffset++] + 1;

					if (type==0xFF)//Raw data
					{
						loader.Load(length, FDef + currentOffset);
						currentOffset+=length;
					}
					else// RLE
					{
						loader.Load(length, type);
					}
					TotalRowLength += length;
				}

				loader.EndLine();
			}
			break;
		}
	case 2:
		{
			currentOffset = BaseOffset + read_le_u16(FDef + BaseOffset);

			for (ui32 i=0; i<sprite.height; i++)
			{
				ui32 TotalRowLength=0;

				while (TotalRowLength<sprite.width)
				{
					ui8 SegmentType=FDef[currentOffset++];
					ui8 code = SegmentType / 32;
					ui8 length = (SegmentType & 31) + 1;

					if (code==7)//Raw data
					{
						loader.Load(length, FDef[currentOffset]);
						currentOffset += length;
					}
					else//RLE
					{
						loader.Load(length, code);
					}
					TotalRowLength+=length;
				}
				loader.EndLine();
			}
			break;
		}
	case 3:
		{
			for (ui32 i=0; i<sprite.height; i++)
			{
				currentOffset = BaseOffset + read_le_u16(FDef + BaseOffset+i*2*(sprite.width/32));
				ui32 TotalRowLength=0;

				while (TotalRowLength<sprite.width)
				{
					ui8 segment = FDef[currentOffset++];
					ui8 code = segment / 32;
					ui8 length = (segment & 31) + 1;

					if (code==7)//Raw data
					{
						loader.Load(length, FDef + currentOffset);
						currentOffset += length;
					}
					else//RLE
					{
						loader.Load(length, code);
					}
					TotalRowLength += length;
				}
				loader.EndLine();
			}
			break;
		}
	default:
	logGlobal->errorStream()<<"Error: unsupported format of def file: "<<sprite.format;
		break;
	}
};

CDefFile::~CDefFile() = default;

const std::map<size_t, size_t > CDefFile::getEntries() const
{
	std::map<size_t, size_t > ret;

	for (auto & elem : offset)
		ret[elem.first] =  elem.second.size();
	return ret;
}

/*************************************************************************
 *  Classes for image loaders - helpers for loading from def files       *
 *************************************************************************/

SDLImageLoader::SDLImageLoader(SDLImage * Img):
	image(Img),
	lineStart(nullptr),
	position(nullptr)
{
}

void SDLImageLoader::init(Point SpriteSize, Point Margins, Point FullSize, SDL_Color *pal)
{
	//Init image
	image->surf = SDL_CreateRGBSurface(SDL_SWSURFACE, SpriteSize.x, SpriteSize.y, 8, 0, 0, 0, 0);
	image->margins  = Margins;
	image->fullSize = FullSize;

	//Prepare surface
	SDL_Palette * p = SDL_AllocPalette(256);
	SDL_SetPaletteColors(p, pal, 0, 256);
	SDL_SetSurfacePalette(image->surf, p);
	SDL_FreePalette(p);

	SDL_LockSurface(image->surf);
	lineStart = position = (ui8*)image->surf->pixels;
}

inline void SDLImageLoader::Load(size_t size, const ui8 * data)
{
	if (size)
	{
		memcpy((void *)position, data, size);
		position += size;
	}
}

inline void SDLImageLoader::Load(size_t size, ui8 color)
{
	if (size)
	{
		memset((void *)position, color, size);
		position += size;
	}
}

inline void SDLImageLoader::EndLine()
{
	lineStart += image->surf->pitch;
	position = lineStart;
}

SDLImageLoader::~SDLImageLoader()
{
	SDL_UnlockSurface(image->surf);
	SDL_SetColorKey(image->surf, SDL_TRUE, 0);
	//TODO: RLE if compressed and bpp>1
}

////////////////////////////////////////////////////////////////////////////////

CompImageLoader::CompImageLoader(CompImage * Img):
	image(Img),
	position(nullptr),
	entry(nullptr),
	currentLine(0)
{

}

void CompImageLoader::init(Point SpriteSize, Point Margins, Point FullSize, SDL_Color *pal)
{
	image->sprite = Rect(Margins, SpriteSize);
	image->fullSize = FullSize;
	if (SpriteSize.x && SpriteSize.y)
	{
		image->palette = new SDL_Color[256];
		memcpy((void*)image->palette, (void*)pal, 256*sizeof(SDL_Color));
		//Allocate enought space for worst possible case,  c-style malloc used due to resizing after load
		image->surf = (ui8*)malloc(SpriteSize.x*SpriteSize.y*3);
		image->line = new ui32[SpriteSize.y+1];
		image->line[0] = 0;
		position = image->surf;
	}
}

inline void CompImageLoader::NewEntry(ui8 color, size_t size)
{
	assert(color != 0xff);
	assert(size && size<256);
	entry = position;
	entry[0] = color;
	entry[1] = size;
	position +=2;
}

inline void CompImageLoader::NewEntry(const ui8 * &data, size_t size)
{
	assert(size && size<256);
	entry = position;
	entry[0] = 0xff;
	entry[1] = size;
	position +=2;
	memcpy(position, data, size);
	position+=size;
	data+=size;
}

inline ui8 CompImageLoader::typeOf(ui8 color)
{
	if (color == 0)
		return 0;

	if (image->palette[color].a != 255)
		return 1;

	return 2;
}

inline void CompImageLoader::Load(size_t size, const ui8 * data)
{
	while (size)
	{
		//Try to compress data
		while(true)
		{
			ui8 color = data[0];
			if (color != 0xff)
			{
				size_t runLength = 1;
				while (runLength < size && color == data[runLength])
					runLength++;

				if (runLength > 1 && runLength < 255)//Row of one color found - use RLE
				{
					Load(runLength, color);
					data += runLength;
					size -= runLength;
					if (!size)
						return;
				}
				else
					break;
			}
			else
				break;
		}
		//Select length for new raw entry
		size_t runLength = 1;
		ui8 color = data[0];
		ui8 type = typeOf(color);
		ui8 color2;
		ui8 type2;

		if (size > 1)
		{
			do
			{
				color2 = data[runLength];
				type2 = typeOf(color2);
				runLength++;
			}
			//While we have data of this type and different colors
			while ((runLength < size) && (type == type2) && ( (color2 != 0xff) || (color2 != color)));
		}
		size -= runLength;

		//add data to last entry
		if (entry && entry[0] == 0xff && type == typeOf(entry[2]))
		{
			size_t toCopy = std::min<size_t>(runLength, 255 - entry[1]);
			runLength -= toCopy;
			entry[1] += toCopy;
			memcpy(position, data, toCopy);
			data+=toCopy;
			position+=toCopy;
		}
		//Create new entries
		while (runLength > 255)
		{
			NewEntry(data, 255);
			runLength -= 255;
		}
		if (runLength)
			NewEntry(data, runLength);
	}
}

inline void CompImageLoader::Load(size_t size, ui8 color)
{
	if (!size)
		return;
	if (color==0xff)
	{
		auto   tmpbuf = new ui8[size];
		memset((void*)tmpbuf, color, size);
		Load(size, tmpbuf);
		delete [] tmpbuf;
		return;
	}
	//Current entry is RLE with same color as new block
	if (entry && entry[0] == color)
	{
		size_t toCopy = std::min<size_t>(size, 255 - entry[1]);
		size -= toCopy;
		entry[1] += toCopy;
	}
	//Create new entries
	while (size > 255)
	{
		NewEntry(color, 255);
		size -= 255;
	}
	if (size)
		NewEntry(color, size);
}

void CompImageLoader::EndLine()
{
	currentLine++;
	image->line[currentLine] = position - image->surf;
	entry = nullptr;

}

CompImageLoader::~CompImageLoader()
{
	if (!image->surf)
		return;

	ui8* newPtr = (ui8*)realloc((void*)image->surf, position - image->surf);
	if (newPtr)
		image->surf = newPtr;
}

/*************************************************************************
 *  Classes for images, support loading from file and drawing on surface *
 *************************************************************************/

IImage::IImage():
	refCount(1)
{

}

bool IImage::decreaseRef()
{
	refCount--;
	return refCount <= 0;
}

void IImage::increaseRef()
{
	refCount++;
}

SDLImage::SDLImage(CDefFile *data, size_t frame, size_t group, bool compressed):
	surf(nullptr)
{
	SDLImageLoader loader(this);
	data->loadFrame(frame, group, loader);
}

SDLImage::SDLImage(SDL_Surface * from, bool extraRef):
	margins(0,0)
{
	surf = from;
	if (extraRef)
		surf->refcount++;
	fullSize.x = surf->w;
	fullSize.y = surf->h;
}

SDLImage::SDLImage(std::string filename, bool compressed):
	margins(0,0)
{
	surf = BitmapHandler::loadBitmap(filename);

	if (surf == nullptr)
	{
		logGlobal->errorStream() << "Error: failed to load image "<<filename;
		return;
	}
	else
	{
		fullSize.x = surf->w;
		fullSize.y = surf->h;
	}
	if (compressed)
	{
		SDL_Surface *temp = surf;
		// add RLE flag
		if (surf->format->palette)
		{
			CSDL_Ext::setColorKey(temp,temp->format->palette->colors[0]);
		}
		SDL_SetSurfaceRLE(temp, SDL_RLEACCEL);

		// convert surface to enable RLE
		surf = SDL_ConvertSurface(temp, temp->format, temp->flags);
		SDL_FreeSurface(temp);
	}
}

void SDLImage::draw(SDL_Surface *where, int posX, int posY, Rect *src, ui8 rotation) const
{
	if (!surf)
		return;
	Rect sourceRect(margins.x, margins.y, surf->w, surf->h);
	//TODO: rotation and scaling
	if (src)
	{
		sourceRect = sourceRect & *src;
	}
	Rect destRect(posX, posY, surf->w, surf->h);
	destRect += sourceRect.topLeft();
	sourceRect -= margins;
	CSDL_Ext::blitSurface(surf, &sourceRect, where, &destRect);
}

void SDLImage::playerColored(PlayerColor player)
{
	graphics->blueToPlayersAdv(surf, player);
}

int SDLImage::width() const
{
	return fullSize.x;
}

int SDLImage::height() const
{
	return fullSize.y;
}

SDLImage::~SDLImage()
{
	SDL_FreeSurface(surf);
}

CompImage::CompImage(const CDefFile *data, size_t frame, size_t group):
	surf(nullptr),
	line(nullptr),
	palette(nullptr)

{
	CompImageLoader loader(this);
	data->loadFrame(frame, group, loader);
}

CompImage::CompImage(SDL_Surface * surf)
{
	//TODO
	assert(0);
}

void CompImage::draw(SDL_Surface *where, int posX, int posY, Rect *src, ui8 alpha) const
{
	int rotation = 0; //TODO
	//rotation & 2 = horizontal rotation
	//rotation & 4 = vertical rotation
	if (!surf)
		return;
	Rect sourceRect(sprite);
	//TODO: rotation and scaling
	if (src)
		sourceRect = sourceRect & *src;
	//Limit source rect to sizes of surface
	sourceRect = sourceRect & Rect(0, 0, where->w, where->h);

	//Starting point on SDL surface
	Point dest(posX+sourceRect.x, posY+sourceRect.y);
	if (rotation & 2)
		dest.y += sourceRect.h;
	if (rotation & 4)
		dest.x += sourceRect.w;

	sourceRect -= sprite.topLeft();

	for (int currY = 0; currY <sourceRect.h; currY++)
	{
		ui8* data = surf + line[currY+sourceRect.y];
		ui8 type = *(data++);
		ui8 size = *(data++);
		int currX = sourceRect.x;

		//Skip blocks until starting position reached
		while ( currX > size )
		{
			currX -= size;
			if (type == 0xff)
				data += size;
			type = *(data++);
			size = *(data++);
		}
		//This block will be shown partially - calculate size\position
		size -= currX;
		if (type == 0xff)
			data += currX;

		currX = 0;
		ui8 bpp = where->format->BytesPerPixel;

		//Calculate position for blitting: pixels + Y + X
		ui8* blitPos = (ui8*) where->pixels;
		if (rotation & 4)
			blitPos += (dest.y - currY) * where->pitch;
		else
			blitPos += (dest.y + currY) * where->pitch;
		blitPos += dest.x * bpp;

		//Blit blocks that must be fully visible
		while (currX + size < sourceRect.w)
		{
			//blit block, pointers will be modified if needed
			BlitBlockWithBpp(bpp, type, size, data, blitPos, alpha, rotation & 2);

			currX += size;
			type = *(data++);
			size = *(data++);
		}
		//Blit last, semi-visible block
		size = sourceRect.w - currX;
		BlitBlockWithBpp(bpp, type, size, data, blitPos, alpha, rotation & 2);
	}
}

#define CASEBPP(x,y) case x: BlitBlock<x,y>(type, size, data, dest, alpha); break

//FIXME: better way to get blitter
void CompImage::BlitBlockWithBpp(ui8 bpp, ui8 type, ui8 size, ui8 *&data, ui8 *&dest, ui8 alpha, bool rotated) const
{
	assert(bpp>1 && bpp<5);

	if (rotated)
		switch (bpp)
		{
			CASEBPP(2,1);
			CASEBPP(3,1);
			CASEBPP(4,1);
		}
	else
		switch (bpp)
		{
			CASEBPP(2,1);
			CASEBPP(3,1);
			CASEBPP(4,1);
		}
}
#undef CASEBPP

//Blit one block from RLE-d surface
template<int bpp, int dir>
void CompImage::BlitBlock(ui8 type, ui8 size, ui8 *&data, ui8 *&dest, ui8 alpha) const
{
	//Raw data
	if (type == 0xff)
	{
		ui8 color = *data;
		if (alpha != 255)//Per-surface alpha is set
		{
			for (size_t i=0; i<size; i++)
			{
				SDL_Color col = palette[*(data++)];
				col.a = (ui32)col.a*alpha/255;
				ColorPutter<bpp, 1>::PutColorAlpha(dest, col);
			}
			return;
		}

		if (palette[color].a == 255)
		{
			//Put row of RGB data
			for (size_t i=0; i<size; i++)
				ColorPutter<bpp, 1>::PutColor(dest, palette[*(data++)]);
		}
		else
		{
			//Put row of RGBA data
			for (size_t i=0; i<size; i++)
				ColorPutter<bpp, 1>::PutColorAlpha(dest, palette[*(data++)]);

		}
	}
	//RLE-d sequence
	else
	{
		if (alpha != 255 && palette[type].a !=0)//Per-surface alpha is set
		{
			SDL_Color col = palette[type];
			col.a = (int)col.a*(255-alpha)/255;
			for (size_t i=0; i<size; i++)
				ColorPutter<bpp, 1>::PutColorAlpha(dest, col);
			return;
		}

		switch (palette[type].a)
		{
			case 0:
			{
				//Skip row
				dest += size*bpp;
				break;
			}
			case 255:
			{
				//Put RGB row
				ColorPutter<bpp, 1>::PutColorRow(dest, palette[type], size);
				break;
			}
			default:
			{
				//Put RGBA row
				for (size_t i=0; i<size; i++)
					ColorPutter<bpp, 1>::PutColorAlpha(dest, palette[type]);
				break;
			}
		}
	}
}

void CompImage::playerColored(PlayerColor player)
{
	SDL_Color *pal = nullptr;
	if(player < PlayerColor::PLAYER_LIMIT)
	{
		pal = graphics->playerColorPalette + 32*player.getNum();
	}
	else if(player == PlayerColor::NEUTRAL)
	{
		pal = graphics->neutralColorPalette;
	}
	else
		assert(0);

	for(int i=0; i<32; ++i)
	{
		CSDL_Ext::colorAssign(palette[224+i],pal[i]);
	}
}

int CompImage::width() const
{
	return fullSize.x;
}

int CompImage::height() const
{
	return fullSize.y;
}

CompImage::~CompImage()
{
	free(surf);
	delete [] line;
	delete [] palette;
}

/*************************************************************************
 *  CAnimation for animations handling, can load part of file if needed  *
 *************************************************************************/

IImage * CAnimation::getFromExtraDef(std::string filename)
{
	size_t pos = filename.find(':');
	if (pos == -1)
		return nullptr;
	CAnimation anim(filename.substr(0, pos));
	pos++;
	size_t frame = atoi(filename.c_str()+pos);
	size_t group = 0;
	pos = filename.find(':', pos);
	if (pos != -1)
	{
		group = frame;
		frame = atoi(filename.c_str()+pos);
	}
	anim.load(frame ,group);
	IImage * ret = anim.images[group][frame];
	anim.images.clear();
	return ret;
}

bool CAnimation::loadFrame(CDefFile * file, size_t frame, size_t group)
{
	if (size(group) <= frame)
	{
		printError(frame, group, "LoadFrame");
		return false;
	}

	IImage *image = getImage(frame, group, false);
	if (image)
	{
		image->increaseRef();
		return true;
	}

	//try to get image from def
	if (source[group][frame].getType() == JsonNode::DATA_NULL)
	{
		if (file)
		{
			auto frameList = file->getEntries();

			if (vstd::contains(frameList, group) && frameList.at(group) > frame) // frame is present
			{
				if (compressed)
					images[group][frame] = new CompImage(file, frame, group);
				else
					images[group][frame] = new SDLImage(file, frame, group);
				return true;
			}
		}
		// still here? image is missing

		printError(frame, group, "LoadFrame");
		images[group][frame] = new SDLImage("DEFAULT", compressed);
	}
	else //load from separate file
	{
		std::string filename = source[group][frame].Struct().find("file")->second.String();

		IImage * img = getFromExtraDef(filename);
		if (!img)
			img = new SDLImage(filename, compressed);

		images[group][frame] = img;
		return true;
	}
	return false;
}

bool CAnimation::unloadFrame(size_t frame, size_t group)
{
	IImage *image = getImage(frame, group, false);
	if (image)
	{
		//decrease ref count for image and delete if needed
		if (image->decreaseRef())
		{
			delete image;
			images[group].erase(frame);
		}
		if (images[group].empty())
			images.erase(group);
		return true;
	}
	return false;
}

void CAnimation::initFromJson(const JsonNode & config)
{
	std::string basepath;
	basepath = config["basepath"].String();

	for(const JsonNode &group : config["sequences"].Vector())
	{
		size_t groupID = group["group"].Float();//TODO: string-to-value conversion("moving" -> MOVING)
		source[groupID].clear();

		for(const JsonNode &frame : group["frames"].Vector())
		{
			source[groupID].push_back(JsonNode());
			std::string filename =  frame.String();
			source[groupID].back()["file"].String() = basepath + filename;
		}
	}

	for(const JsonNode &node : config["images"].Vector())
	{
		size_t group = node["group"].Float();
		size_t frame = node["frame"].Float();

		if (source[group].size() <= frame)
			source[group].resize(frame+1);

		source[group][frame] = node;
		std::string filename =  node["file"].String();
		source[group][frame]["file"].String() = basepath + filename;
	}
}

void CAnimation::init(CDefFile * file)
{
	if (file)
	{
		const std::map<size_t, size_t> defEntries = file->getEntries();

		for (auto & defEntry : defEntries)
			source[defEntry.first].resize(defEntry.second);
	}

	ResourceID resID(std::string("SPRITES/") + name, EResType::TEXT);

	if (vstd::contains(graphics->imageLists, resID.getName()))
		initFromJson(graphics->imageLists[resID.getName()]);

	auto configList = CResourceHandler::get()->getResourcesWithName(resID);

	for(auto & loader : configList)
	{
		auto stream = loader->load(resID);
		std::unique_ptr<ui8[]> textData(new ui8[stream->getSize()]);
		stream->read(textData.get(), stream->getSize());

		const JsonNode config((char*)textData.get(), stream->getSize());

		initFromJson(config);
	}
}

CDefFile * CAnimation::getFile() const
{
	ResourceID identifier(std::string("SPRITES/") + name, EResType::ANIMATION);

	if (CResourceHandler::get()->existsResource(identifier))
		return new CDefFile(name);
	return nullptr;
}

void CAnimation::printError(size_t frame, size_t group, std::string type) const
{
	logGlobal->errorStream() << type << " error: Request for frame not present in CAnimation! "
		<< "\tFile name: " << name << " Group: " << group << " Frame: " << frame;
}

CAnimation::CAnimation(std::string Name, bool Compressed):
	name(Name),
	compressed(Compressed)
{
	size_t dotPos = name.find_last_of('.');
	if ( dotPos!=-1 )
		name.erase(dotPos);
	std::transform(name.begin(), name.end(), name.begin(), toupper);
	CDefFile * file = getFile();
	init(file);
	delete file;
	loadedAnims.insert(this);
}

CAnimation::CAnimation():
	name(""),
	compressed(false)
{
	init(nullptr);
	loadedAnims.insert(this);
}

CAnimation::~CAnimation()
{
	if (!images.empty())
	{
		logGlobal->warnStream()<<"Warning: not all frames were unloaded from "<<name;
		for (auto & elem : images)
			for (auto & _image : elem.second)
				delete _image.second;
	}
	loadedAnims.erase(this);
}

void CAnimation::setCustom(std::string filename, size_t frame, size_t group)
{
	if (source[group].size() <= frame)
		source[group].resize(frame+1);
	source[group][frame]["file"].String() = filename;
	//FIXME: update image if already loaded
}

IImage * CAnimation::getImage(size_t frame, size_t group, bool verbose) const
{
	auto groupIter = images.find(group);
	if (groupIter != images.end())
	{
		auto imageIter = groupIter->second.find(frame);
		if (imageIter != groupIter->second.end())
			return imageIter->second;
	}
	if (verbose)
		printError(frame, group, "GetImage");
	return nullptr;
}

void CAnimation::load()
{
	CDefFile * file = getFile();

	for (auto & elem : source)
		for (size_t image=0; image < elem.second.size(); image++)
			loadFrame(file, image, elem.first);

	delete file;
}

void CAnimation::unload()
{
	for (auto & elem : source)
		for (size_t image=0; image < elem.second.size(); image++)
			unloadFrame(image, elem.first);

}

void CAnimation::loadGroup(size_t group)
{
	CDefFile * file = getFile();

	if (vstd::contains(source, group))
		for (size_t image=0; image < source[group].size(); image++)
			loadFrame(file, image, group);

	delete file;
}

void CAnimation::unloadGroup(size_t group)
{
	if (vstd::contains(source, group))
		for (size_t image=0; image < source[group].size(); image++)
			unloadFrame(image, group);
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

size_t CAnimation::size(size_t group) const
{
	auto iter = source.find(group);
	if (iter != source.end())
		return iter->second.size();
	return 0;
}

std::set<CAnimation*> CAnimation::loadedAnims;

void CAnimation::getAnimInfo()
{
	logGlobal->errorStream() << "Animation stats: Loaded " << loadedAnims.size() << " total";
	for(auto anim : loadedAnims)
	{
		logGlobal->errorStream() << "Name: " << anim->name << " Groups: " << anim->images.size();
		if(!anim->images.empty())
			logGlobal->errorStream() << ", " << anim->images.begin()->second.size() << " image loaded in group " << anim->images.begin()->first;
	}
}


float CFadeAnimation::initialCounter() const
{
	if (fadingMode == EMode::OUT)
		return 1.0f;
	return 0.0f;
}

void CFadeAnimation::update()
{
	if (!fading)
		return;

	if (fadingMode == EMode::OUT)
		fadingCounter -= delta;
	else
		fadingCounter += delta;

	if (isFinished())
	{
		fading = false;
		if (shouldFreeSurface)
		{
			SDL_FreeSurface(fadingSurface);
			fadingSurface = nullptr;
		}
	}
}

bool CFadeAnimation::isFinished() const
{
	if (fadingMode == EMode::OUT)
		return fadingCounter <= 0.0f;
	return fadingCounter >= 1.0f;
}

CFadeAnimation::CFadeAnimation()
	: fadingSurface(nullptr),
	  fading(false),
	  fadingMode(EMode::NONE)
{
}

CFadeAnimation::~CFadeAnimation()
{
	if (fadingSurface && shouldFreeSurface)
		SDL_FreeSurface(fadingSurface);
}

void CFadeAnimation::init(EMode mode, SDL_Surface * sourceSurface, bool freeSurfaceAtEnd /* = false */, float animDelta /* = DEFAULT_DELTA */)
{
	if (fading)
	{
		// in that case, immediately finish the previous fade
		// (alternatively, we could just return here to ignore the new fade request until this one finished (but we'd need to free the passed bitmap to avoid leaks))
		logGlobal->warnStream() << "Tried to init fading animation that is already running.";
		if (fadingSurface && shouldFreeSurface)
			SDL_FreeSurface(fadingSurface);
	}
	if (animDelta <= 0.0f)
	{
		logGlobal->warnStream() << "Fade anim: delta should be positive; " << animDelta << " given.";
		animDelta = DEFAULT_DELTA;
	}

	if (sourceSurface)
		fadingSurface = sourceSurface;

	delta = animDelta;
	fadingMode = mode;
	fadingCounter = initialCounter();
	fading = true;
	shouldFreeSurface = freeSurfaceAtEnd;
}

void CFadeAnimation::draw(SDL_Surface * targetSurface, const SDL_Rect * sourceRect, SDL_Rect * destRect)
{
	if (!fading || !fadingSurface || fadingMode == EMode::NONE)
	{
		fading = false;
		return;
	}

	CSDL_Ext::setAlpha(fadingSurface, fadingCounter * 255);
	SDL_BlitSurface(fadingSurface, const_cast<SDL_Rect *>(sourceRect), targetSurface, destRect); //FIXME
	CSDL_Ext::setAlpha(fadingSurface, 255);
}

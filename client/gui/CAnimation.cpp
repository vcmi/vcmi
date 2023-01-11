/*
 * CAnimation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CAnimation.h"

#include "SDL_Extensions.h"
#include "SDL_Pixels.h"
#include "ColorFilter.h"

#include "../CBitmapHandler.h"
#include "../Graphics.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/ISimpleResourceLoader.h"
#include "../lib/JsonNode.h"
#include "../lib/CRandomGenerator.h"

class SDLImageLoader;

typedef std::map <size_t, std::vector <JsonNode> > source_map;
typedef std::map<size_t, IImage* > image_map;
typedef std::map<size_t, image_map > group_map;

/// Class for def loading
/// After loading will store general info (palette and frame offsets) and pointer to file itself
class CDefFile
{
private:

	PACKED_STRUCT_BEGIN
	struct SSpriteDef
	{
		ui32 size;
		ui32 format;    /// format in which pixel data is stored
		ui32 fullWidth; /// full width and height of frame, including borders
		ui32 fullHeight;
		ui32 width;     /// width and height of pixel data, borders excluded
		ui32 height;
		si32 leftMargin;
		si32 topMargin;
	} PACKED_STRUCT_END;
	//offset[group][frame] - offset of frame data in file
	std::map<size_t, std::vector <size_t> > offset;

	std::unique_ptr<ui8[]>       data;
	std::unique_ptr<SDL_Color[]> palette;

public:
	CDefFile(std::string Name);
	~CDefFile();

	//load frame as SDL_Surface
	template<class ImageLoader>
	void loadFrame(size_t frame, size_t group, ImageLoader &loader) const;

	const std::map<size_t, size_t> getEntries() const;
};


/*
 * Wrapper around SDL_Surface
 */
class SDLImage : public IImage
{
public:
	
	const static int DEFAULT_PALETTE_COLORS = 256;
	
	//Surface without empty borders
	SDL_Surface * surf;
	//size of left and top borders
	Point margins;
	//total size including borders
	Point fullSize;

public:
	//Load image from def file
	SDLImage(CDefFile *data, size_t frame, size_t group=0);
	//Load from bitmap file
	SDLImage(std::string filename);

	SDLImage(const JsonNode & conf);
	//Create using existing surface, extraRef will increase refcount on SDL_Surface
	SDLImage(SDL_Surface * from, bool extraRef);
	~SDLImage();

	// Keep the original palette, in order to do color switching operation
	void savePalette();

	void draw(SDL_Surface * where, int posX=0, int posY=0, const Rect *src=nullptr, ui8 alpha=255) const override;
	void draw(SDL_Surface * where, const SDL_Rect * dest, const SDL_Rect * src, ui8 alpha=255) const override;
	std::shared_ptr<IImage> scaleFast(float scale) const override;
	void exportBitmap(const boost::filesystem::path & path) const override;
	void playerColored(PlayerColor player) override;
	void setFlagColor(PlayerColor player) override;
	bool isTransparent(const Point & coords) const override;
	Point dimensions() const override;

	void horizontalFlip() override;
	void verticalFlip() override;

	void shiftPalette(int from, int howMany) override;
	void adjustPalette(const ColorFilter & shifter) override;
	void resetPalette(int colorID) override;
	void resetPalette() override;

	void setSpecialPallete(const SpecialPalette & SpecialPalette) override;

	friend class SDLImageLoader;

private:
	SDL_Palette * originalPalette;
};

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

std::shared_ptr<IImage> IImage::createFromFile( const std::string & path )
{
	return std::shared_ptr<IImage>(new SDLImage(path));
}

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

bool operator== (const SDL_Color & lhs, const SDL_Color & rhs)
{
	return (lhs.a == rhs.a) && (lhs.b == rhs.b) &&(lhs.g == rhs.g) &&(lhs.r == rhs.r);
}

CDefFile::CDefFile(std::string Name):
	data(nullptr),
	palette(nullptr)
{
	//First 8 colors in def palette used for transparency
	static SDL_Color H3Palette[8] =
	{
		{   0,   0,   0,   0},// transparency                  ( used in most images )
		{   0,   0,   0,  64},// shadow border                 ( used in battle, adventure map def's )
		{   0,   0,   0,  64},// shadow border                 ( used in fog-of-war def's )
		{   0,   0,   0, 128},// shadow body                   ( used in fog-of-war def's )
		{   0,   0,   0, 128},// shadow body                   ( used in battle, adventure map def's )
		{   0,   0,   0,   0},// selection                     ( used in battle def's )
		{   0,   0,   0, 128},// shadow body   below selection ( used in battle def's )
		{   0,   0,   0,  64} // shadow border below selection ( used in battle def's )
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

	switch(static_cast<DefType>(type))
	{
	case DefType::SPELL:
		palette[0] = H3Palette[0];
		break;
	case DefType::SPRITE:
	case DefType::SPRITE_FRAME:
		for(ui32 i= 0; i<8; i++)
			palette[i] = H3Palette[i];
		break;
	case DefType::CREATURE:
		palette[0] = H3Palette[0];
		palette[1] = H3Palette[1];
		palette[4] = H3Palette[4];
		palette[5] = H3Palette[5];
		palette[6] = H3Palette[6];
		palette[7] = H3Palette[7];
		break;
	case DefType::MAP:
	case DefType::MAP_HERO:
		palette[0] = H3Palette[0];
		palette[1] = H3Palette[1];
		palette[4] = H3Palette[4];
		//5 = owner flag, handled separately
		break;
	case DefType::TERRAIN:
		palette[0] = H3Palette[0];
		palette[1] = H3Palette[1];
		palette[2] = H3Palette[2];
		palette[3] = H3Palette[3];
		palette[4] = H3Palette[4];
		break;
	case DefType::CURSOR:
		palette[0] = H3Palette[0];
		break;
	case DefType::INTERFACE:
		palette[0] = H3Palette[0];
		palette[1] = H3Palette[1];
		palette[4] = H3Palette[4];
		//player colors handled separately
		//TODO: disallow colorizing other def types
		break;
	case DefType::BATTLE_HERO:
		palette[0] = H3Palette[0];
		palette[1] = H3Palette[1];
		palette[4] = H3Palette[4];
		break;
	default:
		logAnim->error("Unknown def type %d in %s", type, Name);
		break;
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
				loader.Load(sprite.width, FDef + currentOffset);
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
						loader.Load(length, FDef + currentOffset);
						currentOffset+=length;
					}
					else// RLE
					{
						loader.Load(length, segmentType);
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
						loader.Load(length, FDef + currentOffset);
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
	image->surf = SDL_CreateRGBSurface(0, SpriteSize.x, SpriteSize.y, 8, 0, 0, 0, 0);
	image->margins  = Margins;
	image->fullSize = FullSize;

	//Prepare surface
	SDL_Palette * p = SDL_AllocPalette(SDLImage::DEFAULT_PALETTE_COLORS);
	SDL_SetPaletteColors(p, pal, 0, SDLImage::DEFAULT_PALETTE_COLORS);
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

/*************************************************************************
 *  Classes for images, support loading from file and drawing on surface *
 *************************************************************************/

IImage::IImage() = default;
IImage::~IImage() = default;

int IImage::width() const
{
	return dimensions().x;
}

int IImage::height() const
{
	return dimensions().y;
}

SDLImage::SDLImage(CDefFile * data, size_t frame, size_t group)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	SDLImageLoader loader(this);
	data->loadFrame(frame, group, loader);

	savePalette();
}

SDLImage::SDLImage(SDL_Surface * from, bool extraRef)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	surf = from;
	if (surf == nullptr)
		return;

	savePalette();

	if (extraRef)
		surf->refcount++;
	fullSize.x = surf->w;
	fullSize.y = surf->h;
}

SDLImage::SDLImage(const JsonNode & conf)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	std::string filename = conf["file"].String();

	surf = BitmapHandler::loadBitmap(filename);

	if(surf == nullptr)
		return;

	savePalette();

	const JsonNode & jsonMargins = conf["margins"];

	margins.x = static_cast<int>(jsonMargins["left"].Integer());
	margins.y = static_cast<int>(jsonMargins["top"].Integer());

	fullSize.x = static_cast<int>(conf["width"].Integer());
	fullSize.y = static_cast<int>(conf["height"].Integer());

	if(fullSize.x == 0)
	{
		fullSize.x = margins.x + surf->w + (int)jsonMargins["right"].Integer();
	}

	if(fullSize.y == 0)
	{
		fullSize.y = margins.y + surf->h + (int)jsonMargins["bottom"].Integer();
	}
}

SDLImage::SDLImage(std::string filename)
	: surf(nullptr),
	margins(0, 0),
	fullSize(0, 0),
	originalPalette(nullptr)
{
	surf = BitmapHandler::loadBitmap(filename);

	if(surf == nullptr)
	{
		logGlobal->error("Error: failed to load image %s", filename);
		return;
	}
	else
	{
		savePalette();
		fullSize.x = surf->w;
		fullSize.y = surf->h;
	}
}

void SDLImage::draw(SDL_Surface *where, int posX, int posY, const Rect *src, ui8 alpha) const
{
	if(!surf)
		return;

	Rect destRect(posX, posY, surf->w, surf->h);

	draw(where, &destRect, src);
}

void SDLImage::draw(SDL_Surface* where, const SDL_Rect* dest, const SDL_Rect* src, ui8 alpha) const
{
	if (!surf)
		return;

	Rect sourceRect(0, 0, surf->w, surf->h);

	Point destShift(0, 0);

	if(src)
	{
		if(src->x < margins.x)
			destShift.x += margins.x - src->x;

		if(src->y < margins.y)
			destShift.y += margins.y - src->y;

		sourceRect = Rect(*src) & Rect(margins.x, margins.y, surf->w, surf->h);

		sourceRect -= margins;
	}
	else
		destShift = margins;

	Rect destRect(destShift.x, destShift.y, surf->w, surf->h);

	if(dest)
	{
		destRect.x += dest->x;
		destRect.y += dest->y;
	}

	if(surf->format->BitsPerPixel == 8)
	{
		CSDL_Ext::blit8bppAlphaTo24bpp(surf, &sourceRect, where, &destRect);
	}
	else
	{
		SDL_UpperBlit(surf, &sourceRect, where, &destRect);
	}
}

std::shared_ptr<IImage> SDLImage::scaleFast(float scale) const
{
	auto scaled = CSDL_Ext::scaleSurfaceFast(surf, (int)(surf->w * scale), (int)(surf->h * scale));

	if (scaled->format && scaled->format->palette) // fix color keying, because SDL loses it at this point
		CSDL_Ext::setColorKey(scaled, scaled->format->palette->colors[0]);
	else if(scaled->format && scaled->format->Amask)
		SDL_SetSurfaceBlendMode(scaled, SDL_BLENDMODE_BLEND);//just in case
	else
		CSDL_Ext::setDefaultColorKey(scaled);//just in case

	SDLImage * ret = new SDLImage(scaled, false);

	ret->fullSize.x = (int) round((float)fullSize.x * scale);
	ret->fullSize.y = (int) round((float)fullSize.y * scale);

	ret->margins.x = (int) round((float)margins.x * scale);
	ret->margins.y = (int) round((float)margins.y * scale);

	return std::shared_ptr<IImage>(ret);
}

void SDLImage::exportBitmap(const boost::filesystem::path& path) const
{
	SDL_SaveBMP(surf, path.string().c_str());
}

void SDLImage::playerColored(PlayerColor player)
{
	graphics->blueToPlayersAdv(surf, player);
}

void SDLImage::setFlagColor(PlayerColor player)
{
	if(player < PlayerColor::PLAYER_LIMIT || player==PlayerColor::NEUTRAL)
		CSDL_Ext::setPlayerColor(surf, player);
}

bool SDLImage::isTransparent(const Point & coords) const
{
	return CSDL_Ext::isTransparent(surf, coords.x, coords.y);
}

Point SDLImage::dimensions() const
{
	return fullSize;
}

void SDLImage::horizontalFlip()
{
	margins.y = fullSize.y - surf->h - margins.y;

	//todo: modify in-place
	SDL_Surface * flipped = CSDL_Ext::horizontalFlip(surf);
	SDL_FreeSurface(surf);
	surf = flipped;
}

void SDLImage::verticalFlip()
{
	margins.x = fullSize.x - surf->w - margins.x;

	//todo: modify in-place
	SDL_Surface * flipped = CSDL_Ext::verticalFlip(surf);
	SDL_FreeSurface(surf);
	surf = flipped;
}

// Keep the original palette, in order to do color switching operation
void SDLImage::savePalette()
{
	// For some images that don't have palette, skip this
	if(surf->format->palette == nullptr)
		return;

	if(originalPalette == nullptr)
		originalPalette = SDL_AllocPalette(DEFAULT_PALETTE_COLORS);

	SDL_SetPaletteColors(originalPalette, surf->format->palette->colors, 0, DEFAULT_PALETTE_COLORS);
}

void SDLImage::shiftPalette(int from, int howMany)
{
	//works with at most 16 colors, if needed more -> increase values
	assert(howMany < 16);

	if(surf->format->palette)
	{
		SDL_Color palette[16];

		for(int i=0; i<howMany; ++i)
		{
			palette[(i+1)%howMany] = surf->format->palette->colors[from + i];
		}
		SDL_SetColors(surf, palette, from, howMany);
	}
}

void SDLImage::adjustPalette(const ColorFilter & shifter)
{
	if(originalPalette == nullptr)
		return;

	SDL_Palette* palette = surf->format->palette;

	// Note: here we skip the first 8 colors in the palette that predefined in H3Palette
	for(int i = 8; i < palette->ncolors; i++)
	{
		palette->colors[i] = shifter.shiftColor(originalPalette->colors[i]);
	}
}

void SDLImage::resetPalette()
{
	if(originalPalette == nullptr)
		return;
	
	// Always keept the original palette not changed, copy a new palette to assign to surface
	SDL_SetPaletteColors(surf->format->palette, originalPalette->colors, 0, originalPalette->ncolors);
}

void SDLImage::resetPalette( int colorID )
{
	if(originalPalette == nullptr)
		return;

	// Always keept the original palette not changed, copy a new palette to assign to surface
	SDL_SetPaletteColors(surf->format->palette, originalPalette->colors + colorID, colorID, 1);
}

void SDLImage::setSpecialPallete(const IImage::SpecialPalette & SpecialPalette)
{
	if(surf->format->palette)
	{
		SDL_SetColors(surf, const_cast<SDL_Color *>(SpecialPalette.data()), 1, 7);
	}
}

SDLImage::~SDLImage()
{
	SDL_FreeSurface(surf);

	if(originalPalette != nullptr)
	{
		SDL_FreePalette(originalPalette);
		originalPalette = nullptr;
	}
}

std::shared_ptr<IImage> CAnimation::getFromExtraDef(std::string filename)
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
		pos++;
		group = frame;
		frame = atoi(filename.c_str()+pos);
	}
	anim.load(frame ,group);
	auto ret = anim.images[group][frame];
	anim.images.clear();
	return ret;
}

bool CAnimation::loadFrame(size_t frame, size_t group)
{
	if(size(group) <= frame)
	{
		printError(frame, group, "LoadFrame");
		return false;
	}

	auto image = getImage(frame, group, false);
	if(image)
	{
		return true;
	}

	//try to get image from def
	if(source[group][frame].getType() == JsonNode::JsonType::DATA_NULL)
	{
		if(defFile)
		{
			auto frameList = defFile->getEntries();

			if(vstd::contains(frameList, group) && frameList.at(group) > frame) // frame is present
			{
				images[group][frame] = std::make_shared<SDLImage>(defFile.get(), frame, group);
				return true;
			}
		}
		// still here? image is missing

		printError(frame, group, "LoadFrame");
		images[group][frame] = std::make_shared<SDLImage>("DEFAULT");
	}
	else //load from separate file
	{
		auto img = getFromExtraDef(source[group][frame]["file"].String());
		if(!img)
			img = std::make_shared<SDLImage>(source[group][frame]);

		images[group][frame] = img;
		return true;
	}
	return false;
}

bool CAnimation::unloadFrame(size_t frame, size_t group)
{
	auto image = getImage(frame, group, false);
	if(image)
	{
		images[group].erase(frame);

		if(images[group].empty())
			images.erase(group);
		return true;
	}
	return false;
}

void CAnimation::initFromJson(const JsonNode & config)
{
	std::string basepath;
	basepath = config["basepath"].String();

	JsonNode base(JsonNode::JsonType::DATA_STRUCT);
	base["margins"] = config["margins"];
	base["width"] = config["width"];
	base["height"] = config["height"];

	for(const JsonNode & group : config["sequences"].Vector())
	{
		size_t groupID = group["group"].Integer();//TODO: string-to-value conversion("moving" -> MOVING)
		source[groupID].clear();

		for(const JsonNode & frame : group["frames"].Vector())
		{
			JsonNode toAdd(JsonNode::JsonType::DATA_STRUCT);
			JsonUtils::inherit(toAdd, base);
			toAdd["file"].String() = basepath + frame.String();
			source[groupID].push_back(toAdd);
		}
	}

	for(const JsonNode & node : config["images"].Vector())
	{
		size_t group = node["group"].Integer();
		size_t frame = node["frame"].Integer();

		if (source[group].size() <= frame)
			source[group].resize(frame+1);

		JsonNode toAdd(JsonNode::JsonType::DATA_STRUCT);
		JsonUtils::inherit(toAdd, base);
		toAdd["file"].String() = basepath + node["file"].String();
		source[group][frame] = toAdd;
	}
}

void CAnimation::exportBitmaps(const boost::filesystem::path& path) const
{
	if(images.empty())
	{
		logGlobal->error("Nothing to export, animation is empty");
		return;
	}

	boost::filesystem::path actualPath = path / "SPRITES" / name;
	boost::filesystem::create_directories(actualPath);

	size_t counter = 0;

	for(const auto & groupPair : images)
	{
		size_t group = groupPair.first;

		for(const auto & imagePair : groupPair.second)
		{
			size_t frame = imagePair.first;
			const auto img = imagePair.second;

			boost::format fmt("%d_%d.bmp");
			fmt % group % frame;

			img->exportBitmap(actualPath / fmt.str());
			counter++;
		}
	}

	logGlobal->info("Exported %d frames to %s", counter, actualPath.string());
}

void CAnimation::init()
{
	if(defFile)
	{
		const std::map<size_t, size_t> defEntries = defFile->getEntries();

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

void CAnimation::printError(size_t frame, size_t group, std::string type) const
{
	logGlobal->error("%s error: Request for frame not present in CAnimation! File name: %s, Group: %d, Frame: %d", type, name, group, frame);
}

CAnimation::CAnimation(std::string Name):
	name(Name),
	preloaded(false),
	defFile()
{
	size_t dotPos = name.find_last_of('.');
	if ( dotPos!=-1 )
		name.erase(dotPos);
	std::transform(name.begin(), name.end(), name.begin(), toupper);

	ResourceID resource(std::string("SPRITES/") + name, EResType::ANIMATION);

	if(CResourceHandler::get()->existsResource(resource))
		defFile = std::make_shared<CDefFile>(name);

	init();

	if(source.empty())
		logAnim->error("Animation %s failed to load", Name);
}

CAnimation::CAnimation():
	name(""),
	preloaded(false),
	defFile()
{
	init();
}

CAnimation::~CAnimation() = default;

void CAnimation::duplicateImage(const size_t sourceGroup, const size_t sourceFrame, const size_t targetGroup)
{
	if(!source.count(sourceGroup))
	{
		logAnim->error("Group %d missing in %s", sourceGroup, name);
		return;
	}

	if(source[sourceGroup].size() <= sourceFrame)
	{
		logAnim->error("Frame [%d %d] missing in %s", sourceGroup, sourceFrame, name);
		return;
	}

	//todo: clone actual loaded Image object
	JsonNode clone(source[sourceGroup][sourceFrame]);

	if(clone.getType() == JsonNode::JsonType::DATA_NULL)
	{
		std::string temp =  name+":"+boost::lexical_cast<std::string>(sourceGroup)+":"+boost::lexical_cast<std::string>(sourceFrame);
		clone["file"].String() = temp;
	}

	source[targetGroup].push_back(clone);

	size_t index = source[targetGroup].size() - 1;

	if(preloaded)
		load(index, targetGroup);
}

void CAnimation::setCustom(std::string filename, size_t frame, size_t group)
{
	if (source[group].size() <= frame)
		source[group].resize(frame+1);
	source[group][frame]["file"].String() = filename;
	//FIXME: update image if already loaded
}

std::shared_ptr<IImage> CAnimation::getImage(size_t frame, size_t group, bool verbose) const
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
	for (auto & elem : source)
		for (size_t image=0; image < elem.second.size(); image++)
			loadFrame(image, elem.first);
}

void CAnimation::unload()
{
	for (auto & elem : source)
		for (size_t image=0; image < elem.second.size(); image++)
			unloadFrame(image, elem.first);

}

void CAnimation::preload()
{
	if(!preloaded)
	{
		preloaded = true;
		load();
	}
}

void CAnimation::loadGroup(size_t group)
{
	if (vstd::contains(source, group))
		for (size_t image=0; image < source[group].size(); image++)
			loadFrame(image, group);
}

void CAnimation::unloadGroup(size_t group)
{
	if (vstd::contains(source, group))
		for (size_t image=0; image < source[group].size(); image++)
			unloadFrame(image, group);
}

void CAnimation::load(size_t frame, size_t group)
{
	loadFrame(frame, group);
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

void CAnimation::horizontalFlip()
{
	for(auto & group : images)
		for(auto & image : group.second)
			image.second->horizontalFlip();
}

void CAnimation::verticalFlip()
{
	for(auto & group : images)
		for(auto & image : group.second)
			image.second->verticalFlip();
}

void CAnimation::playerColored(PlayerColor player)
{
	for(auto & group : images)
		for(auto & image : group.second)
			image.second->playerColored(player);
}

void CAnimation::createFlippedGroup(const size_t sourceGroup, const size_t targetGroup)
{
	for(size_t frame = 0; frame < size(sourceGroup); ++frame)
	{
		duplicateImage(sourceGroup, frame, targetGroup);

		auto image = getImage(frame, targetGroup);
		image->verticalFlip();
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
	: delta(0),	fadingSurface(nullptr), fading(false), fadingCounter(0), shouldFreeSurface(false),
	  fadingMode(EMode::NONE)
{
}

CFadeAnimation::~CFadeAnimation()
{
	if (fadingSurface && shouldFreeSurface)
		SDL_FreeSurface(fadingSurface);
}

void CFadeAnimation::init(EMode mode, SDL_Surface * sourceSurface, bool freeSurfaceAtEnd, float animDelta)
{
	if (fading)
	{
		// in that case, immediately finish the previous fade
		// (alternatively, we could just return here to ignore the new fade request until this one finished (but we'd need to free the passed bitmap to avoid leaks))
		logGlobal->warn("Tried to init fading animation that is already running.");
		if (fadingSurface && shouldFreeSurface)
			SDL_FreeSurface(fadingSurface);
	}
	if (animDelta <= 0.0f)
	{
		logGlobal->warn("Fade anim: delta should be positive; %f given.", animDelta);
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

	CSDL_Ext::setAlpha(fadingSurface, (int)(fadingCounter * 255));
	SDL_BlitSurface(fadingSurface, const_cast<SDL_Rect *>(sourceRect), targetSurface, destRect); //FIXME
	CSDL_Ext::setAlpha(fadingSurface, 255);
}

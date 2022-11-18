/*
 * Animation.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

//code is copied from vcmiclient/CAnimation.cpp with minimal changes

#include "StdInc.h"
#include "Animation.h"

#include "BitmapHandler.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/ISimpleResourceLoader.h"
#include "../lib/JsonNode.h"
#include "../lib/CRandomGenerator.h"


typedef std::map<size_t, std::vector<JsonNode>> source_map;

/// Class for def loading
/// After loading will store general info (palette and frame offsets) and pointer to file itself
class DefFile
{
private:

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
	};
	//offset[group][frame] - offset of frame data in file
	std::map<size_t, std::vector <size_t> > offset;

	std::unique_ptr<ui8[]> data;
	std::unique_ptr<QVector<QRgb>> palette;

public:
	DefFile(std::string Name);
	~DefFile();

	std::shared_ptr<QImage> loadFrame(size_t frame, size_t group) const;

	const std::map<size_t, size_t> getEntries() const;
};

class ImageLoader
{
	QImage * image;
	ui8 * lineStart;
	ui8 * position;
	QPoint spriteSize, margins, fullSize;
public:
	//load size raw pixels from data
	inline void Load(size_t size, const ui8 * data);
	//set size pixels to color
	inline void Load(size_t size, ui8 color=0);
	inline void EndLine();
	//init image with these sizes and palette
	inline void init(QPoint SpriteSize, QPoint Margins, QPoint FullSize);

	ImageLoader(QImage * Img);
	~ImageLoader();
};

// Extremely simple file cache. TODO: smarter, more general solution
class FileCache
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
			if(file.name == rid)
				return file.getCopy();
		}
		// Still here? Cache miss
		if(cache.size() > cacheSize)
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

static FileCache animationCache;

/*************************************************************************
 *  DefFile, class used for def loading                                  *
 *************************************************************************/

DefFile::DefFile(std::string Name):
	data(nullptr)
{

	#if 0
	static QRgba H3_ORIG_PALETTE[8] =
	{
	   {  0, 255, 255, SDL_ALPHA_OPAQUE},
	   {255, 150, 255, SDL_ALPHA_OPAQUE},
	   {255, 100, 255, SDL_ALPHA_OPAQUE},
	   {255,  50, 255, SDL_ALPHA_OPAQUE},
	   {255,   0, 255, SDL_ALPHA_OPAQUE},
	   {255, 255, 0,   SDL_ALPHA_OPAQUE},
	   {180,   0, 255, SDL_ALPHA_OPAQUE},
	   {  0, 255, 0,   SDL_ALPHA_OPAQUE}
	};
	#endif // 0

	//First 8 colors in def palette used for transparency
	static QRgb H3Palette[8] =
	{
		qRgba(0, 0, 0,   0), // 100% - transparency
		qRgba(0, 0, 0,  32), //  75% - shadow border,
		qRgba(0, 0, 0,  64), // TODO: find exact value
		qRgba(0, 0, 0, 128), // TODO: for transparency
		qRgba(0, 0, 0, 128), //  50% - shadow body
		qRgba(0, 0, 0,   0), // 100% - selection highlight
		qRgba(0, 0, 0, 128), //  50% - shadow body   below selection
		qRgba(0, 0, 0,  64)  // 75% - shadow border below selection
	};
	data = animationCache.getCachedFile(ResourceID(std::string("SPRITES/") + Name, EResType::ANIMATION));

	palette = std::make_unique<QVector<QRgb>>(256);
	int it = 0;

	ui32 type = read_le_u32(data.get() + it);
	it += 4;
	//int width  = read_le_u32(data + it); it+=4;//not used
	//int height = read_le_u32(data + it); it+=4;
	it += 8;
	ui32 totalBlocks = read_le_u32(data.get() + it);
	it += 4;

	for (ui32 i= 0; i<256; i++)
	{
		ui8 c[3];
		c[0] = data[it++];
		c[1] = data[it++];
		c[2] = data[it++];
		(*palette)[i] = qRgba(c[0], c[1], c[2], 255);
	}

	switch(static_cast<DefType>(type))
	{
	case DefType::SPELL:
		(*palette)[0] = H3Palette[0];
		break;
	case DefType::SPRITE:
	case DefType::SPRITE_FRAME:
		for(ui32 i= 0; i<8; i++)
			(*palette)[i] = H3Palette[i];
		break;
	case DefType::CREATURE:
		(*palette)[0] = H3Palette[0];
		(*palette)[1] = H3Palette[1];
		(*palette)[4] = H3Palette[4];
		(*palette)[5] = H3Palette[5];
		(*palette)[6] = H3Palette[6];
		(*palette)[7] = H3Palette[7];
		break;
	case DefType::MAP:
	case DefType::MAP_HERO:
		(*palette)[0] = H3Palette[0];
		(*palette)[1] = H3Palette[1];
		(*palette)[4] = H3Palette[4];
		//5 = owner flag, handled separately
		break;
	case DefType::TERRAIN:
		(*palette)[0] = H3Palette[0];
		(*palette)[1] = H3Palette[1];
		(*palette)[2] = H3Palette[2];
		(*palette)[3] = H3Palette[3];
		(*palette)[4] = H3Palette[4];
		break;
	case DefType::CURSOR:
		(*palette)[0] = H3Palette[0];
		break;
	case DefType::INTERFACE:
		(*palette)[0] = H3Palette[0];
		(*palette)[1] = H3Palette[1];
		(*palette)[4] = H3Palette[4];
		//player colors handled separately
		//TODO: disallow colorizing other def types
		break;
	case DefType::BATTLE_HERO:
		(*palette)[0] = H3Palette[0];
		(*palette)[1] = H3Palette[1];
		(*palette)[4] = H3Palette[4];
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

std::shared_ptr<QImage> DefFile::loadFrame(size_t frame, size_t group) const
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

	
	std::shared_ptr<QImage> img = std::make_shared<QImage>(sprite.fullWidth, sprite.fullHeight, QImage::Format_Indexed8);
	if(!img)
		throw std::runtime_error("Image memory cannot be allocated");
	
	ImageLoader loader(img.get());
	loader.init(QPoint(sprite.width, sprite.height),
				QPoint(sprite.leftMargin, sprite.topMargin),
				QPoint(sprite.fullWidth, sprite.fullHeight));

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
	
	
	img->setColorTable(*palette);
	return img;
}

DefFile::~DefFile() = default;

const std::map<size_t, size_t > DefFile::getEntries() const
{
	std::map<size_t, size_t > ret;

	for (auto & elem : offset)
		ret[elem.first] =  elem.second.size();
	return ret;
}

/*************************************************************************
 *  Classes for image loaders - helpers for loading from def files       *
 *************************************************************************/

ImageLoader::ImageLoader(QImage * Img):
	image(Img),
	lineStart(Img->bits()),
	position(Img->bits())
{
	
}

void ImageLoader::init(QPoint SpriteSize, QPoint Margins, QPoint FullSize)
{
	spriteSize = SpriteSize;
	margins = Margins;
	fullSize = FullSize;
	
	memset((void *)image->bits(), 0, fullSize.y() * image->bytesPerLine());
	
	lineStart = image->bits();
	lineStart += margins.y() * image->bytesPerLine + margins.x();
	position = lineStart;
}

inline void ImageLoader::Load(size_t size, const ui8 * data)
{
	if(size)
	{
		memcpy((void *)position, data, size);
		position += size;
	}
}

inline void ImageLoader::Load(size_t size, ui8 color)
{
	if(size)
	{
		memset((void *)position, color, size);
		position += size;
	}
}

inline void ImageLoader::EndLine()
{
	lineStart += image->bytesPerLine();
	position = lineStart;
}

ImageLoader::~ImageLoader()
{
}

/*************************************************************************
 *  Classes for images, support loading from file and drawing on surface *
 *************************************************************************/

std::shared_ptr<QImage> Animation::getFromExtraDef(std::string filename)
{
	size_t pos = filename.find(':');
	if(pos == -1)
		return nullptr;
	Animation anim(filename.substr(0, pos));
	pos++;
	size_t frame = atoi(filename.c_str()+pos);
	size_t group = 0;
	pos = filename.find(':', pos);
	if(pos != -1)
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

bool Animation::loadFrame(size_t frame, size_t group)
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
				images[group][frame] = defFile->loadFrame(frame, group);
				return true;
			}
		}
		return false;
		// still here? image is missing

		printError(frame, group, "LoadFrame");
		images[group][frame] = std::make_shared<QImage>("DEFAULT");
	}
	else //load from separate file
	{
		images[group][frame] = getFromExtraDef(source[group][frame]["file"].String());;
		return true;
	}
	return false;
}

bool Animation::unloadFrame(size_t frame, size_t group)
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

void Animation::init()
{
	if(defFile)
	{
		const std::map<size_t, size_t> defEntries = defFile->getEntries();

		for (auto & defEntry : defEntries)
			source[defEntry.first].resize(defEntry.second);
	}

#if 0 //this code is not used but maybe requred if there will be configurable sprites
	ResourceID resID(std::string("SPRITES/") + name, EResType::TEXT);

	//if(vstd::contains(graphics->imageLists, resID.getName()))
		//initFromJson(graphics->imageLists[resID.getName()]);

	auto configList = CResourceHandler::get()->getResourcesWithName(resID);

	for(auto & loader : configList)
	{
		auto stream = loader->load(resID);
		std::unique_ptr<ui8[]> textData(new ui8[stream->getSize()]);
		stream->read(textData.get(), stream->getSize());

		const JsonNode config((char*)textData.get(), stream->getSize());

		//initFromJson(config);
	}
#endif
}

void Animation::printError(size_t frame, size_t group, std::string type) const
{
	logGlobal->error("%s error: Request for frame not present in CAnimation! File name: %s, Group: %d, Frame: %d", type, name, group, frame);
}

Animation::Animation(std::string Name):
	name(Name),
	preloaded(false),
	defFile()
{
	size_t dotPos = name.find_last_of('.');
	if( dotPos!=-1 )
		name.erase(dotPos);
	std::transform(name.begin(), name.end(), name.begin(), toupper);

	ResourceID resource(std::string("SPRITES/") + name, EResType::ANIMATION);

	if(CResourceHandler::get()->existsResource(resource))
		defFile = std::make_shared<DefFile>(name);

	init();

	if(source.empty())
		logAnim->error("Animation %s failed to load", Name);
}

Animation::Animation():
	name(""),
	preloaded(false),
	defFile()
{
	init();
}

Animation::~Animation() = default;

void Animation::duplicateImage(const size_t sourceGroup, const size_t sourceFrame, const size_t targetGroup)
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

void Animation::setCustom(std::string filename, size_t frame, size_t group)
{
	if(source[group].size() <= frame)
		source[group].resize(frame+1);
	source[group][frame]["file"].String() = filename;
	//FIXME: update image if already loaded
}

std::shared_ptr<QImage> Animation::getImage(size_t frame, size_t group, bool verbose) const
{
	auto groupIter = images.find(group);
	if(groupIter != images.end())
	{
		auto imageIter = groupIter->second.find(frame);
		if(imageIter != groupIter->second.end())
			return imageIter->second;
	}
	if(verbose)
		printError(frame, group, "GetImage");
	return nullptr;
}

void Animation::load()
{
	for (auto & elem : source)
		for (size_t image=0; image < elem.second.size(); image++)
			loadFrame(image, elem.first);
}

void Animation::unload()
{
	for (auto & elem : source)
		for (size_t image=0; image < elem.second.size(); image++)
			unloadFrame(image, elem.first);

}

void Animation::preload()
{
	if(!preloaded)
	{
		preloaded = true;
		load();
	}
}

void Animation::loadGroup(size_t group)
{
	if(vstd::contains(source, group))
		for (size_t image=0; image < source[group].size(); image++)
			loadFrame(image, group);
}

void Animation::unloadGroup(size_t group)
{
	if(vstd::contains(source, group))
		for (size_t image=0; image < source[group].size(); image++)
			unloadFrame(image, group);
}

void Animation::load(size_t frame, size_t group)
{
	loadFrame(frame, group);
}

void Animation::unload(size_t frame, size_t group)
{
	unloadFrame(frame, group);
}

size_t Animation::size(size_t group) const
{
	auto iter = source.find(group);
	if(iter != source.end())
		return iter->second.size();
	return 0;
}

void Animation::horizontalFlip()
{
	for(auto & group : images)
		for(auto & image : group.second)
			*image.second = image.second->transformed(QTransform::fromScale(-1, 1));
}

void Animation::verticalFlip()
{
	for(auto & group : images)
		for(auto & image : group.second)
			*image.second = image.second->transformed(QTransform::fromScale(1, -1));
}

void Animation::playerColored(PlayerColor player)
{
#if 0 //can be required in image preview?
	for(auto & group : images)
		for(auto & image : group.second)
			image.second->playerColored(player);
#endif
}

void Animation::createFlippedGroup(const size_t sourceGroup, const size_t targetGroup)
{
	for(size_t frame = 0; frame < size(sourceGroup); ++frame)
	{
		duplicateImage(sourceGroup, frame, targetGroup);

		auto image = getImage(frame, targetGroup);
		*image = image->transformed(QTransform::fromScale(1, -1));
	}
}

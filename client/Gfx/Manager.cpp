#include "StdInc.h"
#include <map>

#include "Manager.h"
#include "Animations.h"
#include "Images.h"
#include "FilesHeaders.h"
#include "../../lib/Filesystem/CResourceLoader.h"


namespace Gfx
{

class Resources
{
	friend class CManager;
	static std::map<std::string, PImage> images;
	static std::map<std::string, PAnimation> anims;
};

std::map<std::string, PImage> Resources::images;
std::map<std::string, PAnimation> Resources::anims;



PImage CManager::getImage(const std::string& fname)
{
	PImage& img_ptr = Resources::images[fname];

	if (img_ptr) return img_ptr;

	ResourceID resImageId("DATA/" + fname, EResType::IMAGE);
	auto stream = CResourceHandler::get()->load(resImageId);

	auto streamSize = stream->getSize();
	if (streamSize < H3PCX_HEADER_SIZE) return nullptr;
	if (streamSize > 0x10000000) streamSize = 0x10000000;

	std::unique_ptr<ui8[]> data(new ui8[(size_t)streamSize]);
	auto readSize = stream->read(data.get(), streamSize);

	assert(readSize == stream->getSize());

	CImage* img_tmp = CImage::makeFromPCX(*(SH3PcxFile*)data.get(), (size_t)readSize);
	if (img_tmp == nullptr) return nullptr;

	return img_ptr = PImage(img_tmp);
}


PAnimation CManager::getAnimation(const std::string& fname)
{
	PAnimation& anim_ptr = Resources::anims[fname];

	if (anim_ptr) return anim_ptr;

	ResourceID resAnimId("SPRITES/" + fname, EResType::ANIMATION);
	auto stream = CResourceHandler::get()->load(resAnimId);

	auto streamSize = stream->getSize();
	if (streamSize < H3DEF_HEADER_SIZE) return nullptr;
	if (streamSize > 0x7FFFFFF0) streamSize = 0x7FFFFFF0;

	std::unique_ptr<ui8[]> data(new ui8[(size_t)streamSize]);
	auto readSize = stream->read(data.get(), streamSize);

	assert(readSize == stream->getSize());

	CAnimation* anim_tmp = CAnimation::makeFromDEF(*(SH3DefFile*)data.get(), (size_t)readSize);
	if (anim_tmp == nullptr) return nullptr;

	return anim_ptr = PAnimation(anim_tmp);
}

}


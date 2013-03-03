#include "StdInc.h"

#include "Animations.h"
#include "Images.h"
#include "FilesHeaders.h"

namespace Gfx
{
#define LE(x) SDL_SwapLE32(x)

/*********** CAnimation ***********/

CAnimation* CAnimation::makeFromDEF(const SH3DefFile& def, size_t fileSize)
{
	if (def.totalBlocks == 0 || def.width == 0 || def.height == 0) return nullptr;

	if (def.fbEntrCount() == LE(1))
	{
		CImage* img = CImage::makeFromDEF(def, fileSize);

		if (img == nullptr) return nullptr;
		return new COneFrameAnimation(*img);
	}

	return new (LE(def.fbEntrCount())) CPalettedAnimation(def, fileSize);
}


CAnimation::~CAnimation() {}


/*********** COneFrameAnimation ***********/

COneFrameAnimation::COneFrameAnimation(CImage& img) :
	CAnimation(1, img.getWidth(), img.getHeight()),
	frame(img)
{
}


COneFrameAnimation::~COneFrameAnimation()
{
	delete &frame;
}


CImage* COneFrameAnimation::getFrame(ui32 fnr)
{
	if (fnr == 0) return &frame;
	return nullptr;
}


/*********** CPalettedAnimation ***********/

CPalettedAnimation::CPalettedAnimation(const SH3DefFile& def, size_t fileSize) :

	CAnimation(LE(def.fbEntrCount()), LE(def.width), LE(def.height)),
	
	palette(def.palette, def.type == LE(71) ? 1 : 10, true)
		//type == 71 - Buttons/buildings don't have shadows\semi-transparency
{
	ua_ui32_ptr offsets = def.firstBlock.offsets();

	for (ui32 j=0; j<framesCount; ++j)
	{
		CPalettedBitmap* fr = CImage::makeFromDEFSprite(def.getSprite(offsets[j]), palette);
		if (fr == nullptr)
		{
			framesCount = j;
			break;
		}
		frames[j] = fr;
	}
}


CPalettedAnimation::~CPalettedAnimation()
{
	for (ui32 j=0; j<framesCount; ++j)
	{
		delete frames[j];
	}
}


CImage* CPalettedAnimation::getFrame(ui32 fnr)
{
	if (fnr >= framesCount) return nullptr;
	return frames[fnr];
}

}

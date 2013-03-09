#pragma once

#include "CPaletteRGBA.h"

namespace Gfx
{
struct SH3DefFile;
class CImage;


class CAnimation
{
protected:
	ui32 framesCount;
	ui32 width;
	ui32 height;

	inline CAnimation(ui32 fc, ui32 w, ui32 h) : framesCount(fc), width(w), height(h) {};

public:
	static CAnimation* makeFromDEF(const SH3DefFile& def, size_t fileSize);

	virtual ~CAnimation();
	virtual CImage* getFrame(ui32 fnr) = 0;

	inline ui32 getFramesCount() { return framesCount; };
	inline ui32 getWidth() { return width; };
	inline ui32 getHeight() { return height; };
};


class COneFrameAnimation : CAnimation
{
	friend class CAnimation;
	CImage& frame;

protected:
	COneFrameAnimation(CImage& img);

public:
	virtual ~COneFrameAnimation();
	virtual CImage* getFrame(ui32 fnr);
};


#ifdef _MSC_VER
	#pragma warning(disable : 4200)
	#pragma warning(disable : 4291)
#endif

class CPalettedAnimation : CAnimation
{
	friend class CAnimation;
	CPaletteRGBA palette;
	CImage* frames[];

protected:
	CPalettedAnimation(const SH3DefFile& def, size_t fileSize);

	inline void* operator new(size_t size, ui32 frCount) {
		return ::operator new(size + frCount * sizeof(void*));
	}

public:
	virtual ~CPalettedAnimation();
	virtual CImage* getFrame(ui32 fnr);
};

#ifdef _MSC_VER
	#pragma warning(default : 4200)
	#pragma warning(default : 4291)
#endif

}

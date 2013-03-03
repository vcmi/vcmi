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


#pragma warning(disable : 4200)

class CPalettedAnimation : CAnimation
{
	friend class CAnimation;
	CPaletteRGBA palette;
	CImage* frames[];

protected:
	inline void* operator new(size_t size, ui32 frCount) {
		return ::operator new(size + frCount * sizeof(void*));
	}
	inline void operator delete(void* ptr, ui32 frCount) {
		::operator delete(ptr);
	}
	CPalettedAnimation(const SH3DefFile& def, size_t fileSize);

public:
	virtual ~CPalettedAnimation();
	virtual CImage* getFrame(ui32 fnr);
};

#pragma warning(default : 4200)

}

#pragma once

#include "Basic.h"
#include "CPaletteRGBA.h"

namespace Gfx
{

enum TransformFlags
{
	NONE = 0,
	MIRROR_HORIZ = 1,
	MIRROR_VERTIC = 2,
	ROTATE_90_DEG = 4
};


class CImage
{
protected:
	ui32 texHandle;
	ui32 width;
	ui32 height;

	inline CImage(ui32 w, ui32 h) : texHandle(0), width(w), height(h) {};
	virtual void textureTransfer() = 0;

public:
	static CImage* makeBySDL(void* data, size_t fileSize, const char* fileExt);
	static CImage* makeFromPCX(const struct SH3PcxFile& pcx, size_t fileSize);
	static CImage* makeFromDEF(const struct SH3DefFile& def, size_t fileSize);
	static class CPalettedBitmap* makeFromDEFSprite(
						const struct SH3DefSprite& spr, CPaletteRGBA& pal);

	virtual ~CImage();

	inline ui32 getWidth() { return width; };
	inline ui32 getHeight() { return height; };

	void loadToVideoRAM();
	void unloadFromVideoRAM();
	void bindTexture();

	virtual void putAt(Point p) = 0;
	virtual void putAt(Point p, TransformFlags flags) = 0;
	virtual void putAt(Point p, TransformFlags flags, float scale) = 0;
	virtual void putAt(Point p, TransformFlags flags, Rect clipRect) = 0;
	virtual void putAt(Point p, TransformFlags flags, const ColorMatrix cm) = 0;
	virtual void putWithPlrColor(Point p, ColorRGBA c) = 0;
	virtual void putWithPlrColor(Point p, ColorRGBA c, float scale) = 0;
};


class CBitmap32 : public CImage
{
	friend class CImage;
	ColorRGBA* buffer;
	bool formatBGRA;

	struct CoordBind {
		Point texture, vertex;
		inline CoordBind() : texture(0, 0) {};
	};

	class QuadInstance {
		CoordBind coords[4];
	public:
		QuadInstance(Point p);
		QuadInstance(const CoordBind c[4]);
		void setOffset(TransformFlags flags, si32 x, si32 y);
		void transform(TransformFlags flags, ui32 w0, ui32 h0, ui32 w, ui32 h);
		void putToGL() const;
	};

protected:
	CBitmap32(ui32 w, ui32 h, const ColorRGB pixBuff[], bool bgra); // 24bit RGB source
	CBitmap32(ui32 w, ui32 h, const ColorRGBA pixBuff[], bool bgra); // 32bit RGBA source
	virtual void textureTransfer();

public:
	virtual ~CBitmap32();

	virtual void putAt(Point p);
	virtual void putAt(Point p, TransformFlags flags);
	virtual void putAt(Point p, TransformFlags flags, float scale);
	virtual void putAt(Point p, TransformFlags flags, Rect clipRect);
	virtual void putAt(Point p, TransformFlags flags, const ColorMatrix cm);
	virtual void putWithPlrColor(Point p, ColorRGBA c);
	virtual void putWithPlrColor(Point p, ColorRGBA c, float scale);
};


class CPalettedBitmap : public CImage
{
	friend class CImage;

protected:
	CPaletteRGBA& palette;
	ui8* buffer;
	ui32 realWidth;
	ui32 realHeight;

	CPalettedBitmap(ui32 w, ui32 h, CPaletteRGBA& pal, const ui8 pixBuff[]);
	CPalettedBitmap(ui32 w, ui32 h, CPaletteRGBA& pal, const ui8 pixBuff[], ui32 format);
	virtual void textureTransfer();

public:
	virtual ~CPalettedBitmap();

	virtual void putAt(Point p);
	virtual void putAt(Point p, TransformFlags flags);
	virtual void putAt(Point p, TransformFlags flags, float scale);
	virtual void putAt(Point p, TransformFlags flags, Rect clipRect);
	virtual void putAt(Point p, TransformFlags flags, const ColorMatrix cm);
	virtual void putWithPlrColor(Point p, ColorRGBA c);
	virtual void putWithPlrColor(Point p, ColorRGBA c, float scale);
};


class CPalBitmapWithMargin : public CPalettedBitmap
{
	friend class CImage;
	ui32 leftMargin;
	ui32 topMargin;

protected:
	CPalBitmapWithMargin(ui32 fw, ui32 fh, ui32 lm, ui32 tm, ui32 iw, ui32 ih,
						 CPaletteRGBA& pal, const ui8 pixBuff[]);
	CPalBitmapWithMargin(ui32 fw, ui32 fh, ui32 lm, ui32 tm, ui32 iw, ui32 ih,
						 CPaletteRGBA& pal, const ui8 pixBuff[], ui32 format);
public:
	virtual void putAt(Point p);
	virtual void putAt(Point p, TransformFlags flags);
	virtual void putAt(Point p, TransformFlags flags, float scale);
	virtual void putAt(Point p, TransformFlags flags, Rect clipRect);
	virtual void putAt(Point p, TransformFlags flags, const ColorMatrix cm);
	virtual void putWithPlrColor(Point p, ColorRGBA c);
	virtual void putWithPlrColor(Point p, ColorRGBA c, float scale);
};


}

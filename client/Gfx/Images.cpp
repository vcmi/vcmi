#include "StdInc.h"
#include <SDL_opengl.h>
#include <SDL_endian.h>
#include "Images.h"

namespace Gfx
{

/*********** CImage ***********/

CImage::~CImage()
{
	unloadFromVideoRAM();
}


void CImage::loadToVideoRAM()
{
	if (texHandle > 0) return;
	glGenTextures(1, &texHandle);
	glBindTexture(GL_TEXTURE_RECTANGLE, texHandle);
	textureTransfer();
}


void CImage::unloadFromVideoRAM()
{
	glDeleteTextures(1, &texHandle);
	texHandle = 0;
}


void CImage::bindTexture()
{
	if (texHandle > 0)
	{
		glBindTexture(GL_TEXTURE_RECTANGLE, texHandle);
		return;
	}
	glGenTextures(1, &texHandle);
	glBindTexture(GL_TEXTURE_RECTANGLE, texHandle);
	textureTransfer();
}



/*********** CBitmap32::QuadInstance ***********/

CBitmap32::QuadInstance::QuadInstance(Point p) : coords()
{
	for (int i=0; i<4; ++i) coords[i].vertex = p;
}


void CBitmap32::QuadInstance::setOffset(TransformFlags flags, si32 x, si32 y)
{
	if (flags & ROTATE_90_DEG)
	{
	}
	else
	{
	}
}


void CBitmap32::QuadInstance::transform(TransformFlags flags, ui32 w0, ui32 h0, ui32 w, ui32 h)
{
	if (flags & MIRROR_HORIZ)
	{
		coords[0].vertex.x += w;
		coords[3].vertex.x += w;
	}
	else
	{
		coords[1].vertex.x += w;
		coords[2].vertex.x += w;
	}

	if (flags & MIRROR_VERTIC)
	{
		coords[0].vertex.y += h;
		coords[1].vertex.y += h;
	}
	else
	{
		coords[2].vertex.y += h;
		coords[3].vertex.y += h;
	}

	if (flags & ROTATE_90_DEG)
	{
		coords[0].texture.x = w0;
		coords[1].texture.x = w0;
		coords[1].texture.y = h0;
		coords[2].texture.y = h0;
	}
	else
	{
		coords[1].texture.x = w0;
		coords[2].texture.x = w0;
		coords[2].texture.y = h0;
		coords[3].texture.y = h0;
	}
}


void CBitmap32::QuadInstance::putToGL() const
{
	glBegin(GL_QUADS);
	for (int i=0; i<4; ++i)
	{
		const CoordBind& row = coords[i];
		glTexCoord2i(row.texture.x, row.texture.y);
		glVertex2i(row.vertex.x, row.vertex.y);
	}
	glEnd();
}


/*********** CBitmap32 ***********/

CBitmap32::CBitmap32(ui32 w, ui32 h, const ColorRGB pixBuff[], bool bgra) : CImage(w, h), formatBGRA(bgra)
{
	const ui32 pixNum = w * h;
	buffer = new ColorRGBA[pixNum];

	for (ui32 it=0; it<pixNum; ++it)
	{
		memcpy(&buffer[it], &pixBuff[it], 3);
		buffer[it].comp.A = 255;
	}
}


CBitmap32::CBitmap32(ui32 w, ui32 h, const ColorRGBA pixBuff[], bool bgra) : CImage(w, h), formatBGRA(bgra)
{
	const ui32 pixNum = w * h;
	buffer = new ColorRGBA[pixNum];

	memcpy(buffer, pixBuff, pixNum * sizeof(ColorRGBA));
}


CBitmap32::~CBitmap32()
{
	delete buffer;
}


void CBitmap32::textureTransfer()
{
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_RGBA8, width, height, 0, formatBGRA ? GL_BGRA : GL_RGBA, GL_UNSIGNED_BYTE, buffer);
}


void CBitmap32::putAt(Point p)
{
	GL2D::useNoShader();
	bindTexture();

	glBegin(GL_QUADS);
	glTexCoord2i(0, 0);
	glVertex2i(p.x, p.y);
	glTexCoord2i(width, 0);
	glVertex2i(p.x + width, p.y);
	glTexCoord2i(width, height);
	glVertex2i(p.x + width, p.y + height);
	glTexCoord2i(0, height);
	glVertex2i(p.x, p.y + height);
	glEnd();
}


void CBitmap32::putAt(Point p, TransformFlags flags)
{
	QuadInstance qi(p);
	qi.transform(flags, width, height, width, height);

	GL2D::useNoShader();
	bindTexture();
	qi.putToGL();
}


void CBitmap32::putAt(Point p, TransformFlags flags, float scale)
{
	QuadInstance qi(p);
	qi.transform(flags, width, height, (ui32)(width*scale), (ui32)(height*scale));

	GL2D::useNoShader();
	bindTexture();
	qi.putToGL();
}


void CBitmap32::putAt(Point p, TransformFlags flags, Rect clipRect)
{
	QuadInstance qi(p);
	qi.setOffset(flags, clipRect.lt.x, clipRect.lt.y);
	qi.transform(flags, clipRect.rb.x - p.x, clipRect.rb.y - p.y, clipRect.width(), clipRect.height());

	GL2D::useNoShader();
	bindTexture();
	qi.putToGL();
}


void CBitmap32::putAt(Point p, TransformFlags flags, const ColorMatrix cm)
{
	QuadInstance qi(p);
	qi.transform(flags, width, height, width, height);

	GL2D::useColorizeShader(cm);
	bindTexture();
	qi.putToGL();
}


void CBitmap32::putWithPlrColor(Point p, ColorRGBA c)
{
	putAt(p);
}


void CBitmap32::putWithPlrColor(Point p, ColorRGBA c, float scale)
{
	putAt(p, NONE, scale);
}


/*********** CPalettedBitmap ***********/

CPalettedBitmap::CPalettedBitmap(ui32 w, ui32 h, CPaletteRGBA& pal, const ui8 pixBuff[]) :
	CImage(w, h),
	palette(pal)
{
	const ui32 rowStride = (w + 3) & ~3;
	const ui32 size = rowStride * h;
	buffer = new ui8[size];

	if (rowStride == w)
	{
		memcpy(buffer, pixBuff, size);
		return;
	}

	for (ui32 y=0; y<h; ++y)
	{
		memset(&buffer[rowStride*(y+1)-4], 0, 4);
		memcpy(&buffer[rowStride*y], &pixBuff[w*y], w);
	}
	width = rowStride;
}


CPalettedBitmap::CPalettedBitmap(ui32 w, ui32 h, CPaletteRGBA& pal, const ui8 pixBuff[], ui32 format) :
	CImage(w, h),
	palette(pal)
{
	const ui32 rowStride = (w + 3) & ~3;
	buffer = new ui8[rowStride * h];

	switch (format)
	{
	case 1:
		{
			const ua_ui32_ptr rowsOffsets = (ua_ui32_ptr)pixBuff;

			for (ui32 y=0; y<h; ++y)
			{
				const ui8*	srcRowPtr = pixBuff + SDL_SwapLE32(rowsOffsets[y]);
				ui8*		dstRowPtr = buffer + (y * rowStride);

				ui32 rowLength = 0;
				do {
					ui8 segmentType = *(srcRowPtr++);
					size_t segmentLength = *(srcRowPtr++) + 1;

					if (segmentType == 0xFF)
					{
						memcpy(dstRowPtr, srcRowPtr, segmentLength);
						srcRowPtr += segmentLength;
					}
					else
					{
						memset(dstRowPtr, segmentType, segmentLength);
					}
					dstRowPtr += segmentLength;
					rowLength += segmentLength;
				}
				while (rowLength < w);
			}
			return;
		}
	default:
		return;
	}
}


CPalettedBitmap::~CPalettedBitmap()
{
	delete buffer;
	palette.Unlink();
}


void CPalettedBitmap::textureTransfer()
{
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R8UI, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, buffer);
	palette.loadToVideoRAM();
}


void CPalettedBitmap::putAt(Point p)
{
	loadToVideoRAM();
	GL2D::assignTexture(GL_TEXTURE1, GL_TEXTURE_1D, palette.getTexHandle());
	GL2D::assignTexture(GL_TEXTURE0, GL_TEXTURE_RECTANGLE, texHandle);
	GL2D::usePaletteBitmapShader(p.x, p.y);
	glRecti(p.x, p.y, p.x + width, p.y + height);
}


void CPalettedBitmap::putAt(Point p, TransformFlags flags)
{
	putAt(p);
}


void CPalettedBitmap::putAt(Point p, TransformFlags flags, float scale)
{
	loadToVideoRAM();
	GL2D::assignTexture(GL_TEXTURE1, GL_TEXTURE_1D, palette.getTexHandle());
	GL2D::assignTexture(GL_TEXTURE0, GL_TEXTURE_RECTANGLE, texHandle);
	GL2D::usePaletteBitmapShader(p.x, p.y);
	glRecti(p.x, p.y, p.x + (ui32)(width*scale), p.y + (ui32)(height*scale));
}


void CPalettedBitmap::putAt(Point p, TransformFlags flags, Rect clipRect)
{
	putAt(p);
}


void CPalettedBitmap::putAt(Point p, TransformFlags flags, const ColorMatrix cm)
{
	loadToVideoRAM();
	GL2D::assignTexture(GL_TEXTURE1, GL_TEXTURE_1D, palette.getTexHandle());
	GL2D::assignTexture(GL_TEXTURE0, GL_TEXTURE_RECTANGLE, texHandle);
	GL2D::usePaletteBitmapShader(p.x, p.y, cm);
	glRecti(p.x, p.y, p.x + width, p.y + height);
}


void CPalettedBitmap::putWithPlrColor(Point p, ColorRGBA c)
{
	putAt(p);
}


void CPalettedBitmap::putWithPlrColor(Point p, ColorRGBA c, float scale)
{
	putAt(p, NONE, scale);
}


/*********** CPalBitmapWithMargin ***********/

CPalBitmapWithMargin::CPalBitmapWithMargin(ui32 fw, ui32 fh, ui32 lm, ui32 tm, ui32 iw, ui32 ih,
										   CPaletteRGBA& pal, const ui8 pixBuff[]) :
	CPalettedBitmap(iw, ih, pal, pixBuff),
	leftMargin(lm), topMargin(tm),
	intWidth(iw), intHeight(ih)
{
	width = fw;
	height = fh;
}


void CPalBitmapWithMargin::textureTransfer()
{
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0, GL_R8UI, intWidth, intHeight, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, buffer);
	palette.loadToVideoRAM();
}


CPalBitmapWithMargin::CPalBitmapWithMargin(ui32 fw, ui32 fh, ui32 lm, ui32 tm, ui32 iw, ui32 ih,
										   CPaletteRGBA& pal, const ui8 pixBuff[], ui32 format) :
	CPalettedBitmap(iw, ih, pal, pixBuff, format),
	leftMargin(lm), topMargin(tm),
	intWidth(iw), intHeight(ih)
{
	width = fw;
	height = fh;
}


void CPalBitmapWithMargin::putAt(Point p)
{
	loadToVideoRAM();
	GL2D::assignTexture(GL_TEXTURE1, GL_TEXTURE_1D, palette.getTexHandle());
	GL2D::assignTexture(GL_TEXTURE0, GL_TEXTURE_RECTANGLE, texHandle);
	GL2D::usePaletteBitmapShader(p.x, p.y);
	glRecti(p.x, p.y, p.x + intWidth, p.y + intHeight);
}


void CPalBitmapWithMargin::putAt(Point p, TransformFlags flags)
{
	putAt(p);
}


void CPalBitmapWithMargin::putAt(Point p, TransformFlags flags, float scale)
{
	putAt(p);
}


void CPalBitmapWithMargin::putAt(Point p, TransformFlags flags, Rect clipRect)
{
	putAt(p);
}


void CPalBitmapWithMargin::putAt(Point p, TransformFlags flags, const ColorMatrix cm)
{
	loadToVideoRAM();
	GL2D::assignTexture(GL_TEXTURE1, GL_TEXTURE_1D, palette.getTexHandle());
	GL2D::assignTexture(GL_TEXTURE0, GL_TEXTURE_RECTANGLE, texHandle);
	GL2D::usePaletteBitmapShader(p.x, p.y, cm);
	glRecti(p.x, p.y, p.x + width, p.y + height);
}


void CPalBitmapWithMargin::putWithPlrColor(Point p, ColorRGBA c)
{
	putAt(p);
}


void CPalBitmapWithMargin::putWithPlrColor(Point p, ColorRGBA c, float scale)
{
	putAt(p, NONE, scale);
}

}

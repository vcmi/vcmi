#include "StdInc.h"
#include <SDL_opengl.h>
#include "CPaletteRGBA.h"


namespace Gfx
{

CPaletteRGBA::CPaletteRGBA(const ColorRGBA palBuff[]) : texHandle(0), shared(false)
{
	memcpy(buffer, palBuff, 256);
}


CPaletteRGBA::CPaletteRGBA(const ui8 palBuff[][3], int alphaMode, bool shr) : texHandle(0), shared(shr)
{
	static const ui8 defPalette[10][4] = {{0,0,0,0}, {0,0,0,32}, {0,0,0,64}, {0,0,0,128}, {0,0,0,128},
		{255,255,255,0}, {255,255,255,0}, {255,255,255,0}, {0,0,0,192}, {0,0,0,192}};

	if (alphaMode > 0)
	{
		if (alphaMode > 10) alphaMode = 10;
		memcpy(buffer, defPalette, alphaMode*sizeof(ColorRGBA));
	}

	for (ui32 it = alphaMode; it<256; ++it)
	{
		memcpy(&buffer[it], palBuff[it], 3);
		buffer[it].comp.A = 255;
	}
}


CPaletteRGBA::~CPaletteRGBA()
{
	unloadFromVideoRAM();
}


void CPaletteRGBA::Unlink()
{
	if (!shared) delete this;
}


void CPaletteRGBA::loadToVideoRAM()
{
	if (texHandle > 0) return;
	glGenTextures(1, &texHandle);
	glBindTexture(GL_TEXTURE_1D, texHandle);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
}


void CPaletteRGBA::unloadFromVideoRAM()
{
	glDeleteTextures(1, &texHandle);
	texHandle = 0;
}


}

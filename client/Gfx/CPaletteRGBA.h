#pragma once

namespace Gfx
{

#pragma pack(1)

typedef ui8 ColorRGB[3]; // 24bit RGB color

union ColorRGBA // 32bit RGBA color
{
	ui32 color32;
	ui8 arr[4];
	struct {
		ui8 R,G,B,A;
	} comp;

	inline ColorRGBA() {};
	inline ColorRGBA(ui32 c) : color32(c) {};
};

#pragma pack()


class CPaletteRGBA
{
	friend class CPalettedAnimation;

	ui32 texHandle; // OpenGL texture handle
	bool shared;
	ColorRGBA buffer[256];

	~CPaletteRGBA();

public:
	CPaletteRGBA(const ColorRGBA palBuff[]); // 32bit RGBA source
	CPaletteRGBA(const ColorRGB palBuff[], int alphaMode, bool shr=false); // 24-bit RGB source

	void Unlink();

	inline ui32 getTexHandle() { return texHandle; };
	void loadToVideoRAM();
	void unloadFromVideoRAM();
};

}

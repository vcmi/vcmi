/*
 * CBitmapFont.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../render/IFont.h"

#include "../../lib/Rect.h"

VCMI_LIB_NAMESPACE_BEGIN
class ResourcePath;
VCMI_LIB_NAMESPACE_END

class CBitmapFont final : public IFont
{
	SDL_Surface * atlasImage;

	using CodePoint = uint32_t;

	struct EntryFNT
	{
		int32_t leftOffset;
		uint32_t width;
		uint32_t height;
		int32_t rightOffset;
		std::vector<uint8_t> pixels;
	};

	struct BitmapChar
	{
		Rect positionInAtlas;
		int32_t leftOffset;
		int32_t rightOffset;
	};

	std::unordered_map<CodePoint, BitmapChar> chars;
	uint32_t height;
	uint32_t ascent;

	void loadFont(const ResourcePath & resource, std::unordered_map<CodePoint, EntryFNT> & loadedChars);

	void renderCharacter(SDL_Surface * surface, const BitmapChar & character, const ColorRGBA & color, int &posX, int &posY) const;
	void renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const override;
public:
	explicit CBitmapFont(const std::string & filename);
	~CBitmapFont();

	size_t getFontAscentScaled() const override;
	size_t getLineHeightScaled() const override;
	size_t getGlyphWidthScaled(const char * data) const override;

	/// returns true if this font contains provided utf-8 character
	bool canRepresentCharacter(const char * data) const override;
	bool canRepresentString(const std::string & data) const;

	friend class CBitmapHanFont;
	friend class CTrueTypeFont;
};


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

VCMI_LIB_NAMESPACE_BEGIN
class ResourcePath;
VCMI_LIB_NAMESPACE_END

class CBitmapFont : public IFont
{
	using CodePoint = uint32_t;

	struct BitmapChar
	{
		int32_t leftOffset;
		uint32_t width;
		uint32_t height;
		int32_t rightOffset;
		std::vector<uint8_t> pixels; // pixels of this character, part of BitmapFont::data
	};

	std::unordered_map<CodePoint, BitmapChar> chars;
	uint32_t maxHeight;

	void loadModFont(const std::string & modName, const ResourcePath & resource);

	void renderCharacter(SDL_Surface * surface, const BitmapChar & character, const ColorRGBA & color, int &posX, int &posY) const;
	void renderText(SDL_Surface * surface, const std::string & data, const ColorRGBA & color, const Point & pos) const override;
public:
	explicit CBitmapFont(const std::string & filename);

	size_t getLineHeight() const override;
	size_t getGlyphWidth(const char * data) const override;

	/// returns true if this font contains provided utf-8 character
	bool canRepresentCharacter(const char * data) const;
	bool canRepresentString(const std::string & data) const;

	friend class CBitmapHanFont;
	friend class CTrueTypeFont;
};


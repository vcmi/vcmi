/*
 * TextTextureCache.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "lib/Color.h"
#include "lib/Point.h"
#include "gui/TextAlignment.h"

class SDLImageShared;
enum EFonts : int8_t;

/// Rendered strings kept as images so they can be drawn onto a GPU layer - the font stack
/// writes glyphs into a surface, which a render target cannot accept.
class TextTextureCache
{
	struct Key
	{
		int font;
		uint32_t color;
		std::string text;

		auto operator<=>(const Key & other) const = default;
	};

	struct Entry
	{
		Key key;
		std::shared_ptr<SDLImageShared> image;
	};

	/// Most recently used first, so the tail is what gets dropped
	std::list<Entry> order;
	std::map<Key, std::list<Entry>::iterator> entries;

public:
	static TextTextureCache & get();

	/// Image holding the rendered string, or null if it could not be produced.
	/// Its top left corner is the string's top left, so callers apply alignment themselves.
	std::shared_ptr<SDLImageShared> getImage(EFonts font, const ColorRGBA & color, const std::string & text);

	/// Offset from the requested position to the string's top left, in scaled pixels
	static Point getAlignmentOffset(EFonts font, ETextAlignment alignment, const std::string & text);

	void clear();
};

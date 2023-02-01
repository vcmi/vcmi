/*
 * CAnimation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"

#ifdef IN
#undef IN
#endif

#ifdef OUT
#undef OUT
#endif

VCMI_LIB_NAMESPACE_BEGIN

class JsonNode;
class Rect;
class Point;

VCMI_LIB_NAMESPACE_END

struct SDL_Surface;
struct SDL_Color;
class CDefFile;
class ColorFilter;


/// Class for def loading
/// After loading will store general info (palette and frame offsets) and pointer to file itself
class CDefFile
{
private:

	PACKED_STRUCT_BEGIN
	struct SSpriteDef
	{
		ui32 size;
		ui32 format;    /// format in which pixel data is stored
		ui32 fullWidth; /// full width and height of frame, including borders
		ui32 fullHeight;
		ui32 width;     /// width and height of pixel data, borders excluded
		ui32 height;
		si32 leftMargin;
		si32 topMargin;
	} PACKED_STRUCT_END;
	//offset[group][frame] - offset of frame data in file
	std::map<size_t, std::vector <size_t> > offset;

	std::unique_ptr<ui8[]>       data;
	std::unique_ptr<SDL_Color[]> palette;

public:
	CDefFile(std::string Name);
	~CDefFile();

	//load frame as SDL_Surface
	template<class ImageLoader>
	void loadFrame(size_t frame, size_t group, ImageLoader &loader) const;

	const std::map<size_t, size_t> getEntries() const;
};



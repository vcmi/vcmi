/*
 * ColorFilter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN
class JsonNode;
class ColorRGBA;
VCMI_LIB_NAMESPACE_END

/// Base class for applying palette transformation on images
class ColorFilter
{
	struct ChannelMuxer {
		float r, g, b, a;
	};

	ChannelMuxer r;
	ChannelMuxer g;
	ChannelMuxer b;
	float a;

	ColorFilter(ChannelMuxer r, ChannelMuxer g, ChannelMuxer b, float a):
		r(r), g(g), b(b), a(a)
	{}
public:
	ColorRGBA shiftColor(const ColorRGBA & in) const;

	bool operator == (const ColorFilter & other) const;
	bool operator != (const ColorFilter & other) const;

	/// Generates empty object that has no effect on image
	static ColorFilter genEmptyShifter();

	/// Generates object that changes alpha (transparency) of the image
	static ColorFilter genAlphaShifter( float alpha );

	/// Generates object that transforms each channel independently
	static ColorFilter genRangeShifter( float minR, float minG, float minB, float maxR, float maxG, float maxB );

	/// Generates object that performs arbitrary mixing between any channels
	static ColorFilter genMuxerShifter( ChannelMuxer r, ChannelMuxer g, ChannelMuxer b, float a );

	/// Combines 2 mixers into a single object
	static ColorFilter genCombined(const ColorFilter & left, const ColorFilter & right);

	/// Scales down strength of a shifter to a specified factor
	static ColorFilter genInterpolated(const ColorFilter & left, const ColorFilter & right, float power);

	/// Generates object using supplied Json config
	static ColorFilter genFromJson(const JsonNode & entry);
};

struct ColorMuxerEffect
{
	std::vector<ColorFilter> filters;
	std::vector<float> timePoints;
};

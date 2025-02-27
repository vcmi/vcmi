/*
 * Canvas.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "ColorFilter.h"

#include "../../lib/Color.h"
#include "../../lib/json/JsonNode.h"

ColorRGBA ColorFilter::shiftColor(const ColorRGBA & in) const
{
	int r_out = in.r * r.r + in.g * r.g + in.b * r.b + 255 * r.a;
	int g_out = in.r * g.r + in.g * g.g + in.b * g.b + 255 * g.a;
	int b_out = in.r * b.r + in.g * b.g + in.b * b.b + 255 * b.a;
	int a_out = in.a * a;

	vstd::abetween(r_out, 0, 255);
	vstd::abetween(g_out, 0, 255);
	vstd::abetween(b_out, 0, 255);
	vstd::abetween(a_out, 0, 255);

	return {
		static_cast<uint8_t>(r_out),
		static_cast<uint8_t>(g_out),
		static_cast<uint8_t>(b_out),
		static_cast<uint8_t>(a_out)
	};
}

bool ColorFilter::operator != (const ColorFilter & other) const
{
	return !(this->operator==(other));
}

bool ColorFilter::operator == (const ColorFilter & other) const
{
	return
		vstd::isAlmostEqual(r.r, other.r.r) && vstd::isAlmostEqual(r.g, other.r.g) && vstd::isAlmostEqual(r.b, other.r.b) && vstd::isAlmostEqual(r.a, other.r.a) &&
		vstd::isAlmostEqual(g.r, other.g.r) && vstd::isAlmostEqual(g.g, other.g.g) && vstd::isAlmostEqual(g.b, other.g.b) && vstd::isAlmostEqual(g.a, other.g.a) &&
		vstd::isAlmostEqual(b.r, other.b.r) && vstd::isAlmostEqual(b.g, other.b.g) && vstd::isAlmostEqual(b.b, other.b.b) && vstd::isAlmostEqual(b.a, other.b.a) &&
		vstd::isAlmostEqual(a, other.a);
}

ColorFilter ColorFilter::genEmptyShifter( )
{
	return genAlphaShifter( 1.f);
}

ColorFilter ColorFilter::genAlphaShifter( float alpha )
{
	return genMuxerShifter(
				{ 1.f, 0.f, 0.f, 0.f },
				{ 0.f, 1.f, 0.f, 0.f },
				{ 0.f, 0.f, 1.f, 0.f },
				alpha);
}

ColorFilter ColorFilter::genRangeShifter( float minR, float minG, float minB, float maxR, float maxG, float maxB )
{
	return genMuxerShifter(
				{ maxR - minR, 0.f, 0.f, minR },
				{ 0.f, maxG - minG, 0.f, minG },
				{ 0.f, 0.f, maxB - minB, minB },
				  1.f);
}

ColorFilter ColorFilter::genRangeShifter( std::vector<float> parameters )
{
	assert(std::size(parameters) == 6);

	return genRangeShifter(parameters[0], parameters[1], parameters[2], parameters[3], parameters[4], parameters[5]);
}

ColorFilter ColorFilter::genMuxerShifter( ChannelMuxer r, ChannelMuxer g, ChannelMuxer b, float a )
{
	return ColorFilter(r, g, b, a);
}

ColorFilter ColorFilter::genInterpolated(const ColorFilter & left, const ColorFilter & right, float power)
{
	auto lerpMuxer = [=]( const ChannelMuxer & left, const ChannelMuxer & right ) -> ChannelMuxer
	{
		return {
			vstd::lerp(left.r, right.r, power),
			vstd::lerp(left.g, right.g, power),
			vstd::lerp(left.b, right.b, power),
			vstd::lerp(left.a, right.a, power)
		};
	};

	return genMuxerShifter(
		lerpMuxer(left.r, right.r),
		lerpMuxer(left.g, right.g),
		lerpMuxer(left.b, right.b),
		vstd::lerp(left.a, right.a, power)
	);
}

ColorFilter ColorFilter::genCombined(const ColorFilter & left, const ColorFilter & right)
{
	// matrix multiplication
	ChannelMuxer r{
		left.r.r * right.r.r + left.g.r * right.r.g + left.b.r * right.r.b,
		left.r.g * right.r.r + left.g.g * right.r.g + left.b.g * right.r.b,
		left.r.b * right.r.r + left.g.b * right.r.g + left.b.b * right.r.b,
		left.r.a * right.r.r + left.g.a * right.r.g + left.b.a * right.r.b + 1.f * right.r.a,
	};

	ChannelMuxer g{
		left.r.r * right.g.r + left.g.r * right.g.g + left.b.r * right.g.b,
		left.r.g * right.g.r + left.g.g * right.g.g + left.b.g * right.g.b,
		left.r.b * right.g.r + left.g.b * right.g.g + left.b.b * right.g.b,
		left.r.a * right.g.r + left.g.a * right.g.g + left.b.a * right.g.b + 1.f * right.g.a,
	};

	ChannelMuxer b{
		left.r.r * right.b.r + left.g.r * right.b.g + left.b.r * right.b.b,
		left.r.g * right.b.r + left.g.g * right.b.g + left.b.g * right.b.b,
		left.r.b * right.b.r + left.g.b * right.b.g + left.b.b * right.b.b,
		left.r.a * right.b.r + left.g.a * right.b.g + left.b.a * right.b.b + 1.f * right.b.a,
	};

	float a = left.a * right.a;
	return genMuxerShifter(r,g,b,a);
}

/*
 * Color.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

/// An object that represents RGBA color
class ColorRGBA
{
public:
	enum : uint8_t
	{
		ALPHA_OPAQUE = 255,
		ALPHA_TRANSPARENT = 0,
	};

	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;

	//constructors
	constexpr ColorRGBA()
		:r(0)
		,g(0)
		,b(0)
		,a(0)
	{
	}

	constexpr ColorRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
		: r(r)
		, g(g)
		, b(b)
		, a(a)
	{}

	constexpr ColorRGBA(uint8_t r, uint8_t g, uint8_t b)
		: r(r)
		, g(g)
		, b(b)
		, a(ALPHA_OPAQUE)
	{}

	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & r;
		h & g;
		h & b;
		h & a;
	}
};

VCMI_LIB_NAMESPACE_END

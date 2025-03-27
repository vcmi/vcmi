/*
 * IFont.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Colors.h"

#include "../../lib/json/JsonNode.h"

#include <boost/algorithm/hex.hpp>

const ColorRGBA Colors::YELLOW = { 229, 215, 123, ColorRGBA::ALPHA_OPAQUE };
const ColorRGBA Colors::WHITE = { 255, 243, 222, ColorRGBA::ALPHA_OPAQUE };
const ColorRGBA Colors::WHITE_TRUE = { 255, 255, 255, ColorRGBA::ALPHA_OPAQUE };
const ColorRGBA Colors::METALLIC_GOLD = { 173, 142, 66, ColorRGBA::ALPHA_OPAQUE };
const ColorRGBA Colors::GREEN = { 0, 255, 0, ColorRGBA::ALPHA_OPAQUE };
const ColorRGBA Colors::CYAN = { 0, 255, 255, ColorRGBA::ALPHA_OPAQUE };
const ColorRGBA Colors::ORANGE = { 232, 184, 32, ColorRGBA::ALPHA_OPAQUE };
const ColorRGBA Colors::BRIGHT_YELLOW = { 242, 226, 110, ColorRGBA::ALPHA_OPAQUE };
const ColorRGBA Colors::DEFAULT_KEY_COLOR = {0, 255, 255, ColorRGBA::ALPHA_OPAQUE};
const ColorRGBA Colors::RED = {255, 0, 0, ColorRGBA::ALPHA_OPAQUE};
const ColorRGBA Colors::PURPLE = {255, 75, 125, ColorRGBA::ALPHA_OPAQUE};
const ColorRGBA Colors::BLACK = {0, 0, 0, ColorRGBA::ALPHA_OPAQUE};
const ColorRGBA Colors::TRANSPARENCY = {0, 0, 0, ColorRGBA::ALPHA_TRANSPARENT};

std::optional<ColorRGBA> Colors::parseColor(std::string text)
{
	std::smatch match;
	std::regex expr("^#([0-9a-fA-F]{6})$");
	std::vector<ui8> rgb;
	rgb.reserve(3);
	if(std::regex_search(text, match, expr))
	{
		boost::algorithm::unhex(match[1].str(), std::back_inserter(rgb)); 
		return ColorRGBA(rgb[0], rgb[1], rgb[2]);
	}

	static const JsonNode config(JsonPath::builtin("CONFIG/textColors"));
	auto colors = config["colors"].Struct();
	for(auto & color : colors) {
		if(boost::algorithm::to_lower_copy(color.first) == boost::algorithm::to_lower_copy(text))
		{
			boost::algorithm::unhex(color.second.String(), std::back_inserter(rgb)); 
			return ColorRGBA(rgb[0], rgb[1], rgb[2]);
		}
	}

	return std::nullopt;
}

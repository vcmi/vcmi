/*
 * ControllerPromptFamily.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/Color.h"

namespace ControllerPrompt
{

enum class Family
{
	UNKNOWN,
	GENERIC,
	NINTENDO,
	PLAYSTATION,
	XBOX
};

enum class State
{
	NORMAL,
	PRESSED,
	DISABLED
};

inline std::string stateSuffix(State state)
{
	switch(state)
	{
	case State::NORMAL: return "normal";
	case State::PRESSED: return "pressed";
	case State::DISABLED: return "disabled";
	}
	return "normal";
}

inline std::string genericFaceSprite(State state)
{
	return "controllerActionBar/generic-face-" + stateSuffix(state) + ".png";
}

constexpr ColorRGBA runtimeFaceLabelColor(State state)
{
	return state == State::DISABLED
		? ColorRGBA(115, 105, 92, 110)
		: ColorRGBA(58, 40, 20, 255);
}

inline bool usesRuntimeFaceLabel(Family family)
{
	return family == Family::GENERIC || family == Family::NINTENDO || family == Family::XBOX;
}

inline bool isFaceButtonBinding(const std::string & binding)
{
	return binding == "a" || binding == "b" || binding == "x" || binding == "y";
}

inline std::string buttonLabel(Family family, const std::string & binding)
{
	struct ButtonLabel
	{
		const char * binding;
		const char * playstation;
		const char * nintendo;
		const char * generic;
	};
	static constexpr std::array<ButtonLabel, 8> labels = {{
		{"a", "×", "B", "A"},
		{"b", "○", "A", "B"},
		{"x", "□", "Y", "X"},
		{"y", "△", "X", "Y"},
		{"leftshoulder", "L1", "L", "LB"},
		{"rightshoulder", "R1", "R", "RB"},
		{"lefttrigger", "L2", "ZL", "LT"},
		{"righttrigger", "R2", "ZR", "RT"}
	}};
	for(const auto & label : labels)
	{
		if(binding != label.binding)
			continue;
		switch(family)
		{
		case Family::PLAYSTATION: return label.playstation;
		case Family::NINTENDO: return label.nintendo;
		case Family::GENERIC:
		case Family::XBOX: return label.generic;
		default: break;
		}
		break;
	}

	std::string result = binding;
	std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character)
	{
		return static_cast<char>(std::toupper(character));
	});
	return result;
}

inline std::optional<std::string> faceButtonSprite(Family family, const std::string & binding, State state)
{
	if(!isFaceButtonBinding(binding))
		return std::nullopt;
	if(usesRuntimeFaceLabel(family))
		return genericFaceSprite(state);
	if(family != Family::PLAYSTATION)
		return std::nullopt;
	return "controllerActionBar/playstation-" + binding + "-" + stateSuffix(state) + ".png";
}

}

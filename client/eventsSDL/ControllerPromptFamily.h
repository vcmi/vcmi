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
	if(family == Family::PLAYSTATION)
	{
		if(binding == "a") return "×";
		if(binding == "b") return "○";
		if(binding == "x") return "□";
		if(binding == "y") return "△";
		if(binding == "leftshoulder") return "L1";
		if(binding == "rightshoulder") return "R1";
		if(binding == "lefttrigger") return "L2";
		if(binding == "righttrigger") return "R2";
	}
	else if(family == Family::NINTENDO)
	{
		if(binding == "a") return "B";
		if(binding == "b") return "A";
		if(binding == "x") return "Y";
		if(binding == "y") return "X";
		if(binding == "leftshoulder") return "L";
		if(binding == "rightshoulder") return "R";
		if(binding == "lefttrigger") return "ZL";
		if(binding == "righttrigger") return "ZR";
	}
	else if(family == Family::GENERIC || family == Family::XBOX)
	{
		if(binding == "a") return "A";
		if(binding == "b") return "B";
		if(binding == "x") return "X";
		if(binding == "y") return "Y";
		if(binding == "leftshoulder") return "LB";
		if(binding == "rightshoulder") return "RB";
		if(binding == "lefttrigger") return "LT";
		if(binding == "righttrigger") return "RT";
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
	if(family != Family::PLAYSTATION || (binding != "a" && binding != "b"))
		return std::nullopt;
	return "controllerActionBar/playstation-" + binding + "-" + stateSuffix(state) + ".png";
}

}

/*
 * constants/StringConstants.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameConstants.h"

///
/// String ID which are pointless to move to config file - these types are mostly hardcoded
///
/// Defined in StringConstants.cpp - keeping the definitions out of the header avoids
/// every including translation unit building its own copy at startup.
///
namespace GameConstants
{
	extern DLL_LINKAGE const std::string RESOURCE_NAMES [RESOURCE_QUANTITY];

	extern DLL_LINKAGE const std::string PLAYER_COLOR_NAMES [PlayerColor::PLAYER_LIMIT_I];

	extern DLL_LINKAGE const std::string ALIGNMENT_NAMES [4];

	extern DLL_LINKAGE const std::string DIFFICULTY_NAMES [5];
}

namespace NPrimarySkill
{
	extern DLL_LINKAGE const std::string names [GameConstants::PRIMARY_SKILLS];
}

namespace NSecondarySkill
{
	extern DLL_LINKAGE const std::string names [GameConstants::SKILL_QUANTITY];

	extern DLL_LINKAGE const std::vector<std::string> levels;
}

namespace EBuildingType
{
	extern DLL_LINKAGE const std::string names [46];
}

namespace NFaction
{
	extern DLL_LINKAGE const std::string names [GameConstants::F_NUMBER];
}

namespace NArtifactPosition
{
	inline constexpr std::array namesHero =
	{
		"head", "shoulders", "neck", "rightHand", "leftHand", "torso", //5
		"rightRing", "leftRing", "feet", //8
		"misc1", "misc2", "misc3", "misc4", //12
		"mach1", "mach2", "mach3", "mach4", //16
		"spellbook", "misc5" //18
	};

	inline constexpr std::array namesCreature =
	{
		"creature1"
	};

	inline constexpr std::array namesCommander =
	{
		"commander1", "commander2", "commander3", "commander4", "commander5", "commander6", "commander7", "commander8", "commander9"
	};


	extern DLL_LINKAGE const std::string backpack;
}

namespace NPathfindingLayer
{
	extern DLL_LINKAGE const std::string names[EPathfindingLayer::NUM_LAYERS];
}

namespace MappedKeys
{
	extern DLL_LINKAGE const std::map<std::string, BuildingSubID::EBuildingSubID> SPECIAL_BUILDINGS;

	extern DLL_LINKAGE const std::map<std::string, EMarketMode> MARKET_NAMES_TO_TYPES;
}

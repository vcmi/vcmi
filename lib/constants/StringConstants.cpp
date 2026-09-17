/*
 * constants/StringConstants.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "StringConstants.h"

namespace MappedKeys
{
	const std::map<std::string, BuildingSubID::EBuildingSubID> SPECIAL_BUILDINGS =
	{
		{ "mysticPond", BuildingSubID::MYSTIC_POND },
		{ "castleGate", BuildingSubID::CASTLE_GATE },
		{ "portalOfSummoning", BuildingSubID::PORTAL_OF_SUMMONING },
		{ "library", BuildingSubID::LIBRARY },
		{ "treasury", BuildingSubID::TREASURY },
		{ "bank", BuildingSubID::BANK },
		{ "auroraBorealis", BuildingSubID::AURORA_BOREALIS }
	};

	const std::map<std::string, EMarketMode> MARKET_NAMES_TO_TYPES =
	{
		{ "resource-resource", EMarketMode::RESOURCE_RESOURCE },
		{ "resource-player", EMarketMode::RESOURCE_PLAYER },
		{ "creature-resource", EMarketMode::CREATURE_RESOURCE },
		{ "resource-artifact", EMarketMode::RESOURCE_ARTIFACT },
		{ "artifact-resource", EMarketMode::ARTIFACT_RESOURCE },
		{ "artifact-experience", EMarketMode::ARTIFACT_EXP },
		{ "creature-experience", EMarketMode::CREATURE_EXP },
		{ "creature-undead", EMarketMode::CREATURE_UNDEAD },
		{ "resource-skill", EMarketMode::RESOURCE_SKILL },
	};
}

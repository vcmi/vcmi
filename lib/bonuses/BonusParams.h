/*
 * BonusParams.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Bonus.h"

#include "../GameConstants.h"
#include "../JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE BonusParams {
	bool isConverted;
	BonusType type = BonusType::NONE;
	std::optional<TBonusSubtype> subtype = std::nullopt;
	std::optional<std::string> subtypeStr = std::nullopt;
	std::optional<BonusValueType> valueType = std::nullopt;
	std::optional<si32> val = std::nullopt;
	std::optional<BonusSource> targetType = std::nullopt;

	BonusParams(bool isConverted = true) : isConverted(isConverted) {};
	BonusParams(std::string deprecatedTypeStr, std::string deprecatedSubtypeStr = "", int deprecatedSubtype = 0);
	const JsonNode & toJson();
	CSelector toSelector();
private:
	JsonNode ret;
	bool jsonCreated = false;
};

extern DLL_LINKAGE const std::set<std::string> deprecatedBonusSet;

VCMI_LIB_NAMESPACE_END
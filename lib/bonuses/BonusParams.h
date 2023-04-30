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

#include "HeroBonus.h"

#include "../GameConstants.h"
#include "../JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE BonusParams {
	bool isConverted;
	Bonus::BonusType type = Bonus::NONE;
	TBonusSubtype subtype = -1;
	std::string subtypeStr;
	bool subtypeRelevant = false;
	Bonus::ValueType valueType = Bonus::BASE_NUMBER;
	bool valueTypeRelevant = false;
	si32 val = 0;
	bool valRelevant = false;
	Bonus::BonusSource targetType = Bonus::SECONDARY_SKILL;
	bool targetTypeRelevant = false;

	BonusParams(bool isConverted = true) : isConverted(isConverted) {};
	BonusParams(std::string deprecatedTypeStr, std::string deprecatedSubtypeStr = "", int deprecatedSubtype = 0);
	const JsonNode & toJson();
	CSelector toSelector();
private:
	JsonNode ret;
	bool jsonCreated = false;
};

VCMI_LIB_NAMESPACE_END
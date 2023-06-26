/*
 * CampaignConstants.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class CampaignVersion : uint8_t
{
	NONE = 0,
	RoE = 4,
	AB = 5,
	SoD = 6,
	WoG = 6,
	//		Chr = 7, // Heroes Chronicles, likely identical to SoD, untested

	VCMI = 1,
	VCMI_MIN = 1,
	VCMI_MAX = 1,
};

enum class CampaignStartOptions: int8_t
{
	NONE = 0,
	START_BONUS,
	HERO_CROSSOVER,
	HERO_OPTIONS
};

enum class CampaignBonusType : int8_t
{
	NONE = -1,
	SPELL,
	MONSTER,
	BUILDING,
	ARTIFACT,
	SPELL_SCROLL,
	PRIMARY_SKILL,
	SECONDARY_SKILL,
	RESOURCE,
	HEROES_FROM_PREVIOUS_SCENARIO,
	HERO
};

enum class CampaignScenarioID : int8_t
{
	NONE = -1,
	// no members - fake enum to create integer type that is not implicitly convertible to int
};

VCMI_LIB_NAMESPACE_END

/*
 * PandoraBoxConfig.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../mapObjects/army/CStackBasicDescriptor.h"
#include "../rewardable/Reward.h"

class JsonSerializeFormat;

struct DLL_LINKAGE PandoraBoxConfig
{
	struct RandomSpells
	{
		ui16 count = 0;
		std::optional<ui8> level;
		std::optional<SpellSchool> school;
	};

	struct NativeCreatures
	{
		ui8 level = 0;
		std::optional<ui32> amount;
	};

	Rewardable::Reward reward;
	std::optional<RandomSpells> randomSpells;
	std::optional<NativeCreatures> nativeCreatures;
	std::optional<ui32> value;
	std::optional<ui16> rarity;
	ui32 zoneLimit = 0;
	std::set<FactionID> zoneFactions;

	bool isAllowedForFaction(FactionID zoneFaction) const;
	ui16 getRarity() const;
	/// Explicit value, or the default value from the standard parts. nativeCreaturesValue comes from TreasurePlacer (0 if none).
	template<typename GeneratorConfig>
	ui32 getValue(const GeneratorConfig & config, ui32 nativeCreaturesValue) const
	{
		return getValueFromMultipliers(
			config.pandoraMultiplierGold,
			config.pandoraMultiplierExperience,
			config.pandoraMultiplierSpells,
			config.pandoraSpellSchool,
			config.pandoraSpell60,
			nativeCreaturesValue);
	}
	void serializeJson(JsonSerializeFormat & handler);

private:
	void loadReward(JsonSerializeFormat & handler);
	ui32 getValueFromMultipliers(
		int pandoraMultiplierGold,
		int pandoraMultiplierExperience,
		int pandoraMultiplierSpells,
		int pandoraSpellSchool,
		int pandoraSpell60,
		ui32 nativeCreaturesValue) const;
};

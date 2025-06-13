/*
 * CampaignBonus.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "CampaignConstants.h"
#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class CBinaryReader;
class MapIdentifiersH3M;
class JsonNode;

struct CampaignBonusSpell
{
	HeroTypeID hero;
	SpellID spell;

	template <typename Handler> void serialize(Handler &h)
	{
		h & hero;
		h & spell;
	}
};

struct CampaignBonusCreatures
{
	HeroTypeID hero;
	CreatureID creature;
	int32_t amount = 0;

	template <typename Handler> void serialize(Handler &h)
	{
		h & hero;
		h & creature;
		h & amount;
	}
};

struct CampaignBonusBuilding
{
	BuildingID buildingH3M;
	BuildingID buildingDecoded;

	template <typename Handler> void serialize(Handler &h)
	{
		h & buildingH3M;
		h & buildingDecoded;
	}
};

struct CampaignBonusArtifact
{
	HeroTypeID hero;
	ArtifactID artifact;

	template <typename Handler> void serialize(Handler &h)
	{
		h & hero;
		h & artifact;
	}
};

struct CampaignBonusSpellScroll
{
	HeroTypeID hero;
	SpellID spell;

	template <typename Handler> void serialize(Handler &h)
	{
		h & hero;
		h & spell;
	}
};

struct CampaignBonusPrimarySkill
{
	HeroTypeID hero;
	std::array<uint8_t, 4> amounts = {};

	template <typename Handler> void serialize(Handler &h)
	{
		h & hero;
		h & amounts;
	}
};

struct CampaignBonusSecondarySkill
{
	HeroTypeID hero;
	SecondarySkill skill;
	int32_t mastery = 0;

	template <typename Handler> void serialize(Handler &h)
	{
		h & hero;
		h & skill;
		h & mastery;
	}
};

struct CampaignBonusStartingResources
{
	GameResID resource;
	int32_t amount = 0;

	template <typename Handler> void serialize(Handler &h)
	{
		h & resource;
		h & amount;
	}
};

struct CampaignBonusHeroesFromScenario
{
	PlayerColor startingPlayer;
	CampaignScenarioID scenario;

	template <typename Handler> void serialize(Handler &h)
	{
		h & startingPlayer;
		h & scenario;
	}
};

struct CampaignBonusStartingHero
{
	PlayerColor startingPlayer;
	HeroTypeID hero;

	template <typename Handler> void serialize(Handler &h)
	{
		h & startingPlayer;
		h & hero;
	}
};

class CampaignBonus
{
	using Variant = std::variant<
		CampaignBonusSpell, // UNTESTED
		CampaignBonusCreatures,
		CampaignBonusBuilding, // UNTESTED - broken, Long Live the King, Liberation last
		CampaignBonusArtifact,
		CampaignBonusSpellScroll,
		CampaignBonusPrimarySkill,
		CampaignBonusSecondarySkill,
		CampaignBonusStartingResources,
		CampaignBonusHeroesFromScenario,
		CampaignBonusStartingHero>;

	Variant data;

	template<typename T, typename = void>
	struct HasHero : std::false_type { };

	template<typename T>
	struct HasHero<T, decltype(std::declval<T>().hero, void())> : std::true_type { };

public:
	CampaignBonus() = default;

	template<typename BonusType>
	CampaignBonus(const BonusType & value)
		:data(value)
	{}

	DLL_LINKAGE CampaignBonus(CBinaryReader & reader, const MapIdentifiersH3M & remapper, CampaignStartOptions mode);
	DLL_LINKAGE CampaignBonus(const JsonNode & json, CampaignStartOptions mode);

	template<typename T>
	const T & getValue() const
	{
		return std::get<T>(data);
	}

	HeroTypeID getTargetedHero() const
	{
		HeroTypeID result;

		std::visit([&result](const auto & bonusValue){
			if constexpr (HasHero<decltype(bonusValue)>::value)
			{
				result = bonusValue.hero;
			}
			else
				throw std::runtime_error("Attempt to get targeted hero on invalid type!");
		}, data);

		return result;
	}

	bool isBonusForHero() const
	{
		auto type = getType();
		return type == CampaignBonusType::SPELL ||
			   type == CampaignBonusType::MONSTER ||
			   type == CampaignBonusType::ARTIFACT ||
			   type == CampaignBonusType::SPELL_SCROLL ||
			   type == CampaignBonusType::PRIMARY_SKILL ||
			   type == CampaignBonusType::SECONDARY_SKILL;
	}

	CampaignBonusType getType() const
	{
		return static_cast<CampaignBonusType>(data.index());
	}

	DLL_LINKAGE JsonNode toJson() const;

	template <typename Handler> void serialize(Handler &h)
	{
		h & data;
	}
};

VCMI_LIB_NAMESPACE_END

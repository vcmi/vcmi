/*
 * CampaignBonus.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CampaignBonus.h"

#include "../filesystem/CBinaryReader.h"
#include "../mapping/MapIdentifiersH3M.h"
#include "../json/JsonNode.h"
#include "../constants/StringConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

static const std::map<std::string, CampaignBonusType> bonusTypeMap = {
	{"spell", CampaignBonusType::SPELL},
	{"creature", CampaignBonusType::MONSTER},
	{"building", CampaignBonusType::BUILDING},
	{"artifact", CampaignBonusType::ARTIFACT},
	{"scroll", CampaignBonusType::SPELL_SCROLL},
	{"primarySkill", CampaignBonusType::PRIMARY_SKILL},
	{"secondarySkill", CampaignBonusType::SECONDARY_SKILL},
	{"resource", CampaignBonusType::RESOURCE},
	{"prevHero", CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO},
	{"hero", CampaignBonusType::HERO},
};

static const std::map<std::string, ui16> heroSpecialMap = {
	{"strongest", HeroTypeID::CAMP_STRONGEST},
	{"generated", HeroTypeID::CAMP_GENERATED},
	{"random", HeroTypeID::CAMP_RANDOM}
};

static const std::map<std::string, ui8> resourceTypeMap = {
	{"wood", EGameResID::WOOD},
	{"mercury", EGameResID::MERCURY},
	{"ore", EGameResID::ORE},
	{"sulfur", EGameResID::SULFUR},
	{"crystal", EGameResID::CRYSTAL},
	{"gems", EGameResID::GEMS},
	{"gold", EGameResID::GOLD},
	{"common", EGameResID::COMMON},
	{"rare", EGameResID::RARE}
};

CampaignBonus::CampaignBonus(CBinaryReader & reader, const MapIdentifiersH3M & remapper, CampaignStartOptions mode)
{
	switch(mode)
	{
		case CampaignStartOptions::NONE:
			// in this mode game should not attempt to create instances of bonuses
			throw std::runtime_error("Attempt to create empty campaign bonus");

		case CampaignStartOptions::START_BONUS:
		{
			auto bonusType = static_cast<CampaignBonusType>(reader.readUInt8());

			switch(bonusType)
			{
				case CampaignBonusType::SPELL:
				{
					HeroTypeID hero(reader.readInt16());
					SpellID spell(reader.readUInt8());
					data = CampaignBonusSpell{remapper.remap(hero), spell};
					break;
				}
				case CampaignBonusType::MONSTER:
				{
					HeroTypeID hero(reader.readInt16());
					CreatureID creature(reader.readUInt16());
					int32_t amount = reader.readUInt16();
					data = CampaignBonusCreatures{remapper.remap(hero), remapper.remap(creature), amount};
					break;
				}
				case CampaignBonusType::BUILDING:
				{
					BuildingID building(reader.readUInt8());
					data = CampaignBonusBuilding{building, remapper.remapBuilding(std::nullopt, building)};
					break;
				}
				case CampaignBonusType::ARTIFACT:
				{
					HeroTypeID hero(reader.readInt16());
					ArtifactID artifact(reader.readUInt16());
					data = CampaignBonusArtifact{remapper.remap(hero), remapper.remap(artifact)};
					break;
				}
				case CampaignBonusType::SPELL_SCROLL:
				{
					HeroTypeID hero(reader.readInt16());
					SpellID spell(reader.readUInt8());
					data = CampaignBonusSpellScroll{remapper.remap(hero), spell};
					break;
				}
				case CampaignBonusType::PRIMARY_SKILL:
				{
					HeroTypeID hero(reader.readInt16());
					std::array<uint8_t, 4> amounts = {};
					for(auto & value : amounts)
						value = reader.readUInt8();

					data = CampaignBonusPrimarySkill{remapper.remap(hero), amounts};
					break;
				}
				case CampaignBonusType::SECONDARY_SKILL:
				{
					HeroTypeID hero(reader.readInt16());
					SecondarySkill skill(reader.readUInt8());
					int32_t skillMastery(reader.readUInt8());
					data = CampaignBonusSecondarySkill{remapper.remap(hero), remapper.remap(skill), skillMastery};
					break;
				}
				case CampaignBonusType::RESOURCE:
				{
					GameResID resource(reader.readInt8());
					int32_t amount(reader.readInt32());
					data = CampaignBonusStartingResources{resource, amount};
					break;
				}
				default:
					throw std::runtime_error("Corrupted or unsupported h3c file");
			}
			break;
		}
		case CampaignStartOptions::HERO_CROSSOVER: //reading of players (colors / scenarios ?) player can choose
		{
			PlayerColor player(reader.readUInt8());
			CampaignScenarioID scenario(reader.readUInt8());
			data = CampaignBonusHeroesFromScenario{player, scenario};
			break;
		}
		case CampaignStartOptions::HERO_OPTIONS: //heroes player can choose between
		{
			PlayerColor player(reader.readUInt8());
			HeroTypeID hero(reader.readInt16());
			data = CampaignBonusStartingHero{player, remapper.remap(hero)};
			break;
		}
		default:
		{
			throw std::runtime_error("Corrupted or unsupported h3c file");
		}
	}
}

CampaignBonus::CampaignBonus(const JsonNode & bjson, CampaignStartOptions mode)
{
	CampaignBonusType bonusType;

	if (!bjson["what"].isNull())
	{
		bonusType = bonusTypeMap.at(bjson["what"].String());
	}
	else if (mode == CampaignStartOptions::HERO_CROSSOVER)
	{
		bonusType = CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO;
	}
	else if (mode == CampaignStartOptions::HERO_OPTIONS)
	{
		bonusType = CampaignBonusType::HERO;
	}
	else
	{
		throw std::runtime_error("Corrupted or unsupported vcmp file");
	}

	const auto & decodeHeroTypeID = [](JsonNode heroType) -> HeroTypeID
	{
		if(vstd::contains(heroSpecialMap, heroType.String()))
			return HeroTypeID(heroSpecialMap.at(heroType.String()));
		else
			return HeroTypeID(HeroTypeID::decode(heroType.String()));
	};

	switch(bonusType)
	{
		case CampaignBonusType::SPELL:
		{
			HeroTypeID hero(decodeHeroTypeID(bjson["hero"]));
			SpellID spell(SpellID::decode(bjson["spellType"].String()));
			data = CampaignBonusSpell{hero, spell};
			break;
		}
		case CampaignBonusType::MONSTER:
		{
			HeroTypeID hero(decodeHeroTypeID(bjson["hero"]));
			CreatureID creature(CreatureID::decode(bjson["creatureType"].String()));
			int32_t amount = bjson["creatureAmount"].Integer();
			data = CampaignBonusCreatures{hero, creature, amount};
			break;
		}
		case CampaignBonusType::BUILDING:
		{
			BuildingID building(vstd::find_pos(EBuildingType::names, bjson["buildingType"].String()));
			data = CampaignBonusBuilding{building, building};
			break;
		}
		case CampaignBonusType::ARTIFACT:
		{
			HeroTypeID hero(decodeHeroTypeID(bjson["hero"]));
			ArtifactID artifact(ArtifactID::decode(bjson["artifactType"].String()));
			data = CampaignBonusArtifact{hero, artifact};
			break;
		}
		case CampaignBonusType::SPELL_SCROLL:
		{
			HeroTypeID hero(decodeHeroTypeID(bjson["hero"]));
			SpellID spell(SpellID::decode(bjson["spellType"].String()));
			data = CampaignBonusSpellScroll{hero, spell};
			break;
		}
		case CampaignBonusType::PRIMARY_SKILL:
		{
			HeroTypeID hero(decodeHeroTypeID(bjson["hero"]));
			std::array<uint8_t, 4> amounts = {};
			for(size_t i = 0; i < amounts.size(); ++i)
				amounts[i] = bjson[NPrimarySkill::names[i]].Integer();

			data = CampaignBonusPrimarySkill{hero, amounts};
			break;
		}
		case CampaignBonusType::SECONDARY_SKILL:
		{
			HeroTypeID hero(decodeHeroTypeID(bjson["hero"]));
			SecondarySkill skill(SecondarySkill::decode(bjson["skillType"].String()));
			int32_t skillMastery = bjson["skillMastery"].Integer();
			data = CampaignBonusSecondarySkill{hero, skill, skillMastery};
			break;
		}
		case CampaignBonusType::RESOURCE:
		{
			GameResID resource(resourceTypeMap.at(bjson["resourceType"].String()));
			int32_t resourceAmount(bjson["resourceAmount"].Integer());
			data = CampaignBonusStartingResources{resource, resourceAmount};
			break;
		}
		case CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO: //reading of players (colors / scenarios ?) player can choose
		{
			PlayerColor player(SecondarySkill::decode(bjson["playerColor"].String()));
			CampaignScenarioID scenario(bjson["scenario"].Integer());
			data = CampaignBonusHeroesFromScenario{player, scenario};
			break;
		}
		case CampaignBonusType::HERO: //heroes player can choose between
		{
			PlayerColor player(SecondarySkill::decode(bjson["playerColor"].String()));
			HeroTypeID hero(decodeHeroTypeID(bjson["hero"]));
			data = CampaignBonusStartingHero{player, hero};
			break;
		}
		default:
			throw std::runtime_error("Corrupted or unsupported h3c file");
	}
}

JsonNode CampaignBonus::toJson() const
{
	JsonNode bnode;

	const auto & encodeHeroTypeID = [](HeroTypeID heroType) -> JsonNode
	{
		if(vstd::contains(vstd::reverseMap(heroSpecialMap), heroType))
			return JsonNode(vstd::reverseMap(heroSpecialMap)[heroType]);
		else
			return JsonNode(HeroTypeID::encode(heroType));
	};

	bnode["what"].String() = vstd::reverseMap(bonusTypeMap).at(getType());

	switch (getType())
	{
		case CampaignBonusType::SPELL:
		{
			const auto & bonusValue = getValue<CampaignBonusSpell>();
			bnode["hero"] = encodeHeroTypeID(bonusValue.hero);
			bnode["spellType"].String() = SpellID::encode(bonusValue.spell.getNum());
			break;
		}
		case CampaignBonusType::MONSTER:
		{
			const auto & bonusValue = getValue<CampaignBonusCreatures>();
			bnode["hero"] = encodeHeroTypeID(bonusValue.hero);
			bnode["creatureType"].String() = CreatureID::encode(bonusValue.creature.getNum());
			bnode["creatureAmount"].Integer() = bonusValue.amount;
			break;
		}
		case CampaignBonusType::BUILDING:
		{
			const auto & bonusValue = getValue<CampaignBonusBuilding>();
			bnode["buildingType"].String() = EBuildingType::names[bonusValue.buildingDecoded.getNum()];
			break;
		}
		case CampaignBonusType::ARTIFACT:
		{
			const auto & bonusValue = getValue<CampaignBonusArtifact>();
			bnode["hero"] = encodeHeroTypeID(bonusValue.hero);
			bnode["artifactType"].String() = ArtifactID::encode(bonusValue.artifact.getNum());
			break;
		}
		case CampaignBonusType::SPELL_SCROLL:
		{
			const auto & bonusValue = getValue<CampaignBonusSpellScroll>();
			bnode["hero"] = encodeHeroTypeID(bonusValue.hero);
			bnode["spellType"].String() = SpellID::encode(bonusValue.spell.getNum());
			break;
		}
		case CampaignBonusType::PRIMARY_SKILL:
		{
			const auto & bonusValue = getValue<CampaignBonusPrimarySkill>();
			bnode["hero"] = encodeHeroTypeID(bonusValue.hero);
			for(size_t i = 0; i < bonusValue.amounts.size(); ++i)
				bnode[NPrimarySkill::names[i]].Integer() = bonusValue.amounts[i];
			break;
		}
		case CampaignBonusType::SECONDARY_SKILL:
		{
			const auto & bonusValue = getValue<CampaignBonusSecondarySkill>();
			bnode["hero"] = encodeHeroTypeID(bonusValue.hero);
			bnode["skillType"].String() = SecondarySkill::encode(bonusValue.skill.getNum());
			bnode["skillMastery"].Integer() = bonusValue.mastery;
			break;
		}
		case CampaignBonusType::RESOURCE:
		{
			const auto & bonusValue = getValue<CampaignBonusStartingResources>();
			bnode["resourceType"].String() = vstd::reverseMap(resourceTypeMap)[bonusValue.resource.getNum()];
			bnode["resourceAmount"].Integer() = bonusValue.amount;
			break;
		}
		case CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO:
		{
			const auto & bonusValue = getValue<CampaignBonusHeroesFromScenario>();
			bnode["playerColor"].String() = PlayerColor::encode(bonusValue.startingPlayer);
			bnode["scenario"].Integer() = bonusValue.scenario.getNum();
			break;
		}
		case CampaignBonusType::HERO:
		{
			const auto & bonusValue = getValue<CampaignBonusStartingHero>();
			bnode["playerColor"].String() = PlayerColor::encode(bonusValue.startingPlayer);
			bnode["hero"] = encodeHeroTypeID(bonusValue.hero);
			break;
		}
	}
	return bnode;
}

VCMI_LIB_NAMESPACE_END

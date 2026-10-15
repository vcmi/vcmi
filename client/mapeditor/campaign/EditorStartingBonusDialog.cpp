/*
 * EditorStartingBonusDialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorStartingBonusDialog.h"

#include "../EditorTexts.h"

#include "../../widgets/CDropDown.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/CSkillHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/entities/building/CBuilding.h"
#include "../../../lib/entities/faction/CTown.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/spells/CSpellHandler.h"
#include "../../../lib/texts/MetaString.h"

namespace
{
constexpr int LABEL_WIDTH = 140;
constexpr int BONUS_TYPES = 8;
constexpr int MAX_CREATURES = 99999;
constexpr int MAX_PRIMARY_SKILL = 99;
constexpr int MAX_RESOURCES = 999999;
constexpr int SKILL_LEVELS = 3;

const std::array<ArtifactID, 7> EXCLUDED_ARTIFACTS{ArtifactID::SPELL_SCROLL, ArtifactID::SPELLBOOK, ArtifactID::GRAIL, ArtifactID::AMMO_CART, ArtifactID::BALLISTA, ArtifactID::CATAPULT, ArtifactID::FIRST_AID_TENT};

int indexOf(const std::vector<int> & ids, int id)
{
	const auto found = std::ranges::find(ids, id);
	return found == ids.end() ? 0 : static_cast<int>(found - ids.begin());
}

template<class Id>
std::string entityName(const Id & id)
{
	MetaString text;
	text.appendName(id);
	return EditorTexts::translate(text);
}

std::string resourceName(GameResID resource)
{
	return EditorTexts::translate(MetaString::createFromName(resource));
}
}

EditorStartingBonusDialog::EditorStartingBonusDialog(PlayerColor color, const CMap & map, const CampaignBonus & bonus, const std::function<void(const CampaignBonus &)> & onSaved)
	: CDialogWindow(Point(520, 340), EditorTexts::get("bonus.title"))
	, color(color)
	, map(map)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	std::map<int, std::string> heroSelection;
	for(const ObjectInstanceID heroID : map.getHeroesOnMap())
	{
		const auto * hero = dynamic_cast<const CGHeroInstance *>(map.objects.at(heroID.getNum()).get());
		if(hero->getOwner() == color || color == PlayerColor::CANNOT_DETERMINE)
			heroSelection.try_emplace(hero->getHeroTypeID(), EditorTexts::translate(TextIdentifier(hero->getNameTextID())));
	}
	heroSelection.try_emplace(HeroTypeID::CAMP_STRONGEST, EditorTexts::get("bonus.strongest"));
	heroSelection.try_emplace(HeroTypeID::CAMP_GENERATED, EditorTexts::get("bonus.generated"));
	heroSelection.try_emplace(HeroTypeID::CAMP_RANDOM, EditorTexts::get("bonus.random"));
	for(const auto & [id, name] : heroSelection)
	{
		heroIds.push_back(id);
		heroNames.push_back(name);
	}

	const std::vector<std::string> typeNames{EditorTexts::get("bonus.spell"), EditorTexts::get("bonus.creature"), EditorTexts::get("bonus.building"), EditorTexts::get("bonus.artifact"),
		EditorTexts::get("bonus.scroll"), EditorTexts::get("bonus.primarySkill"), EditorTexts::get("bonus.secondarySkill"), EditorTexts::get("bonus.resource")};
	static_assert(BONUS_TYPES == 8);

	load(bonus);

	typeChoice = std::make_shared<CDropDown>(Rect(0, 0, size.x, UiTheme::CONTROL_HEIGHT), typeNames, type, [this](int index)
	{
		type = index;
		buildForm();
	}, EditorTexts::get("bonus.title"));

	form = std::make_shared<CFormView>(Rect(0, UiTheme::CONTROL_HEIGHT + UiTheme::SPACING, size.x, size.y - UiTheme::CONTROL_HEIGHT - UiTheme::SPACING), LABEL_WIDTH);
	buildForm();

	onAccept = [this, onSaved]()
	{
		if(type == static_cast<int>(CampaignBonusType::BUILDING) && !buildingAvailable)
			return false;

		onSaved(save());
		return true;
	};
}

void EditorStartingBonusDialog::addIdChoice(const std::string & label, const std::vector<int> & ids, const std::vector<std::string> & names, int & value)
{
	int * target = &value;
	auto idList = std::make_shared<std::vector<int>>(ids);
	form->addChoice(label, names, indexOf(ids, value), [target, idList](int index)
	{
		*target = idList->at(index);
	});
	// the first option is selected when the value is not listed
	if(!ids.empty() && !vstd::contains(ids, value))
		value = ids.front();
}

void EditorStartingBonusDialog::buildForm()
{
	form->clear();
	buildingAvailable = true;

	switch(static_cast<CampaignBonusType>(type))
	{
		case CampaignBonusType::SPELL:
		case CampaignBonusType::SPELL_SCROLL:
		{
			addIdChoice(EditorTexts::get("bonus.recipient"), heroIds, heroNames, values.hero);

			std::vector<int> ids;
			std::vector<std::string> names;
			for(const auto & spell : LIBRARY->spellh->objects)
			{
				ids.push_back(spell->getIndex());
				names.push_back(spell->getNameTranslated());
			}
			addIdChoice(EditorTexts::get("bonus.spellName"), ids, names, values.spell);
			break;
		}
		case CampaignBonusType::MONSTER:
		{
			addIdChoice(EditorTexts::get("bonus.recipient"), heroIds, heroNames, values.hero);

			std::vector<int> ids;
			std::vector<std::string> names;
			for(const auto & creature : LIBRARY->creh->objects)
			{
				ids.push_back(creature->getIndex());
				names.push_back(creature->getNameSingularTranslated());
			}
			addIdChoice(EditorTexts::get("bonus.creatureType"), ids, names, values.creature);
			form->addNumber(EditorTexts::get("bonus.quantity"), values.amount, 1, MAX_CREATURES, [this](int value) { values.amount = value; });
			break;
		}
		case CampaignBonusType::BUILDING:
		{
			std::vector<int> ids;
			std::vector<std::string> names;
			std::string problem;

			if(map.players.at(color).hasMainTown)
			{
				for(const ObjectInstanceID townID : map.getAllTowns())
				{
					const auto * town = dynamic_cast<const CGTownInstance *>(map.objects.at(townID.getNum()).get());
					if(town->pos != map.players.at(color).posOfMainTown)
						continue;

					if(town->getTown()->faction->getId() == FactionID::NEUTRAL)
					{
						problem = EditorTexts::get("bonus.randomTown");
						break;
					}

					for(const auto & [id, building] : town->getTown()->buildings)
					{
						ids.push_back(id.getNum());
						names.push_back(building->getNameTranslated());
					}
					break;
				}
			}
			else
				problem = EditorTexts::get("bonus.noTown");

			if(!problem.empty() || ids.empty())
			{
				buildingAvailable = false;
				form->addParagraph(problem.empty() ? EditorTexts::get("bonus.noTown") : problem);
				break;
			}

			addIdChoice(EditorTexts::get("bonus.buildingName"), ids, names, values.building);
			break;
		}
		case CampaignBonusType::ARTIFACT:
		{
			addIdChoice(EditorTexts::get("bonus.recipient"), heroIds, heroNames, values.hero);

			std::vector<int> ids;
			std::vector<std::string> names;
			for(const auto & artifact : LIBRARY->arth->objects)
			{
				if(!vstd::contains(EXCLUDED_ARTIFACTS, artifact->getId()))
				{
					ids.push_back(artifact->getIndex());
					names.push_back(artifact->getNameTranslated());
				}
			}
			addIdChoice(EditorTexts::get("bonus.artifactName"), ids, names, values.artifact);
			break;
		}
		case CampaignBonusType::PRIMARY_SKILL:
		{
			addIdChoice(EditorTexts::get("bonus.recipient"), heroIds, heroNames, values.hero);
			for(size_t i = 0; i < values.primary.size(); ++i)
			{
				int * target = &values.primary[i];
				form->addNumber(EditorTexts::get(std::string("heroSkills.") + NPrimarySkill::names[i]), *target, 0, MAX_PRIMARY_SKILL, [target](int value) { *target = value; });
			}
			break;
		}
		case CampaignBonusType::SECONDARY_SKILL:
		{
			addIdChoice(EditorTexts::get("bonus.recipient"), heroIds, heroNames, values.hero);

			std::vector<int> ids;
			std::vector<std::string> names;
			for(const auto & skill : LIBRARY->skillh->objects)
			{
				ids.push_back(skill->getIndex());
				names.push_back(skill->getNameTranslated());
			}
			addIdChoice(EditorTexts::get("bonus.skillName"), ids, names, values.skill);

			std::vector<int> levelIds;
			std::vector<std::string> levelNames;
			for(int i = 0; i < SKILL_LEVELS; ++i)
			{
				levelIds.push_back(i);
				levelNames.push_back(EditorTexts::translate(TextIdentifier("core.skilllev", i)));
			}
			addIdChoice(EditorTexts::get("bonus.mastery"), levelIds, levelNames, values.mastery);
			break;
		}
		case CampaignBonusType::RESOURCE:
		{
			std::vector<int> ids;
			std::vector<std::string> names;
			for(const GameResID resource : {GameResID::WOOD, GameResID::MERCURY, GameResID::ORE, GameResID::SULFUR, GameResID::CRYSTAL, GameResID::GEMS, GameResID::GOLD})
			{
				ids.push_back(resource.getNum());
				names.push_back(resourceName(resource));
			}
			ids.push_back(GameResID::COMMON);
			names.push_back(EditorTexts::format("bonus.common", {resourceName(GameResID::WOOD), resourceName(GameResID::ORE)}));
			ids.push_back(GameResID::RARE);
			names.push_back(EditorTexts::format("bonus.rare", {resourceName(GameResID::MERCURY), resourceName(GameResID::SULFUR), resourceName(GameResID::CRYSTAL), resourceName(GameResID::GEMS)}));

			addIdChoice(EditorTexts::get("bonus.resourceType"), ids, names, values.resource);
			form->addNumber(EditorTexts::get("bonus.quantity"), values.amount, 1, MAX_RESOURCES, [this](int value) { values.amount = value; });
			break;
		}
		default:
			break;
	}

	form->finish();
}

void EditorStartingBonusDialog::load(const CampaignBonus & bonus)
{
	type = std::clamp(static_cast<int>(bonus.getType()), 0, BONUS_TYPES - 1);

	switch(bonus.getType())
	{
		case CampaignBonusType::SPELL:
		{
			const auto & value = bonus.getValue<CampaignBonusSpell>();
			values.hero = value.hero.getNum();
			values.spell = value.spell.getNum();
			break;
		}
		case CampaignBonusType::MONSTER:
		{
			const auto & value = bonus.getValue<CampaignBonusCreatures>();
			values.hero = value.hero.getNum();
			values.creature = value.creature.getNum();
			values.amount = value.amount;
			break;
		}
		case CampaignBonusType::BUILDING:
			values.building = bonus.getValue<CampaignBonusBuilding>().buildingDecoded.getNum();
			break;
		case CampaignBonusType::ARTIFACT:
		{
			const auto & value = bonus.getValue<CampaignBonusArtifact>();
			values.hero = value.hero.getNum();
			values.artifact = value.artifact.getNum();
			break;
		}
		case CampaignBonusType::SPELL_SCROLL:
		{
			const auto & value = bonus.getValue<CampaignBonusSpellScroll>();
			values.hero = value.hero.getNum();
			values.spell = value.spell.getNum();
			break;
		}
		case CampaignBonusType::PRIMARY_SKILL:
		{
			const auto & value = bonus.getValue<CampaignBonusPrimarySkill>();
			values.hero = value.hero.getNum();
			std::ranges::copy(value.amounts, values.primary.begin());
			break;
		}
		case CampaignBonusType::SECONDARY_SKILL:
		{
			const auto & value = bonus.getValue<CampaignBonusSecondarySkill>();
			values.hero = value.hero.getNum();
			values.skill = value.skill.getNum();
			values.mastery = value.mastery;
			break;
		}
		case CampaignBonusType::RESOURCE:
		{
			const auto & value = bonus.getValue<CampaignBonusStartingResources>();
			values.resource = value.resource.getNum();
			values.amount = value.amount;
			break;
		}
		default:
			break;
	}
}

CampaignBonus EditorStartingBonusDialog::save() const
{
	switch(static_cast<CampaignBonusType>(type))
	{
		case CampaignBonusType::SPELL:
			return CampaignBonusSpell{HeroTypeID(values.hero), SpellID(values.spell)};
		case CampaignBonusType::MONSTER:
			return CampaignBonusCreatures{HeroTypeID(values.hero), CreatureID(values.creature), values.amount};
		case CampaignBonusType::BUILDING:
			return CampaignBonusBuilding{BuildingID{}, BuildingID(values.building)};
		case CampaignBonusType::ARTIFACT:
			return CampaignBonusArtifact{HeroTypeID(values.hero), ArtifactID(values.artifact)};
		case CampaignBonusType::SPELL_SCROLL:
			return CampaignBonusSpellScroll{HeroTypeID(values.hero), SpellID(values.spell)};
		case CampaignBonusType::PRIMARY_SKILL:
			return CampaignBonusPrimarySkill{HeroTypeID(values.hero), {static_cast<uint8_t>(values.primary[0]), static_cast<uint8_t>(values.primary[1]), static_cast<uint8_t>(values.primary[2]), static_cast<uint8_t>(values.primary[3])}};
		case CampaignBonusType::SECONDARY_SKILL:
			return CampaignBonusSecondarySkill{HeroTypeID(values.hero), SecondarySkill(values.skill), values.mastery};
		case CampaignBonusType::RESOURCE:
			return CampaignBonusStartingResources{GameResID(values.resource), values.amount};
		default:
			throw std::runtime_error("Unsupported starting bonus type");
	}
}

std::string EditorStartingBonusDialog::heroName(const CMap & map, int heroId)
{
	if(heroId == HeroTypeID::CAMP_STRONGEST)
		return EditorTexts::get("bonus.strongestHero");
	if(heroId == HeroTypeID::CAMP_GENERATED)
		return EditorTexts::get("bonus.generatedHero");
	if(heroId == HeroTypeID::CAMP_RANDOM)
		return EditorTexts::get("bonus.randomHero");

	for(const auto & object : map.objects)
		if(const auto * hero = dynamic_cast<const CGHeroInstance *>(object.get()))
			if(hero->getHeroTypeID().getNum() == heroId)
				return EditorTexts::translate(TextIdentifier(hero->getNameTextID()));

	return std::to_string(heroId);
}

std::string EditorStartingBonusDialog::describe(const CampaignBonus & bonus, const CMap & map)
{
	switch(bonus.getType())
	{
		case CampaignBonusType::SPELL:
		{
			const auto & value = bonus.getValue<CampaignBonusSpell>();
			return EditorTexts::format("bonus.spellFor", {entityName(value.spell), heroName(map, value.hero.getNum())});
		}
		case CampaignBonusType::MONSTER:
		{
			const auto & value = bonus.getValue<CampaignBonusCreatures>();
			MetaString creature;
			creature.appendName(value.creature, value.amount);
			return EditorTexts::format("bonus.creatureFor", {std::to_string(value.amount), EditorTexts::translate(creature), heroName(map, value.hero.getNum())});
		}
		case CampaignBonusType::BUILDING:
			return EditorTexts::get("bonus.building");
		case CampaignBonusType::ARTIFACT:
		{
			const auto & value = bonus.getValue<CampaignBonusArtifact>();
			return EditorTexts::format("bonus.artifactFor", {entityName(value.artifact), heroName(map, value.hero.getNum())});
		}
		case CampaignBonusType::SPELL_SCROLL:
		{
			const auto & value = bonus.getValue<CampaignBonusSpellScroll>();
			return EditorTexts::format("bonus.scrollFor", {entityName(value.spell), heroName(map, value.hero.getNum())});
		}
		case CampaignBonusType::PRIMARY_SKILL:
		{
			const auto & value = bonus.getValue<CampaignBonusPrimarySkill>();
			return EditorTexts::format("bonus.primaryFor", {std::to_string(value.amounts[0]), std::to_string(value.amounts[1]), std::to_string(value.amounts[2]), std::to_string(value.amounts[3]), heroName(map, value.hero.getNum())});
		}
		case CampaignBonusType::SECONDARY_SKILL:
			return EditorTexts::get("bonus.secondarySkill");
		case CampaignBonusType::RESOURCE:
			return EditorTexts::get("bonus.resource");
		default:
			return "";
	}
}

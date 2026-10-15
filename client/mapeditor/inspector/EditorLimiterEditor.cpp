/*
 * EditorLimiterEditor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorLimiterEditor.h"

#include "../EditorTexts.h"

#include "../../widgets/CDropDown.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CNumberInput.h"
#include "../../widgets/CPlayerSetEditor.h"
#include "../../widgets/CResourceEditor.h"
#include "../../widgets/CSetPicker.h"
#include "../../widgets/UiTheme.h"

#include <vcmi/HeroClassService.h>
#include <vcmi/HeroClass.h>

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/CSkillHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/entities/hero/CHeroClass.h"
#include "../../../lib/entities/hero/CHeroHandler.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/mapObjects/army/CStackBasicDescriptor.h"
#include "../../../lib/spells/CSpellHandler.h"

namespace
{
constexpr int MAX_VALUE = 99999;
constexpr int MAX_CREATURES = 99999;

/// The entities are (identifier, name, listed) triples
struct Entry
{
	int id;
	std::string name;
	bool allowed;
};

EditorIdPicker makePicker(const Rect & area, const std::string & title, const std::vector<Entry> & entries, const std::set<int> & selected)
{
	EditorIdPicker result;
	std::vector<CChoiceDialog::Item> items;
	std::set<int> chosen;

	for(const auto & entry : entries)
	{
		if(!entry.allowed && !vstd::contains(selected, entry.id))
			continue;

		if(vstd::contains(selected, entry.id))
			chosen.insert(static_cast<int>(items.size()));

		CChoiceDialog::Item item;
		item.text = entry.name;
		items.push_back(std::move(item));
		result.ids.push_back(entry.id);
	}

	result.picker = std::make_shared<CSetPicker>(area, title, std::move(items), chosen);
	return result;
}

template<class Id>
std::set<int> toSet(const std::vector<Id> & ids)
{
	std::set<int> result;
	for(const auto & id : ids)
		result.insert(id.getNum());
	return result;
}
}

std::vector<int> EditorIdPicker::getIds() const
{
	std::vector<int> result;
	for(int index : picker->getSelected())
		result.push_back(ids.at(index));
	return result;
}

std::vector<std::string> EditorPickers::skillLevelNames()
{
	return {EditorTexts::get("level.basic"), EditorTexts::get("level.advanced"), EditorTexts::get("level.expert")};
}

std::vector<std::string> EditorPickers::dayOfWeekNames()
{
	std::vector<std::string> result{EditorTexts::get("rewards.none")};
	for(int i = 1; i <= 7; ++i)
		result.push_back(EditorTexts::format("rewards.day", {std::to_string(i)}));
	return result;
}

EditorIdPicker EditorPickers::artifacts(const CMap & map, const Rect & area, const std::vector<ArtifactID> & selected)
{
	std::vector<Entry> entries;
	for(const auto & artifact : LIBRARY->arth->objects)
		entries.push_back({artifact->getIndex(), artifact->getNameTranslated(), vstd::contains(map.allowedArtifact, artifact->getId())});

	return makePicker(area, EditorTexts::get("rewards.artifacts"), entries, toSet(selected));
}

EditorIdPicker EditorPickers::spells(const CMap & map, const Rect & area, const std::vector<SpellID> & selected)
{
	std::vector<Entry> entries;
	for(const auto & spell : LIBRARY->spellh->objects)
		entries.push_back({spell->getIndex(), spell->getNameTranslated(), vstd::contains(map.allowedSpells, spell->getId())});

	return makePicker(area, EditorTexts::get("rewards.spells"), entries, toSet(selected));
}

EditorIdPicker EditorPickers::heroes(const Rect & area, const std::vector<HeroTypeID> & selected)
{
	std::vector<Entry> entries;
	for(const auto & hero : LIBRARY->heroh->objects)
		entries.push_back({hero->getIndex(), hero->getNameTranslated(), true});

	return makePicker(area, EditorTexts::get("rewards.heroes"), entries, toSet(selected));
}

EditorIdPicker EditorPickers::heroClasses(const Rect & area, const std::vector<HeroClassID> & selected)
{
	std::vector<Entry> entries;
	LIBRARY->heroClasses()->forEach([&entries](const HeroClass * heroClass, bool &)
	{
		entries.push_back({heroClass->getIndex(), heroClass->getNameTranslated(), true});
	});

	return makePicker(area, EditorTexts::get("rewards.heroClasses"), entries, toSet(selected));
}

std::shared_ptr<CItemValuePicker> EditorPickers::skills(const CMap & map, const Rect & area, const std::map<SecondarySkill, si32> & selected)
{
	ItemValueConfig config;
	config.title = EditorTexts::get("rewards.skills");
	config.minValue = 1;
	config.maxValue = 3;
	config.valueNames = skillLevelNames();
	for(const auto & skill : LIBRARY->skillh->objects)
		config.itemNames.push_back(skill->getNameTranslated());

	ItemValueEntries entries;
	for(const auto & [skill, level] : selected)
		entries.emplace_back(skill.getNum(), level);

	return std::make_shared<CItemValuePicker>(area, config, entries);
}

std::shared_ptr<CItemValuePicker> EditorPickers::creatures(const Rect & area, const std::vector<CStackBasicDescriptor> & selected)
{
	ItemValueConfig config;
	config.title = EditorTexts::get("rewards.creatures");
	config.minValue = 1;
	config.maxValue = MAX_CREATURES;
	for(const auto & creature : LIBRARY->creh->objects)
		config.itemNames.push_back(creature->getNameSingularTranslated());

	ItemValueEntries entries;
	for(const auto & stack : selected)
		entries.emplace_back(stack.getType()->getIndex(), stack.getCount());

	return std::make_shared<CItemValuePicker>(area, config, entries);
}

std::map<SecondarySkill, si32> EditorPickers::skillsValue(const std::shared_ptr<CItemValuePicker> & picker)
{
	std::map<SecondarySkill, si32> result;
	for(const auto & [skill, level] : picker->getEntries())
		result[SecondarySkill(skill)] = level;
	return result;
}

std::vector<CStackBasicDescriptor> EditorPickers::creaturesValue(const std::shared_ptr<CItemValuePicker> & picker)
{
	std::vector<CStackBasicDescriptor> result;
	for(const auto & [creature, amount] : picker->getEntries())
		result.emplace_back(CreatureID(creature), amount);
	return result;
}

EditorLimiterEditor::EditorLimiterEditor(CFormView & form, int controlWidth, const CMap & map, const Rewardable::Limiter & limiter)
{
	const Rect pickerArea(0, 0, controlWidth, UiTheme::CONTROL_HEIGHT);

	dayOfWeek = form.addChoice(EditorTexts::get("rewards.dayOfWeek"), EditorPickers::dayOfWeekNames(), limiter.dayOfWeek, nullptr);
	daysPassed = form.addNumber(EditorTexts::get("rewards.daysPassed"), limiter.daysPassed, 0, MAX_VALUE, nullptr);
	heroLevel = form.addNumber(EditorTexts::get("rewards.heroLevel"), limiter.heroLevel, 0, MAX_VALUE, nullptr);
	heroExperience = form.addNumber(EditorTexts::get("rewards.heroExperience"), limiter.heroExperience, 0, MAX_VALUE, nullptr);
	manaPoints = form.addNumber(EditorTexts::get("rewards.manaPoints"), limiter.manaPoints, 0, MAX_VALUE, nullptr);
	manaPercentage = form.addNumber(EditorTexts::get("rewards.manaPercentage"), limiter.manaPercentage, 0, 100, nullptr);

	for(size_t i = 0; i < primary.size(); ++i)
		primary[i] = form.addNumber(EditorTexts::get(std::string("heroSkills.") + NPrimarySkill::names[i]), i < limiter.primary.size() ? limiter.primary[i] : 0, 0, MAX_VALUE, nullptr);

	resources = std::make_shared<CResourceEditor>(Rect(0, 0, controlWidth, 0), limiter.resources);
	form.addWidget(EditorTexts::get("rewards.resources"), resources);

	artifacts = EditorPickers::artifacts(map, pickerArea, limiter.artifacts);
	form.addWidget(EditorTexts::get("rewards.artifacts"), artifacts.picker);
	spells = EditorPickers::spells(map, pickerArea, limiter.spells);
	form.addWidget(EditorTexts::get("rewards.spells"), spells.picker);

	skills = EditorPickers::skills(map, pickerArea, limiter.secondary);
	form.addWidget(EditorTexts::get("rewards.skills"), skills);
	creatures = EditorPickers::creatures(pickerArea, limiter.creatures);
	form.addWidget(EditorTexts::get("rewards.creatures"), creatures);

	heroes = EditorPickers::heroes(pickerArea, limiter.heroes);
	form.addWidget(EditorTexts::get("rewards.heroes"), heroes.picker);
	heroClasses = EditorPickers::heroClasses(pickerArea, limiter.heroClasses);
	form.addWidget(EditorTexts::get("rewards.heroClasses"), heroClasses.picker);

	std::set<PlayerColor> playerSet(limiter.players.begin(), limiter.players.end());
	players = std::make_shared<CPlayerSetEditor>(Rect(0, 0, controlWidth, 0), playerSet);
	form.addWidget(EditorTexts::get("rewards.players"), players);
}

void EditorLimiterEditor::apply(Rewardable::Limiter & limiter) const
{
	limiter.dayOfWeek = dayOfWeek->getSelected();
	limiter.daysPassed = daysPassed->getValue();
	limiter.heroLevel = heroLevel->getValue();
	limiter.heroExperience = heroExperience->getValue();
	limiter.manaPoints = manaPoints->getValue();
	limiter.manaPercentage = manaPercentage->getValue();

	limiter.primary.resize(primary.size());
	for(size_t i = 0; i < primary.size(); ++i)
		limiter.primary[i] = primary[i]->getValue();

	limiter.resources = resources->getValue();

	limiter.artifacts.clear();
	for(int id : artifacts.getIds())
		limiter.artifacts.emplace_back(id);

	limiter.spells.clear();
	for(int id : spells.getIds())
		limiter.spells.emplace_back(id);

	limiter.secondary = EditorPickers::skillsValue(skills);
	limiter.creatures = EditorPickers::creaturesValue(creatures);

	limiter.heroes.clear();
	for(int id : heroes.getIds())
		limiter.heroes.emplace_back(id);

	limiter.heroClasses.clear();
	for(int id : heroClasses.getIds())
		limiter.heroClasses.emplace_back(id);

	limiter.players.clear();
	for(const auto & player : players->getValue())
		limiter.players.push_back(player);
}

/*
 * EditorRewardsDialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorRewardsDialog.h"

#include "EditorLimiterEditor.h"

#include "../EditorMap.h"
#include "../EditorTexts.h"

#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../../widgets/CCheckBox.h"
#include "../../widgets/CDropDown.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CListEditor.h"
#include "../../widgets/CNumberInput.h"
#include "../../widgets/CResourceEditor.h"
#include "../../widgets/CSetPicker.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/CBonusTypeHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/constants/StringConstants.h"
#include "../../../lib/mapObjects/CGPandoraBox.h"
#include "../../../lib/mapObjects/CRewardableObject.h"
#include "../../../lib/mapObjects/Quest.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/spells/CSpellHandler.h"

namespace
{
constexpr int LABEL_WIDTH = 170;
constexpr int BONUS_LIST_HEIGHT = 120;
constexpr int REWARD_LIST_HEIGHT = 160;
constexpr int MAX_RESET = 99;

int controlWidthOf(const Point & size)
{
	return size.x - LABEL_WIDTH - UiTheme::SLIDER_WIDTH - UiTheme::SPACING;
}

std::vector<std::string> windowModeNames()
{
	return {"AUTO", "MODAL", "INFO"};
}
}

// ---------------------------------------------------------------------------------------------

EditorBonusDialog::EditorBonusDialog(const std::shared_ptr<Bonus> & bonus, const std::function<void(BonusDuration::Type, BonusType, int)> & onSaved)
	: CDialogWindow(Point(460, 190), EditorTexts::get("rewards.bonus"))
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	std::vector<std::string> durationNames;
	for(const auto & [name, duration] : bonusDurationMap)
	{
		durationNames.push_back(name);
		durations.push_back(duration);
	}

	std::vector<std::string> typeNames;
	for(const auto & type : LIBRARY->bth->getAllObjets())
	{
		typeNames.push_back(LIBRARY->bth->bonusToString(type));
		types.push_back(type);
	}

	int durationIndex = 0;
	int typeIndex = 0;
	int value = 0;
	if(bonus)
	{
		durationIndex = std::max(0, static_cast<int>(vstd::find_pos(durations, bonus->duration)));
		typeIndex = std::max(0, static_cast<int>(vstd::find_pos(types, bonus->type)));
		value = bonus->val;
	}

	form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);
	durationChoice = form->addChoice(EditorTexts::get("rewards.bonusDuration"), durationNames, durationIndex, nullptr);
	typeChoice = form->addChoice(EditorTexts::get("rewards.bonusType"), typeNames, typeIndex, nullptr);
	valueInput = form->addNumber(EditorTexts::get("rewards.bonusValue"), value, -999, 999, nullptr);
	form->finish();

	onAccept = [this, onSaved]()
	{
		onSaved(durations.at(durationChoice->getSelected()), types.at(typeChoice->getSelected()), valueInput->getValue());
		return true;
	};
}

// ---------------------------------------------------------------------------------------------

struct EditorRewardInfoDialog::Pickers
{
	EditorIdPicker artifacts;
	EditorIdPicker spells;
	std::shared_ptr<CItemValuePicker> skills;
	std::shared_ptr<CItemValuePicker> creatures;
};

EditorRewardInfoDialog::~EditorRewardInfoDialog() = default;

EditorRewardInfoDialog::EditorRewardInfoDialog(const CMap & map, CRewardableObject & object, const Draft & draft, const std::function<void(const Draft &)> & onSaved)
	: CDialogWindow(Point(760, 660), EditorTexts::get("rewards.rewardTitle"))
	, draft(draft)
	, bonuses(draft.info.reward.heroBonuses)
	, pickers(std::make_shared<Pickers>())
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int controlWidth = controlWidthOf(size);
	const Rect pickerArea(0, 0, controlWidth, UiTheme::CONTROL_HEIGHT);
	const Rewardable::Reward & reward = draft.info.reward;

	form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);

	form->addHeading(EditorTexts::get("rewards.reward"));
	messageField = form->addText(EditorTexts::get("rewards.message"), draft.messageText, nullptr);
	heroLevel = form->addNumber(EditorTexts::get("rewards.heroLevel"), reward.heroLevel, 0, 999, nullptr);
	heroExperience = form->addNumber(EditorTexts::get("rewards.heroExperience"), reward.heroExperience, 0, 100000, nullptr);
	manaDiff = form->addNumber(EditorTexts::get("rewards.manaDiff"), reward.manaDiff, -999, 999, nullptr);
	manaPercentage = form->addNumber(EditorTexts::get("rewards.manaPercentage"), reward.manaPercentage, -100, 1000, nullptr);
	overflowFactor = form->addNumber(EditorTexts::get("rewards.overflowFactor"), reward.manaOverflowFactor, 0, 100, nullptr);
	movePoints = form->addNumber(EditorTexts::get("rewards.movePoints"), reward.movePoints, -999, 999, nullptr);
	movePercentage = form->addNumber(EditorTexts::get("rewards.movePercentage"), reward.movePercentage, -100, 1000, nullptr);
	removeObject = form->addCheck(EditorTexts::get("rewards.removeObject"), reward.removeObject, nullptr);

	for(size_t i = 0; i < primary.size(); ++i)
		primary[i] = form->addNumber(EditorTexts::get(std::string("heroSkills.") + NPrimarySkill::names[i]), i < reward.primary.size() ? reward.primary[i] : 0, -99, 99, nullptr);

	resources = std::make_shared<CResourceEditor>(Rect(0, 0, controlWidth, 0), reward.resources);
	form->addWidget(EditorTexts::get("rewards.resources"), resources);

	pickers->artifacts = EditorPickers::artifacts(map, pickerArea, reward.grantedArtifacts);
	form->addWidget(EditorTexts::get("rewards.artifacts"), pickers->artifacts.picker);
	pickers->spells = EditorPickers::spells(map, pickerArea, reward.spells);
	form->addWidget(EditorTexts::get("rewards.spells"), pickers->spells.picker);

	pickers->skills = EditorPickers::skills(map, pickerArea, reward.secondary);
	form->addWidget(EditorTexts::get("rewards.skills"), pickers->skills);
	pickers->creatures = EditorPickers::creatures(pickerArea, reward.creatures);
	form->addWidget(EditorTexts::get("rewards.creatures"), pickers->creatures);

	std::vector<std::string> spellNames;
	int castIndex = 0;
	for(const auto & spell : LIBRARY->spellh->objects)
	{
		if(!spell->isAdventure())
			continue;

		if(spell->getId() == reward.spellCast.first)
			castIndex = static_cast<int>(castSpellIds.size());

		castSpellIds.push_back(spell->getIndex());
		spellNames.push_back(spell->getNameTranslated());
	}

	castCheck = form->addCheck(EditorTexts::get("rewards.castSpell"), reward.spellCast.first != SpellID::NONE, [this](bool on)
	{
		castSpell->block(!on);
		castLevel->block(!on);
	});
	castSpell = form->addChoice(EditorTexts::get("rewards.castSpellId"), spellNames, castIndex, nullptr);
	castLevel = form->addChoice(EditorTexts::get("rewards.castLevel"), {NSecondarySkill::levels.begin(), NSecondarySkill::levels.end()}, reward.spellCast.second, nullptr);
	castSpell->block(!castCheck->isChecked());
	castLevel->block(!castCheck->isChecked());

	bonusList = std::make_shared<CListEditor>(Rect(0, 0, controlWidth, BONUS_LIST_HEIGHT), true);
	bonusList->onAdd = [this]()
	{
		bonuses.push_back(nullptr);
		editBonus(static_cast<int>(bonuses.size()) - 1);
	};
	bonusList->onEdit = [this](int index) { editBonus(index); };
	bonusList->onRemove = [this](int index)
	{
		bonuses.erase(bonuses.begin() + index);
		refreshBonuses(-1);
	};
	form->addWidget(EditorTexts::get("rewards.bonuses"), bonusList);
	refreshBonuses(-1);

	form->addHeading(EditorTexts::get("rewards.limiter"));
	limiter = std::make_shared<EditorLimiterEditor>(*form, controlWidth, map, draft.info.limiter);

	form->finish();

	onAccept = [this, onSaved, &object]()
	{
		Draft result = this->draft;
		Rewardable::Reward & reward = result.info.reward;

		result.messageText = messageField->getText();
		reward.heroLevel = heroLevel->getValue();
		reward.heroExperience = heroExperience->getValue();
		reward.manaDiff = manaDiff->getValue();
		reward.manaPercentage = manaPercentage->getValue();
		reward.manaOverflowFactor = overflowFactor->getValue();
		reward.movePoints = movePoints->getValue();
		reward.movePercentage = movePercentage->getValue();
		reward.removeObject = removeObject->isChecked();

		reward.primary.resize(primary.size());
		for(size_t i = 0; i < primary.size(); ++i)
			reward.primary[i] = primary[i]->getValue();

		reward.resources = resources->getValue();

		reward.grantedArtifacts.clear();
		for(int id : pickers->artifacts.getIds())
			reward.grantedArtifacts.emplace_back(id);

		reward.spells.clear();
		for(int id : pickers->spells.getIds())
			reward.spells.emplace_back(id);

		reward.secondary.clear();
		for(const auto & [skill, level] : EditorPickers::skillsValue(pickers->skills))
			reward.secondary[skill] = level;

		reward.creatures = EditorPickers::creaturesValue(pickers->creatures);

		reward.spellCast.first = SpellID::NONE;
		if(castCheck->isChecked() && !castSpellIds.empty())
		{
			reward.spellCast.first = SpellID(castSpellIds.at(castSpell->getSelected()));
			reward.spellCast.second = castLevel->getSelected();
		}

		reward.heroBonuses.clear();
		for(const auto & bonus : bonuses)
		{
			if(bonus)
			{
				auto stored = std::make_shared<Bonus>(*bonus);
				stored->sid = BonusSourceID(object.id);
				reward.heroBonuses.push_back(stored);
			}
		}

		result.info.visitType = Rewardable::EEventType::EVENT_FIRST_VISIT;
		limiter->apply(result.info.limiter);

		onSaved(result);
		return true;
	};
}

void EditorRewardInfoDialog::refreshBonuses(int selected)
{
	std::vector<std::string> names;
	for(const auto & bonus : bonuses)
	{
		if(!bonus)
			continue;

		std::string duration = std::to_string(static_cast<int>(bonus->duration));
		for(const auto & [name, value] : bonusDurationMap)
			if(value == bonus->duration)
				duration = name;

		names.push_back(duration + " " + LIBRARY->bth->bonusToString(bonus->type) + " (" + std::to_string(bonus->val) + ")");
	}

	bonusList->setEntries(names, selected);
}

void EditorRewardInfoDialog::editBonus(int index)
{
	ENGINE->windows().createAndPushWindow<EditorBonusDialog>(bonuses.at(index), [this, index](BonusDuration::Type duration, BonusType type, int value)
	{
		std::shared_ptr<Bonus> bonus = bonuses.at(index) ? std::make_shared<Bonus>(*bonuses.at(index)) : std::make_shared<Bonus>();
		bonus->duration = duration;
		bonus->type = type;
		bonus->val = value;
		bonus->source = BonusSource::OBJECT_INSTANCE;
		bonuses.at(index) = bonus;
		refreshBonuses(index);
	});
}

// ---------------------------------------------------------------------------------------------

EditorRewardsDialog::EditorRewardsDialog(EditorMap & editor, CRewardableObject & object, const std::function<void()> & onChanged)
	: CDialogWindow(Point(640, 560), EditorTexts::get("rewards.title"))
	, editor(editor)
	, object(object)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int controlWidth = controlWidthOf(size);
	const auto & configuration = object.configuration;

	for(const auto & info : configuration.info)
		rewards.push_back({info, EditorTexts::translate(info.message)});

	form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);

	std::vector<std::string> visitNames;
	for(const auto & name : Rewardable::VisitModeString)
		visitNames.emplace_back(name);
	std::vector<std::string> selectNames;
	for(const auto & name : Rewardable::SelectModeString)
		selectNames.emplace_back(name);

	visitMode = form->addChoice(EditorTexts::get("rewards.visitMode"), visitNames, configuration.visitMode, nullptr);
	selectMode = form->addChoice(EditorTexts::get("rewards.selectMode"), selectNames, configuration.selectMode, [this](int)
	{
		updateEnabled();
	});
	windowMode = form->addChoice(EditorTexts::get("rewards.windowMode"), windowModeNames(), static_cast<int>(configuration.infoWindowType), nullptr);
	onSelectText = form->addText(EditorTexts::get("rewards.onSelect"), EditorTexts::translate(configuration.onSelect), nullptr);
	canRefuse = form->addCheck(EditorTexts::get("rewards.canRefuse"), configuration.canRefuse, nullptr);

	const auto & reset = configuration.resetParameters;
	auto resetChanged = [this](int) { updateEnabled(); };
	resetPeriod = form->addNumber(EditorTexts::get("rewards.resetPeriod"), reset.days, 0, MAX_RESET, resetChanged);
	resetWeeks = form->addNumber(EditorTexts::get("rewards.resetWeeks"), reset.weeks, 0, MAX_RESET, resetChanged);
	resetMonths = form->addNumber(EditorTexts::get("rewards.resetMonths"), reset.months, 0, MAX_RESET, resetChanged);
	resetVisitors = form->addCheck(EditorTexts::get("rewards.resetVisitors"), reset.visitors, nullptr);
	resetRewards = form->addCheck(EditorTexts::get("rewards.resetRewards"), reset.rewards, nullptr);

	rewardList = std::make_shared<CListEditor>(Rect(0, 0, controlWidth, REWARD_LIST_HEIGHT), true);
	rewardList->onAdd = [this]()
	{
		rewards.emplace_back();
		refreshList(static_cast<int>(rewards.size()) - 1);
	};
	rewardList->onEdit = [this](int index) { edit(index); };
	rewardList->onRemove = [this](int index)
	{
		rewards.erase(rewards.begin() + index);
		refreshList(-1);
	};
	form->addWidget(EditorTexts::get("rewards.list"), rewardList);
	form->finish();

	// the object kinds below have a fixed behavior
	if(dynamic_cast<CGPandoraBox *>(&object) && !dynamic_cast<CGEvent *>(&object))
	{
		visitMode->setSelected(static_cast<int>(vstd::find_pos(Rewardable::VisitModeString, "once")));
		visitMode->block(true);
		selectMode->setSelected(static_cast<int>(vstd::find_pos(Rewardable::SelectModeString, "selectFirst")));
		selectMode->block(true);
		windowMode->block(true);
		canRefuse->block(true);
	}

	if(dynamic_cast<SeerHut *>(&object))
	{
		visitMode->setSelected(static_cast<int>(vstd::find_pos(Rewardable::VisitModeString, "once")));
		visitMode->block(true);
		windowMode->block(true);
		canRefuse->setChecked(true);
		canRefuse->block(true);
	}

	refreshList(-1);
	updateEnabled();

	onAccept = [this, onChanged]()
	{
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorRewardsDialog::updateEnabled()
{
	onSelectText->setReadOnly(selectMode->getSelected() != static_cast<int>(vstd::find_pos(Rewardable::SelectModeString, "selectPlayer")));

	const bool anyPeriodSet = resetPeriod->getValue() > 0 || resetWeeks->getValue() > 0 || resetMonths->getValue() > 0;
	resetVisitors->block(!anyPeriodSet);
	resetRewards->block(!anyPeriodSet);
}

void EditorRewardsDialog::refreshList(int selected)
{
	std::vector<std::string> names;
	for(size_t i = 0; i < rewards.size(); ++i)
		names.push_back(EditorTexts::format("rewards.entry", {std::to_string(i + 1)}));

	rewardList->setEntries(names, selected);
}

void EditorRewardsDialog::edit(int index)
{
	ENGINE->windows().createAndPushWindow<EditorRewardInfoDialog>(*editor.getMap(), object, rewards.at(index), [this, index](const Draft & draft)
	{
		rewards.at(index) = draft;
	});
}

void EditorRewardsDialog::commit()
{
	CMap & map = *editor.getMap();
	auto & configuration = object.configuration;

	configuration.visitMode = static_cast<Rewardable::EVisitMode>(visitMode->getSelected());
	configuration.selectMode = static_cast<Rewardable::ESelectMode>(selectMode->getSelected());
	configuration.infoWindowType = static_cast<EInfoWindowMode>(windowMode->getSelected());
	configuration.canRefuse = canRefuse->isChecked();

	if(onSelectText->getText().empty())
		configuration.onSelect.clear();
	else
		configuration.onSelect = MetaString::createFromTextID(mapRegisterLocalizedString("map", map, TextIdentifier("reward", object.instanceName, "onSelect"), onSelectText->getText()));

	auto & reset = configuration.resetParameters;
	reset.days = resetPeriod->getValue();
	reset.weeks = resetWeeks->getValue();
	reset.months = resetMonths->getValue();
	reset.visitors = resetVisitors->isChecked();
	reset.rewards = resetRewards->isChecked();

	configuration.info.clear();
	for(size_t i = 0; i < rewards.size(); ++i)
	{
		auto info = rewards[i].info;
		if(rewards[i].messageText.empty())
			info.message.clear();
		else
			info.message = MetaString::createFromTextID(mapRegisterLocalizedString("map", map, TextIdentifier("reward", object.instanceName, "info", static_cast<int>(i), "message"), rewards[i].messageText));

		configuration.info.push_back(info);
	}
}

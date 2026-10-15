/*
 * EditorQuestDialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorQuestDialog.h"

#include "EditorLimiterEditor.h"

#include "../EditorMap.h"
#include "../EditorMapHelpers.h"
#include "../EditorTexts.h"

#include "../../widgets/CCheckBox.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CListEditor.h"
#include "../../widgets/CMultiLineEdit.h"
#include "../../widgets/CNumberInput.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/mapObjects/CGCreature.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/Quest.h"
#include "../../../lib/mapping/CMap.h"

namespace
{
constexpr int LIST_WIDTH = 220;
constexpr int LABEL_WIDTH = 150;
constexpr int TEXT_HEIGHT = 80;
constexpr int MAX_DEADLINE = 9999;
}

EditorQuestDialog::EditorQuestDialog(EditorMap & editor, QuestSource & source, const std::function<void()> & onChanged)
	: CDialogWindow(Point(1000, 660), EditorTexts::get("quest.title"))
	, editor(editor)
	, source(source)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();

	for(const auto & quest : source.allQuests())
		quests.push_back(std::make_shared<Quest>(*quest));

	questList = std::make_shared<CListEditor>(Rect(0, 0, LIST_WIDTH, size.y), false);
	questList->onAdd = [this]()
	{
		commitSelected();
		quests.push_back(std::make_shared<Quest>());
		sortQuests();
		refreshList(static_cast<int>(quests.size()) - 1);
		select(static_cast<int>(quests.size()) - 1);
	};
	questList->onRemove = [this](int index)
	{
		commitSelected();
		quests.erase(quests.begin() + index);
		selected = -1;
		refreshList(-1);
		select(quests.empty() ? -1 : 0);
	};
	questList->onSelect = [this](int index)
	{
		if(index == selected)
			return;

		commitSelected();
		select(index);
	};

	form = std::make_shared<CFormView>(Rect(LIST_WIDTH + UiTheme::SPACING, 0, size.x - LIST_WIDTH - UiTheme::SPACING, size.y), LABEL_WIDTH);

	refreshList(quests.empty() ? -1 : 0);
	select(quests.empty() ? -1 : 0);

	onAccept = [this, onChanged]()
	{
		commitSelected();
		commit();
		if(onChanged)
			onChanged();
		return true;
	};
}

void EditorQuestDialog::refreshList(int index)
{
	std::vector<std::string> names;
	for(size_t i = 0; i < quests.size(); ++i)
		names.push_back(EditorTexts::format("quest.entry", {EditorTexts::get(quests[i]->repeatedQuest ? "quest.repeated" : "quest.oneTime"), std::to_string(i)}));

	questList->setEntries(names, index);
}

void EditorQuestDialog::select(int index)
{
	selected = index;
	buildForm();
}

void EditorQuestDialog::buildForm()
{
	limiter.reset();
	killTargetLabel.reset();
	form->clear();

	if(selected < 0)
	{
		form->finish();
		return;
	}

	const Quest & quest = *quests.at(selected);
	const int controlWidth = form->contentWidth() - LABEL_WIDTH;

	limiter = std::make_shared<EditorLimiterEditor>(*form, controlWidth, *editor.getMap(), quest.mission);

	killTargetLabel = form->addInfo(EditorTexts::get("quest.killTarget"), "");
	form->addButton("", EditorTexts::get("quest.pickTarget"), [this]() { pickKillTarget(); });
	form->addButton("", EditorTexts::get("quest.clearTarget"), [this]() { clearKillTarget(); });
	updateKillTargetText();

	firstVisitField = std::make_shared<CMultiLineEdit>(Rect(0, 0, controlWidth, TEXT_HEIGHT), EditorTexts::translate(quest.firstVisitText));
	form->addWidget(EditorTexts::get("quest.firstVisit"), firstVisitField);
	nextVisitField = std::make_shared<CMultiLineEdit>(Rect(0, 0, controlWidth, TEXT_HEIGHT), EditorTexts::translate(quest.nextVisitText));
	form->addWidget(EditorTexts::get("quest.nextVisit"), nextVisitField);
	completedField = std::make_shared<CMultiLineEdit>(Rect(0, 0, controlWidth, TEXT_HEIGHT), EditorTexts::translate(quest.completedText));
	form->addWidget(EditorTexts::get("quest.completed"), completedField);

	repeatedCheck = form->addCheck(EditorTexts::get("quest.repeatedQuest"), quest.repeatedQuest, [this](bool)
	{
		// the order of the quests depends on the flag
		const auto current = quests.at(selected);
		commitSelected();
		sortQuests();
		selected = static_cast<int>(vstd::find_pos(quests, current));
		refreshList(selected);
		buildForm();
	});
	deadlineCheck = form->addCheck(EditorTexts::get("quest.deadline"), quest.lastDay >= 0, [this](bool on)
	{
		deadlineInput->block(!on);
	});
	deadlineInput = form->addNumber(EditorTexts::get("quest.lastDay"), std::max<int>(quest.lastDay, 0), 0, MAX_DEADLINE, nullptr);
	deadlineInput->block(quest.lastDay < 0);

	form->finish();
}

void EditorQuestDialog::commitSelected()
{
	if(selected < 0 || !limiter)
		return;

	Quest & quest = *quests.at(selected);
	limiter->apply(quest.mission);

	// text identifiers depend on the position of the quest and are assigned on accept
	quest.firstVisitText = MetaString::createFromRawString(firstVisitField->getText());
	quest.nextVisitText = MetaString::createFromRawString(nextVisitField->getText());
	quest.completedText = MetaString::createFromRawString(completedField->getText());
	quest.repeatedQuest = repeatedCheck->isChecked();
	quest.lastDay = deadlineCheck->isChecked() ? deadlineInput->getValue() : -1;
}

void EditorQuestDialog::updateKillTargetText()
{
	const Quest & quest = *quests.at(selected);
	std::string text;

	if(!quest.mission.destroyedObjects.empty())
	{
		const auto * object = editor.getMap()->getObject(quest.mission.destroyedObjects.front());
		text = object ? object->instanceName : std::to_string(quest.mission.destroyedObjects.front().getNum());
	}

	killTargetLabel->setText(text);
}

void EditorQuestDialog::pickKillTarget()
{
	const auto filter = [](const CGObjectInstance * object)
	{
		if(const auto * hero = dynamic_cast<const CGHeroInstance *>(object))
			return hero->ID != Obj::PRISON;

		return dynamic_cast<const CGCreature *>(object) != nullptr;
	};

	EditorMapHelpers::pickObject(editor, 1, filter, [this](const CGObjectInstance * object)
	{
		auto & destroyed = quests.at(selected)->mission.destroyedObjects;
		destroyed.clear();
		if(object)
			destroyed.push_back(object->id);

		updateKillTargetText();
	});
}

void EditorQuestDialog::clearKillTarget()
{
	quests.at(selected)->mission.destroyedObjects.clear();
	updateKillTargetText();
}

void EditorQuestDialog::sortQuests()
{
	std::ranges::stable_sort(quests, [](const auto & a, const auto & b)
	{
		return !a->repeatedQuest && b->repeatedQuest;
	});
}

void EditorQuestDialog::commit()
{
	CMap & map = *editor.getMap();

	sortQuests();
	for(size_t i = 0; i < quests.size(); ++i)
	{
		Quest & quest = *quests[i];
		const auto registerText = [&](MetaString & text, const std::string & name)
		{
			const std::string raw = EditorTexts::translate(text);
			if(raw.empty())
				text.clear();
			else
				text = MetaString::createFromTextID(mapRegisterLocalizedString("map", map, TextIdentifier("quest", source.instanceName, static_cast<int>(i), name), raw));
		};

		registerText(quest.firstVisitText, "firstVisit");
		registerText(quest.nextVisitText, "nextVisit");
		registerText(quest.completedText, "completed");
	}

	source.allQuestsEditor() = quests;
}

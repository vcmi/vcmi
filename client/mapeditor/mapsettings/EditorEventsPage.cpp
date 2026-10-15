/*
 * EditorEventsPage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorEventsPage.h"

#include "../EditorMap.h"
#include "../EditorMapHelpers.h"
#include "../EditorTexts.h"

#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../../widgets/CCheckBox.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CListEditor.h"
#include "../../widgets/CMultiLineEdit.h"
#include "../../widgets/CNumberInput.h"
#include "../../widgets/CPlayerSetEditor.h"
#include "../../widgets/CResourceEditor.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/mapping/CMap.h"

namespace
{
constexpr int DIALOG_LABEL_WIDTH = 190;
constexpr int MESSAGE_HEIGHT = 100;
constexpr int DELETED_LIST_HEIGHT = 140;
}

EditorEventsPage::EditorEventsPage(EditorMap & editor, const Rect & area)
	: EditorSettingsPage(editor, area)
{
	OBJECT_CONSTRUCTION;

	for(const auto & event : map().events)
		events.push_back({event, EditorTexts::translate(event.message)});

	listEditor = std::make_shared<CListEditor>(Rect(0, 0, area.w, area.h), true);
	listEditor->onAdd = [this]()
	{
		Draft draft;
		draft.event.name = EditorTexts::get("events.new");
		events.push_back(draft);
		refreshList(static_cast<int>(events.size()) - 1);
		edit(static_cast<int>(events.size()) - 1);
	};
	listEditor->onEdit = [this](int index) { edit(index); };
	listEditor->onRemove = [this](int index)
	{
		events.erase(events.begin() + index);
		refreshList(-1);
	};

	refreshList(-1);
}

void EditorEventsPage::refreshList(int selected)
{
	std::vector<std::string> names;
	for(const auto & draft : events)
		names.push_back(draft.event.name);

	listEditor->setEntries(names, selected);
}

void EditorEventsPage::edit(int index)
{
	ENGINE->windows().pushWindow(std::make_shared<EditorTimedEventDialog>(editor, events.at(index), [this, index](const Draft & saved)
	{
		events.at(index) = saved;
		refreshList(index);
	}));
}

void EditorEventsPage::apply()
{
	CMap & m = map();

	m.events.clear();
	for(const auto & draft : events)
	{
		CMapEvent event = draft.event;
		event.message = MetaString();
		event.message.appendTextID(mapRegisterLocalizedString("map", m, TextIdentifier("header", "event", event.name, "message"), draft.messageText));
		m.events.push_back(event);
	}
}

EditorTimedEventDialog::EditorTimedEventDialog(EditorMap & editor, const EditorEventsPage::Draft & draft, const std::function<void(const EditorEventsPage::Draft &)> & onSaved)
	: CDialogWindow(Point(640, 600), EditorTexts::get("events.title"))
	, editor(editor)
	, draft(draft)
	, deletedObjects(draft.event.deletedObjectsInstances)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const CMapEvent & event = draft.event;
	const int controlWidth = size.x - DIALOG_LABEL_WIDTH - UiTheme::SLIDER_WIDTH - UiTheme::SPACING;

	form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), DIALOG_LABEL_WIDTH);

	nameField = form->addText(EditorTexts::get("events.name"), event.name, nullptr);

	messageField = std::make_shared<CMultiLineEdit>(Rect(0, 0, controlWidth, MESSAGE_HEIGHT), draft.messageText);
	form->addWidget(EditorTexts::get("events.message"), messageField);

	humanCheck = form->addCheck(EditorTexts::get("events.affectsHuman"), event.humanAffected, nullptr);
	cpuCheck = form->addCheck(EditorTexts::get("events.affectsAi"), event.computerAffected, nullptr);
	firstInput = form->addNumber(EditorTexts::get("events.firstDay"), static_cast<int>(event.firstOccurrence) + 1, 1, 9999, nullptr);
	repeatInput = form->addNumber(EditorTexts::get("events.repeat"), static_cast<int>(event.nextOccurrence), 0, 9999, nullptr);

	playersEditor = std::make_shared<CPlayerSetEditor>(Rect(0, 0, controlWidth, 0), event.players);
	form->addWidget(EditorTexts::get("events.players"), playersEditor);

	resourcesEditor = std::make_shared<CResourceEditor>(Rect(0, 0, controlWidth, 0), event.resources);
	form->addWidget(EditorTexts::get("events.resources"), resourcesEditor);

	deletedEditor = std::make_shared<CListEditor>(Rect(0, 0, controlWidth, DELETED_LIST_HEIGHT), false);
	deletedEditor->onAdd = [this]()
	{
		EditorMapHelpers::pickObject(this->editor, 2, [](const CGObjectInstance *) { return true; }, [this](const CGObjectInstance * object)
		{
			if(object)
			{
				deletedObjects.push_back(object->id);
				refreshDeleted(static_cast<int>(deletedObjects.size()) - 1);
			}
		});
	};
	deletedEditor->onRemove = [this](int index)
	{
		deletedObjects.erase(deletedObjects.begin() + index);
		refreshDeleted(-1);
	};
	form->addWidget(EditorTexts::get("events.deleted"), deletedEditor);

	form->finish();
	refreshDeleted(-1);

	onAccept = [this, onSaved]()
	{
		EditorEventsPage::Draft result = this->draft;
		result.event.name = nameField->getText();
		result.messageText = messageField->getText();
		result.event.humanAffected = humanCheck->isChecked();
		result.event.computerAffected = cpuCheck->isChecked();
		result.event.firstOccurrence = static_cast<ui32>(firstInput->getValue() - 1);
		result.event.nextOccurrence = static_cast<ui32>(repeatInput->getValue());
		result.event.players = playersEditor->getValue();
		result.event.resources = resourcesEditor->getValue();
		result.event.deletedObjectsInstances = deletedObjects;

		onSaved(result);
		return true;
	};
}

void EditorTimedEventDialog::refreshDeleted(int selected)
{
	std::vector<std::string> names;
	for(const auto & id : deletedObjects)
	{
		const auto * object = editor.getMap()->getObject(id);
		names.push_back(object ? EditorMapHelpers::describeObject(object) : std::to_string(id.getNum()));
	}

	deletedEditor->setEntries(names, selected);
}

/*
 * EditorEventsPage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "EditorSettingsPage.h"

#include "../../windows/CDialogWindow.h"

#include "../../../lib/mapping/CMapEvent.h"

class CListEditor;
class CFormView;
class CMultiLineEdit;
class CTextField;
class CCheckBox;
class CNumberInput;
class CPlayerSetEditor;
class CResourceEditor;

/// Events that happen on the days of the game
class EditorEventsPage : public EditorSettingsPage
{
public:
	struct Draft
	{
		CMapEvent event;
		std::string messageText;
	};

private:
	std::vector<Draft> events;
	std::shared_ptr<CListEditor> listEditor;

	void refreshList(int selected);
	void edit(int index);

public:
	EditorEventsPage(EditorMap & editor, const Rect & area);

	void apply() override;
};

/// Window that edits one event
class EditorTimedEventDialog : public CDialogWindow
{
	EditorMap & editor;
	EditorEventsPage::Draft draft;
	std::vector<ObjectInstanceID> deletedObjects;

	std::shared_ptr<CFormView> form;
	std::shared_ptr<CTextField> nameField;
	std::shared_ptr<CMultiLineEdit> messageField;
	std::shared_ptr<CCheckBox> humanCheck;
	std::shared_ptr<CCheckBox> cpuCheck;
	std::shared_ptr<CNumberInput> firstInput;
	std::shared_ptr<CNumberInput> repeatInput;
	std::shared_ptr<CPlayerSetEditor> playersEditor;
	std::shared_ptr<CResourceEditor> resourcesEditor;
	std::shared_ptr<CListEditor> deletedEditor;

	void refreshDeleted(int selected);

public:
	EditorTimedEventDialog(EditorMap & editor, const EditorEventsPage::Draft & draft, const std::function<void(const EditorEventsPage::Draft &)> & onSaved);
};

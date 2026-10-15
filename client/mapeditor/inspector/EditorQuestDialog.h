/*
 * EditorQuestDialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../windows/CDialogWindow.h"

class CCheckBox;
class CFormView;
class CLabel;
class CListEditor;
class CMultiLineEdit;
class CNumberInput;
class EditorLimiterEditor;
class EditorMap;
class Quest;
class QuestSource;

/// Window that edits the quests of a quest giver
class EditorQuestDialog : public CDialogWindow
{
	EditorMap & editor;
	QuestSource & source;
	std::vector<std::shared_ptr<Quest>> quests;
	int selected = -1;

	std::shared_ptr<CListEditor> questList;
	std::shared_ptr<CFormView> form;
	std::shared_ptr<EditorLimiterEditor> limiter;
	std::shared_ptr<CLabel> killTargetLabel;
	std::shared_ptr<CMultiLineEdit> firstVisitField;
	std::shared_ptr<CMultiLineEdit> nextVisitField;
	std::shared_ptr<CMultiLineEdit> completedField;
	std::shared_ptr<CCheckBox> repeatedCheck;
	std::shared_ptr<CCheckBox> deadlineCheck;
	std::shared_ptr<CNumberInput> deadlineInput;

	void refreshList(int index);
	void select(int index);
	void buildForm();
	void commitSelected();
	void updateKillTargetText();
	void pickKillTarget();
	void clearKillTarget();
	void sortQuests();
	void commit();

public:
	EditorQuestDialog(EditorMap & editor, QuestSource & source, const std::function<void()> & onChanged);
};

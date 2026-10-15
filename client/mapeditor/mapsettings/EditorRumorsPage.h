/*
 * EditorRumorsPage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "EditorSettingsPage.h"

class CListEditor;
class CTextField;
class CMultiLineEdit;
class CLabel;

/// Rumors that are shown in the taverns
class EditorRumorsPage : public EditorSettingsPage
{
	struct Draft
	{
		std::string name;
		std::string text;
	};

	std::vector<Draft> rumors;
	int selected = -1;
	std::shared_ptr<CListEditor> listEditor;
	std::shared_ptr<CLabel> nameLabel;
	std::shared_ptr<CTextField> nameField;
	std::shared_ptr<CLabel> textLabel;
	std::shared_ptr<CMultiLineEdit> textField;

	void refreshList();
	void select(int index);

public:
	EditorRumorsPage(EditorMap & editor, const Rect & area);

	void apply() override;
};

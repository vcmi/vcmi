/*
 * EditorGeneralPage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "EditorSettingsPage.h"

class CFormView;
class CTextField;
class CMultiLineEdit;
class CCheckBox;
class CNumberInput;
class CDropDown;

/// Name, description, author and the other general properties of the map
class EditorGeneralPage : public EditorSettingsPage
{
	std::shared_ptr<CFormView> form;
	std::shared_ptr<CTextField> nameField;
	std::shared_ptr<CMultiLineEdit> descriptionField;
	std::shared_ptr<CTextField> authorField;
	std::shared_ptr<CTextField> contactField;
	std::shared_ptr<CTextField> dateField;
	std::shared_ptr<CTextField> versionField;
	std::shared_ptr<CCheckBox> levelLimitCheck;
	std::shared_ptr<CNumberInput> levelLimitInput;
	std::shared_ptr<CDropDown> difficultyChoice;

public:
	EditorGeneralPage(EditorMap & editor, const Rect & area);

	void apply() override;
};

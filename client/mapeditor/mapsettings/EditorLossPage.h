/*
 * EditorLossPage.h, part of VCMI engine
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

/// Condition that loses the map
class EditorLossPage : public EditorSettingsPage
{
	enum Condition
	{
		NONE,
		TOWN,
		HERO,
		TIME,
		DAYS_WITHOUT_TOWN,
		COUNT
	};

	std::shared_ptr<CFormView> form;

	int condition = NONE;
	bool standardDefeat = true;
	std::string message;

	std::vector<int> objectValues;
	std::vector<std::string> objectNames;
	int selectedObject = 0;
	int days = 0;
	int weeks = 0;
	int months = 0;
	int daysWithoutTown = 7;

	void loadFromMap();
	void prepareChoices();
	void rebuildForm();
	void pickObject();

public:
	EditorLossPage(EditorMap & editor, const Rect & area);

	void apply() override;
};

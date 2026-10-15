/*
 * EditorVictoryPage.h, part of VCMI engine
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

/// Condition that wins the map
class EditorVictoryPage : public EditorSettingsPage
{
	enum Condition
	{
		NONE,
		ARTIFACT,
		CREATURES,
		RESOURCES,
		BUILDING,
		TOWN,
		HERO,
		TRANSPORT,
		MONSTER,
		MINES,
		COUNT
	};

	std::shared_ptr<CFormView> form;

	int condition = NONE;
	bool standardVictory = true;
	bool onlyForHumans = false;

	/// Values of the options of the first and the second choice, the choice numbers are indexes in them
	std::vector<int> primaryValues;
	std::vector<std::string> primaryNames;
	std::vector<int> secondaryValues;
	std::vector<std::string> secondaryNames;
	int primary = 0;
	int secondary = 0;
	int amount = 1;

	void loadFromMap();
	void prepareChoices();
	void rebuildForm();
	void pickObject();

public:
	EditorVictoryPage(EditorMap & editor, const Rect & area);

	void apply() override;
};

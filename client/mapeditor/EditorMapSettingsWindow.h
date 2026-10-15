/*
 * EditorMapSettingsWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CDialogWindow.h"

class EditorMap;
class EditorSettingsPage;
class CTabBar;

/// Window with the general settings of the map
class EditorMapSettingsWindow : public CDialogWindow
{
	EditorMap & editor;
	std::vector<std::shared_ptr<EditorSettingsPage>> pages;
	std::shared_ptr<CTabBar> tabs;

	void showPage(int index);
	void apply();

public:
	explicit EditorMapSettingsWindow(EditorMap & editor);

	static void show(EditorMap & editor);
};

/*
 * EditorPlayerSettingsWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CDialogWindow.h"

#include "../../lib/mapping/CMapHeader.h"

class EditorMap;
class CFormView;
class CNumberInput;
class CTabBar;

/// Window with the settings of the players that play the map
class EditorPlayerSettingsWindow : public CDialogWindow
{
	struct Entry
	{
		int color;
		PlayerInfo info;
	};

	EditorMap & editor;
	std::vector<Entry> entries;
	std::set<int> originalColors;
	std::vector<int> townObjects;
	std::function<void()> onClosed;
	int active = 0;

	std::shared_ptr<CNumberInput> countInput;
	std::shared_ptr<CTabBar> tabs;
	std::shared_ptr<CFormView> form;

	Entry makeEntry(int color) const;
	void rebuildTabs();
	void rebuildForm();
	void setPlayerCount(int count);
	void pickMainTown();
	void apply();

public:
	EditorPlayerSettingsWindow(EditorMap & editor, const std::function<void()> & onClosed);

	static void show(EditorMap & editor, const std::function<void()> & onClosed);
};

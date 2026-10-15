/*
 * EditorModsPage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "EditorSettingsPage.h"

#include "../EditorMap.h"

class CSelectList;
class CTextButton;

/// Chooses the mods that are required to play the map
class EditorModsPage : public EditorSettingsPage
{
	struct Entry
	{
		std::string id;
		int depth;
		bool required;
		std::string name;
		std::string version;
	};

	std::vector<Entry> entries;
	std::shared_ptr<CSelectList> list;
	std::shared_ptr<CTextButton> mapModsButton;
	std::shared_ptr<CTextButton> allModsButton;

	void refreshList();
	void setRequired(size_t index, bool required);
	void requireParents(size_t index);
	void selectMods(const ModCompatibilityInfo & mods);

public:
	EditorModsPage(EditorMap & editor, const Rect & area);

	void apply() override;
};

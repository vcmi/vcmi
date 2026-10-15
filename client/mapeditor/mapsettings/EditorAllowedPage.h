/*
 * EditorAllowedPage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "EditorSettingsPage.h"

class CSelectList;
class CTextField;
class CTextButton;

/// Chooses which secondary skills, spells or artifacts are allowed on the map
class EditorAllowedPage : public EditorSettingsPage
{
public:
	enum class Kind
	{
		ABILITIES,
		SPELLS,
		ARTIFACTS
	};

private:
	struct Entry
	{
		int id;
		std::string name;
		bool allowed;
	};

	Kind kind;
	std::vector<Entry> entries;
	std::vector<int> shown;
	std::shared_ptr<CTextField> filterField;
	std::shared_ptr<CSelectList> list;
	std::shared_ptr<CTextButton> allButton;
	std::shared_ptr<CTextButton> noneButton;

	void applyFilter();
	void setShownChecked(bool on);

public:
	EditorAllowedPage(EditorMap & editor, const Rect & area, Kind kind);

	void apply() override;
};

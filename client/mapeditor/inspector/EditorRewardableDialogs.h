/*
 * EditorRewardableDialogs.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../windows/CDialogWindow.h"

#include "../../../lib/constants/EntityIdentifiers.h"

class CCheckBox;
class CDropDown;
class CRewardableObject;
class CSelectList;
class CTextField;
class EditorMap;

/// Window that sets the skills that a witch hut can teach
class EditorAbilitiesDialog : public CDialogWindow
{
	struct Entry
	{
		SecondarySkill id;
		std::string name;
		bool allowed;
		bool checked;
	};

	EditorMap & editor;
	CRewardableObject & hut;
	std::vector<Entry> entries;
	std::vector<int> shown;
	std::shared_ptr<CCheckBox> customizeCheck;
	std::shared_ptr<CTextField> filterField;
	std::shared_ptr<CSelectList> list;

	std::set<SecondarySkill> presetAbilities() const;
	std::set<SecondarySkill> defaultAbilities() const;
	void refreshList();
	void setAllChecked(bool on);
	void commit();

public:
	EditorAbilitiesDialog(EditorMap & editor, CRewardableObject & hut, const std::function<void()> & onChanged);
};

/// Window that sets what a scholar teaches
class EditorScholarDialog : public CDialogWindow
{
	struct Kind
	{
		std::string variableCategory;
		std::string variableName;
		int dice;
		std::vector<std::string> keys;
		std::vector<std::string> names;
	};

	CRewardableObject & scholar;
	std::array<Kind, 3> kinds;
	std::shared_ptr<CDropDown> kindChoice;
	std::shared_ptr<CDropDown> valueChoice;

	void refreshValues(int kind, int value);
	void commit();

public:
	EditorScholarDialog(CRewardableObject & scholar, const std::function<void()> & onChanged);
};

/// Window that sets the spell of a shrine
class EditorShrineDialog : public CDialogWindow
{
	CRewardableObject & shrine;
	std::vector<std::string> keys;
	std::shared_ptr<CCheckBox> customCheck;
	std::shared_ptr<CDropDown> spellChoice;

	void commit();

public:
	/// Opens the dialog, or a message if the spell level of the shrine type is invalid
	static void show(CRewardableObject & shrine, const std::function<void()> & onChanged);

	EditorShrineDialog(CRewardableObject & shrine, int spellLevel, const std::function<void()> & onChanged);
};

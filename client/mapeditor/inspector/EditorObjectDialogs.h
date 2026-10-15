/*
 * EditorObjectDialogs.h, part of VCMI engine
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
#include "../../../lib/entities/artifact/CArtifactFittingSet.h"

class CAnimImage;
class CArmedInstance;
class CCheckBox;
class CDropDown;
class CFormView;
class CGHeroInstance;
class CItemValuePicker;
class CMap;
class CMultiLineEdit;
class CNumberInput;
class CPlayerSetEditor;
class CSelectList;
class CTabBar;
class CTextButton;
class CTextField;
class EditorMap;

/// Window that edits the army of an object
class EditorArmyDialog : public CDialogWindow
{
	static constexpr int SLOTS = 7;

	CArmedInstance & army;
	std::array<std::shared_ptr<CDropDown>, SLOTS> creatures;
	std::array<std::shared_ptr<CNumberInput>, SLOTS> counts;
	std::shared_ptr<CDropDown> formation;

	void commit();

public:
	EditorArmyDialog(CArmedInstance & army, const std::function<void()> & onChanged);
};

/// Window that edits a text of many lines
class EditorMessageDialog : public CDialogWindow
{
	std::shared_ptr<CMultiLineEdit> edit;

public:
	EditorMessageDialog(const std::string & title, const std::string & text, const std::function<void(const std::string &)> & onSaved);
};

/// Window that chooses a set of players
class EditorPlayerSetDialog : public CDialogWindow
{
	std::shared_ptr<CPlayerSetEditor> players;

public:
	EditorPlayerSetDialog(const std::string & title, const std::set<PlayerColor> & selected, const std::function<void(const std::set<PlayerColor> &)> & onSaved);
};

/// Window that chooses the portrait of a hero
class EditorPortraitDialog : public CDialogWindow
{
	CGHeroInstance & hero;
	std::shared_ptr<CCheckBox> defaultCheck;
	std::shared_ptr<CAnimImage> portrait;
	std::array<std::shared_ptr<CTextButton>, 2> navButtons;
	int portraitIndex = 0;

	void drawPortrait();
	void commit();

public:
	EditorPortraitDialog(CGHeroInstance & hero, const std::function<void()> & onChanged);
};

/// Window that edits the skills of a hero
class EditorHeroSkillsDialog : public CDialogWindow
{
	CGHeroInstance & hero;
	std::shared_ptr<CFormView> form;
	std::array<std::shared_ptr<CNumberInput>, 4> primary;
	std::shared_ptr<CCheckBox> customizeCheck;
	std::shared_ptr<CItemValuePicker> secondary;

	void commit();

public:
	EditorHeroSkillsDialog(CGHeroInstance & hero, const std::function<void()> & onChanged);
};

/// Window that edits the spell book of a hero
class EditorHeroSpellsDialog : public CDialogWindow
{
	struct Entry
	{
		int id;
		std::string name;
		bool known;
	};

	CGHeroInstance & hero;
	std::array<std::vector<Entry>, 5> levels;
	std::vector<int> shown;
	int level = 0;
	std::shared_ptr<CCheckBox> customizeCheck;
	std::shared_ptr<CTextField> filterField;
	std::shared_ptr<CTabBar> tabs;
	std::shared_ptr<CSelectList> list;

	void refreshList();
	void commit();

public:
	EditorHeroSpellsDialog(CGHeroInstance & hero, const std::function<void()> & onChanged);
};

/// Window that edits the artifacts of a hero
class EditorHeroArtifactsDialog : public CDialogWindow
{
	struct Row
	{
		int artifactIndex;
		ArtifactPosition slot;
	};

	EditorMap & editor;
	CGHeroInstance & hero;
	CArtifactFittingSet fittingSet;
	std::vector<Row> rows;
	std::shared_ptr<CSelectList> list;
	std::shared_ptr<CTextButton> addButtonControl;
	std::shared_ptr<CTextButton> removeButtonControl;

	void refreshRows();
	void addArtifact();
	void removeSelected();
	void commit();

public:
	EditorHeroArtifactsDialog(EditorMap & editor, CGHeroInstance & hero, const std::function<void()> & onChanged);
};

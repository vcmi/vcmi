/*
 * EditorTownDialogs.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../windows/CDialogWindow.h"

#include "../../../lib/mapping/CCastleEvent.h"

class CCheckBox;
class CFormView;
class CGTownInstance;
class CListEditor;
class CMultiLineEdit;
class CMultiLineLabel;
class CNumberInput;
class CPlayerSetEditor;
class CResourceEditor;
class CSelectList;
class CTabBar;
class CTextField;
class CTown;
class EditorMap;

/// Buildings of a town as a tree, a building is a child of the building that it upgrades
struct EditorBuildingTree
{
	struct Node
	{
		BuildingID id;
		std::string name;
		int parent; ///< index of the parent node or -1
		int indent;
	};

	/// The nodes are ordered so that the descendants of a node follow the node
	std::vector<Node> nodes;

	explicit EditorBuildingTree(const CTown & town);

	/// Indexes of the node and of all its descendants
	std::vector<int> subtree(int index) const;
	/// Indexes of the node and of all its ancestors
	std::vector<int> ancestors(int index) const;
};

/// Window that sets which buildings of a town are allowed and which are built
class EditorTownBuildingsDialog : public CDialogWindow
{
	CGTownInstance & town;
	EditorBuildingTree tree;
	std::vector<bool> enabled;
	std::vector<bool> built;
	std::shared_ptr<CMultiLineLabel> hintLabel;
	std::shared_ptr<CSelectList> list;

	void refresh();
	void setEnabledFlag(int index, bool on);
	void setBuiltFlag(int index, bool on);
	void commit();

public:
	EditorTownBuildingsDialog(CGTownInstance & town, const std::function<void()> & onChanged);
};

/// Window that sets the spells that can appear in the mage guild of a town
class EditorTownSpellsDialog : public CDialogWindow
{
	struct Entry
	{
		SpellID id;
		std::string name;
		bool possible;
		bool required;
	};

	CGTownInstance & town;
	std::array<std::vector<Entry>, 5> levels;
	int level = 0;
	std::shared_ptr<CCheckBox> customizeCheck;
	std::shared_ptr<CTabBar> tabs;
	std::shared_ptr<CSelectList> list;

	void refresh();
	void commit();

public:
	EditorTownSpellsDialog(CGTownInstance & town, const std::function<void()> & onChanged);
};

/// Window that edits one event of a town
class EditorTownEventDialog : public CDialogWindow
{
public:
	struct Draft
	{
		CCastleEvent event;
		std::string messageText;
	};

private:
	Draft draft;
	EditorBuildingTree tree;
	std::vector<bool> buildingChecks;
	std::shared_ptr<CFormView> form;
	std::shared_ptr<CTextField> nameField;
	std::shared_ptr<CMultiLineEdit> messageField;
	std::shared_ptr<CCheckBox> humanCheck;
	std::shared_ptr<CCheckBox> cpuCheck;
	std::shared_ptr<CNumberInput> firstInput;
	std::shared_ptr<CNumberInput> repeatInput;
	std::shared_ptr<CPlayerSetEditor> playersEditor;
	std::shared_ptr<CResourceEditor> resourcesEditor;
	std::shared_ptr<CSelectList> buildingsList;
	std::vector<std::shared_ptr<CNumberInput>> creatureInputs;

	void refreshBuildings();

public:
	EditorTownEventDialog(const CGTownInstance & town, const Draft & draft, const std::function<void(const Draft &)> & onSaved);
};

/// Window that edits the events of a town
class EditorTownEventsDialog : public CDialogWindow
{
	using Draft = EditorTownEventDialog::Draft;

	EditorMap & editor;
	CGTownInstance & town;
	std::vector<Draft> events;
	std::shared_ptr<CListEditor> listEditor;

	void refreshList(int selected);
	void edit(int index);
	void commit();

public:
	EditorTownEventsDialog(EditorMap & editor, CGTownInstance & town, const std::function<void()> & onChanged);
};

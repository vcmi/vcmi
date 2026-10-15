/*
 * EditorTemplateDialogs.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../windows/CDialogWindow.h"

#include "../../../lib/rmg/CRmgTemplate.h"

class CDropDown;
class CFormView;
class CListEditor;
class CTabBar;
class IGameInfoCallback;

/// Windows that edit the lists and the sets of a random map template
namespace EditorTemplateDialogs
{
	void editTerrains(const std::string & title, std::set<TerrainId> & target);
	void editSpells(const std::string & title, std::set<SpellID> & target);
	void editArtifacts(const std::string & title, std::set<ArtifactID> & target);
	void editSkills(const std::string & title, std::set<SecondarySkill> & target);
	void editHeroes(const std::string & title, std::set<HeroTypeID> & target);
	void editFactions(const std::string & title, std::set<FactionID> & target);
	void editMines(std::map<GameResID, ui16> & target);
	void editTreasures(std::vector<CTreasureInfo> & target);
	void editTownHints(std::vector<rmg::ZoneOptions::CTownHints> & target);
}

/// Window that edits the objects that a zone of a template contains or forbids
class EditorTemplateObjectDialog : public CDialogWindow
{
	struct Required
	{
		CompoundMapObjectID object;
		int count;
		int guard; ///< -1 for no guard
	};

	ObjectConfig & config;
	std::vector<CompoundMapObjectID> objectIds;
	std::vector<std::string> objectNames;
	std::vector<ObjectConfig::EObjectCategory> bannedCategories;
	std::vector<CompoundMapObjectID> bannedObjects;
	std::vector<ObjectInfo> customObjects;
	std::vector<Required> requiredObjects;

	std::shared_ptr<CTabBar> tabs;
	std::array<std::shared_ptr<CListEditor>, 4> lists;

	void selectTab(int index);
	void refreshList(int tab, int selected);
	void editCategory(int index);
	void editBanned(int index);
	void editCustom(int index);
	void editRequired(int index);
	std::string objectName(const CompoundMapObjectID & id) const;
	void commit();

public:
	EditorTemplateObjectDialog(ObjectConfig & config, IGameInfoCallback * callback, const std::function<void()> & onChanged);
};

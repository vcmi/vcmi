/*
 * EditorHeroesPage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "EditorSettingsPage.h"

#include "../../../lib/mapping/CMapHeader.h"

class CAnimImage;
class CDropDown;
class CLabel;
class CPlayerSetEditor;
class CSelectList;
class CTextField;

/// Chooses the heroes that can be hired on the map and the players they are exclusive for
class EditorHeroesPage : public EditorSettingsPage
{
	struct Entry
	{
		int id;
		int imageIndex;
		std::string name;
		bool allowed;
	};

	std::vector<Entry> entries;
	std::vector<int> shown;
	std::vector<DisposedHero> disposedHeroes;
	int selectedEntry = -1;

	std::shared_ptr<CTextField> filterField;
	std::shared_ptr<CDropDown> modeChoice;
	std::shared_ptr<CSelectList> list;
	std::shared_ptr<CAnimImage> portrait;
	std::shared_ptr<CLabel> playersLabel;
	std::shared_ptr<CPlayerSetEditor> playersEditor;

	void applyFilter();
	void selectEntry(int entry);
	void updateDetails();
	const DisposedHero * findDisposed(int heroId) const;
	void setDisposed(int heroId, const std::set<PlayerColor> & players);
	void removeDisposed(int heroId);

public:
	EditorHeroesPage(EditorMap & editor, const Rect & area);

	void apply() override;
};

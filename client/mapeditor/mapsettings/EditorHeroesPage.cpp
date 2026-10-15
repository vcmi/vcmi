/*
 * EditorHeroesPage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorHeroesPage.h"

#include "../EditorMap.h"
#include "../EditorTexts.h"

#include "../../widgets/CDropDown.h"
#include "../../widgets/CPlayerSetEditor.h"
#include "../../widgets/CSelectList.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/Images.h"
#include "../../widgets/TextControls.h"
#include "../../widgets/UiTheme.h"

#include "../../../lib/GameLibrary.h"
#include "../../../lib/entities/hero/CHeroHandler.h"
#include "../../../lib/mapping/CMap.h"

#include <boost/algorithm/string/predicate.hpp>

namespace
{
constexpr int DETAILS_WIDTH = 220;
constexpr int PORTRAIT_SIZE = 100;
}

EditorHeroesPage::EditorHeroesPage(EditorMap & editor, const Rect & area)
	: EditorSettingsPage(editor, area)
{
	OBJECT_CONSTRUCTION;

	const CMap & m = map();

	for(const auto & hero : LIBRARY->heroh->objects)
		entries.push_back({hero->getIndex(), hero->imageIndex, hero->getNameTranslated(), m.allowedHeroes.count(hero->getId()) != 0});

	disposedHeroes = m.disposedHeroes;

	const int listWidth = area.w - DETAILS_WIDTH - UiTheme::SPACING;
	const int modeWidth = 130;

	filterField = std::make_shared<CTextField>(Rect(0, 0, listWidth - modeWidth - UiTheme::SPACING, UiTheme::CONTROL_HEIGHT), "", [this](const std::string &) { applyFilter(); });
	modeChoice = std::make_shared<CDropDown>(Rect(listWidth - modeWidth, 0, modeWidth, UiTheme::CONTROL_HEIGHT),
		std::vector<std::string>{EditorTexts::get("heroes.all"), EditorTexts::get("heroes.exclusive"), EditorTexts::get("heroes.banned")}, 0, [this](int) { applyFilter(); },
		EditorTexts::get("heroes.filter"));

	const int listTop = UiTheme::CONTROL_HEIGHT + UiTheme::SPACING;
	list = std::make_shared<CSelectList>(Rect(0, listTop, listWidth, area.h - listTop), CSelectList::Mode::CHECK_SELECT, UiTheme::ROW_HEIGHT);
	list->onSelect = [this](int row) { selectEntry(shown.at(row)); };
	list->onCheck = [this](int row, bool on)
	{
		const int entry = shown.at(row);
		entries.at(entry).allowed = on;
		if(!on)
			removeDisposed(entries.at(entry).id);

		selectEntry(entry);
	};

	const int detailsLeft = listWidth + UiTheme::SPACING;
	playersLabel = std::make_shared<CLabel>(detailsLeft, PORTRAIT_SIZE + 24, UiTheme::FONT, ETextAlignment::TOPLEFT, UiTheme::TEXT, EditorTexts::get("heroes.exclusiveFor"));
	playersEditor = std::make_shared<CPlayerSetEditor>(Rect(detailsLeft, PORTRAIT_SIZE + 24 + UiTheme::CONTROL_HEIGHT, DETAILS_WIDTH, 0), std::set<PlayerColor>(), [this](const std::set<PlayerColor> & players)
	{
		if(selectedEntry < 0)
			return;

		const int heroId = entries.at(selectedEntry).id;
		if(static_cast<int>(players.size()) == PlayerColor::PLAYER_LIMIT_I)
			removeDisposed(heroId);
		else
			setDisposed(heroId, players);
	}, 1);

	playersLabel->disable();
	playersEditor->disable();

	applyFilter();
}

void EditorHeroesPage::applyFilter()
{
	const std::string keyword = filterField->getText();
	const int mode = modeChoice->getSelected();

	shown.clear();
	std::vector<CSelectList::Row> rows;
	int selectedRow = -1;

	for(size_t i = 0; i < entries.size(); ++i)
	{
		const auto & entry = entries[i];

		bool modeCheck = true;
		if(mode == 1)
			modeCheck = findDisposed(entry.id) != nullptr;
		else if(mode == 2)
			modeCheck = !entry.allowed;

		if(!modeCheck || (!keyword.empty() && !boost::algorithm::icontains(entry.name, keyword)))
			continue;

		CSelectList::Row row;
		row.text = entry.name;
		row.checked = entry.allowed;
		if(static_cast<int>(i) == selectedEntry)
			selectedRow = static_cast<int>(rows.size());

		rows.push_back(std::move(row));
		shown.push_back(static_cast<int>(i));
	}

	list->setRows(std::move(rows));
	list->setSelected(selectedRow);

	// the selected hero has been hidden by the filter
	if(selectedRow < 0 && selectedEntry >= 0)
		selectEntry(-1);
}

void EditorHeroesPage::selectEntry(int entry)
{
	selectedEntry = entry;
	updateDetails();
}

void EditorHeroesPage::updateDetails()
{
	{
		OBJECT_CONSTRUCTION;
		portrait.reset();

		if(selectedEntry >= 0)
		{
			const int listWidth = pos.w - DETAILS_WIDTH - UiTheme::SPACING;
			portrait = std::make_shared<CAnimImage>(AnimationPath::builtin("PortraitsLarge"), entries.at(selectedEntry).imageIndex, Rect(listWidth + UiTheme::SPACING, 0, PORTRAIT_SIZE, PORTRAIT_SIZE));
		}
	}

	const bool show = selectedEntry >= 0 && entries.at(selectedEntry).allowed;
	playersLabel->setEnabled(show);
	playersEditor->setEnabled(show);

	if(show)
	{
		std::set<PlayerColor> lockedPlayers;
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
			if(!map().players[i].canAnyonePlay())
				lockedPlayers.insert(PlayerColor(i));

		const auto * disposed = findDisposed(entries.at(selectedEntry).id);
		std::set<PlayerColor> players;
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
			if(!disposed || disposed->players.count(PlayerColor(i)))
				players.insert(PlayerColor(i));

		playersEditor->setValue(players);
		playersEditor->setLocked(lockedPlayers);
	}

	redraw();
}

const DisposedHero * EditorHeroesPage::findDisposed(int heroId) const
{
	for(const auto & disposed : disposedHeroes)
		if(disposed.heroId == heroId)
			return &disposed;

	return nullptr;
}

void EditorHeroesPage::setDisposed(int heroId, const std::set<PlayerColor> & players)
{
	for(auto & disposed : disposedHeroes)
	{
		if(disposed.heroId == heroId)
		{
			disposed.players = players;
			return;
		}
	}

	DisposedHero newHero; // portrait and name is used only in h3m; in vcmi format you can change those by adding custom heroes
	newHero.heroId = HeroTypeID(heroId);
	newHero.players = players;
	disposedHeroes.push_back(newHero);
}

void EditorHeroesPage::removeDisposed(int heroId)
{
	vstd::erase_if(disposedHeroes, [heroId](const DisposedHero & disposed) { return disposed.heroId == heroId; });
}

void EditorHeroesPage::apply()
{
	CMap & m = map();

	m.allowedHeroes.clear();
	for(const auto & entry : entries)
		if(entry.allowed)
			m.allowedHeroes.emplace(entry.id);

	m.disposedHeroes = disposedHeroes;
}

/*
 * EditorMapSettingsWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorMapSettingsWindow.h"

#include "EditorMap.h"
#include "EditorTexts.h"
#include "mapsettings/EditorAllowedPage.h"
#include "mapsettings/EditorEventsPage.h"
#include "mapsettings/EditorGeneralPage.h"
#include "mapsettings/EditorHeroesPage.h"
#include "mapsettings/EditorLossPage.h"
#include "mapsettings/EditorModsPage.h"
#include "mapsettings/EditorRumorsPage.h"
#include "mapsettings/EditorSettingsPage.h"
#include "mapsettings/EditorVictoryPage.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CTabBar.h"
#include "../widgets/UiTheme.h"

#include "../../lib/mapping/CMap.h"

EditorMapSettingsWindow::EditorMapSettingsWindow(EditorMap & editor)
	: CDialogWindow(Point(820, 620), EditorTexts::get("settings.title"))
	, editor(editor)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int tabHeight = UiTheme::CONTROL_HEIGHT + 4;
	const Rect pageArea(0, tabHeight + UiTheme::SPACING, size.x, size.y - tabHeight - UiTheme::SPACING);

	tabs = std::make_shared<CTabBar>(Rect(0, 0, size.x, tabHeight),
		std::vector<std::string>{
			EditorTexts::get("settings.general"),
			EditorTexts::get("settings.abilities"),
			EditorTexts::get("settings.spells"),
			EditorTexts::get("settings.artifacts"),
			EditorTexts::get("settings.mods"),
			EditorTexts::get("settings.victory"),
			EditorTexts::get("settings.loss"),
			EditorTexts::get("settings.events"),
			EditorTexts::get("settings.rumors"),
			EditorTexts::get("settings.heroes")
		}, 0, [this](int index) { showPage(index); });

	pages.push_back(std::make_shared<EditorGeneralPage>(editor, pageArea));
	pages.push_back(std::make_shared<EditorAllowedPage>(editor, pageArea, EditorAllowedPage::Kind::ABILITIES));
	pages.push_back(std::make_shared<EditorAllowedPage>(editor, pageArea, EditorAllowedPage::Kind::SPELLS));
	pages.push_back(std::make_shared<EditorAllowedPage>(editor, pageArea, EditorAllowedPage::Kind::ARTIFACTS));
	pages.push_back(std::make_shared<EditorModsPage>(editor, pageArea));
	pages.push_back(std::make_shared<EditorVictoryPage>(editor, pageArea));
	pages.push_back(std::make_shared<EditorLossPage>(editor, pageArea));
	pages.push_back(std::make_shared<EditorEventsPage>(editor, pageArea));
	pages.push_back(std::make_shared<EditorRumorsPage>(editor, pageArea));
	pages.push_back(std::make_shared<EditorHeroesPage>(editor, pageArea));

	for(size_t i = 1; i < pages.size(); ++i)
		pages[i]->disable();

	onAccept = [this]()
	{
		apply();
		return true;
	};
}

void EditorMapSettingsWindow::showPage(int index)
{
	for(size_t i = 0; i < pages.size(); ++i)
		pages[i]->setEnabled(static_cast<int>(i) == index);

	redraw();
}

void EditorMapSettingsWindow::apply()
{
	// the victory and loss pages add their events again
	editor.getMap()->triggeredEvents.clear();

	for(auto & page : pages)
		page->apply();

	editor.commitChangeWithoutRedraw();
}

void EditorMapSettingsWindow::show(EditorMap & editor)
{
	ENGINE->windows().createAndPushWindow<EditorMapSettingsWindow>(editor);
}

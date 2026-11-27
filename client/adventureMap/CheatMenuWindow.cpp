/*
 * CheatMenuWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CheatMenuWindow.h"

#include "../CServerHandler.h"
#include "../GameChatHandler.h"
#include "../GameInstance.h"
#include "../gui/Shortcut.h"
#include "../render/Colors.h"
#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"

#include "../../lib/GameLibrary.h"
#include "../../lib/texts/CGeneralTextHandler.h"

namespace
{
	struct CheatMenuEntry
	{
		bool isHeader;
		std::string textID;
		std::string helpTextID;
		std::string cheatCode;
	};

	const std::vector<CheatMenuEntry> leftColumn = {
		{ true,  "vcmi.cheatMenu.category.vision", "", "" },
		{ false, "vcmi.cheatMenu.revealMap", "vcmi.cheatMenu.revealMap.help", "vcmimap" },
		{ false, "vcmi.cheatMenu.hideMap", "vcmi.cheatMenu.hideMap.help", "vcmihidemap" },
		{ true,  "vcmi.cheatMenu.category.hero", "", "" },
		{ false, "vcmi.cheatMenu.giveSpells", "vcmi.cheatMenu.giveSpells.help", "vcmispells" },
		{ false, "vcmi.cheatMenu.giveArtifacts", "vcmi.cheatMenu.giveArtifacts.help", "vcmiartifacts" },
		{ false, "vcmi.cheatMenu.unlimitedMovement", "vcmi.cheatMenu.unlimitedMovement.help", "vcmimove" },
		{ false, "vcmi.cheatMenu.levelUp", "vcmi.cheatMenu.levelUp.help", "vcmilevel" },
	};

	const std::vector<CheatMenuEntry> rightColumn = {
		{ true,  "vcmi.cheatMenu.category.army", "", "" },
		{ false, "vcmi.cheatMenu.giveArmy", "vcmi.cheatMenu.giveArmy.help", "vcmiarchangel" },
		{ false, "vcmi.cheatMenu.buildTown", "vcmi.cheatMenu.buildTown.help", "vcmibuild" },
		{ true,  "vcmi.cheatMenu.category.resources", "", "" },
		{ false, "vcmi.cheatMenu.giveResources", "vcmi.cheatMenu.giveResources.help", "vcmiresources" },
		{ true,  "vcmi.cheatMenu.category.game", "", "" },
		{ false, "vcmi.cheatMenu.victory", "vcmi.cheatMenu.victory.help", "vcmiwin" },
		{ false, "vcmi.cheatMenu.defeat", "vcmi.cheatMenu.defeat.help", "vcmilose" },
	};

	const int columnX[2] = { 20, 250 };
	const int contentStartY = 55;
	const int headerStep = 20;
	const int buttonStep = 36;
}

CheatMenuWindow::CheatMenuWindow()
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	pos.w = 460;
	pos.h = 373;

	updateShadow();
	center();

	backgroundTexture = std::make_shared<CFilledTexture>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));

	labels.push_back(std::make_shared<CLabel>(
		pos.w / 2, 20, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, LIBRARY->generaltexth->translate("vcmi.cheatMenu.title")));

	const std::array<std::pair<int, const std::vector<CheatMenuEntry> &>, 2> columns = { {
		{ columnX[0], leftColumn },
		{ columnX[1], rightColumn }
	} };

	int maxY = contentStartY;
	for(const auto & [x, column] : columns)
	{
		int y = contentStartY;

		for(const auto & entry : column)
		{
			if(entry.isHeader)
			{
				labels.push_back(std::make_shared<CLabel>(
					x, y + 8, FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE, LIBRARY->generaltexth->translate(entry.textID)));
				y += headerStep;
				continue;
			}

			auto button = std::make_shared<CButton>(
				Point(x, y),
				AnimationPath::builtin("settingsWindow/button190"),
				CButton::tooltip("", LIBRARY->generaltexth->translate(entry.helpTextID)),
				[this, cheatCode = entry.cheatCode]() { castCheat(cheatCode); });
			button->setTextOverlay(LIBRARY->generaltexth->translate(entry.textID), FONT_SMALL, Colors::WHITE);
			buttons.push_back(button);

			y += buttonStep;
		}

		maxY = std::max(maxY, y);
	}

	buttonClose = std::make_shared<CButton>(
		Point((pos.w - 80) / 2, maxY + 10),
		AnimationPath::builtin("settingsWindow/button80"),
		CButton::tooltip(),
		[this]() { close(); },
		EShortcut::GLOBAL_CANCEL);
	buttonClose->setTextOverlay(LIBRARY->generaltexth->translate("vcmi.cheatMenu.close"), FONT_SMALL, Colors::YELLOW);
}

void CheatMenuWindow::castCheat(const std::string & cheatCode)
{
	GAME->server().getGameChat().sendMessageGameplay(cheatCode);
	close();
}

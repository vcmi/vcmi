/*
 * CStatisticScreen.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CStatisticScreen.h"
#include "../CGameInfo.h"

#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../gui/Shortcut.h"

#include "../widgets/Images.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/TextControls.h"
#include "../widgets/Buttons.h"
#include "../windows/InfoWindows.h"

#include "../../lib/VCMIDirs.h"
#include "../../lib/gameState/GameStatistics.h"
#include "../../lib/texts/CGeneralTextHandler.h"

#include <vstd/DateUtils.h>

CStatisticScreen::CStatisticScreen(StatisticDataSet stat)
	: CWindowObject(BORDERED), statistic(stat)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos = center(Rect(0, 0, 800, 600));
	filledBackground = std::make_shared<FilledTexturePlayerColored>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));
	filledBackground->setPlayerColor(PlayerColor(1));

	layout.push_back(std::make_shared<CMultiLineLabel>(Rect(0, 0, 800, 30), FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->translate("vcmi.statisticWindow.statistic")));
	layout.push_back(std::make_shared<TransparentFilledRectangle>(Rect(10, 30, 780, 530), ColorRGBA(0, 0, 0, 64), ColorRGBA(64, 80, 128, 255), 1));
	layout.push_back(std::make_shared<CButton>(Point(725, 564), AnimationPath::builtin("MUBCHCK"), CButton::tooltip(), [this](){ close(); }, EShortcut::GLOBAL_ACCEPT));

	buttonCsvSave = std::make_shared<CToggleButton>(Point(10, 564), AnimationPath::builtin("GSPBUT2"), CButton::tooltip(), [this](bool on){
		const boost::filesystem::path outPath = VCMIDirs::get().userCachePath() / "statistic";
		boost::filesystem::create_directories(outPath);

		const boost::filesystem::path filePath = outPath / (vstd::getDateTimeISO8601Basic(std::time(nullptr)) + ".csv");
		std::ofstream file(filePath.c_str());
		std::string csv = statistic.toCsv();
		file << csv;

		CInfoWindow::showInfoDialog(CGI->generaltexth->translate("vcmi.statisticWindow.csvSaved") + "\n\n" + filePath.string(), {});
	});
	buttonCsvSave->setTextOverlay(CGI->generaltexth->translate("vcmi.statisticWindow.csvSave"), EFonts::FONT_SMALL, Colors::YELLOW);
}

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
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"

#include "../widgets/Images.h"

#include "../../lib/gameState/GameStatistics.h"

CStatisticScreen::CStatisticScreen(StatisticDataSet statistic)
	: CWindowObject(BORDERED)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	pos = center(Rect(0, 0, 800, 600));
	filledBackground = std::make_shared<FilledTexturePlayerColored>(ImagePath::builtin("DiBoxBck"), Rect(0, 0, pos.w, pos.h));

	addUsedEvents(LCLICK);
}

void CStatisticScreen::clickPressed(const Point & cursorPosition)
{
	close();
}

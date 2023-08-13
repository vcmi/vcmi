/*
 * TurnTimerWidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "TurnTimerWidget.h"

#include "../CPlayerInterface.h"
#include "../render/Canvas.h"
#include "../render/Colors.h"
#include "../render/EFont.h"
#include "../gui/CGuiHandler.h"
#include "../gui/TextAlignment.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../../CCallback.h"
#include "../../lib/CPlayerState.h"

#include <SDL_render.h>

TurnTimerWidget::TurnTimerWidget():
	CIntObject(TIME),
	turnTime(0), lastTurnTime(0)
{
	OBJECT_CONSTRUCTION_CAPTURING(255-DISPOSE);
	
	watches = std::make_shared<CAnimImage>("VCMI/BATTLEQUEUE/STATESSMALL", 1, 0, 4, 6);
	label = std::make_shared<CLabel>(24, 2, FONT_BIG, ETextAlignment::TOPLEFT, Colors::YELLOW, std::to_string(turnTime));
	
	recActions &= ~DEACTIVATE;
}

void TurnTimerWidget::showAll(Canvas & to)
{
	to.drawColor(Rect(4, 4, 68, 24), ColorRGBA(10, 10, 10, 255));
	
	CIntObject::showAll(to);
}

void TurnTimerWidget::show(Canvas & to)
{
	showAll(to);
}

void TurnTimerWidget::setTime(int time)
{
	turnTime = time / 1000;
	std::ostringstream oss;
	oss << turnTime / 60 << ":" << std::setw(2) << std::setfill('0') << turnTime % 60;
	label->setText(oss.str());
}

void TurnTimerWidget::tick(uint32_t msPassed)
{
	if(LOCPLINT && LOCPLINT->cb && !LOCPLINT->battleInt)
	{
		auto player = LOCPLINT->cb->getCurrentPlayer();
		auto time = LOCPLINT->cb->getPlayerTurnTime(player);
		cachedTurnTime -= msPassed;
		if(time / 1000 != lastTurnTime / 1000)
		{
			//do not update timer on this tick
			lastTurnTime = time;
			cachedTurnTime = time;
		}
		else setTime(cachedTurnTime);
	}
}

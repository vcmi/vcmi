/*
 * TurnTimerWidget.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TurnTimerWidget.h"

#include "../CPlayerInterface.h"

#include "../render/EFont.h"
#include "../gui/CGuiHandler.h"
#include "../gui/TextAlignment.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../../CCallback.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/filesystem/ResourceID.h"

TurnTimerWidget::DrawRect::DrawRect(const Rect & r, const ColorRGBA & c):
	CIntObject(), rect(r), color(c)
{
}

void TurnTimerWidget::DrawRect::showAll(Canvas & to)
{
	to.drawColor(rect, color);
	
	CIntObject::showAll(to);
}

TurnTimerWidget::TurnTimerWidget():
	InterfaceObjectConfigurable(TIME),
	turnTime(0), lastTurnTime(0)
{
	REGISTER_BUILDER("drawRect", &TurnTimerWidget::buildDrawRect);
	
	recActions &= ~DEACTIVATE;
	
	const JsonNode config(ResourceID("config/widgets/turnTimer.json"));
	
	build(config);
}

std::shared_ptr<TurnTimerWidget::DrawRect> TurnTimerWidget::buildDrawRect(const JsonNode & config) const
{
	logGlobal->debug("Building widget TurnTimerWidget::DrawRect");
	auto rect = readRect(config["rect"]);
	auto color = readColor(config["color"]);
	return std::make_shared<TurnTimerWidget::DrawRect>(rect, color);
}

void TurnTimerWidget::show(Canvas & to)
{
	showAll(to);
}

void TurnTimerWidget::setTime(int time)
{
	turnTime = time / 1000;
	if(auto w = widget<CLabel>("timer"))
	{
		std::ostringstream oss;
		oss << turnTime / 60 << ":" << std::setw(2) << std::setfill('0') << turnTime % 60;
		w->setText(oss.str());
	}
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

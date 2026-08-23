/*
 * TurnOptionsTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TurnOptionsTab.h"

#include "../widgets/Buttons.h"
#include "../widgets/TextControls.h"

namespace
{
void extendButtonHitArea(const std::shared_ptr<CToggleButton> & button, const std::shared_ptr<CLabel> & label)
{
	if(button && label)
		button->pos = button->pos.include(label->pos);
}
}

TurnOptionsTab::TurnOptionsTab()
	: OptionsTabBase(JsonPath::builtin("config/widgets/turnOptionsTab.json"))
{
	extendButtonHitArea(widget<CToggleButton>("buttonTurnTimerAccumulate"), widget<CLabel>("labelTurnTimerAccumulate"));
	extendButtonHitArea(widget<CToggleButton>("buttonUnitTimerAccumulate"), widget<CLabel>("labelUnitTimerAccumulate"));
	extendButtonHitArea(widget<CToggleButton>("buttonSimturnsAI"), widget<CLabel>("labelSimturnsAI"));
}

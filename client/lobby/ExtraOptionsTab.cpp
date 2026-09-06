/*
 * ExtraOptionsTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ExtraOptionsTab.h"

#include "../widgets/Buttons.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"

namespace
{
void extendButtonHitArea(const std::shared_ptr<CToggleButton> & button, const std::shared_ptr<CLabel> & label)
{
	if(button && label)
		button->pos = button->pos.include(label->pos);
}
}

ExtraOptionsTab::ExtraOptionsTab()
	: OptionsTabBase(JsonPath::builtin("config/widgets/extraOptionsTab.json"))
{
	extendButtonHitArea(widget<CToggleButton>("buttonCheatAllowed"), widget<CLabel>("labelCheatAllowed"));
	extendButtonHitArea(widget<CToggleButton>("buttonUnlimitedReplay"), widget<CLabel>("labelUnlimitedReplay"));
	extendButtonHitArea(widget<CToggleButton>("buttonRecordGame"), widget<CLabel>("labelRecordGame"));

	if(auto textureCampaignOverdraw = widget<CFilledTexture>("textureCampaignOverdraw"))
		textureCampaignOverdraw->disable();
}

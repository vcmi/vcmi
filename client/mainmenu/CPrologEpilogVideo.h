/*
 * CPrologEpilogVideo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../windows/CWindowObject.h"

#include "../../lib/campaign/CampaignScenarioPrologEpilog.h"

class CMultiLineLabel;

class CPrologEpilogVideo : public CWindowObject
{
	CampaignScenarioPrologEpilog spe;
	int positionCounter;
	int voiceSoundHandle;
	std::function<void()> exitCb;

	std::shared_ptr<CMultiLineLabel> text;

public:
	CPrologEpilogVideo(CampaignScenarioPrologEpilog _spe, std::function<void()> callback);

	void clickPressed(const Point & cursorPosition) override;
	void show(Canvas & to) override;
};

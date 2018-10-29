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
#include "../../lib/mapping/CCampaignHandler.h"

class CMultiLineLabel;
class SDL_Surface;

class CPrologEpilogVideo : public CWindowObject
{
	CCampaignScenario::SScenarioPrologEpilog spe;
	int positionCounter;
	int voiceSoundHandle;
	std::function<void()> exitCb;

	std::shared_ptr<CMultiLineLabel> text;

public:
	CPrologEpilogVideo(CCampaignScenario::SScenarioPrologEpilog _spe, std::function<void()> callback);

	void clickLeft(const SDL_Event &event, tribool down) override;
	void show(SDL_Surface * to) override;
};

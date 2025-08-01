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
class VideoWidget;
class CFilledTexture;

class CPrologEpilogVideo : public CWindowObject
{
	CampaignScenarioPrologEpilog spe;
	int positionCounter;
	int voiceSoundHandle;
	uint32_t voiceDurationMilliseconds;
	uint32_t elapsedTimeMilliseconds;
	int videoSoundHandle;
	std::function<void()> exitCb;

	std::shared_ptr<CMultiLineLabel> text;
	std::shared_ptr<VideoWidget> videoPlayer;
	std::shared_ptr<CFilledTexture> backgroundAroundMenu;

	void exit();

	bool voiceStopped = false;

public:
	CPrologEpilogVideo(CampaignScenarioPrologEpilog _spe, std::function<void()> callback);

	void tick(uint32_t msPassed) override;
	void clickPressed(const Point & cursorPosition) override;
	void keyPressed(EShortcut key) override;
	void show(Canvas & to) override;
};

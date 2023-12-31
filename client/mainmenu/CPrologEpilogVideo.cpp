/*
 * CPrologEpilogVideo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "CPrologEpilogVideo.h"
#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/TextControls.h"
#include "../render/Canvas.h"


CPrologEpilogVideo::CPrologEpilogVideo(CampaignScenarioPrologEpilog _spe, std::function<void()> callback)
	: CWindowObject(BORDERED), spe(_spe), positionCounter(0), voiceSoundHandle(-1), videoSoundHandle(-1), exitCb(callback)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	addUsedEvents(LCLICK);
	pos = center(Rect(0, 0, 800, 600));
	updateShadow();

	auto audioData = CCS->videoh->getAudio(spe.prologVideo);
	videoSoundHandle = CCS->soundh->playSound(audioData);
	CCS->videoh->open(spe.prologVideo);
	CCS->musich->playMusic(spe.prologMusic, true, true);
	voiceSoundHandle = CCS->soundh->playSound(spe.prologVoice);
	auto onVoiceStop = [this]()
	{
		voiceStopped = true;
	};
	CCS->soundh->setCallback(voiceSoundHandle, onVoiceStop);

	text = std::make_shared<CMultiLineLabel>(Rect(100, 500, 600, 100), EFonts::FONT_BIG, ETextAlignment::CENTER, Colors::METALLIC_GOLD, spe.prologText.toString());
	text->scrollTextTo(-100);
}

void CPrologEpilogVideo::show(Canvas & to)
{
	to.drawColor(pos, Colors::BLACK);
	//BUG: some videos are 800x600 in size while some are 800x400
	//VCMI should center them in the middle of the screen. Possible but needs modification
	//of video player API which I'd like to avoid until we'll get rid of Windows-specific player
	CCS->videoh->update(pos.x, pos.y, to.getInternalSurface(), true, false);

	//move text every 5 calls/frames; seems to be good enough
	++positionCounter;
	if(positionCounter % 5 == 0)
		text->scrollTextBy(1);
	else
		text->showAll(to); // blit text over video, if needed

	if(text->textSize.y + 100 < positionCounter / 5 && voiceStopped)
		clickPressed(GH.getCursorPosition());
}

void CPrologEpilogVideo::clickPressed(const Point & cursorPosition)
{
	close();
	CCS->soundh->stopSound(voiceSoundHandle);
	CCS->soundh->stopSound(videoSoundHandle);
	exitCb();
}

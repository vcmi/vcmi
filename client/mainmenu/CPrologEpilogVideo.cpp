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
#include "../gui/WindowHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/FramerateManager.h"
#include "../widgets/TextControls.h"
#include "../render/Canvas.h"


CPrologEpilogVideo::CPrologEpilogVideo(CampaignScenarioPrologEpilog _spe, std::function<void()> callback)
	: CWindowObject(BORDERED), spe(_spe), positionCounter(0), voiceSoundHandle(-1), videoSoundHandle(-1), exitCb(callback), elapsedTime(0)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	addUsedEvents(LCLICK);
	pos = center(Rect(0, 0, 800, 600));
	updateShadow();

	auto audioData = CCS->videoh->getAudio(spe.prologVideo);
	videoSoundHandle = CCS->soundh->playSound(audioData);
	CCS->videoh->open(spe.prologVideo);
	CCS->musich->playMusic(spe.prologMusic, true, true);
	voiceDuration = CCS->soundh->getSoundDuration(spe.prologVoice);
	voiceSoundHandle = CCS->soundh->playSound(spe.prologVoice);
	auto onVoiceStop = [this]()
	{
		voiceStopped = true;
		elapsedTime = 0.0;
	};
	CCS->soundh->setCallback(voiceSoundHandle, onVoiceStop);

	text = std::make_shared<CMultiLineLabel>(Rect(100, 500, 600, 100), EFonts::FONT_BIG, ETextAlignment::CENTER, Colors::METALLIC_GOLD, spe.prologText.toString());
	text->scrollTextTo(-50);
}

void CPrologEpilogVideo::show(Canvas & to)
{
	elapsedTime += GH.framerate().getElapsedMilliseconds() / 1000.0;

	to.drawColor(pos, Colors::BLACK);
	//some videos are 800x600 in size while some are 800x400
	CCS->videoh->update(pos.x, pos.y + (CCS->videoh->size().y == 400 ? 100 : 0), to.getInternalSurface(), true, false);

	const double speed = (voiceDuration == 0.0) ? 0.1 : (voiceDuration / (text->textSize.y));

	if(elapsedTime > speed && text->textSize.y - 50 > positionCounter)
	{
		text->scrollTextBy(1);
		elapsedTime = 0.0;
		++positionCounter;
	}
	else
	{
		text->showAll(to); // blit text over video, if needed

		if(elapsedTime > (voiceDuration == 0.0 ? 6.0 : 3.0) && voiceStopped)
			clickPressed(GH.getCursorPosition());
	}
}

void CPrologEpilogVideo::clickPressed(const Point & cursorPosition)
{
	close();
	CCS->soundh->stopSound(voiceSoundHandle);
	CCS->soundh->stopSound(videoSoundHandle);
	exitCb();
}

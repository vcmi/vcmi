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
	: CWindowObject(BORDERED), spe(_spe), positionCounter(0), voiceSoundHandle(-1), videoSoundHandle(-1), exitCb(callback), elapsedTimeMilliseconds(0)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	addUsedEvents(LCLICK | TIME);
	pos = center(Rect(0, 0, 800, 600));
	updateShadow();

	auto audioData = CCS->videoh->getAudio(spe.prologVideo);
	videoSoundHandle = CCS->soundh->playSound(audioData);
	CCS->videoh->open(spe.prologVideo);
	CCS->musich->playMusic(spe.prologMusic, true, true);
	voiceDurationMilliseconds = CCS->soundh->getSoundDurationMilliseconds(spe.prologVoice);
	voiceSoundHandle = CCS->soundh->playSound(spe.prologVoice);
	auto onVoiceStop = [this]()
	{
		voiceStopped = true;
		elapsedTimeMilliseconds = 0;
	};
	CCS->soundh->setCallback(voiceSoundHandle, onVoiceStop);

	text = std::make_shared<CMultiLineLabel>(Rect(100, 500, 600, 100), EFonts::FONT_BIG, ETextAlignment::CENTER, Colors::METALLIC_GOLD, spe.prologText.toString());
	text->scrollTextTo(-50); // beginning of text in the vertical middle of black area
}

void CPrologEpilogVideo::tick(uint32_t msPassed)
{
	elapsedTimeMilliseconds += msPassed;

	const uint32_t speed = (voiceDurationMilliseconds == 0) ? 150 : (voiceDurationMilliseconds / (text->textSize.y));

	if(elapsedTimeMilliseconds > speed && text->textSize.y - 50 > positionCounter)
	{
		text->scrollTextBy(1);
		elapsedTimeMilliseconds -= speed;
		++positionCounter;
	}
	else if(elapsedTimeMilliseconds > (voiceDurationMilliseconds == 0 ? 8000 : 3000) && voiceStopped) // pause after completed scrolling (longer for intros missing voice)
		clickPressed(GH.getCursorPosition());
}

void CPrologEpilogVideo::show(Canvas & to)
{
	to.drawColor(pos, Colors::BLACK);
	//some videos are 800x600 in size while some are 800x400
	CCS->videoh->update(pos.x, pos.y + (CCS->videoh->size().y == 400 ? 100 : 0), to.getInternalSurface(), true, false);

	text->showAll(to); // blit text over video, if needed
}

void CPrologEpilogVideo::clickPressed(const Point & cursorPosition)
{
	close();
	CCS->soundh->stopSound(voiceSoundHandle);
	CCS->soundh->stopSound(videoSoundHandle);
	if(exitCb)
		exitCb();
}

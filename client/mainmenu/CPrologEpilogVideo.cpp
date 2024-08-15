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
#include "../media/IMusicPlayer.h"
#include "../media/ISoundPlayer.h"
//#include "../gui/WindowHandler.h"
#include "../gui/CGuiHandler.h"
//#include "../gui/FramerateManager.h"
#include "../widgets/TextControls.h"
#include "../widgets/VideoWidget.h"
#include "../widgets/Images.h"
#include "../render/Canvas.h"

CPrologEpilogVideo::CPrologEpilogVideo(CampaignScenarioPrologEpilog _spe, std::function<void()> callback)
	: CWindowObject(BORDERED), spe(_spe), positionCounter(0), voiceSoundHandle(-1), videoSoundHandle(-1), exitCb(callback), elapsedTimeMilliseconds(0)
{
	OBJECT_CONSTRUCTION;
	addUsedEvents(LCLICK | TIME);
	pos = center(Rect(0, 0, 800, 600));

	backgroundAroundMenu = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(-pos.x, -pos.y, GH.screenDimensions().x, GH.screenDimensions().y));

	//TODO: remove hardcoded paths. Some of campaigns video actually consist from 2 parts
	// however, currently our campaigns format expects only	a single video file
	static const std::map<VideoPath, VideoPath> pairedVideoFiles = {
		{ VideoPath::builtin("EVIL2AP1"),  VideoPath::builtin("EVIL2AP2") },
		{ VideoPath::builtin("H3ABdb4"),   VideoPath::builtin("H3ABdb4b") },
		{ VideoPath::builtin("H3x2_RNe1"), VideoPath::builtin("H3x2_RNe2") },
	};

	if (pairedVideoFiles.count(spe.prologVideo))
		videoPlayer = std::make_shared<VideoWidget>(Point(0, 0), spe.prologVideo, pairedVideoFiles.at(spe.prologVideo), true);
	else
		videoPlayer = std::make_shared<VideoWidget>(Point(0, 0), spe.prologVideo, true);

	//some videos are 800x600 in size while some are 800x400
	if (videoPlayer->pos.h == 400)
		videoPlayer->moveBy(Point(0, 100));

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

	videoPlayer->show(to);
	text->showAll(to); // blit text over video, if needed
}

void CPrologEpilogVideo::clickPressed(const Point & cursorPosition)
{
	close();
	CCS->soundh->resetCallback(voiceSoundHandle); // reset callback to avoid memory corruption since 'this' will be destroyed
	CCS->soundh->stopSound(voiceSoundHandle);
	CCS->soundh->stopSound(videoSoundHandle);
	if(exitCb)
		exitCb();
}

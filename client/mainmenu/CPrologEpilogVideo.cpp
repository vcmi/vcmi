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

#include "../media/IMusicPlayer.h"
#include "../media/ISoundPlayer.h"
#include "../GameEngine.h"
#include "../gui/Shortcut.h"
#include "../widgets/TextControls.h"
#include "../widgets/VideoWidget.h"
#include "../widgets/Images.h"
#include "../render/Canvas.h"
#include "../lib/CConfigHandler.h"

CPrologEpilogVideo::CPrologEpilogVideo(CampaignScenarioPrologEpilog _spe, std::function<void()> callback)
	: CWindowObject(BORDERED), spe(_spe), positionCounter(0), voiceSoundHandle(-1), videoSoundHandle(-1), exitCb(callback), elapsedTimeMilliseconds(0)
{
	OBJECT_CONSTRUCTION;
	addUsedEvents(LCLICK | TIME);
	pos = center(Rect(0, 0, 800, 600));

	backgroundAroundMenu = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), Rect(-pos.x, -pos.y, ENGINE->screenDimensions().x, ENGINE->screenDimensions().y));

	if (!spe.prologVideo.second.empty())
		videoPlayer = std::make_shared<VideoWidget>(Point(0, 0), spe.prologVideo.first, spe.prologVideo.second, true);
	else
		videoPlayer = std::make_shared<VideoWidget>(Point(0, 0), spe.prologVideo.first, true);

	//some videos are 800x600 in size while some are 800x400
	if (videoPlayer->pos.h == 400)
		videoPlayer->moveBy(Point(0, 100));

	ENGINE->music().setVolume(ENGINE->music().getVolume() / 2); // background volume is too loud by default
	ENGINE->music().playMusic(spe.prologMusic, true, true);
	voiceDurationMilliseconds = ENGINE->sound().getSoundDurationMilliseconds(spe.prologVoice);
	voiceSoundHandle = ENGINE->sound().playSound(spe.prologVoice);
	auto onVoiceStop = [this]()
	{
		voiceStopped = true;
		elapsedTimeMilliseconds = 0;
	};
	ENGINE->sound().setCallback(voiceSoundHandle, onVoiceStop);

	if(!settings["general"]["enableSubtitle"].Bool())
		return;

	text = std::make_shared<CMultiLineLabel>(Rect(100, 500, 600, 100), EFonts::FONT_BIG, ETextAlignment::CENTER, Colors::METALLIC_GOLD, spe.prologText.toString());
	if(text->getLines().size() == 3)
		text->scrollTextTo(-25); // beginning of text in the vertical middle of black area
	else if(text->getLines().size() > 3)
		text->scrollTextTo(-50); // beginning of text in the vertical middle of black area
}

void CPrologEpilogVideo::tick(uint32_t msPassed)
{
	elapsedTimeMilliseconds += msPassed;

	const uint32_t speed = (!text || voiceDurationMilliseconds == 0) ? 150 : (voiceDurationMilliseconds / (text->textSize.y));

	if(text && elapsedTimeMilliseconds > speed && text->textSize.y - 50 > positionCounter)
	{
		text->scrollTextBy(1);
		elapsedTimeMilliseconds -= speed;
		++positionCounter;
	}
	else if(elapsedTimeMilliseconds > (voiceDurationMilliseconds == 0 ? 8000 : 3000) && voiceStopped) // pause after completed scrolling (longer for intros missing voice)
		clickPressed(ENGINE->getCursorPosition());
}

void CPrologEpilogVideo::show(Canvas & to)
{
	to.drawColor(pos, Colors::BLACK);

	videoPlayer->show(to);
	if(text)
		text->showAll(to); // blit text over video, if needed
}

void CPrologEpilogVideo::exit()
{
	ENGINE->music().setVolume(ENGINE->music().getVolume() * 2); // restore background volume
	close();
	ENGINE->sound().resetCallback(voiceSoundHandle); // reset callback to avoid memory corruption since 'this' will be destroyed
	ENGINE->sound().stopSound(voiceSoundHandle);
	ENGINE->sound().stopSound(videoSoundHandle);
	if(exitCb)
		exitCb();
}

void CPrologEpilogVideo::clickPressed(const Point & cursorPosition)
{
	exit();
}

void CPrologEpilogVideo::keyPressed(EShortcut key)
{
	if(key == EShortcut::GLOBAL_RETURN)
		exit();
}

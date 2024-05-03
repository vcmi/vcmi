/*
 * TextControls.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "VideoWidget.h"

#include "../CGameInfo.h"
#include "../gui/CGuiHandler.h"
#include "../gui/WindowHandler.h"
#include "../media/ISoundPlayer.h"
#include "../media/IVideoPlayer.h"
#include "../render/Canvas.h"

VideoWidget::VideoWidget(const Point & position, const VideoPath & looped)
	: VideoWidget(position, VideoPath(), looped)
{
	addUsedEvents(TIME);
}

VideoWidget::VideoWidget(const Point & position, const VideoPath & prologue, const VideoPath & looped)
	: current(prologue)
	, next(looped)
	, videoSoundHandle(-1)
{
	if(current.empty())
		videoInstance = CCS->videoh->open(looped, false);
	else
		videoInstance = CCS->videoh->open(current, false);
}

VideoWidget::~VideoWidget() = default;

void VideoWidget::show(Canvas & to)
{
	if(videoInstance)
		videoInstance->show(pos.topLeft(), to);
}

void VideoWidget::activate()
{
	if(videoInstance)
		videoInstance->playAudio();

	if(videoSoundHandle != -1)
	{
		CCS->soundh->setCallback(
			videoSoundHandle,
			[this]()
			{
				if(GH.windows().isTopWindow(this))
					this->videoSoundHandle = -1;
			}
		);
	}
}

void VideoWidget::deactivate()
{
	CCS->soundh->stopSound(videoSoundHandle);
}

void VideoWidget::showAll(Canvas & to)
{
	if(videoInstance)
		videoInstance->show(pos.topLeft(), to);
}

void VideoWidget::tick(uint32_t msPassed)
{
	if(videoInstance)
	{
		videoInstance->tick(msPassed);

		if(videoInstance->videoEnded())
			videoInstance = CCS->videoh->open(next, false);
	}
}

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
#include "../media/ISoundPlayer.h"
#include "../media/IVideoPlayer.h"
#include "../render/Canvas.h"

VideoWidgetBase::VideoWidgetBase(const Point & position, const VideoPath & video, bool playAudio)
	: playAudio(playAudio)
{
	addUsedEvents(TIME);
	pos += position;
	playVideo(video);
}

VideoWidgetBase::~VideoWidgetBase() = default;

void VideoWidgetBase::playVideo(const VideoPath & fileToPlay)
{
	videoInstance = CCS->videoh->open(fileToPlay, false);
	if (videoInstance)
	{
		pos.w = videoInstance->size().x;
		pos.h = videoInstance->size().y;
	}

	if (playAudio)
	{
		loadAudio(fileToPlay);
		if (isActive())
			startAudio();
	}
}

void VideoWidgetBase::show(Canvas & to)
{
	if(videoInstance)
		videoInstance->show(pos.topLeft(), to);
}

void VideoWidgetBase::loadAudio(const VideoPath & fileToPlay)
{
	if (!playAudio)
		return;

	audioData = CCS->videoh->getAudio(fileToPlay);
}

void VideoWidgetBase::startAudio()
{
	if(audioData.first == nullptr)
		return;

	audioHandle = CCS->soundh->playSound(audioData);

	if(audioHandle != -1)
	{
		CCS->soundh->setCallback(
			audioHandle,
			[this]()
			{
				this->audioHandle = -1;
			}
			);
	}
}

void VideoWidgetBase::stopAudio()
{
	if(audioHandle != -1)
	{
		CCS->soundh->resetCallback(audioHandle);
		CCS->soundh->stopSound(audioHandle);
		audioHandle = -1;
	}
}

void VideoWidgetBase::activate()
{
	CIntObject::activate();
	startAudio();
}

void VideoWidgetBase::deactivate()
{
	CIntObject::deactivate();
	stopAudio();
}

void VideoWidgetBase::showAll(Canvas & to)
{
	if(videoInstance)
		videoInstance->show(pos.topLeft(), to);
}

void VideoWidgetBase::tick(uint32_t msPassed)
{
	if(videoInstance)
	{
		videoInstance->tick(msPassed);

		if(videoInstance->videoEnded())
		{
			videoInstance.reset();
			stopAudio();
			onPlaybackFinished();
		}
	}
}

VideoWidget::VideoWidget(const Point & position, const VideoPath & prologue, const VideoPath & looped, bool playAudio)
	: VideoWidgetBase(position, prologue, playAudio)
	, loopedVideo(looped)
{
}

VideoWidget::VideoWidget(const Point & position, const VideoPath & looped, bool playAudio)
	: VideoWidgetBase(position, looped, playAudio)
	, loopedVideo(looped)
{
}

void VideoWidget::onPlaybackFinished()
{
	playVideo(loopedVideo);
}

VideoWidgetOnce::VideoWidgetOnce(const Point & position, const VideoPath & video, bool playAudio, const std::function<void()> & callback)
	: VideoWidgetBase(position, video, playAudio)
	, callback(callback)
{
}

void VideoWidgetOnce::onPlaybackFinished()
{
	callback();
}

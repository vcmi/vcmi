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
#include "TextControls.h"
#include "IVideoHolder.h"

#include "../GameEngine.h"
#include "../media/ISoundPlayer.h"
#include "../media/IVideoPlayer.h"
#include "../render/Canvas.h"
#include "../render/IScreenHandler.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/filesystem/Filesystem.h"

VideoWidgetBase::VideoWidgetBase(const Point & position, const VideoPath & video, bool playAudio)
	: VideoWidgetBase(position, video, playAudio, 1.0)
{
}

VideoWidgetBase::VideoWidgetBase(const Point & position, const VideoPath & video, bool playAudio, float scaleFactor)
	: playAudio(playAudio), scaleFactor(scaleFactor)
{
	addUsedEvents(TIME);
	pos += position;
	playVideo(video);
}

VideoWidgetBase::~VideoWidgetBase() = default;

void VideoWidgetBase::playVideo(const VideoPath & fileToPlay)
{
	OBJECT_CONSTRUCTION;

	if(settings["general"]["enableSubtitle"].Bool())
	{
		JsonPath subTitlePath = fileToPlay.toType<EResType::JSON>();
		JsonPath subTitlePathVideoDir = subTitlePath.addPrefix("VIDEO/");
		if(CResourceHandler::get()->existsResource(subTitlePath))
			subTitleData = JsonNode(subTitlePath);
		else if(CResourceHandler::get()->existsResource(subTitlePathVideoDir))
			subTitleData = JsonNode(subTitlePathVideoDir);
	}

	float preScaleFactor = 1;
	VideoPath videoFile = fileToPlay;
	if(ENGINE->screenHandler().getScalingFactor() > 1)
	{
		std::vector<int> factorsToCheck = {ENGINE->screenHandler().getScalingFactor(), 4, 3, 2};
		for(auto factorToCheck : factorsToCheck)
		{
			std::string name = boost::algorithm::to_upper_copy(videoFile.getName());
			boost::replace_all(name, "VIDEO/", std::string("VIDEO") + std::to_string(factorToCheck) + std::string("X/"));
			auto p = VideoPath::builtin(name).addPrefix("VIDEO" + std::to_string(factorToCheck) + "X/");
			if(CResourceHandler::get()->existsResource(p))
			{
				preScaleFactor = 1.0 / static_cast<float>(factorToCheck);
				videoFile = p;
				break;
			}
		}
	}

	videoInstance = ENGINE->video().open(videoFile, scaleFactor * preScaleFactor);
	if (videoInstance)
	{
		pos.w = videoInstance->size().x;
		pos.h = videoInstance->size().y;
		if(!subTitleData.isNull())
			subTitle = std::make_unique<CMultiLineLabel>(Rect(0, (pos.h / 5) * 4, pos.w, pos.h / 5), EFonts::FONT_HIGH_SCORE, ETextAlignment::CENTER, Colors::WHITE);
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
		to.draw(*videoInstance, pos.topLeft());
	if(subTitle)
		subTitle->showAll(to);
}

void VideoWidgetBase::loadAudio(const VideoPath & fileToPlay)
{
	if (!playAudio)
		return;

	audioData = ENGINE->video().getAudio(fileToPlay);
}

void VideoWidgetBase::startAudio()
{
	if(audioData.first == nullptr)
		return;

	audioHandle = ENGINE->sound().playSound(audioData);

	if(audioHandle != -1)
	{
		ENGINE->sound().setCallback(
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
		ENGINE->sound().resetCallback(audioHandle);
		ENGINE->sound().stopSound(audioHandle);
		audioHandle = -1;
	}
}

std::string VideoWidgetBase::getSubTitleLine(double timestamp)
{
	if(subTitleData.isNull())
		return {};

	for(auto & segment : subTitleData.Vector())
		if(timestamp > segment["timeStart"].Float() && timestamp < segment["timeEnd"].Float())
			return segment["text"].String();
	
	return {};
}

void VideoWidgetBase::activate()
{
	CIntObject::activate();
	if(audioHandle != -1)
		ENGINE->sound().resumeSound(audioHandle);
	else
		startAudio();
	if(videoInstance)
		videoInstance->activate();
}

void VideoWidgetBase::deactivate()
{
	CIntObject::deactivate();
	ENGINE->sound().pauseSound(audioHandle);
	if(videoInstance)
		videoInstance->deactivate();
}

void VideoWidgetBase::showAll(Canvas & to)
{
	if(videoInstance)
		to.draw(*videoInstance, pos.topLeft());
	if(subTitle)
		subTitle->showAll(to);
}

void VideoWidgetBase::tick(uint32_t msPassed)
{
	if(videoInstance)
	{
		videoInstance->tick(msPassed);

		if(!videoInstance->videoEnded() && subTitle)
			subTitle->setText(getSubTitleLine(videoInstance->timeStamp()));

		if(videoInstance->videoEnded())
		{
			videoInstance.reset();
			stopAudio();
			onPlaybackFinished();
			// WARNING: onPlaybackFinished call may destoy `this`. Make sure that this is the very last operation in this method!
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

VideoWidgetOnce::VideoWidgetOnce(const Point & position, const VideoPath & video, bool playAudio, IVideoHolder * owner)
	: VideoWidgetBase(position, video, playAudio)
	, owner(owner)
{
}

VideoWidgetOnce::VideoWidgetOnce(const Point & position, const VideoPath & video, bool playAudio, float scaleFactor, IVideoHolder * owner)
	: VideoWidgetBase(position, video, playAudio, scaleFactor)
	, owner(owner)
{
}

void VideoWidgetOnce::onPlaybackFinished()
{
	owner->onVideoPlaybackFinished();
}

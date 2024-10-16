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

#include "../CGameInfo.h"
#include "../gui/CGuiHandler.h"
#include "../media/ISoundPlayer.h"
#include "../media/IVideoPlayer.h"
#include "../render/Canvas.h"

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

	using SubTitlePath = ResourcePathTempl<EResType::SUBTITLE>;
	SubTitlePath subTitlePath = fileToPlay.toType<EResType::SUBTITLE>();
	SubTitlePath subTitlePathVideoDir = subTitlePath.addPrefix("VIDEO/");
	if(CResourceHandler::get()->existsResource(subTitlePath))
	{
		auto rawData = CResourceHandler::get()->load(subTitlePath)->readAll();
		srtContent = std::string(reinterpret_cast<char *>(rawData.first.get()), rawData.second);
	}
	if(CResourceHandler::get()->existsResource(subTitlePathVideoDir))
	{
		auto rawData = CResourceHandler::get()->load(subTitlePathVideoDir)->readAll();
		srtContent = std::string(reinterpret_cast<char *>(rawData.first.get()), rawData.second);
	}

	videoInstance = CCS->videoh->open(fileToPlay, scaleFactor);
	if (videoInstance)
	{
		pos.w = videoInstance->size().x;
		pos.h = videoInstance->size().y;
		subTitle = std::make_unique<CMultiLineLabel>(Rect(0, (pos.h / 5) * 4, pos.w, pos.h / 5), EFonts::FONT_HIGH_SCORE, ETextAlignment::CENTER, Colors::WHITE, "");
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
	if(subTitle)
		subTitle->showAll(to);
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

std::string VideoWidgetBase::getSubTitleLine(double timestamp)
{
	if(srtContent.empty())
		return "";
	
    std::regex exp("^\\s*(\\d+:\\d+:\\d+,\\d+)[^\\S\\n]+-->[^\\S\\n]+(\\d+:\\d+:\\d+,\\d+)((?:\\n(?!\\d+:\\d+:\\d+,\\d+\\b|\\n+\\d+$).*)*)", std::regex::multiline);
    std::smatch res;

    std::string::const_iterator searchStart(srtContent.cbegin());
    while (std::regex_search(searchStart, srtContent.cend(), res, exp))
    {
		std::vector<std::string> timestamp1Str;
		boost::split(timestamp1Str, static_cast<std::string>(res[1]), boost::is_any_of(":,"));
		std::vector<std::string> timestamp2Str;
		boost::split(timestamp2Str, static_cast<std::string>(res[2]), boost::is_any_of(":,"));
		double timestamp1 = std::stoi(timestamp1Str[0]) * 3600 + std::stoi(timestamp1Str[1]) * 60 + std::stoi(timestamp1Str[2]) + (std::stoi(timestamp1Str[3]) / 1000.0);
		double timestamp2 = std::stoi(timestamp2Str[0]) * 3600 + std::stoi(timestamp2Str[1]) * 60 + std::stoi(timestamp2Str[2]) + (std::stoi(timestamp2Str[3]) / 1000.0);
        std::string text = res[3];
		text.erase(0, text.find_first_not_of("\r\n\t "));

		if(timestamp > timestamp1 && timestamp < timestamp2)
			return text;
		
        searchStart = res.suffix().first;
    }
	return "";
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
	if(subTitle)
		subTitle->showAll(to);
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
	if(subTitle && videoInstance)
		subTitle->setText(getSubTitleLine(videoInstance->timeStamp()));
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

VideoWidgetOnce::VideoWidgetOnce(const Point & position, const VideoPath & video, bool playAudio, float scaleFactor, const std::function<void()> & callback)
	: VideoWidgetBase(position, video, playAudio, scaleFactor)
	, callback(callback)
{
}

void VideoWidgetOnce::onPlaybackFinished()
{
	callback();
}

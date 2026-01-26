/*
 * VideoWidget.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

#include "../lib/filesystem/ResourcePath.h"
#include "../lib/json/JsonNode.h"

class IVideoHolder;
class IVideoInstance;
class CMultiLineLabel;

class VideoWidgetBase : public CIntObject
{
	std::unique_ptr<IVideoInstance> videoInstance;
	std::unique_ptr<CMultiLineLabel> subTitle;

	std::pair<std::unique_ptr<ui8[]>, si64> audioData = {nullptr, 0};
	int audioHandle = -1;
	bool playAudio = false;
	float scaleFactor = 1.0;
	JsonNode subTitleData;

	void loadAudio(const VideoPath & file);
	void startAudio();
	void stopAudio();
	std::string getSubTitleLine(double timestamp);

	std::function<void()> playBackFinishedCallback;

protected:
	VideoWidgetBase(const Point & position, const VideoPath & video, bool playAudio);
	VideoWidgetBase(const Point & position, const VideoPath & video, bool playAudio, float scaleFactor);

	virtual void onPlaybackFinished() = 0;
	void playVideo(const VideoPath & video);

public:
	~VideoWidgetBase();

	void activate() override;
	void deactivate() override;
	void show(Canvas & to) override;
	void showAll(Canvas & to) override;
	void tick(uint32_t msPassed) override;

	void setPlaybackFinishedCallback(std::function<void()> cb);
};

class VideoWidget final: public VideoWidgetBase
{
	VideoPath loopedVideo;

	void onPlaybackFinished() final;
public:
	VideoWidget(const Point & position, const VideoPath & prologue, const VideoPath & looped, bool playAudio);
	VideoWidget(const Point & position, const VideoPath & looped, bool playAudio);
};

class VideoWidgetOnce final: public VideoWidgetBase
{
	IVideoHolder * owner;

	void onPlaybackFinished() final;
public:
	VideoWidgetOnce(const Point & position, const VideoPath & video, bool playAudio, IVideoHolder * owner);
	VideoWidgetOnce(const Point & position, const VideoPath & video, bool playAudio, float scaleFactor, IVideoHolder * owner);
};

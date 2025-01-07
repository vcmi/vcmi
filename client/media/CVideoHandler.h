/*
 * CVideoHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#ifndef DISABLE_VIDEO

#include "../lib/Point.h"
#include "IVideoPlayer.h"

struct SDL_Surface;
struct SDL_Texture;
struct AVFormatContext;
struct AVCodecContext;
struct AVCodecParameters;
struct AVCodec;
struct AVFrame;
struct AVIOContext;

VCMI_LIB_NAMESPACE_BEGIN
class CInputStream;
class Point;
VCMI_LIB_NAMESPACE_END

class FFMpegStream : boost::noncopyable
{
	std::unique_ptr<CInputStream> input;

	AVIOContext * context = nullptr;
	AVFormatContext * formatContext = nullptr;

	const AVCodec * codec = nullptr;
	AVCodecContext * codecContext = nullptr;
	int streamIndex = -1;

	AVFrame * frame = nullptr;

protected:
	bool openContext();
	void openCodec(int streamIndex);

	int findVideoStream() const;
	int findAudioStream() const;

	const AVCodecParameters * getCodecParameters() const;
	const AVCodecContext * getCodecContext() const;
	void decodeNextFrame();
	const AVFrame * getCurrentFrame() const;
	double getCurrentFrameEndTime() const;
	double getCurrentFrameDuration() const;

public:
	virtual ~FFMpegStream();

	bool openInput(const VideoPath & fname);
};

class CAudioInstance final : public FFMpegStream
{
public:
	std::pair<std::unique_ptr<ui8[]>, si64> extractAudio(const VideoPath & videoToOpen);
};

class CVideoInstance final : public IVideoInstance, public FFMpegStream
{
	friend class CVideoPlayer;

	struct SwsContext * sws = nullptr;
	SDL_Texture * textureRGB = nullptr;
	SDL_Texture * textureYUV = nullptr;
	SDL_Surface * surface = nullptr;
	Point dimensions;

	/// video playback start time point
	bool startTimeInitialized;
	bool deactivationStartTimeHandling;
	std::chrono::steady_clock::time_point startTime;
	std::chrono::steady_clock::time_point deactivationStartTime;

	void prepareOutput(float scaleFactor, bool useTextureOutput);
	
	const int MAX_FRAMESKIP = 5;

public:
	CVideoInstance();
	~CVideoInstance();

	bool openVideo();
	bool loadNextFrame();

	double timeStamp() final;
	bool videoEnded() final;
	Point size() final;

	void show(const Point & position, Canvas & canvas) final;
	void tick(uint32_t msPassed) final;
	void activate() final;
	void deactivate() final;
};

class CVideoPlayer final : public IVideoPlayer
{
	void openVideoFile(CVideoInstance & state, const VideoPath & fname);

public:
	std::unique_ptr<IVideoInstance> open(const VideoPath & name, float scaleFactor) final;
	std::pair<std::unique_ptr<ui8[]>, si64> getAudio(const VideoPath & videoToOpen) final;
};

#endif

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

#	include "IVideoPlayer.h"

#	include "../lib/Rect.h"

struct SDL_Surface;
struct SDL_Texture;
struct AVFormatContext;
struct AVCodecContext;
struct AVCodec;
struct AVFrame;
struct AVIOContext;

VCMI_LIB_NAMESPACE_BEGIN
class CInputStream;
VCMI_LIB_NAMESPACE_END

struct FFMpegStreamState
{
	AVIOContext * context = nullptr;
	AVFormatContext * formatContext = nullptr;

	const AVCodec * codec = nullptr;
	AVCodecContext * codecContext = nullptr;
	int streamIndex = -1;
};

struct FFMpegVideoOutput
{
	AVFrame * frame = nullptr;
	struct SwsContext * sws = nullptr;
	SDL_Texture * textureRGB = nullptr;
	SDL_Texture * textureYUV = nullptr;
	SDL_Surface * surface = nullptr;
	Point dimensions;

	/// video playback current progress, in seconds
	double frameTime = 0.0;
	bool videoEnded = false;
};

class CVideoInstance final : public IVideoInstance
{
	friend class CVideoPlayer;

	std::unique_ptr<CInputStream> input;

	FFMpegStreamState video;
	FFMpegVideoOutput output;

	void open(const VideoPath & fname);
	void openContext(FFMpegStreamState & streamState);
	void openCodec(FFMpegStreamState & streamState, int streamIndex);
	void openVideo();
	void prepareOutput(bool scaleToScreenSize, bool useTextureOutput);

	bool nextFrame();
	void close();
	void closeState(FFMpegStreamState & streamState);

public:
	~CVideoInstance();

	bool videoEnded() final;
	Point size() final;

	void show(const Point & position, Canvas & canvas) final;
	void tick(uint32_t msPassed) final;
	void playAudio() final;
};

class CVideoPlayer final : public IVideoPlayer
{
	bool openAndPlayVideoImpl(const VideoPath & name, const Point & position, bool useOverlay, bool scale, bool stopOnKey);
	void openVideoFile(CVideoInstance & state, const VideoPath & fname);

public:
	bool playIntroVideo(const VideoPath & name) final;
	void playSpellbookAnimation(const VideoPath & name, const Point & position) final;
	std::unique_ptr<IVideoInstance> open(const VideoPath & name, bool scaleToScreen) final;
};

#endif

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

struct SDL_Rect;
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
	void openContext();
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

	/// video playback current progress, in seconds
	double frameTime = 0.0;

	void prepareOutput(bool scaleToScreenSize, bool useTextureOutput);

public:
	~CVideoInstance();

	void openVideo();
	bool loadNextFrame();

	bool videoEnded() override;
	Point size() override;

	void show(const Point & position, Canvas & canvas) override;
	void tick(uint32_t msPassed) override;
};

class CVideoPlayer final : public IVideoPlayer
{
	void getVideoAndBackgroundRects(std::string_view name, const Point & position, SDL_Rect & video, SDL_Rect & RectbackgroundRect, const Point preferredLogicalResolution, const CVideoInstance & instance) const;
	bool getIntroRimTexture(SDL_Texture **introRimTexture) const;
	bool openAndPlayVideoImpl(const VideoPath & name, const Point & position, bool useOverlay, bool scale, bool stopOnKey, Point preferredLogicalResolution) const;
	void openVideoFile(CVideoInstance & state, const VideoPath & fname);

public:
	bool playIntroVideo(const VideoPath & name, const Point preferredLogicalResolution) override;
	void playSpellbookAnimation(const VideoPath & name, const Point & position, const Point preferredLogicalResolution) override;
	std::unique_ptr<IVideoInstance> open(const VideoPath & name, bool scaleToScreen) override;
	std::pair<std::unique_ptr<ui8[]>, si64> getAudio(const VideoPath & videoToOpen) override;
};

#endif

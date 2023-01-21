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

#include "../lib/Rect.h"

struct SDL_Surface;
struct SDL_Texture;

class IVideoPlayer
{
public:
	virtual bool open(std::string name, bool scale = false)=0; //true - succes
	virtual void close()=0;
	virtual bool nextFrame()=0;
	virtual void show(int x, int y, SDL_Surface *dst, bool update = true)=0;
	virtual void redraw(int x, int y, SDL_Surface *dst, bool update = true)=0; //reblits buffer
	virtual bool wait()=0;
	virtual int curFrame() const =0;
	virtual int frameCount() const =0;
};

class IMainVideoPlayer : public IVideoPlayer
{
public:
	std::string fname;  //name of current video file (empty if idle)

	virtual void update(int x, int y, SDL_Surface *dst, bool forceRedraw, bool update = true){}
	virtual bool openAndPlayVideo(std::string name, int x, int y, bool stopOnKey = false, bool scale = false)
	{
		return false;
	}
};

class CEmptyVideoPlayer : public IMainVideoPlayer
{
public:
	int curFrame() const override {return -1;};
	int frameCount() const override {return -1;};
	void redraw( int x, int y, SDL_Surface *dst, bool update = true ) override {};
	void show( int x, int y, SDL_Surface *dst, bool update = true ) override {};
	bool nextFrame() override {return false;};
	void close() override {};
	bool wait() override {return false;};
	bool open(std::string name, bool scale = false) override {return false;};
};

#ifndef DISABLE_VIDEO

#include "../lib/filesystem/CInputStream.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

//compatibility for libav 9.18 in ubuntu 14.04, 52.66.100 is ffmpeg 2.2.3
#if (LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 66, 100))
inline AVFrame * av_frame_alloc()
{
	return avcodec_alloc_frame();
}

inline void av_frame_free(AVFrame ** frame)
{
	av_free(*frame);
	*frame = nullptr;
}
#endif // VCMI_USE_OLD_AVUTIL

//fix for travis-ci
#if (LIBAVUTIL_VERSION_INT < AV_VERSION_INT(52, 0, 0))
	#define AVPixelFormat PixelFormat
	#define AV_PIX_FMT_NONE PIX_FMT_NONE
	#define AV_PIX_FMT_YUV420P PIX_FMT_YUV420P
	#define AV_PIX_FMT_BGR565 PIX_FMT_BGR565
	#define AV_PIX_FMT_BGR24 PIX_FMT_BGR24
	#define AV_PIX_FMT_BGR32 PIX_FMT_BGR32
	#define AV_PIX_FMT_RGB565 PIX_FMT_RGB565
	#define AV_PIX_FMT_RGB24 PIX_FMT_RGB24
	#define AV_PIX_FMT_RGB32 PIX_FMT_RGB32
#endif

class CVideoPlayer : public IMainVideoPlayer
{
	int stream;					// stream index in video
	AVFormatContext *format;
	AVCodecContext *codecContext; // codec context for stream
	const AVCodec *codec;
	AVFrame *frame;
	struct SwsContext *sws;

	AVIOContext * context;

	// Destination. Either overlay or dest.

	SDL_Texture *texture;
	SDL_Surface *dest;
	Rect destRect;			// valid when dest is used
	Rect pos;				// destination on screen

	int refreshWait; // Wait several refresh before updating the image
	int refreshCount;
	bool doLoop;				// loop through video

	bool playVideo(int x, int y, bool stopOnKey);
	bool open(std::string fname, bool loop, bool useOverlay = false, bool scale = false);

public:
	CVideoPlayer();
	~CVideoPlayer();

	bool init();
	bool open(std::string fname, bool scale = false) override;
	void close() override;
	bool nextFrame() override;			// display next frame

	void show(int x, int y, SDL_Surface *dst, bool update = true) override; //blit current frame
	void redraw(int x, int y, SDL_Surface *dst, bool update = true) override; //reblits buffer
	void update(int x, int y, SDL_Surface *dst, bool forceRedraw, bool update = true) override; //moves to next frame if appropriate, and blits it or blits only if redraw parameter is set true

	// Opens video, calls playVideo, closes video; returns playVideo result (if whole video has been played)
	bool openAndPlayVideo(std::string name, int x, int y, bool stopOnKey = false, bool scale = false) override;

	//TODO:
	bool wait() override {return false;};
	int curFrame() const override {return -1;};
	int frameCount() const override {return -1;};

	// public to allow access from ffmpeg IO functions
	std::unique_ptr<CInputStream> data;
};

#endif


#pragma once

struct SDL_Surface;


class IVideoPlayer
{
public:
	virtual bool open(std::string name)=0; //true - succes
	virtual void close()=0;
	virtual bool nextFrame()=0;
	virtual void show(int x, int y, SDL_Surface *dst, bool update = true)=0;
	virtual void redraw(int x, int y, SDL_Surface *dst, bool update = true)=0; //reblits buffer
	virtual bool wait()=0;
	virtual int curFrame() const =0;
	virtual int frameCount() const =0;
	
	virtual void setScaling(bool enabled) =0;

};

class IMainVideoPlayer : public IVideoPlayer
{
public:
	std::string fname;  //name of current video file (empty if idle)

	virtual void update(int x, int y, SDL_Surface *dst, bool forceRedraw, bool update = true){}
	virtual bool openAndPlayVideo(std::string name, int x, int y, SDL_Surface *dst, bool stopOnKey = false) 
	{
		return false;
	}
};

class CEmptyVideoPlayer : public IMainVideoPlayer
{
public:
	virtual int curFrame() const {return -1;};
	virtual int frameCount() const {return -1;};
	virtual void redraw( int x, int y, SDL_Surface *dst, bool update = true ) {};
	virtual void show( int x, int y, SDL_Surface *dst, bool update = true ) {};
	virtual bool nextFrame() {return false;};
	virtual void close() {};
	virtual bool wait() {return false;};
	virtual bool open( std::string name ) {return false;};
	void setScaling(bool enabled) override {};
};


#ifndef DISABLE_VIDEO

#include "../lib/filesystem/CInputStream.h"

#include <SDL.h>
#include <SDL_video.h>
#if SDL_VERSION_ATLEAST(1,3,0) && !SDL_VERSION_ATLEAST(2,0,0)
#include <SDL_compat.h>
#endif

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

class CVideoPlayer : public IMainVideoPlayer
{
	int stream;					// stream index in video
	AVFormatContext *format;
	AVCodecContext *codecContext; // codec context for stream
	AVCodec *codec;
	AVFrame *frame; 
	struct SwsContext *sws;

	AVIOContext * context;

	// Destination. Either overlay or dest.
#ifdef VCMI_SDL1
	SDL_Overlay * overlay;
#else
	SDL_Texture *texture;
#endif

	SDL_Surface *dest;
	SDL_Rect destRect;			// valid when dest is used
	SDL_Rect pos;				// destination on screen

	int refreshWait; // Wait several refresh before updating the image
	int refreshCount;
	bool doLoop;				// loop through video
	
	bool doScale;

	bool playVideo(int x, int y, SDL_Surface *dst, bool stopOnKey);
	bool open(std::string fname, bool loop, bool useOverlay = false);

public:
	CVideoPlayer();
	~CVideoPlayer();

	bool init();
	bool open(std::string fname);
	void close();
	bool nextFrame();			// display next frame

	void show(int x, int y, SDL_Surface *dst, bool update = true); //blit current frame
	void redraw(int x, int y, SDL_Surface *dst, bool update = true); //reblits buffer
	void update(int x, int y, SDL_Surface *dst, bool forceRedraw, bool update = true); //moves to next frame if appropriate, and blits it or blits only if redraw parameter is set true
	
	// Opens video, calls playVideo, closes video; returns playVideo result (if whole video has been played)
	bool openAndPlayVideo(std::string name, int x, int y, SDL_Surface *dst, bool stopOnKey = false);

	//TODO:
	bool wait(){return false;};
	int curFrame() const {return -1;};
	int frameCount() const {return -1;};

	void setScaling(bool enabled) override {doScale = enabled;};

	// public to allow access from ffmpeg IO functions
	std::unique_ptr<CInputStream> data;
};

#endif


#include "StdInc.h"
#include <SDL.h>
#include "CVideoHandler.h"

#include "gui/CGuiHandler.h"
#include "gui/SDL_Extensions.h"
#include "CPlayerInterface.h"
#include "../lib/filesystem/Filesystem.h"

extern CGuiHandler GH; //global gui handler

#ifndef DISABLE_VIDEO
//reads events and returns true on key down
static bool keyDown()
{
	SDL_Event ev;
	while(SDL_PollEvent(&ev))
	{
		if(ev.type == SDL_KEYDOWN || ev.type == SDL_MOUSEBUTTONDOWN)
			return true;
	}
	return false;
}

#ifdef _MSC_VER
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#endif // _MSC_VER

// Define a set of functions to read data
static int lodRead(void* opaque, uint8_t* buf, int size)
{
	auto video = reinterpret_cast<CVideoPlayer *>(opaque);

	return video->data->read(buf, size);
}

static si64 lodSeek(void * opaque, si64 pos, int whence)
{
	auto video = reinterpret_cast<CVideoPlayer *>(opaque);

	if (whence & AVSEEK_SIZE)
		return video->data->getSize();

	return video->data->seek(pos);
}

CVideoPlayer::CVideoPlayer()
{
	format = nullptr;
	frame = nullptr;
	codec = nullptr;
	sws = nullptr;
	texture = nullptr;
	dest = nullptr;
	context = nullptr;

	// Register codecs. TODO: May be overkill. Should call a
	// combination of av_register_input_format() /
	// av_register_output_format() / av_register_protocol() instead.
	av_register_all();
}

bool CVideoPlayer::open(std::string fname, bool scale/* = false*/)
{
	return open(fname, true, false);
}

// loop = to loop through the video
// useOverlay = directly write to the screen.
bool CVideoPlayer::open(std::string fname, bool loop, bool useOverlay, bool scale /*= false*/)
{
	close();

	this->fname = fname;
	refreshWait = 3;
	refreshCount = -1;
	doLoop = loop;

	ResourceID resource(std::string("Video/") + fname, EResType::VIDEO);

	if (!CResourceHandler::get()->existsResource(resource))
	{
		logGlobal->errorStream() << "Error: video " << resource.getName() << " was not found";
		return false;
	}

	data = CResourceHandler::get()->load(resource);

	static const int BUFFER_SIZE = 4096;

	unsigned char * buffer  = (unsigned char *)av_malloc(BUFFER_SIZE);// will be freed by ffmpeg
	context = avio_alloc_context( buffer, BUFFER_SIZE, 0, (void *)this, lodRead, nullptr, lodSeek);

	format = avformat_alloc_context();
	format->pb = context;
	// filename is not needed - file was already open and stored in this->data;
	int avfopen = avformat_open_input(&format, "dummyFilename", nullptr, nullptr);

	if (avfopen != 0)
	{
		return false;
	}
	// Retrieve stream information
	if (avformat_find_stream_info(format, nullptr) < 0)
		return false;

	// Find the first video stream
	stream = -1;
	for(ui32 i=0; i<format->nb_streams; i++)
	{
		if (format->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			stream = i;
			break;
		}
	}

	if (stream < 0)
		// No video stream in that file
		return false;

	// Get a pointer to the codec context for the video stream
	codecContext = format->streams[stream]->codec;

	// Find the decoder for the video stream
	codec = avcodec_find_decoder(codecContext->codec_id);

	if (codec == nullptr)
	{
		// Unsupported codec
		return false;
	}

	// Open codec
	if ( avcodec_open2(codecContext, codec, nullptr) < 0 )
	{
		// Could not open codec
		codec = nullptr;
		return false;
	}
	// Allocate video frame
	frame = av_frame_alloc();
	
	//setup scaling
	if(scale)
	{
		pos.w = screen->w;		
		pos.h = screen->h;
	}
	else
	{
		pos.w  = codecContext->width;		
		pos.h = codecContext->height;		
	}

	// Allocate a place to put our YUV image on that screen
	if (useOverlay)
	{
		texture = SDL_CreateTexture( mainRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STATIC, pos.w, pos.h);
	}
	else
	{
		dest = CSDL_Ext::newSurface(pos.w, pos.h);
		destRect.x = destRect.y = 0;
		destRect.w = pos.w;
		destRect.h = pos.h;
	}

	if (texture == nullptr && dest == nullptr)
		return false;

	if (texture)
	{ // Convert the image into YUV format that SDL uses
		sws = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt, 
							 pos.w, pos.h,
							 AV_PIX_FMT_YUV420P,
							 SWS_BICUBIC, nullptr, nullptr, nullptr);
	}
	else
	{
		AVPixelFormat screenFormat = AV_PIX_FMT_NONE;
		if (screen->format->Bshift > screen->format->Rshift)
		{
			// this a BGR surface
			switch (screen->format->BytesPerPixel)
			{
				case 2: screenFormat = AV_PIX_FMT_BGR565; break;
				case 3: screenFormat = AV_PIX_FMT_BGR24; break;
				case 4: screenFormat = AV_PIX_FMT_BGR32; break;
				default: return false;
			}
		}
		else
		{
			// this a RGB surface
			switch (screen->format->BytesPerPixel)
			{
				case 2: screenFormat = AV_PIX_FMT_RGB565; break;
				case 3: screenFormat = AV_PIX_FMT_RGB24; break;
				case 4: screenFormat = AV_PIX_FMT_RGB32; break;
				default: return false;
			}
		}

		sws = sws_getContext(codecContext->width, codecContext->height, codecContext->pix_fmt, 
							 pos.w, pos.h, screenFormat, 
							 SWS_BICUBIC, nullptr, nullptr, nullptr);
	}

	if (sws == nullptr)
		return false;

	return true;
}

// Read the next frame. Return false on error/end of file.
bool CVideoPlayer::nextFrame()
{
	AVPacket packet;
	int frameFinished = 0;
	bool gotError = false;

	if (sws == nullptr)
		return false;

	while(!frameFinished)
	{
		int ret = av_read_frame(format, &packet);
		if (ret < 0)
		{
			// Error. It's probably an end of file.
			if (doLoop && !gotError)
			{
				// Rewind
				if (av_seek_frame(format, stream, 0, AVSEEK_FLAG_BYTE) < 0)
					break;
				gotError = true;
			}
			else
			{
				break;
			}
		}
		else
		{
			// Is this a packet from the video stream?
			if (packet.stream_index == stream)
			{
				// Decode video frame

				avcodec_decode_video2(codecContext, frame, &frameFinished, &packet);

				// Did we get a video frame?
				if (frameFinished)
				{
					AVPicture pict;

					if (texture) {
						avpicture_alloc(&pict, AV_PIX_FMT_YUV420P, pos.w, pos.h);

						sws_scale(sws, frame->data, frame->linesize,
								  0, codecContext->height, pict.data, pict.linesize);

						SDL_UpdateYUVTexture(texture, NULL, pict.data[0], pict.linesize[0],
								pict.data[1], pict.linesize[1],
								pict.data[2], pict.linesize[2]);
						avpicture_free(&pict);
					}
					else
					{
						pict.data[0] = (ui8 *)dest->pixels;
						pict.linesize[0] = dest->pitch;

						sws_scale(sws, frame->data, frame->linesize,
								  0, codecContext->height, pict.data, pict.linesize);
					}
				}
			}

			av_free_packet(&packet);
		}
	}

	return frameFinished != 0;
}

void CVideoPlayer::show( int x, int y, SDL_Surface *dst, bool update )
{
	if (sws == nullptr)
		return;

	pos.x = x;
	pos.y = y;
	CSDL_Ext::blitSurface(dest, &destRect, dst, &pos);

	if (update)
		SDL_UpdateRect(dst, pos.x, pos.y, pos.w, pos.h);
}

void CVideoPlayer::redraw( int x, int y, SDL_Surface *dst, bool update )
{
	show(x, y, dst, update);
}

void CVideoPlayer::update( int x, int y, SDL_Surface *dst, bool forceRedraw, bool update )
{
	if (sws == nullptr)
		return;

	if (refreshCount <= 0)
	{
		refreshCount = refreshWait;
		if (nextFrame())
			show(x,y,dst,update);
		else
		{
			open(fname);
			nextFrame();

			// The y position is wrong at the first frame.
			// Note: either the windows player or the linux player is
			// broken. Compensate here until the bug is found.
			show(x, y--, dst, update);
		}
	}
	else
	{
		redraw(x, y, dst, update);
	}

	refreshCount --;
}

void CVideoPlayer::close()
{
	fname = "";
	if (sws)
	{
		sws_freeContext(sws);
		sws = nullptr;
	}

	if (texture)
	{
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}

	if (dest)
	{
		SDL_FreeSurface(dest);
		dest = nullptr;
	}

	if (frame)
	{
		av_frame_free(&frame);//will be set to null		
	}

	if (codec)
	{
		avcodec_close(codecContext);
		codec = nullptr;
		codecContext = nullptr;
	}

	if (format)
	{
		avformat_close_input(&format);
	}

	if (context)
	{
		av_free(context);
		context = nullptr;
	}
}

// Plays a video. Only works for overlays.
bool CVideoPlayer::playVideo(int x, int y, SDL_Surface *dst, bool stopOnKey)
{
	// Note: either the windows player or the linux player is
	// broken. Compensate here until the bug is found.
	y--;

	pos.x = x;
	pos.y = y;

	while(nextFrame())
	{

		if(stopOnKey && keyDown())
			return false;

		SDL_RenderCopy(mainRenderer, texture, NULL, &pos);
		SDL_RenderPresent(mainRenderer);

		// Wait 3 frames
		GH.mainFPSmng->framerateDelay();
		GH.mainFPSmng->framerateDelay();
		GH.mainFPSmng->framerateDelay();
	}

	return true;
}

bool CVideoPlayer::openAndPlayVideo(std::string name, int x, int y, SDL_Surface *dst, bool stopOnKey, bool scale/* = false*/)
{
	open(name, false, true, scale);
	bool ret = playVideo(x, y, dst, stopOnKey);
	close();
	return ret;
}

CVideoPlayer::~CVideoPlayer()
{
	close();
}

#endif


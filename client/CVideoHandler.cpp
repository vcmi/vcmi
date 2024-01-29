/*
 * CVideoHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CVideoHandler.h"

#include "CMT.h"
#include "gui/CGuiHandler.h"
#include "eventsSDL/InputHandler.h"
#include "gui/FramerateManager.h"
#include "renderSDL/SDL_Extensions.h"
#include "CPlayerInterface.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CInputStream.h"

#include <SDL_render.h>

#ifndef DISABLE_VIDEO

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
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
	int bytes = static_cast<int>(video->data->read(buf, size));
	if(bytes == 0)
    	return AVERROR_EOF;

	return bytes;
}

static si64 lodSeek(void * opaque, si64 pos, int whence)
{
	auto video = reinterpret_cast<CVideoPlayer *>(opaque);

	if (whence & AVSEEK_SIZE)
		return video->data->getSize();

	return video->data->seek(pos);
}

// Define a set of functions to read data
static int lodReadAudio(void* opaque, uint8_t* buf, int size)
{
	auto video = reinterpret_cast<CVideoPlayer *>(opaque);
	int bytes = static_cast<int>(video->dataAudio->read(buf, size));
	if(bytes == 0)
    	return AVERROR_EOF;

	return bytes;
}

static si64 lodSeekAudio(void * opaque, si64 pos, int whence)
{
	auto video = reinterpret_cast<CVideoPlayer *>(opaque);

	if (whence & AVSEEK_SIZE)
		return video->dataAudio->getSize();

	return video->dataAudio->seek(pos);
}

CVideoPlayer::CVideoPlayer()
	: stream(-1)
	, format (nullptr)
	, codecContext(nullptr)
	, codec(nullptr)
	, frame(nullptr)
	, sws(nullptr)
	, context(nullptr)
	, texture(nullptr)
	, dest(nullptr)
	, destRect(0,0,0,0)
	, pos(0,0,0,0)
	, frameTime(0)
	, doLoop(false)
{}

bool CVideoPlayer::open(const VideoPath & fname, bool scale)
{
	return open(fname, true, false);
}

// loop = to loop through the video
// overlay = directly write to the screen.
bool CVideoPlayer::open(const VideoPath & videoToOpen, bool loop, bool overlay, bool scale)
{
	close();

	doLoop = loop;
	frameTime = 0;

	if (CResourceHandler::get()->existsResource(videoToOpen))
		fname = videoToOpen;
	else
		fname = videoToOpen.addPrefix("VIDEO/");

	if (!CResourceHandler::get()->existsResource(fname))
	{
		logGlobal->error("Error: video %s was not found", fname.getName());
		return false;
	}

	data = CResourceHandler::get()->load(fname);

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
		if (format->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			stream = i;
			break;
		}
	}

	if (stream < 0)
		// No video stream in that file
		return false;

	// Find the decoder for the video stream
	codec = avcodec_find_decoder(format->streams[stream]->codecpar->codec_id);

	if (codec == nullptr)
	{
		// Unsupported codec
		return false;
	}

	codecContext = avcodec_alloc_context3(codec);
	if(!codecContext)
		return false;
	// Get a pointer to the codec context for the video stream
	int ret = avcodec_parameters_to_context(codecContext, format->streams[stream]->codecpar);
	if (ret < 0)
	{
		//We cannot get codec from parameters
		avcodec_free_context(&codecContext);
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
	if (overlay)
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
				frameTime = 0;
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
				int rc = avcodec_send_packet(codecContext, &packet);
				if (rc >=0)
					packet.size = 0;
				rc = avcodec_receive_frame(codecContext, frame);
				if (rc >= 0)
					frameFinished = 1;
				// Did we get a video frame?
				if (frameFinished)
				{
					uint8_t *data[4];
					int linesize[4];

					if (texture) {
						av_image_alloc(data, linesize, pos.w, pos.h, AV_PIX_FMT_YUV420P, 1);

						sws_scale(sws, frame->data, frame->linesize,
								  0, codecContext->height, data, linesize);

						SDL_UpdateYUVTexture(texture, nullptr, data[0], linesize[0],
								data[1], linesize[1],
								data[2], linesize[2]);
						av_freep(&data[0]);
					}
					else
					{
						/* Avoid buffer overflow caused by sws_scale():
						 *     http://trac.ffmpeg.org/ticket/9254
						 * Currently (ffmpeg-4.4 with SSE3 enabled) sws_scale()
						 * has a few requirements for target data buffers on rescaling:
						 * 1. buffer has to be aligned to be usable for SIMD instructions
						 * 2. buffer has to be padded to allow small overflow by SIMD instructions
						 * Unfortunately SDL_Surface does not provide these guarantees.
						 * This means that atempt to rescale directly into SDL surface causes
						 * memory corruption. Usually it happens on campaign selection screen
						 * where short video moves start spinning on mouse hover.
						 *
						 * To fix [1.] we use av_malloc() for memory allocation.
						 * To fix [2.] we add an `ffmpeg_pad` that provides plenty of space.
						 * We have to use intermdiate buffer and then use memcpy() to land it
						 * to SDL_Surface.
						 */
						size_t pic_bytes = dest->pitch * dest->h;
						size_t ffmped_pad = 1024; /* a few bytes of overflow will go here */
						void * for_sws = av_malloc (pic_bytes + ffmped_pad);
						data[0] = (ui8 *)for_sws;
						linesize[0] = dest->pitch;

						sws_scale(sws, frame->data, frame->linesize,
								  0, codecContext->height, data, linesize);
						memcpy(dest->pixels, for_sws, pic_bytes);
						av_free(for_sws);
					}
				}
			}

			av_packet_unref(&packet);
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
	CSDL_Ext::blitSurface(dest, destRect, dst, pos.topLeft());

	if (update)
		CSDL_Ext::updateRect(dst, pos);
}

void CVideoPlayer::redraw( int x, int y, SDL_Surface *dst, bool update )
{
	show(x, y, dst, update);
}

void CVideoPlayer::update( int x, int y, SDL_Surface *dst, bool forceRedraw, bool update, std::function<void()> onVideoRestart)
{
	if (sws == nullptr)
		return;

#if (LIBAVUTIL_VERSION_MAJOR < 58)   
	auto packet_duration = frame->pkt_duration;
#else
	auto packet_duration = frame->duration;
#endif
	double frameEndTime = (frame->pts + packet_duration) * av_q2d(format->streams[stream]->time_base);
	frameTime += GH.framerate().getElapsedMilliseconds() / 1000.0;

	if (frameTime >= frameEndTime )
	{
		if (nextFrame())
			show(x,y,dst,update);
		else
		{
			if(onVideoRestart)
				onVideoRestart();
			VideoPath filenameToReopen = fname; // create copy to backup this->fname
			open(filenameToReopen);
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
}

void CVideoPlayer::close()
{
	fname = VideoPath();

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
	}
	if (codecContext)
	{
		avcodec_free_context(&codecContext);
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

std::pair<std::unique_ptr<ui8 []>, si64> CVideoPlayer::getAudio(const VideoPath & videoToOpen)
{
	std::pair<std::unique_ptr<ui8 []>, si64> dat(std::make_pair(nullptr, 0));

	VideoPath fnameAudio;

	if (CResourceHandler::get()->existsResource(videoToOpen))
		fnameAudio = videoToOpen;
	else
		fnameAudio = videoToOpen.addPrefix("VIDEO/");

	if (!CResourceHandler::get()->existsResource(fnameAudio))
	{
		logGlobal->error("Error: video %s was not found", fnameAudio.getName());
		return dat;
	}

	dataAudio = CResourceHandler::get()->load(fnameAudio);

	static const int BUFFER_SIZE = 4096;

	unsigned char * bufferAudio  = (unsigned char *)av_malloc(BUFFER_SIZE);// will be freed by ffmpeg
	AVIOContext * contextAudio = avio_alloc_context( bufferAudio, BUFFER_SIZE, 0, (void *)this, lodReadAudio, nullptr, lodSeekAudio);

	AVFormatContext * formatAudio = avformat_alloc_context();
	formatAudio->pb = contextAudio;
	// filename is not needed - file was already open and stored in this->data;
	int avfopen = avformat_open_input(&formatAudio, "dummyFilename", nullptr, nullptr);

	if (avfopen != 0)
	{
		return dat;
	}
	// Retrieve stream information
	if (avformat_find_stream_info(formatAudio, nullptr) < 0)
		return dat;

	// Find the first audio stream
	int streamAudio = -1;
	for(ui32 i = 0; i < formatAudio->nb_streams; i++)
	{
		if (formatAudio->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			streamAudio = i;
			break;
		}
	}

	if(streamAudio < 0)
		return dat;

	const AVCodec *codecAudio = avcodec_find_decoder(formatAudio->streams[streamAudio]->codecpar->codec_id);
		
	AVCodecContext *codecContextAudio;
	if (codecAudio != nullptr)
		codecContextAudio = avcodec_alloc_context3(codecAudio);

	// Get a pointer to the codec context for the audio stream
	if (streamAudio > -1)
	{
		int ret = avcodec_parameters_to_context(codecContextAudio, formatAudio->streams[streamAudio]->codecpar);
		if (ret < 0)
		{
			//We cannot get codec from parameters
			avcodec_free_context(&codecContextAudio);
		}
	}
	
	// Open codec
	AVFrame *frameAudio;
	if (codecAudio != nullptr)
	{
		if ( avcodec_open2(codecContextAudio, codecAudio, nullptr) < 0 )
		{
			// Could not open codec
			codecAudio = nullptr;
		}
		// Allocate audio frame
		frameAudio = av_frame_alloc();
	}
		
	AVPacket packet;

	std::vector<ui8> samples;

	while (av_read_frame(formatAudio, &packet) >= 0)
	{
		if(packet.stream_index == streamAudio)
		{
			int rc = avcodec_send_packet(codecContextAudio, &packet);
			if (rc >= 0)
				packet.size = 0;
			rc = avcodec_receive_frame(codecContextAudio, frameAudio);
			int bytesToRead = (frameAudio->nb_samples * 2 * (formatAudio->streams[streamAudio]->codecpar->bits_per_coded_sample / 8));
			if (rc >= 0)
				for (int s = 0; s < bytesToRead; s += sizeof(ui8))
				{
					ui8 value;
					memcpy(&value, &frameAudio->data[0][s], sizeof(ui8));
					samples.push_back(value);
				}
		}

		av_packet_unref(&packet);
	}

	typedef struct WAV_HEADER {
		ui8 RIFF[4] = {'R', 'I', 'F', 'F'};
		ui32 ChunkSize;
		ui8 WAVE[4] = {'W', 'A', 'V', 'E'};
		ui8 fmt[4] = {'f', 'm', 't', ' '};
		ui32 Subchunk1Size = 16;
		ui16 AudioFormat = 1;
		ui16 NumOfChan = 2;
		ui32 SamplesPerSec = 22050;
		ui32 bytesPerSec = 22050 * 2;
		ui16 blockAlign = 2;
		ui16 bitsPerSample = 16;
		ui8 Subchunk2ID[4] = {'d', 'a', 't', 'a'};
		ui32 Subchunk2Size;
	} wav_hdr;

	wav_hdr wav;
	wav.ChunkSize = samples.size() + sizeof(wav_hdr) - 8;
  	wav.Subchunk2Size = samples.size() + sizeof(wav_hdr) - 44;
	wav.SamplesPerSec = formatAudio->streams[streamAudio]->codecpar->sample_rate;
	wav.bitsPerSample = formatAudio->streams[streamAudio]->codecpar->bits_per_coded_sample;
	auto wavPtr = reinterpret_cast<ui8*>(&wav);

	dat = std::make_pair(std::make_unique<ui8[]>(samples.size() + sizeof(wav_hdr)), samples.size() + sizeof(wav_hdr));
	std::copy(wavPtr, wavPtr + sizeof(wav_hdr), dat.first.get());
	std::copy(samples.begin(), samples.end(), dat.first.get() + sizeof(wav_hdr));

	if (frameAudio)
		av_frame_free(&frameAudio);

	if (codecAudio)
	{
		avcodec_close(codecContextAudio);
		codecAudio = nullptr;
	}
	if (codecContextAudio)
		avcodec_free_context(&codecContextAudio);

	if (formatAudio)
		avformat_close_input(&formatAudio);

	if (contextAudio)
	{
		av_free(contextAudio);
		contextAudio = nullptr;
	}

	return dat;
}

Point CVideoPlayer::size()
{
	if(frame)
		return Point(frame->width, frame->height);
	else
		return Point(0, 0);
}

// Plays a video. Only works for overlays.
bool CVideoPlayer::playVideo(int x, int y, bool stopOnKey, bool overlay)
{
	// Note: either the windows player or the linux player is
	// broken. Compensate here until the bug is found.
	y--;

	pos.x = x;
	pos.y = y;
	frameTime = 0.0;

	auto lastTimePoint = boost::chrono::steady_clock::now();

	while(nextFrame())
	{
		if(stopOnKey)
		{
			GH.input().fetchEvents();
			if(GH.input().ignoreEventsUntilInput())
				return false;
		}

		SDL_Rect rect = CSDL_Ext::toSDL(pos);

		if(overlay)
		{
			SDL_RenderFillRect(mainRenderer, &rect);
		}
		else
		{
			SDL_RenderClear(mainRenderer);
		}
		SDL_RenderCopy(mainRenderer, texture, nullptr, &rect);
		SDL_RenderPresent(mainRenderer);

#if (LIBAVUTIL_VERSION_MAJOR < 58)
		auto packet_duration = frame->pkt_duration;
#else
		auto packet_duration = frame->duration;
#endif
		// Framerate delay
		double targetFrameTimeSeconds = packet_duration * av_q2d(format->streams[stream]->time_base);
		auto targetFrameTime = boost::chrono::milliseconds(static_cast<int>(1000 * (targetFrameTimeSeconds)));

		auto timePointAfterPresent = boost::chrono::steady_clock::now();
		auto timeSpentBusy = boost::chrono::duration_cast<boost::chrono::milliseconds>(timePointAfterPresent - lastTimePoint);

		if (targetFrameTime > timeSpentBusy)
			boost::this_thread::sleep_for(targetFrameTime - timeSpentBusy);

		lastTimePoint = boost::chrono::steady_clock::now();
	}

	return true;
}

bool CVideoPlayer::openAndPlayVideo(const VideoPath & name, int x, int y, EVideoType videoType)
{
	bool scale;
	bool stopOnKey;
	bool overlay;

	switch(videoType)
	{
		case EVideoType::INTRO:
			stopOnKey = true;
			scale = true;
			overlay = false;
			break;
		case EVideoType::SPELLBOOK:
		default:
			stopOnKey = false;
			scale = false;
			overlay = true;
	}
	open(name, false, true, scale);
	bool ret = playVideo(x, y,  stopOnKey, overlay);
	close();
	return ret;
}

CVideoPlayer::~CVideoPlayer()
{
	close();
}

#endif


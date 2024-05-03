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

#ifndef DISABLE_VIDEO

#include "../CMT.h"
#include "../CPlayerInterface.h"
#include "../eventsSDL/InputHandler.h"
#include "../gui/CGuiHandler.h"
#include "../gui/FramerateManager.h"
#include "../render/Canvas.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../lib/filesystem/CInputStream.h"
#include "../../lib/filesystem/Filesystem.h"

#include <SDL_render.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

// Define a set of functions to read data
static int lodRead(void * opaque, uint8_t * buf, int size)
{
	auto * data = static_cast<CInputStream *>(opaque);
	int bytes = static_cast<int>(data->read(buf, size));
	if(bytes == 0)
		return AVERROR_EOF;

	return bytes;
}

static si64 lodSeek(void * opaque, si64 pos, int whence)
{
	auto * data = static_cast<CInputStream *>(opaque);

	if(whence & AVSEEK_SIZE)
		return data->getSize();

	return data->seek(pos);
}

[[noreturn]] static void throwFFmpegError(int errorCode)
{
	std::array<char, AV_ERROR_MAX_STRING_SIZE> errorMessage{};
	av_strerror(errorCode, errorMessage.data(), errorMessage.size());

	throw std::runtime_error(errorMessage.data());
}

static std::unique_ptr<CInputStream> findVideoData(const VideoPath & videoToOpen)
{
	if(CResourceHandler::get()->existsResource(videoToOpen))
		return CResourceHandler::get()->load(videoToOpen);

	auto highQualityVideoToOpenWithDir = videoToOpen.addPrefix("VIDEO/");
	auto lowQualityVideo = videoToOpen.toType<EResType::VIDEO_LOW_QUALITY>();
	auto lowQualityVideoWithDir = highQualityVideoToOpenWithDir.toType<EResType::VIDEO_LOW_QUALITY>();

	if(CResourceHandler::get()->existsResource(highQualityVideoToOpenWithDir))
		return CResourceHandler::get()->load(highQualityVideoToOpenWithDir);

	if(CResourceHandler::get()->existsResource(lowQualityVideo))
		return CResourceHandler::get()->load(lowQualityVideo);

	return CResourceHandler::get()->load(lowQualityVideoWithDir);
}

void CVideoInstance::open(const VideoPath & videoToOpen)
{
	state.videoData = findVideoData(videoToOpen);

	static const int BUFFER_SIZE = 4096;

	auto * buffer = static_cast<unsigned char *>(av_malloc(BUFFER_SIZE)); // will be freed by ffmpeg
	state.context = avio_alloc_context(buffer, BUFFER_SIZE, 0, state.videoData.get(), lodRead, nullptr, lodSeek);

	state.formatContext = avformat_alloc_context();
	state.formatContext->pb = state.context;
	// filename is not needed - file was already open and stored in this->data;
	int avfopen = avformat_open_input(&state.formatContext, "dummyFilename", nullptr, nullptr);

	if(avfopen != 0)
		throwFFmpegError(avfopen);

	// Retrieve stream information
	int findStreamInfo = avformat_find_stream_info(state.formatContext, nullptr);

	if(avfopen < 0)
		throwFFmpegError(findStreamInfo);

	for(int i = 0; i < state.formatContext->nb_streams; i++)
	{
		if(state.formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video.streamIndex == -1)
		{
			openStream(video, i);
		}

		if(state.formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio.streamIndex == -1)
			openStream(audio, i);
	}
}

void CVideoInstance::openStream(FFMpegStreamState & streamState, int streamIndex)
{
	streamState.streamIndex = streamIndex;

	// Find the decoder for the stream
	streamState.codec = avcodec_find_decoder(state.formatContext->streams[streamIndex]->codecpar->codec_id);

	if(streamState.codec == nullptr)
		throw std::runtime_error("Unsupported codec");

	streamState.codecContext = avcodec_alloc_context3(streamState.codec);
	if(streamState.codecContext == nullptr)
		throw std::runtime_error("Failed to create codec context");

	// Get a pointer to the codec context for the video stream
	int ret = avcodec_parameters_to_context(streamState.codecContext, state.formatContext->streams[streamIndex]->codecpar);
	if(ret < 0)
	{
		//We cannot get codec from parameters
		avcodec_free_context(&streamState.codecContext);
		throwFFmpegError(ret);
	}

	// Open codec
	ret = avcodec_open2(streamState.codecContext, streamState.codec, nullptr);
	if(ret < 0)
	{
		// Could not open codec
		streamState.codec = nullptr;
		throwFFmpegError(ret);
	}
}

void CVideoInstance::prepareOutput(bool scaleToScreenSize, bool useTextureOutput)
{
	if (video.streamIndex == -1)
		throw std::runtime_error("Invalid file state! No video stream!");

	// Allocate video frame
	output.frame = av_frame_alloc();

	//setup scaling
	if(scaleToScreenSize)
	{
		output.dimensions.x = screen->w;
		output.dimensions.y = screen->h;
	}
	else
	{
		output.dimensions.x  = video.codecContext->width;
		output.dimensions.y = video.codecContext->height;
	}

	// Allocate a place to put our YUV image on that screen
	if (useTextureOutput)
	{
		output.texture = SDL_CreateTexture( mainRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, output.dimensions.x, output.dimensions.y);
		output.sws = sws_getContext(video.codecContext->width, video.codecContext->height, video.codecContext->pix_fmt,
							output.dimensions.x, output.dimensions.y, AV_PIX_FMT_YUV420P,
							 SWS_BICUBIC, nullptr, nullptr, nullptr);
	}
	else
	{
		output.surface = CSDL_Ext::newSurface(output.dimensions.x, output.dimensions.y);
		output.sws = sws_getContext(video.codecContext->width, video.codecContext->height, video.codecContext->pix_fmt,
							 output.dimensions.x, output.dimensions.y, AV_PIX_FMT_RGB32,
							 SWS_BICUBIC, nullptr, nullptr, nullptr);
	}

	if (output.sws == nullptr)
		throw std::runtime_error("Failed to create sws");
}

bool CVideoInstance::nextFrame()
{
	AVPacket packet;

	for(;;)
	{
		int ret = av_read_frame(state.formatContext, &packet);
		if(ret < 0)
		{
			if(ret == AVERROR_EOF)
				return false;
			throwFFmpegError(ret);
		}

		// Is this a packet from the video stream?
		if(packet.stream_index == video.streamIndex)
		{
			// Decode video frame
			int rc = avcodec_send_packet(video.codecContext, &packet);
			if(rc < 0)
				throwFFmpegError(ret);

			rc = avcodec_receive_frame(video.codecContext, output.frame);
			if(rc < 0)
				throwFFmpegError(ret);

			uint8_t * data[4];
			int linesize[4];

			if(output.texture)
			{
				av_image_alloc(data, linesize, output.dimensions.x, output.dimensions.y, AV_PIX_FMT_YUV420P, 1);
				sws_scale(output.sws, output.frame->data, output.frame->linesize, 0, video.codecContext->height, data, linesize);
				SDL_UpdateYUVTexture(output.texture, nullptr, data[0], linesize[0], data[1], linesize[1], data[2], linesize[2]);
				av_freep(&data[0]);
			}
			else
			{
				// Avoid buffer overflow caused by sws_scale():
				// http://trac.ffmpeg.org/ticket/9254

				size_t pic_bytes = output.surface->pitch * output.surface->h;
				size_t ffmped_pad = 1024; /* a few bytes of overflow will go here */
				void * for_sws = av_malloc(pic_bytes + ffmped_pad);
				data[0] = (ui8 *)for_sws;
				linesize[0] = output.surface->pitch;

				sws_scale(output.sws, output.frame->data, output.frame->linesize, 0, video.codecContext->height, data, linesize);
				memcpy(output.surface->pixels, for_sws, pic_bytes);
				av_free(for_sws);
			}
			av_packet_unref(&packet);
			return true;
		}
	}
}

bool CVideoInstance::videoEnded()
{
	return output.videoEnded;
}

void CVideoInstance::close()
{
	sws_freeContext(output.sws);
	av_frame_free(&output.frame);

	SDL_DestroyTexture(output.texture);
	SDL_FreeSurface(output.surface);

	// state.videoStream.codec???
	// state.audioStream.codec???

	avcodec_close(video.codecContext);
	avcodec_free_context(&video.codecContext);

	avcodec_close(audio.codecContext);
	avcodec_free_context(&audio.codecContext);

	avformat_close_input(&state.formatContext);
	av_free(state.context);

	output = FFMpegVideoOutput();
	video = FFMpegStreamState();
	audio = FFMpegStreamState();
	state = FFMpegFileState();
}

CVideoInstance::~CVideoInstance()
{
	close();
}

Point CVideoInstance::size()
{
	if(!output.frame)
		throw std::runtime_error("Invalid video frame!");

	return Point(output.frame->width, output.frame->height);
}

void CVideoInstance::show(const Point & position, Canvas & canvas)
{
	if(output.sws == nullptr)
		throw std::runtime_error("No video to show!");

	CSDL_Ext::blitSurface(output.surface, canvas.getInternalSurface(), position);
}

void CVideoInstance::tick(uint32_t msPassed)
{
	if(output.sws == nullptr)
		throw std::runtime_error("No video to show!");

	if(output.videoEnded)
		throw std::runtime_error("Video already ended!");

#	if(LIBAVUTIL_VERSION_MAJOR < 58)
	auto packet_duration = output.frame->pkt_duration;
#	else
	auto packet_duration = frame->duration;
#	endif
	double frameEndTime = (output.frame->pts + packet_duration) * av_q2d(state.formatContext->streams[video.streamIndex]->time_base);
	output.frameTime += msPassed / 1000.0;

	if(output.frameTime >= frameEndTime)
	{
		if(!nextFrame())
			output.videoEnded = true;
	}
}

#	if 0

std::pair<std::unique_ptr<ui8 []>, si64> CVideoPlayer::getAudio(const VideoPath & videoToOpen)
{
	std::pair<std::unique_ptr<ui8 []>, si64> dat(std::make_pair(nullptr, 0));

	FFMpegFileState audio;
	openVideoFile(audio, videoToOpen);

	if (audio.audioStream.streamIndex < 0)
	{
		closeVideoFile(audio);
		return { nullptr, 0};
	}

	// Open codec
	AVFrame *frameAudio = av_frame_alloc();
		
	AVPacket packet;

	std::vector<ui8> samples;

	while (av_read_frame(audio.formatContext, &packet) >= 0)
	{
		if(packet.stream_index == audio.audioStream.streamIndex)
		{
			int rc = avcodec_send_packet(audio.audioStream.codecContext, &packet);
			if (rc >= 0)
				packet.size = 0;
			rc = avcodec_receive_frame(audio.audioStream.codecContext, frameAudio);
			int bytesToRead = (frameAudio->nb_samples * 2 * (audio.formatContext->streams[audio.audioStream.streamIndex]->codecpar->bits_per_coded_sample / 8));
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
	wav.SamplesPerSec = audio.formatContext->streams[audio.audioStream.streamIndex]->codecpar->sample_rate;
	wav.bitsPerSample = audio.formatContext->streams[audio.audioStream.streamIndex]->codecpar->bits_per_coded_sample;
	auto wavPtr = reinterpret_cast<ui8*>(&wav);

	dat = std::make_pair(std::make_unique<ui8[]>(samples.size() + sizeof(wav_hdr)), samples.size() + sizeof(wav_hdr));
	std::copy(wavPtr, wavPtr + sizeof(wav_hdr), dat.first.get());
	std::copy(samples.begin(), samples.end(), dat.first.get() + sizeof(wav_hdr));

	if (frameAudio)
		av_frame_free(&frameAudio);

	closeVideoFile(audio);

	return dat;
}

#	endif

bool CVideoPlayer::openAndPlayVideoImpl(const VideoPath & name, const Point & position, bool useOverlay, bool scale, bool stopOnKey)
{
	CVideoInstance instance;

	instance.open(name);
	instance.prepareOutput(scale, useOverlay);

	auto lastTimePoint = boost::chrono::steady_clock::now();

	while(instance.nextFrame())
	{
		if(stopOnKey)
		{
			GH.input().fetchEvents();
			if(GH.input().ignoreEventsUntilInput())
				return false;
		}

		SDL_Rect rect;
		rect.x = position.x;
		rect.y = position.y;
		rect.w = instance.output.dimensions.x;
		rect.h = instance.output.dimensions.y;

		if(useOverlay)
			SDL_RenderFillRect(mainRenderer, &rect);
		else
			SDL_RenderClear(mainRenderer);

		SDL_RenderCopy(mainRenderer, instance.output.texture, nullptr, &rect);
		SDL_RenderPresent(mainRenderer);

#if (LIBAVUTIL_VERSION_MAJOR < 58)
		auto packet_duration = instance.output.frame->pkt_duration;
#else
		auto packet_duration = output.frame->duration;
#endif

		// Framerate delay
		double targetFrameTimeSeconds = packet_duration * av_q2d(instance.state.formatContext->streams[instance.video.streamIndex]->time_base);
		auto targetFrameTime = boost::chrono::milliseconds(static_cast<int>(1000 * (targetFrameTimeSeconds)));

		auto timePointAfterPresent = boost::chrono::steady_clock::now();
		auto timeSpentBusy = boost::chrono::duration_cast<boost::chrono::milliseconds>(timePointAfterPresent - lastTimePoint);

		if (targetFrameTime > timeSpentBusy)
			boost::this_thread::sleep_for(targetFrameTime - timeSpentBusy);

		lastTimePoint = boost::chrono::steady_clock::now();
	}
	return true;
}

bool CVideoPlayer::playIntroVideo(const VideoPath & name)
{
	return openAndPlayVideoImpl(name, Point(0,0), true, true, true);
}

void CVideoPlayer::playSpellbookAnimation(const VideoPath & name, const Point & position)
{
	openAndPlayVideoImpl(name, position, false, false, false);
}

std::unique_ptr<IVideoInstance> CVideoPlayer::open(const VideoPath & name, bool scaleToScreen)
{
	auto result = std::make_unique<CVideoInstance>();

	result->open(name);
	result->prepareOutput(scaleToScreen, false);

	return result;
}

std::pair<std::unique_ptr<ui8 []>, si64> CVideoPlayer::getAudio(const VideoPath & videoToOpen)
{
	return {nullptr, 0};
}

#endif

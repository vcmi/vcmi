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

#ifdef ENABLE_VIDEO

#include "ISoundPlayer.h"

#include "../CMT.h"
#include "../eventsSDL/InputHandler.h"
#include "../GameEngine.h"
#include "../render/Canvas.h"
#include "../render/IScreenHandler.h"
#include "../renderSDL/SDL_Extensions.h"

#include "../../lib/filesystem/CInputStream.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/texts/Languages.h"
#include "../../lib/GameLibrary.h"

#include <SDL_render.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

// Define a set of functions to read data
static int lodRead(void * opaque, uint8_t * buf, int size)
{
	auto * data = static_cast<CInputStream *>(opaque);
	auto bytesRead = data->read(buf, size);
	if(bytesRead == 0)
		return AVERROR_EOF;

	return bytesRead;
}

static si64 lodSeek(void * opaque, si64 pos, int whence)
{
	auto * data = static_cast<CInputStream *>(opaque);

	if(whence & AVSEEK_SIZE)
		return data->getSize();

	return data->seek(pos);
}

static void logFFmpegError(int errorCode)
{
	std::array<char, AV_ERROR_MAX_STRING_SIZE> errorMessage{};
	av_strerror(errorCode, errorMessage.data(), errorMessage.size());

	logGlobal->warn("Failed to open video file! Reason: %s", errorMessage.data());
}

[[noreturn]] static void throwFFmpegError(int errorCode)
{
	logFFmpegError(errorCode);

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

	if(CResourceHandler::get()->existsResource(lowQualityVideoWithDir))
		return CResourceHandler::get()->load(lowQualityVideoWithDir);

	return nullptr;
}

bool FFMpegStream::openInput(const VideoPath & videoToOpen)
{
	input = findVideoData(videoToOpen);

	return input != nullptr;
}

bool FFMpegStream::openContext()
{
	static const int BUFFER_SIZE = 4096;
	input->seek(0);

	auto * buffer = static_cast<unsigned char *>(av_malloc(BUFFER_SIZE)); // will be freed by ffmpeg
	context = avio_alloc_context(buffer, BUFFER_SIZE, 0, input.get(), lodRead, nullptr, lodSeek);

	formatContext = avformat_alloc_context();
	formatContext->pb = context;
	// filename is not needed - file was already open and stored in this->data;
	int avfopen = avformat_open_input(&formatContext, "dummyFilename", nullptr, nullptr);

	if(avfopen != 0)
	{
		logFFmpegError(avfopen);
		return false;
	}

	// Retrieve stream information
	int findStreamInfo = avformat_find_stream_info(formatContext, nullptr);

	if(avfopen < 0)
	{
		logFFmpegError(findStreamInfo);
		return false;
	}

	return true;
}

void FFMpegStream::openCodec(int desiredStreamIndex)
{
	streamIndex = desiredStreamIndex;

	// Find the decoder for the stream
	codec = avcodec_find_decoder(formatContext->streams[streamIndex]->codecpar->codec_id);

	if(codec == nullptr)
		throw std::runtime_error("Unsupported codec");

	codecContext = avcodec_alloc_context3(codec);
	if(codecContext == nullptr)
		throw std::runtime_error("Failed to create codec context");

	// Get a pointer to the codec context for the video stream
	int ret = avcodec_parameters_to_context(codecContext, formatContext->streams[streamIndex]->codecpar);
	if(ret < 0)
	{
		//We cannot get codec from parameters
		avcodec_free_context(&codecContext);
		throwFFmpegError(ret);
	}

	// Open codec
	ret = avcodec_open2(codecContext, codec, nullptr);
	if(ret < 0)
	{
		// Could not open codec
		codec = nullptr;
		throwFFmpegError(ret);
	}

	// Allocate video frame
	frame = av_frame_alloc();
}

const AVCodecParameters * FFMpegStream::getCodecParameters() const
{
	return formatContext->streams[streamIndex]->codecpar;
}

const AVCodecContext * FFMpegStream::getCodecContext() const
{
	return codecContext;
}

const AVFrame * FFMpegStream::getCurrentFrame() const
{
	return frame;
}

bool CVideoInstance::openVideo()
{
	if (!openContext())
		return false;

	openCodec(findVideoStream());
	return true;
}

void CVideoInstance::prepareOutput(float scaleFactor, bool useTextureOutput)
{
	//setup scaling
	dimensions = Point(getCodecContext()->width * scaleFactor, getCodecContext()->height * scaleFactor) * ENGINE->screenHandler().getScalingFactor();

	// Allocate a place to put our YUV image on that screen
	if (useTextureOutput)
	{
		std::array potentialFormats = {
			AV_PIX_FMT_YUV420P, // -> SDL_PIXELFORMAT_IYUV - most of H3 videos use YUV format, so it is preferred to save some space & conversion time
			AV_PIX_FMT_RGB32,   // -> SDL_PIXELFORMAT_ARGB8888 - some .smk videos actually use palette, so RGB > YUV. This is also our screen texture format
			AV_PIX_FMT_NONE
		};

		auto preferredFormat = avcodec_find_best_pix_fmt_of_list(potentialFormats.data(), getCodecContext()->pix_fmt, false, nullptr);

		if (preferredFormat == AV_PIX_FMT_YUV420P)
			textureYUV = SDL_CreateTexture( mainRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, dimensions.x, dimensions.y);
		else
			textureRGB = SDL_CreateTexture( mainRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, dimensions.x, dimensions.y);
		sws = sws_getContext(getCodecContext()->width, getCodecContext()->height, getCodecContext()->pix_fmt,
							dimensions.x, dimensions.y, preferredFormat,
							 SWS_BICUBIC, nullptr, nullptr, nullptr);
	}
	else
	{
		surface = CSDL_Ext::newSurface(dimensions);
		sws = sws_getContext(getCodecContext()->width, getCodecContext()->height, getCodecContext()->pix_fmt,
							 dimensions.x, dimensions.y, AV_PIX_FMT_RGB32,
							 SWS_BICUBIC, nullptr, nullptr, nullptr);
	}

	if (sws == nullptr)
		throw std::runtime_error("Failed to create sws");
}

void FFMpegStream::decodeNextFrame()
{
	int rc = avcodec_receive_frame(codecContext, frame);

	// frame extracted - data that was sent to codecContext before was sufficient
	if (rc == 0)
		return;

	// returning AVERROR(EAGAIN) is legal - this indicates that codec requires more data from input stream to decode next frame
	if(rc != AVERROR(EAGAIN))
		throwFFmpegError(rc);

	for(;;)
	{
		AVPacket packet;

		// codecContext does not have enough input data - read next packet from input stream
		int ret = av_read_frame(formatContext, &packet);
		if(ret < 0)
		{
			if(ret == AVERROR_EOF)
			{
				av_packet_unref(&packet);
				av_frame_free(&frame);
				frame = nullptr;
				return;
			}
			throwFFmpegError(ret);
		}

		// Is this a packet from the stream that needs decoding?
		if(packet.stream_index == streamIndex)
		{
			// Decode read packet
			// Note: this method may return AVERROR(EAGAIN). However this should never happen with ffmpeg API
			// since there is guaranteed call to avcodec_receive_frame and ffmpeg API promises that *both* of these methods will never return AVERROR(EAGAIN).
			int rc = avcodec_send_packet(codecContext, &packet);
			if(rc < 0)
				throwFFmpegError(rc);

			rc = avcodec_receive_frame(codecContext, frame);
			if(rc == AVERROR(EAGAIN))
			{
				// still need more data - read next packet
				av_packet_unref(&packet);
				continue;
			}
			else if(rc < 0)
			{
				throwFFmpegError(rc);
			}
			else
			{
				// read succesful. Exit the loop
				av_packet_unref(&packet);
				return;
			}
		}
		av_packet_unref(&packet);
	}
}

bool CVideoInstance::loadNextFrame()
{
	decodeNextFrame();
	const AVFrame * frame = getCurrentFrame();

	if(!frame)
		return false;

	uint8_t * data[4] = {};
	int linesize[4] = {};

	if(textureYUV)
	{
		av_image_alloc(data, linesize, dimensions.x, dimensions.y, AV_PIX_FMT_YUV420P, 1);
		sws_scale(sws, frame->data, frame->linesize, 0, getCodecContext()->height, data, linesize);
		SDL_UpdateYUVTexture(textureYUV, nullptr, data[0], linesize[0], data[1], linesize[1], data[2], linesize[2]);
		av_freep(&data[0]);
	}
	if(textureRGB)
	{
		av_image_alloc(data, linesize, dimensions.x, dimensions.y, AV_PIX_FMT_RGB32, 1);
		sws_scale(sws, frame->data, frame->linesize, 0, getCodecContext()->height, data, linesize);
		SDL_UpdateTexture(textureRGB, nullptr, data[0], linesize[0]);
		av_freep(&data[0]);
	}
	if(surface)
	{
		// Avoid buffer overflow caused by sws_scale():
		// http://trac.ffmpeg.org/ticket/9254

		size_t pic_bytes = surface->pitch * surface->h;
		size_t ffmped_pad = 1024; /* a few bytes of overflow will go here */
		void * for_sws = av_malloc(pic_bytes + ffmped_pad);
		data[0] = (ui8 *)for_sws;
		linesize[0] = surface->pitch;

		sws_scale(sws, frame->data, frame->linesize, 0, getCodecContext()->height, data, linesize);
		memcpy(surface->pixels, for_sws, pic_bytes);
		av_free(for_sws);
	}
	return true;
}


double CVideoInstance::timeStamp()
{
	return getCurrentFrameEndTime();
}

bool CVideoInstance::videoEnded()
{
	return getCurrentFrame() == nullptr;
}

CVideoInstance::CVideoInstance()
	: startTimeInitialized(false), deactivationStartTimeHandling(false)
{
}

CVideoInstance::~CVideoInstance()
{
	sws_freeContext(sws);
	SDL_DestroyTexture(textureYUV);
	SDL_DestroyTexture(textureRGB);
	SDL_FreeSurface(surface);
}

FFMpegStream::~FFMpegStream()
{
	av_frame_free(&frame);

#if (LIBAVCODEC_VERSION_MAJOR < 61 )
	// deprecated, apparently no longer necessary - avcodec_free_context should suffice
	avcodec_close(codecContext);
#endif

	avcodec_free_context(&codecContext);

	avformat_close_input(&formatContext);

	// for some reason, buffer is managed (e.g. reallocated) by FFmpeg
	// however, it must still be freed manually by user
	if (context->buffer)
		av_free(context->buffer);
	av_free(context);
}

Point CVideoInstance::size()
{
	return dimensions / ENGINE->screenHandler().getScalingFactor();
}

void CVideoInstance::show(const Point & position, SDL_Surface * to)
{
	if(sws == nullptr)
		throw std::runtime_error("No video to show!");

	CSDL_Ext::blitSurface(surface, to, position * ENGINE->screenHandler().getScalingFactor());
}

double FFMpegStream::getCurrentFrameEndTime() const
{
#if(LIBAVUTIL_VERSION_MAJOR < 58)
	auto packet_duration = frame->pkt_duration;
#else
	auto packet_duration = frame->duration;
#endif
	return (frame->pts + packet_duration) * av_q2d(formatContext->streams[streamIndex]->time_base);
}

double FFMpegStream::getCurrentFrameDuration() const
{
#if(LIBAVUTIL_VERSION_MAJOR < 58)
	auto packet_duration = frame->pkt_duration;
#else
	auto packet_duration = frame->duration;
#endif
	return packet_duration * av_q2d(formatContext->streams[streamIndex]->time_base);
}

void CVideoInstance::tick(uint32_t msPassed)
{
	if(sws == nullptr)
		throw std::runtime_error("No video to show!");

	if(videoEnded())
		throw std::runtime_error("Video already ended!");

	if(!startTimeInitialized)
	{
		startTime = std::chrono::steady_clock::now();
		startTimeInitialized = true;
	}

	auto nowTime = std::chrono::steady_clock::now();
	double difference = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - startTime).count() / 1000.0;

	int frameskipCounter = 0;
	while(!videoEnded() && difference >= getCurrentFrameEndTime() + getCurrentFrameDuration() && frameskipCounter < MAX_FRAMESKIP) // Frameskip
	{
		decodeNextFrame();
		frameskipCounter++;
	}
	if(!videoEnded() && difference >= getCurrentFrameEndTime())
		loadNextFrame();
}


void CVideoInstance::activate()
{
	if(deactivationStartTimeHandling)
	{
		auto pauseDuration = std::chrono::steady_clock::now() - deactivationStartTime;
		startTime += pauseDuration;
		deactivationStartTimeHandling = false;
	}
}

void CVideoInstance::deactivate()
{
	deactivationStartTime = std::chrono::steady_clock::now();
	deactivationStartTimeHandling = true;
}

struct FFMpegFormatDescription
{
	uint8_t sampleSizeBytes;
	uint8_t wavFormatID;
	bool isPlanar;
};

static FFMpegFormatDescription getAudioFormatProperties(int audioFormat)
{
	switch (audioFormat)
	{
		case AV_SAMPLE_FMT_U8:   return { 1, 1, false};
		case AV_SAMPLE_FMT_U8P:  return { 1, 1, true};
		case AV_SAMPLE_FMT_S16:  return { 2, 1, false};
		case AV_SAMPLE_FMT_S16P: return { 2, 1, true};
		case AV_SAMPLE_FMT_S32:  return { 4, 1, false};
		case AV_SAMPLE_FMT_S32P: return { 4, 1, true};
		case AV_SAMPLE_FMT_S64:  return { 8, 1, false};
		case AV_SAMPLE_FMT_S64P: return { 8, 1, true};
		case AV_SAMPLE_FMT_FLT:  return { 4, 3, false};
		case AV_SAMPLE_FMT_FLTP: return { 4, 3, true};
		case AV_SAMPLE_FMT_DBL:  return { 8, 3, false};
		case AV_SAMPLE_FMT_DBLP: return { 8, 3, true};
	}
	throw std::runtime_error("Invalid audio format");
}

int FFMpegStream::findAudioStream() const
{
	std::vector<int> audioStreamIndices;

	for(int i = 0; i < formatContext->nb_streams; i++)
		if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			audioStreamIndices.push_back(i);

	if (audioStreamIndices.empty())
		return -1;

	if (audioStreamIndices.size() == 1)
		return audioStreamIndices.front();

	// multiple audio streams - try to pick best one based on language settings
	std::map<int, std::string> streamToLanguage;

	// Approach 1 - check if stream has language set in metadata
	for (auto const & index : audioStreamIndices)
	{
		const AVDictionaryEntry *e = av_dict_get(formatContext->streams[index]->metadata, "language", nullptr, 0);
		if (e)
			streamToLanguage[index]	= e->value;
	}

	// Approach 2 - no metadata found. This may be video from Chronicles which have predefined (presumably hardcoded) list of languages
	if (streamToLanguage.empty())
	{
		if (audioStreamIndices.size() == 2)
		{
			streamToLanguage[audioStreamIndices[0]] = Languages::getLanguageOptions(Languages::ELanguages::ENGLISH).tagISO2;
			streamToLanguage[audioStreamIndices[1]] = Languages::getLanguageOptions(Languages::ELanguages::GERMAN).tagISO2;
		}

		if (audioStreamIndices.size() == 5)
		{
			streamToLanguage[audioStreamIndices[0]] = Languages::getLanguageOptions(Languages::ELanguages::ENGLISH).tagISO2;
			streamToLanguage[audioStreamIndices[1]] = Languages::getLanguageOptions(Languages::ELanguages::FRENCH).tagISO2;
			streamToLanguage[audioStreamIndices[2]] = Languages::getLanguageOptions(Languages::ELanguages::GERMAN).tagISO2;
			streamToLanguage[audioStreamIndices[3]] = Languages::getLanguageOptions(Languages::ELanguages::ITALIAN).tagISO2;
			streamToLanguage[audioStreamIndices[4]] = Languages::getLanguageOptions(Languages::ELanguages::SPANISH).tagISO2;
		}
	}

	std::string preferredLanguageName = LIBRARY->generaltexth->getPreferredLanguage();
	std::string preferredTag = Languages::getLanguageOptions(preferredLanguageName).tagISO2;

	for (auto const & entry : streamToLanguage)
		if (entry.second == preferredTag)
			return entry.first;

	return audioStreamIndices.front();
}

int FFMpegStream::findVideoStream() const
{
	for(int i = 0; i < formatContext->nb_streams; i++)
		if(formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			return i;

	return -1;
}

std::pair<std::unique_ptr<ui8 []>, si64> CAudioInstance::extractAudio(const VideoPath & videoToOpen)
{
	if (!openInput(videoToOpen))
		return { nullptr, 0};

	if (!openContext())
		return { nullptr, 0};

	int audioStreamIndex = findAudioStream();
	if (audioStreamIndex == -1)
		return { nullptr, 0};
	openCodec(audioStreamIndex);

	const auto * codecpar = getCodecParameters();

	std::vector<ui8> samples;

	auto formatProperties = getAudioFormatProperties(codecpar->format);
#if(LIBAVUTIL_VERSION_MAJOR < 58)
	int numChannels = codecpar->channels;
#else
	int numChannels = codecpar->ch_layout.nb_channels;
#endif

	samples.reserve(44100 * 5); // arbitrary 5-second buffer to reduce reallocations

	if (formatProperties.isPlanar && numChannels > 1)
	{
		// Format is 'planar', which is not supported by wav / SDL
		// Use swresample part of ffmpeg to deplanarize audio into format supported by wav / SDL

		auto sourceFormat = static_cast<AVSampleFormat>(codecpar->format);
		auto targetFormat = av_get_alt_sample_fmt(sourceFormat, false);

		SwrContext * swr_ctx = swr_alloc();

#if (LIBAVUTIL_VERSION_MAJOR < 58)
		av_opt_set_channel_layout(swr_ctx, "in_chlayout", codecpar->channel_layout, 0);
		av_opt_set_channel_layout(swr_ctx, "out_chlayout", codecpar->channel_layout, 0);
#else
		av_opt_set_chlayout(swr_ctx, "in_chlayout", &codecpar->ch_layout, 0);
		av_opt_set_chlayout(swr_ctx, "out_chlayout", &codecpar->ch_layout, 0);
#endif
		av_opt_set_int(swr_ctx, "in_sample_rate", codecpar->sample_rate, 0);
		av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", sourceFormat, 0);
		av_opt_set_int(swr_ctx, "out_sample_rate", codecpar->sample_rate, 0);
		av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", targetFormat, 0);

		int initResult = swr_init(swr_ctx);
		if (initResult < 0)
			throwFFmpegError(initResult);

		std::vector<uint8_t> frameSamplesBuffer;
		for (;;)
		{
			decodeNextFrame();
			const AVFrame * frame = getCurrentFrame();

			if (!frame)
				break;

			size_t samplesToRead = frame->nb_samples * numChannels;
			size_t bytesToRead = samplesToRead * formatProperties.sampleSizeBytes;
			frameSamplesBuffer.resize(std::max(frameSamplesBuffer.size(), bytesToRead));
			uint8_t * frameSamplesPtr = frameSamplesBuffer.data();

			int result = swr_convert(swr_ctx, &frameSamplesPtr, frame->nb_samples, const_cast<const uint8_t **>(frame->data), frame->nb_samples);

			if (result < 0)
				throwFFmpegError(result);

			size_t samplesToCopy = result * numChannels;
			size_t bytesToCopy = samplesToCopy * formatProperties.sampleSizeBytes;
			samples.insert(samples.end(), frameSamplesBuffer.begin(), frameSamplesBuffer.begin() + bytesToCopy);
		}
		swr_free(&swr_ctx);
	}
	else
	{
		for (;;)
		{
			decodeNextFrame();
			const AVFrame * frame = getCurrentFrame();

			if (!frame)
				break;

			size_t samplesToRead = frame->nb_samples * numChannels;
			size_t bytesToRead = samplesToRead * formatProperties.sampleSizeBytes;
			samples.insert(samples.end(), frame->data[0], frame->data[0] + bytesToRead);
		}
	}

	struct WavHeader {
		ui8 RIFF[4] = {'R', 'I', 'F', 'F'};
		ui32 ChunkSize;
		ui8 WAVE[4] = {'W', 'A', 'V', 'E'};
		ui8 fmt[4] = {'f', 'm', 't', ' '};
		ui32 Subchunk1Size = 16;
		ui16 AudioFormat = 1;
		ui16 NumOfChan = 2;
		ui32 SamplesPerSec = 22050;
		ui32 bytesPerSec = 22050 * 2;
		ui16 blockAlign = 1;
		ui16 bitsPerSample = 32;
		ui8 Subchunk2ID[4] = {'d', 'a', 't', 'a'};
		ui32 Subchunk2Size;
	};

	WavHeader wav;
	wav.ChunkSize = samples.size() + sizeof(WavHeader) - 8;
	wav.AudioFormat = formatProperties.wavFormatID; // 1 = PCM, 3 = IEEE float
	wav.NumOfChan = numChannels;
	wav.SamplesPerSec = codecpar->sample_rate;
	wav.bytesPerSec = codecpar->sample_rate * formatProperties.sampleSizeBytes;
	wav.bitsPerSample = formatProperties.sampleSizeBytes * 8;
	wav.Subchunk2Size = samples.size() + sizeof(WavHeader) - 44;
	auto * wavPtr = reinterpret_cast<ui8*>(&wav);

	auto dat = std::make_pair(std::make_unique<ui8[]>(samples.size() + sizeof(WavHeader)), samples.size() + sizeof(WavHeader));
	std::copy(wavPtr, wavPtr + sizeof(WavHeader), dat.first.get());
	std::copy(samples.begin(), samples.end(), dat.first.get() + sizeof(WavHeader));

	return dat;
}

std::unique_ptr<IVideoInstance> CVideoPlayer::open(const VideoPath & name, float scaleFactor)
{
	logGlobal->trace("Opening video: %s", name.getOriginalName());

	auto result = std::make_unique<CVideoInstance>();

	if (!result->openInput(name))
		return nullptr;

	if (!result->openVideo())
		return nullptr;

	result->prepareOutput(scaleFactor, false);
	result->loadNextFrame(); // prepare 1st frame

	return result;
}

std::pair<std::unique_ptr<ui8[]>, si64> CVideoPlayer::getAudio(const VideoPath & videoToOpen)
{
	logGlobal->trace("Opening video: %s", videoToOpen.getOriginalName());

	AudioPath audioPath = videoToOpen.toType<EResType::SOUND>();
	AudioPath audioPathVideoDir = audioPath.addPrefix("VIDEO/");

	if(CResourceHandler::get()->existsResource(audioPath))
		return CResourceHandler::get()->load(audioPath)->readAll();

	if(CResourceHandler::get()->existsResource(audioPathVideoDir))
		return CResourceHandler::get()->load(audioPathVideoDir)->readAll();

	CAudioInstance audio;
	return audio.extractAudio(videoToOpen);
}

#endif

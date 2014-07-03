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
#endif

#if defined(_WIN32)  &&  (_MSC_VER < 1800 ||  !defined(USE_FFMPEG))

void checkForError(bool throwing = true)
{
	int error = GetLastError();
	if(!error)
		return;

	logGlobal->errorStream() << "Error " << error << " encountered!";
	std::string msg;
	char* pTemp = nullptr;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr, error,  MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR)&pTemp, 1, nullptr );
	logGlobal->errorStream() << "Error: " << pTemp;
	msg = pTemp;
	LocalFree( pTemp );
	pTemp = nullptr;
	if(throwing)
		throw std::runtime_error(msg);
}

void blitBuffer(char *buffer, int x, int y, int w, int h, SDL_Surface *dst)
{
	const int bpp = dst->format->BytesPerPixel;
	char *dest;
	for(int i = h; i > 0; i--)
	{
		dest = (char*)dst->pixels + dst->pitch*(y+h-i) + x*dst->format->BytesPerPixel;
		memcpy(dest, buffer, bpp*w);
		buffer += bpp*w;
	}
}

void DLLHandler::Instantiate(const char *filename)
{
	name = filename;
	dll = LoadLibraryA(filename);
	if(!dll)
	{
		logGlobal->errorStream() << "Failed loading " << filename;
		checkForError(true);
	}
}

void *DLLHandler::FindAddress(const char *symbol)
{
	void *ret;

	if(!dll)
	{
		logGlobal->errorStream() << "Cannot look for " << symbol << " because DLL hasn't been appropriately loaded!";
		return nullptr;
	}
	ret = (void*) GetProcAddress(dll,symbol);
	if(!ret)
	{
		logGlobal->errorStream() << "Failed to find " << symbol << " in " << name;
		checkForError();
	}
	return ret;
}

DLLHandler::~DLLHandler()
{
	if(dll)
	{
		if(!FreeLibrary(dll))
		{
			logGlobal->errorStream() << "Failed to free " << name;
			checkForError();
		}
	}
}

DLLHandler::DLLHandler()
{
	dll = nullptr;
}

CBIKHandler::CBIKHandler()
{
	Instantiate("BINKW32.DLL");
	//binkGetError = FindAddress("_BinkGetError@0");
	binkOpen = (BinkOpen)FindAddress("_BinkOpen@8");
	binkSetSoundSystem = (BinkSetSoundSystem)FindAddress("_BinkSetSoundSystem@8");
	//getPalette = (BinkGetPalette)FindAddress("_BinkGetPalette@4");
	binkNextFrame = (BinkNextFrame)FindAddress("_BinkNextFrame@4");
	binkDoFrame = (BinkDoFrame)FindAddress("_BinkDoFrame@4");
	binkCopyToBuffer = (BinkCopyToBuffer)FindAddress("_BinkCopyToBuffer@28");
	binkWait = (BinkWait)FindAddress("_BinkWait@4");
	binkClose =  (BinkClose)FindAddress("_BinkClose@4");


	hBinkFile = nullptr;
	hBink = nullptr;

	buffer = nullptr;
	bufferSize = 0;
}

bool CBIKHandler::open(std::string name)
{
	hBinkFile = CreateFileA
	(
		name.c_str(),				// file name
		GENERIC_READ,						// access mode
		FILE_SHARE_READ,	// share mode
		nullptr,								// Security Descriptor
		OPEN_EXISTING,						// how to create
		FILE_ATTRIBUTE_NORMAL,//FILE_FLAG_SEQUENTIAL_SCAN,			// file attributes
		0								// handle to template file
	);

	if(hBinkFile == INVALID_HANDLE_VALUE)
	{
		logGlobal->errorStream() << "BIK handler: failed to open " << name;
		goto checkErrorAndClean;
	}
	//GCC wants scope of waveout to don`t cross labels/swith/goto
	{
		void *waveout = GetProcAddress(dll,"_BinkOpenWaveOut@4");
		if(waveout)
			binkSetSoundSystem(waveout,nullptr);

	}

	hBink = binkOpen(hBinkFile, 0x8a800000);
	if(!hBink)
	{
		logGlobal->errorStream() << "bink failed to open " << name;
		goto checkErrorAndClean;
	}

	allocBuffer();
	return true;

checkErrorAndClean:
	CloseHandle(hBinkFile);
	hBinkFile = nullptr;
	checkForError();
	throw std::runtime_error("BIK failed opening video!");
}

void CBIKHandler::show( int x, int y, SDL_Surface *dst, bool update )
{
	const int w = hBink->width,
		h = hBink->height,
		Bpp = dst->format->BytesPerPixel;

	int mode = -1;

	//screen color depth might have changed... (eg. because F4)
	if(bufferSize != w * h * Bpp)
	{
		freeBuffer();
		allocBuffer(Bpp);
	}

	switch(Bpp)
	{
	case 2:
		mode = 3; //565, mode 2 is 555 probably
		break;
	case 3:
		mode = 0;
		break;
	case 4:
		mode = 1;
		break;
	default:
		return; //not supported screen depth
	}

	binkDoFrame(hBink);
	binkCopyToBuffer(hBink, buffer, w*Bpp, h, 0, 0, mode);
	blitBuffer(buffer, x, y, w, h, dst);
	if(update)
		SDL_UpdateRect(dst, x, y, w, h);
}

bool CBIKHandler::nextFrame()
{
	binkNextFrame(hBink);
	return true;
}

void CBIKHandler::close()
{
	binkClose(hBink);
	hBink = nullptr;
	CloseHandle(hBinkFile);
	hBinkFile = nullptr;
	delete [] buffer;

	buffer = nullptr;
	bufferSize = 0;
}

bool CBIKHandler::wait()
{
	return binkWait(hBink);
}

int CBIKHandler::curFrame() const
{
	return hBink->currentFrame;
}

int CBIKHandler::frameCount() const
{
	return hBink->frameCount;
}

void CBIKHandler::redraw( int x, int y, SDL_Surface *dst, bool update )
{
	int w = hBink->width, h = hBink->height;
	blitBuffer(buffer, x, y, w, h, dst);
	if(update)
		SDL_UpdateRect(dst, x, y, w, h);
}

void CBIKHandler::allocBuffer(int Bpp)
{
	if(!Bpp) Bpp = screen->format->BytesPerPixel;

	bufferSize = hBink->width * hBink->height * Bpp;
	buffer = new char[bufferSize];
}

void CBIKHandler::freeBuffer()
{
	delete [] buffer;
	buffer = nullptr;
	bufferSize = 0;
}

bool CSmackPlayer::nextFrame()
{
	ptrSmackNextFrame(data);
	return true;
}

bool CSmackPlayer::wait()
{
	return ptrSmackWait(data);
}

CSmackPlayer::CSmackPlayer() : data(nullptr)
{
	Instantiate("smackw32.dll");
	ptrSmackNextFrame = (SmackNextFrame)FindAddress("_SmackNextFrame@4");
	ptrSmackWait = (SmackWait)FindAddress("_SmackWait@4");
	ptrSmackDoFrame = (SmackDoFrame)FindAddress("_SmackDoFrame@4");
	ptrSmackToBuffer = (SmackToBuffer)FindAddress("_SmackToBuffer@28");
	ptrSmackOpen = (SmackOpen)FindAddress("_SmackOpen@12");
	ptrSmackSoundOnOff = (SmackSoundOnOff)FindAddress("_SmackSoundOnOff@8");
	ptrSmackClose = (SmackClose)FindAddress("_SmackClose@4");
	ptrVolumePan = (SmackVolumePan)FindAddress("_SmackVolumePan@16");
}

CSmackPlayer::~CSmackPlayer()
{
	if(data)
		close();
}

void CSmackPlayer::close()
{
	ptrSmackClose(data);
	data = nullptr;
}

bool CSmackPlayer::open( std::string name )
{
	Uint32 flags[2] = {0xff400, 0xfe400};

	data = ptrSmackOpen( (void*)name.c_str(), flags[1], -1);
	if (!data)
	{
		logGlobal->errorStream() << "Smack cannot open " << name;
		checkForError();
		throw std::runtime_error("SMACK failed opening video");
	}

	buffer = new char[data->width*data->height*2];
	buf = buffer+data->width*(data->height-1)*2;	// adjust pointer position for later use by 'SmackToBuffer'

	//ptrVolumePan(data, 0xfe000, 3640 * GDefaultOptions.musicVolume / 11, 0x8000); //set volume
	return true;
}

void CSmackPlayer::show( int x, int y, SDL_Surface *dst, bool update)
{
	int w = data->width;
	int stripe = (-w*2) & (~3);

	//put frame to the buffer
	ptrSmackToBuffer(data, 0, 0, stripe, w, buf, 0x80000000);
	ptrSmackDoFrame(data);
	redraw(x, y, dst, update);
}

int CSmackPlayer::curFrame() const
{
	return data->currentFrame;
}

int CSmackPlayer::frameCount() const
{
	return data->frameCount;
}

void CSmackPlayer::redraw( int x, int y, SDL_Surface *dst, bool update )
{
	int w = std::min<int>(data->width, dst->w - x), h = std::min<int>(data->height, dst->h - y);
	/* Lock the screen for direct access to the pixels */
	if ( SDL_MUSTLOCK(dst) )
	{
		if ( SDL_LockSurface(dst) < 0 )
		{
			fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
			return;
		}
	}

	// draw the frame
	Uint16* addr = (Uint16*) (buffer+w*(h-1)*2-2);
	if(dst->format->BytesPerPixel >= 3)
	{
		for( int j=0; j<h-1; j++)	// why -1 ?
		{
			for ( int i=w-1; i>=0; i--)
			{
				Uint16 pixel = *addr;

				Uint8 *p = (Uint8 *)dst->pixels + (j+y) * dst->pitch + (i + x) * dst->format->BytesPerPixel;
				p[2] = ((pixel & 0x7c00) >> 10) * 8;
				p[1] = ((pixel & 0x3e0) >> 5) * 8;
				p[0] = ((pixel & 0x1F)) * 8;

				addr--;
			}
		}
	}
	else if(dst->format->BytesPerPixel == 2)
	{
		for( int j=0; j<h-1; j++)	// why -1 ?
		{
			for ( int i=w-1; i>=0; i--)
			{
				//convert rgb 555 to 565
				Uint16 pixel = *addr;
				Uint16 *p = (Uint16 *)((Uint8 *)dst->pixels + (j+y) * dst->pitch + (i + x) * dst->format->BytesPerPixel);
				*p =	(pixel & 0x1F)
					  +	((pixel & 0x3e0) << 1)
					  +	((pixel & 0x7c00) << 1);

				addr--;
			}
		}
	}

	if ( SDL_MUSTLOCK(dst) )
	{
		SDL_UnlockSurface(dst);
	}

	if(update)
		SDL_UpdateRect(dst, x, y, w, h);
}

CVideoPlayer::CVideoPlayer()
{
	current = nullptr;
}

CVideoPlayer::~CVideoPlayer()
{
}

bool CVideoPlayer::open(std::string name)
{
	fname = name;
	first = true;

	try
	{
		// Extract video from video.vid so we can play it.
		// We can handle only videos in form of single file, no archive support yet.
		{
			ResourceID videoID = ResourceID("VIDEO/" + name, EResType::VIDEO);
			auto data = CResourceHandler::get()->load(videoID)->readAll();

			// try to determine video format using magic number from header (3 bytes, SMK or BIK)
			std::string magic(reinterpret_cast<char*>(data.first.get()), 3);
			if (magic == "BIK")
				current = &bikPlayer;
			else if (magic == "SMK")
				current = &smkPlayer;
			else
				throw std::runtime_error("Unknown video format: " + magic);

			std::ofstream out(name, std::ofstream::binary);
			out.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			out.write(reinterpret_cast<char*>(data.first.get()), data.second);
		}

		current->open(name);
		return true;
	}
	catch(std::exception &e)
	{
		current = nullptr;
		logGlobal->warnStream() << "Failed to open video file " << name << ": " << e.what();
	}

	return false;
}

void CVideoPlayer::close()
{
	if(!current)
	{
		logGlobal->warnStream() << "Closing no opened player...?";
		return;
	}

	current->close();
	current = nullptr;
	if(!DeleteFileA(fname.c_str()))
	{
		logGlobal->errorStream() << "Cannot remove temporarily extracted video file: " << fname;
		checkForError(false);
	}
	fname.clear();
}

bool CVideoPlayer::nextFrame()
{
	if(current)
	{
		current->nextFrame();
		return true;
	}
	else
		return false;
}

void CVideoPlayer::show(int x, int y, SDL_Surface *dst, bool update)
{
	if(current)
		current->show(x, y, dst, update);
}

bool CVideoPlayer::wait()
{
	if(current)
		return current->wait();
	else
		return false;
}

int CVideoPlayer::curFrame() const
{
	if(current)
		return current->curFrame();
	else
		return -1;
}

int CVideoPlayer::frameCount() const
{
	if(current)
		return current->frameCount();
	else
		return -1;
}

bool CVideoPlayer::openAndPlayVideo(std::string name, int x, int y, SDL_Surface *dst, bool stopOnKey)
{
	if(!open(name))
		return false;

	bool ret = playVideo(x, y, dst, stopOnKey);
	close();
	return ret;
}

void CVideoPlayer::update( int x, int y, SDL_Surface *dst, bool forceRedraw, bool update )
{
	if(!current)
		return;

	bool w = false;
	if(!first)
	{
		w = wait(); //check if should keep current frame
		if(!w)
			nextFrame();
	}
	else
	{
		first = false;
	}



	if(!w)
	{
		show(x,y,dst,update);
	}
	else if (forceRedraw)
	{
		redraw(x, y, dst, update);
	}
}

void CVideoPlayer::redraw( int x, int y, SDL_Surface *dst, bool update )
{
	if(current)
		current->redraw(x, y, dst, update);
}

bool CVideoPlayer::playVideo(int x, int y, SDL_Surface *dst, bool stopOnKey)
{
	if(!current)
		return false;

	int frame = 0;
	while(frame < frameCount()) //play all frames
	{
		if(stopOnKey && keyDown())
			return false;

		if(!wait())
		{
			show(x, y, dst);
			nextFrame();
			frame++;
		}
		SDL_Delay(20);
	}

	return true;
}

#else

#ifdef _MSC_VER
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#endif // _MSC_VER


#ifndef DISABLE_VIDEO

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
#ifdef VCMI_SDL1
	overlay = nullptr;
#else
	texture = nullptr;
#endif
	dest = nullptr;
	context = nullptr;

	// Register codecs. TODO: May be overkill. Should call a
	// combination of av_register_input_format() /
	// av_register_output_format() / av_register_protocol() instead.
	av_register_all();
}

bool CVideoPlayer::open(std::string fname)
{
	return open(fname, true, false);
}

// loop = to loop through the video
// useOverlay = directly write to the screen.
bool CVideoPlayer::open(std::string fname, bool loop, bool useOverlay)
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
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 17, 0)
	if (av_find_stream_info(format) < 0)
#else
	if (avformat_find_stream_info(format, nullptr) < 0)
#endif
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
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 6, 0)
	if ( avcodec_open(codecContext, codec) < 0 )
#else
	if ( avcodec_open2(codecContext, codec, nullptr) < 0 )
#endif
	{
		// Could not open codec
		codec = nullptr;
		return false;
	}

	// Allocate video frame
	frame = avcodec_alloc_frame();

	// Allocate a place to put our YUV image on that screen
	if (useOverlay)
	{
#ifdef VCMI_SDL1
		overlay = SDL_CreateYUVOverlay(codecContext->width, codecContext->height,
									   SDL_YV12_OVERLAY, screen);
#else
		texture = SDL_CreateTexture( mainRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STATIC, codecContext->width, codecContext->height);
#endif

	}
	else
	{
		dest = CSDL_Ext::newSurface(codecContext->width, codecContext->height);
		destRect.x = destRect.y = 0;
		destRect.w = codecContext->width;
		destRect.h = codecContext->height;
	}
#ifdef VCMI_SDL1
	if (overlay == nullptr && dest == nullptr)
		return false;

	if (overlay)
#else
	if (texture == nullptr && dest == nullptr)
		return false;

	if (texture)
#endif
	{ // Convert the image into YUV format that SDL uses
		sws = sws_getContext(codecContext->width, codecContext->height,
							 codecContext->pix_fmt, codecContext->width, codecContext->height,
							 PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);
	}
	else
	{

		PixelFormat screenFormat = PIX_FMT_NONE;
		if (screen->format->Bshift > screen->format->Rshift)
		{
			// this a BGR surface
			switch (screen->format->BytesPerPixel)
			{
				case 2: screenFormat = PIX_FMT_BGR565; break;
				case 3: screenFormat = PIX_FMT_BGR24; break;
				case 4: screenFormat = PIX_FMT_BGR32; break;
				default: return false;
			}
		}
		else
		{
			// this a RGB surface
			switch (screen->format->BytesPerPixel)
			{
				case 2: screenFormat = PIX_FMT_RGB565; break;
				case 3: screenFormat = PIX_FMT_RGB24; break;
				case 4: screenFormat = PIX_FMT_RGB32; break;
				default: return false;
			}
		}

		sws = sws_getContext(codecContext->width, codecContext->height,
							 codecContext->pix_fmt, codecContext->width, codecContext->height,
							 screenFormat, SWS_BICUBIC, nullptr, nullptr, nullptr);
	}

	if (sws == nullptr)
		return false;

	pos.w = codecContext->width;
	pos.h = codecContext->height;

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
				if (av_seek_frame(format, stream, 0, 0) < 0)
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

#ifdef VCMI_SDL1
					if (overlay) {
						SDL_LockYUVOverlay(overlay);

						pict.data[0] = overlay->pixels[0];
						pict.data[1] = overlay->pixels[2];
						pict.data[2] = overlay->pixels[1];

						pict.linesize[0] = overlay->pitches[0];
						pict.linesize[1] = overlay->pitches[2];
						pict.linesize[2] = overlay->pitches[1];

						sws_scale(sws, frame->data, frame->linesize,
								  0, codecContext->height, pict.data, pict.linesize);

						SDL_UnlockYUVOverlay(overlay);
#else
					if (texture) {
						avpicture_alloc(&pict, AV_PIX_FMT_YUV420P, codecContext->width, codecContext->height);

						sws_scale(sws, frame->data, frame->linesize,
								  0, codecContext->height, pict.data, pict.linesize);

						SDL_UpdateYUVTexture(texture, NULL, pict.data[0], pict.linesize[0],
								pict.data[1], pict.linesize[1],
								pict.data[2], pict.linesize[2]);
						avpicture_free(&pict);
#endif
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

#ifdef VCMI_SDL1
	if (overlay)
	{
		SDL_FreeYUVOverlay(overlay);
		overlay = nullptr;
	}
#else
	if (texture)
	{
		SDL_DestroyTexture(texture);
		texture = nullptr;
	}

#endif


	if (dest)
	{
		SDL_FreeSurface(dest);
		dest = nullptr;
	}

	if (frame)
	{
		av_free(frame);
		frame = nullptr;
	}

	if (codec)
	{
		avcodec_close(codecContext);
		codec = nullptr;
		codecContext = nullptr;
	}

	if (format)
	{
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 17, 0)
		av_close_input_file(format);
		format = nullptr;
#else
		avformat_close_input(&format);
#endif
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

#ifdef VCMI_SDL1
		SDL_DisplayYUVOverlay(overlay, &pos);
#else
		SDL_RenderCopy(mainRenderer, texture, NULL, NULL);
		SDL_RenderPresent(mainRenderer);
#endif


		// Wait 3 frames
		GH.mainFPSmng->framerateDelay();
		GH.mainFPSmng->framerateDelay();
		GH.mainFPSmng->framerateDelay();
	}

	return true;
}

bool CVideoPlayer::openAndPlayVideo(std::string name, int x, int y, SDL_Surface *dst, bool stopOnKey)
{
	open(name, false, true);
	bool ret = playVideo(x, y, dst, stopOnKey);
	close();
	return ret;
}

CVideoPlayer::~CVideoPlayer()
{
	close();
}

#endif

#endif

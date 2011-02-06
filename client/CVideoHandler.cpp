#include "../stdafx.h"
#include <iostream>
#include "CSndHandler.h"
#include "CVideoHandler.h"
#include <SDL.h>
#include "../client/SDL_Extensions.h"
#include "../client/CPlayerInterface.h"

extern SystemOptions GDefaultOptions; 
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

#ifdef _WIN32

#include <boost/algorithm/string/predicate.hpp>


void checkForError(bool throwing = true)
{
#ifdef _WIN32
	int error = GetLastError();
	if(!error)
		return;


	tlog1 << "Error " << error << " encountered!\n";
	std::string msg;
	char* pTemp = NULL;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, error,  MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR)&pTemp, 1, NULL );
	tlog1 << "Error: " << pTemp << std::endl;
	msg = pTemp;
	LocalFree( pTemp );
	pTemp = NULL;
	if(throwing)
		throw msg;
#endif
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
#ifdef _WIN32
	dll = LoadLibraryA(filename);
	if(!dll)
	{
		tlog1 << "Failed loading " << filename << std::endl;
		checkForError();
	}
#else
	dll = dlopen(filename,RTLD_LOCAL | RTLD_LAZY);
#endif
}

void *DLLHandler::FindAddress(const char *symbol)
{
	void *ret;
#ifdef _WIN32
	ret = (void*) GetProcAddress(dll,symbol);
	if(!ret)
	{
		tlog1 << "Failed to find " << symbol << " in " << name << std::endl;
		checkForError();
	}
#else
	ret = (void *)dlsym(dll, symbol);
#endif
	return ret;
}

DLLHandler::~DLLHandler()
{
	if(dll)
	{
	#ifdef _WIN32
		if(!FreeLibrary(dll))
		{
			tlog1 << "Failed to free " << name << std::endl;
			checkForError();
		}
	#else
		dlclose(dll);
	#endif
	}
}

DLLHandler::DLLHandler()
{
	dll = NULL;
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

	hBinkFile = NULL;
	hBink = NULL;

	buffer = NULL;
	bufferSize = 0;
}

bool CBIKHandler::open(std::string name)
{
	hBinkFile = CreateFileA
	(
		name.c_str(),				// file name
		GENERIC_READ,						// access mode
		FILE_SHARE_READ,	// share mode
		NULL,								// Security Descriptor
		OPEN_EXISTING,						// how to create
		FILE_ATTRIBUTE_NORMAL,//FILE_FLAG_SEQUENTIAL_SCAN,			// file attributes
		0								// handle to template file
	);

	if(hBinkFile == INVALID_HANDLE_VALUE)
	{
		tlog1 << "BIK handler: failed to open " << name << std::endl;
		goto checkErrorAndClean;
	}

	void *waveout = GetProcAddress(dll,"_BinkOpenWaveOut@4");
	if(waveout)
		binkSetSoundSystem(waveout,NULL);

	hBink = binkOpen(hBinkFile, 0x8a800000);
	if(!hBink)
	{
		tlog1 << "bink failed to open " << name << std::endl;
		goto checkErrorAndClean;
	}

	allocBuffer();
	return true;

checkErrorAndClean:
	CloseHandle(hBinkFile);
	hBinkFile = NULL;
	checkForError(false);
	return false;
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

void CBIKHandler::nextFrame()
{
	binkNextFrame(hBink);
}

void CBIKHandler::close()
{
	binkClose(hBink);
	hBink = NULL;
	CloseHandle(hBinkFile);
	hBinkFile = NULL;
	delete [] buffer;

	buffer = NULL;
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
	buffer = NULL;
	bufferSize = 0;
}

void CSmackPlayer::nextFrame()
{
	ptrSmackNextFrame(data);
}

bool CSmackPlayer::wait()
{
	return ptrSmackWait(data);
}

CSmackPlayer::CSmackPlayer()
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
	data = NULL;
}

bool CSmackPlayer::open( std::string name )
{
	Uint32 flags[2] = {0xff400, 0xfe400};

	data = ptrSmackOpen( (void*)name.c_str(), flags[1], -1);
	if (!data) 
	{
		tlog1 << "Smack cannot open " << name << std::endl;
		checkForError(false);
		return false;
	}

	buffer = new char[data->width*data->height*2];
	buf = buffer+data->width*(data->height-1)*2;	// adjust pointer position for later use by 'SmackToBuffer'

	//ptrVolumePan(data, 0xfe000, 3640 * GDefaultOptions.musicVolume / 11, 0x8000); //set volume
	return true;
}

void CSmackPlayer::show( int x, int y, SDL_Surface *dst, bool update)
{
	int w = data->width, h = data->height;
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
	vidh = new CVidHandler(std::string(DATA_DIR "/Data/VIDEO.VID"));
	current = NULL;
}

CVideoPlayer::~CVideoPlayer()
{
	delete vidh;
}

bool CVideoPlayer::open(std::string name)
{
	if(boost::algorithm::ends_with(name, ".BIK"))
		current = &bikPlayer;
	else
		current = &smkPlayer;

	fname = name;
	first = true;

	//extract video from video.vid so we can play it
	vidh->extract(name, name);
	bool opened = current->open(name);
	if(!opened)
	{
		current = NULL;
		tlog3 << "Failed to open video file " << name << std::endl;
	}

	return opened;
}

void CVideoPlayer::close()
{
	if(!current)
	{
		tlog2 << "Closing no opened player...?" << std::endl;
		return;
	}

	current->close();
	current = NULL;
	if(!DeleteFileA(fname.c_str()))
	{
		tlog1 << "Cannot remove temporarily extracted video file: " << fname;
		checkForError(false);
	}
	fname.clear();
}

void CVideoPlayer::nextFrame()
{
	if(current)
		current->nextFrame();
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

//Workaround for compile error in ffmpeg (UINT_64C was not declared)
#define __STDC_CONSTANT_MACROS
#ifdef _STDINT_H
#undef _STDINT_H
#endif
#include <stdint.h>

#include <../client/SDL_framerate.h>

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

static const char *protocol_name = "lod";

// Open a pseudo file. Name is something like 'lod:0x56432ab6c43df8fe'
static int lod_open(URLContext *context, const char *filename, int flags)
{
	CVideoPlayer *video;

	// Retrieve pointer to CVideoPlayer object
	filename += strlen(protocol_name) + 1;
	video = (CVideoPlayer *)(uintptr_t)strtoull(filename, NULL, 16);

	// TODO: check flags ?

	context->priv_data = video;

	return 0;
}

static int lod_close(URLContext* h)
{
	return 0;
}

// Define a set of functions to read data
static int lod_read(URLContext *context, unsigned char *buf, int size)
{
	CVideoPlayer *video = (CVideoPlayer *)context->priv_data;

	amin(size, video->length - video->offset);

	if (size < 0)
		return -1;

	// TODO: can we avoid that copy ?
	memcpy(buf, video->data + video->offset, size);

	video->offset += size;

	return size;
}

static int64_t lod_seek(URLContext *context, int64_t pos, int whence)
{
	CVideoPlayer *video = (CVideoPlayer *)context->priv_data;

	// Not sure what the parameter whence is. Assuming it always
	// indicates an absolute value;
	video->offset = pos;
	amin(video->offset, video->length);

	return -1;//video->offset;
}

static URLProtocol lod_protocol = {
	protocol_name,
	lod_open,
	lod_read,
	NULL,						// no write
    lod_seek,
	lod_close
};

CVideoPlayer::CVideoPlayer()
{
	format = NULL;
	frame = NULL;
	codec = NULL;
	sws = NULL;
	overlay = NULL;
	dest = NULL;

	// Register codecs. TODO: May be overkill. Should call a
	// combination of av_register_input_format() /
	// av_register_output_format() / av_register_protocol() instead.
	av_register_all();

	// Register our protocol 'lod' so we can directly read from mmaped memory
	av_register_protocol(&lod_protocol);

	vidh = new CVidHandler(std::string(DATA_DIR "/Data/VIDEO.VID"));
}

// loop = to loop through the video
// useOverlay = directly write to the screen.
bool CVideoPlayer::open(std::string fname, bool loop, bool useOverlay)
{
	close();

	offset = 0;
	refreshWait = 3;
	refreshCount = -1;
	doLoop = loop;

	data = vidh->extract(fname, length);

	if (data) {
		// Create our URL name with the 'lod' protocol as a prefix and a
		// back pointer to our object. Should be 32 and 64 bits compatible.
		char url[100];
		sprintf(url, "%s:0x%016llx", protocol_name, (unsigned long long)(uintptr_t)this);

		if (av_open_input_file(&format, url, NULL, 0, NULL)!=0) {
			return false;
		}
	} else {
		// File is not in a container
		if (av_open_input_file(&format, (DATA_DIR "/Data/video/" + fname).c_str(), NULL, 0, NULL)!=0) {
			// tlog1 << "Video file not found: " DATA_DIR "/Data/video/" + fname << std::endl;
			return false;
		}
	}

	// Retrieve stream information
	if (av_find_stream_info(format) < 0)
		return false;

	// Find the first video stream
	stream = -1;
	for(unsigned int i=0; i<format->nb_streams; i++) {
		if (format->streams[i]->codec->codec_type==CODEC_TYPE_VIDEO) {
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

	if (codec == NULL) {
		// Unsupported codec
		return false;
	}
  
	// Open codec
	if (avcodec_open(codecContext, codec) <0 ) {
		// Could not open codec
		codec = NULL;
		return false;
	}
  
	// Allocate video frame
	frame = avcodec_alloc_frame();

	// Allocate a place to put our YUV image on that screen
	if (useOverlay) {
		overlay = SDL_CreateYUVOverlay(codecContext->width, codecContext->height,
									   SDL_YV12_OVERLAY, screen);
	} else {
		dest = CSDL_Ext::newSurface(codecContext->width, codecContext->height);
		destRect.x = destRect.y = 0;
		destRect.w = codecContext->width;
		destRect.h = codecContext->height;
	}

	if (overlay == NULL && dest == NULL)
		return false;

	// Convert the image into YUV format that SDL uses
	if (overlay) {
		sws = sws_getContext(codecContext->width, codecContext->height, 
							 codecContext->pix_fmt, codecContext->width, codecContext->height, 
							 PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	} else {
		PixelFormat screenFormat = PIX_FMT_NONE;
		switch(screen->format->BytesPerPixel)
		{
		case 2: screenFormat = PIX_FMT_RGB565; break;
		case 3: screenFormat = PIX_FMT_RGB24; break;
		case 4: screenFormat = PIX_FMT_RGB32; break;
		default: return false;
		}

		sws = sws_getContext(codecContext->width, codecContext->height, 
							 codecContext->pix_fmt, codecContext->width, codecContext->height, 
							 screenFormat, SWS_BICUBIC, NULL, NULL, NULL);
	}

	if (sws == NULL)
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

	if (sws == NULL)
		return false;

	while(!frameFinished) {

		int ret = av_read_frame(format, &packet);
		if (ret < 0) {
			// Error. It's probably an end of file.
			if (doLoop && !gotError) {
				// Rewind
				if (av_seek_frame(format, stream, 0, 0) < 0)
					break;
				gotError = true;
			} else {
				break;
			}
		} else {
			// Is this a packet from the video stream?
			if (packet.stream_index == stream) {
				// Decode video frame
				avcodec_decode_video(codecContext, frame, &frameFinished, 
									 packet.data, packet.size);

				// Did we get a video frame?
				if (frameFinished) {
					AVPicture pict;

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
					} else {
						pict.data[0] = (uint8_t *)dest->pixels;
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
	if (sws == NULL)
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
	if (sws == NULL)
		return;

	if (refreshCount <= 0)
	{
		refreshCount = refreshWait;
		if (nextFrame())
			show(x,y,dst,update);
	} else {
		redraw(x, y, dst, update);
	}

	refreshCount --;
}

void CVideoPlayer::close()
{
	if (sws) {
		sws_freeContext(sws);
		sws = NULL;
	}

	if (overlay) {
		SDL_FreeYUVOverlay(overlay);
		overlay = NULL;
	}

	if (dest) {
		SDL_FreeSurface(dest);
		dest = NULL;
	}

	if (frame) {
		av_free(frame);
		frame = NULL;
	}

	if (codec) {
		avcodec_close(codecContext);
		codec = NULL;
		codecContext = NULL;
	}

	if (format) {
		av_close_input_file(format);
		format = NULL;
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

	FPSmanager mainFPSmng;
    SDL_initFramerate(&mainFPSmng);
    SDL_setFramerate(&mainFPSmng, 48);

	while(nextFrame()) {
		
		if(stopOnKey && keyDown())
			return false;

		SDL_DisplayYUVOverlay(overlay, &pos);

		// Wait 3 frames
		SDL_framerateDelay(&mainFPSmng);
		SDL_framerateDelay(&mainFPSmng);
		SDL_framerateDelay(&mainFPSmng);
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

 	  	 

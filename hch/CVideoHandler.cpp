#include "../stdafx.h"
#include <iostream>
#include "CSndHandler.h"
#include "CVideoHandler.h"
#include <SDL.h>

#ifdef _WIN32

#include "../client/SDL_Extensions.h"



void DLLHandler::Instantiate(const char *filename)
{
	name = filename;
#ifdef _WIN32
	dll = LoadLibraryA(filename);
#else
	dll = dlopen(filename,RTLD_LOCAL | RTLD_LAZY);
#endif
}

void *DLLHandler::FindAddress(const char *symbol)
{
#ifdef _WIN32
	return (void*) GetProcAddress(dll,symbol);
#else
	return (void *)dlsym(dll, symbol);
#endif
}

DLLHandler::~DLLHandler()
{
	if(dll)
	{
	#ifdef _WIN32
		FreeLibrary(dll);
	#else
		dlclose(dll);
	#endif
	}
}

DLLHandler::DLLHandler()
{
	dll = NULL;
}


void checkForError()
{
#ifdef _WIN32
	int error = GetLastError();
	if(error)
		tlog1 << "Error " << error << " encountered!\n";
#endif
}


CBIKHandler::CBIKHandler()
{
	Instantiate("BINKW32.DLL");

	binkGetError = FindAddress("_BinkGetError@0");
	binkOpen = (BinkOpen)FindAddress("_BinkOpen@8");
	binkSetSoundSystem = (BinkSetSoundSystem)FindAddress("_BinkSetSoundSystem@8");
	getPalette = (BinkGetPalette)FindAddress("_BinkGetPalette@4");
	binkNextFrame = (BinkNextFrame)FindAddress("_BinkNextFrame@4");
	binkDoFrame = (BinkDoFrame)FindAddress("_BinkDoFrame@4");
	binkCopyToBuffer = (BinkCopyToBuffer)FindAddress("_BinkCopyToBuffer@28");
	binkWait = (BinkWait)FindAddress("_BinkWait@4");
}

void CBIKHandler::open(std::string name)
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
		checkForError();
		return ;
	}

	void *waveout = FindAddress("_BinkOpenWaveOut@4");
	if(waveout)
		binkSetSoundSystem(waveout,NULL);

	hBink = binkOpen(hBinkFile, 0x8a800000);
	width = hBink->width;
	height = hBink->height;
	buffer = new char[width * height * 3];
}

void CBIKHandler::show( int x, int y, SDL_Surface *dst )
{
	int w = hBink->width, h = hBink->height;
	//memset(buffer,0,w * h * 3);
	binkDoFrame(hBink);
	binkCopyToBuffer(hBink, buffer, w*3, h, 0, 0, 0);
	char *src = buffer;
	char *dest;
	for(int i = h; i > 0; i--)
	{
		dest = (char*)dst->pixels + dst->pitch*(h-i) + x*dst->format->BytesPerPixel;
		memcpy(dest,src,3*w);
		src += 3*w;
	}

	SDL_UpdateRect(dst,x,y,hBink->width, hBink->height);
}

void CBIKHandler::nextFrame()
{
	binkNextFrame(hBink);
}

void CBIKHandler::close()
{
	delete [] buffer;
}

bool CBIKHandler::wait()
{
	return binkWait(hBink);
}

// Reference RSGrapics.RSGetPixelFormat
PixelFormat getPixelFormat(TBitmap &b)
{
	DIBSECTION DS;
	DS.dsBmih.biBitCount = 2;
	DS.dsBmih.biCompression = 0; //not sure about that
	PixelFormat result = b.pixelFormat;

	  if ( (result!= pfCustom)
		  || (b.handleType = bmDDB)
		 // || (GetObject(b.Handle, SizeOf(DS), @DS) = 0) 
		  )
		  exit(0);

	  switch (DS.dsBmih.biBitCount)
	  {
		case 16:
			switch (DS.dsBmih.biCompression)
			{
				case BI_RGB:
					result = pf15bit;
					break;
				case BI_BITFIELDS:
					if ( DS.dsBitfields[1]==0x7E0 )
						result = pf16bit;
					if ( DS.dsBitfields[1]==0x7E0 )
						result = pf15bit;
					break;
			}
			break;
		case 32:
			switch (DS.dsBmih.biCompression)
			{
				case BI_RGB:
					result = pf32bit;
					break;
				case BI_BITFIELDS:
					if ( DS.dsBitfields[1]==0xFF0000 )
						result = pf32bit;
					break;
			}
			break;
	  }
	return result;
}

void CSmackPlayer::preparePic(TBitmap &b)
{
	switch (getPixelFormat(b))
	{
	case pf15bit:
	case pf16bit:
		break;
	default:
		b.pixelFormat = pf16bit;
	};
}


void CSmackPlayer::nextFrame()
{
	ptrSmackNextFrame(data);
}


bool CSmackPlayer::wait()
{
	return ptrSmackWait(data);
}

void CSmackPlayer::init()
{
	Instantiate("smackw32.dll");
	tlog0 << "smackw32.dll loaded" << std::endl;

	ptrSmackNextFrame = (SmackNextFrame)FindAddress("_SmackNextFrame@4");
	ptrSmackWait = (SmackWait)FindAddress("_SmackWait@4");
	ptrSmackDoFrame = (SmackDoFrame)FindAddress("_SmackDoFrame@4");
	ptrSmackToBuffer = (SmackToBuffer)FindAddress("_SmackToBuffer@28");
	ptrSmackOpen = (SmackOpen)FindAddress("_SmackOpen@12");
	ptrSmackSoundOnOff = (SmackSoundOnOff)FindAddress("_SmackSoundOnOff@8");
	tlog0 << "Functions located" << std::endl;
}

CVideoPlayer::CVideoPlayer()
{
	vidh = new CVidHandler(std::string(DATA_DIR "Data" PATHSEPARATOR "VIDEO.VID"));
	smkPlayer = new CSmackPlayer;
	smkPlayer->init();
}

CVideoPlayer::~CVideoPlayer()
{
	delete smkPlayer;
	delete vidh;
}

bool CVideoPlayer::init()
{
	return true;
}

bool CVideoPlayer::open(std::string fname, int x, int y)
{
	vidh->extract(fname, fname);

	Uint32 flags[2] = {0xff400, 0xfe400};

	smkPlayer->data = smkPlayer->ptrSmackOpen( (void*)fname.c_str(), flags[1], -1);
	if (smkPlayer->data ==NULL) 
	{
		tlog1<<"No "<<fname<<" file!"<<std::endl;
		return false;
	}

	buffer = new char[smkPlayer->data->width*smkPlayer->data->height*2];
	buf = buffer+smkPlayer->data->width*(smkPlayer->data->height-1)*2;	// adjust pointer position for later use by 'SmackToBuffer'

	xPos = x;
	yPos = y;
	frame = 0;

	return true;
}

void CVideoPlayer::close()
{
	delete [] buffer;
}

bool CVideoPlayer::nextFrame()
{
	if(frame < smkPlayer->data->frameCount)
	{
		++frame;

		int stripe = (-smkPlayer->data->width*2) & (~3);
		Uint32 unknown = 0x80000000;
		smkPlayer->ptrSmackToBuffer(smkPlayer->data , 0, 0, stripe, smkPlayer->data->width, buf, unknown);
		smkPlayer->ptrSmackDoFrame(smkPlayer->data );
		// now bitmap is in buffer
		// but I don't know exactly how to parse these 15bit color and draw it onto 16bit screen


		/* Lock the screen for direct access to the pixels */
		if ( SDL_MUSTLOCK(screen) ) 
		{
			if ( SDL_LockSurface(screen) < 0 ) 
			{
				fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
				return 0;
			}
		}

		// draw the frame!!
		Uint16* addr = (Uint16*) (buffer+smkPlayer->data->width*(smkPlayer->data->height-1)*2-2);
		for( int j=0; j<smkPlayer->data->height-1; j++)	// why -1 ?
		{
			for ( int i=smkPlayer->data->width-1; i>=0; i--)
			{
				Uint16 pixel = *addr;

				Uint8 *p = (Uint8 *)screen->pixels + (j+yPos) * screen->pitch + (i + xPos) * screen->format->BytesPerPixel;
				p[2] = ((pixel & 0x7c00) >> 10) * 8;
				p[1] = ((pixel & 0x3e0) >> 5) * 8;
				p[0] = ((pixel & 0x1F)) * 8;

				addr--;
			}
		}

		if ( SDL_MUSTLOCK(screen) ) 
		{
			SDL_UnlockSurface(screen);
		}
		/* Update just the part of the display that we've changed */
		SDL_UpdateRect(screen, xPos, yPos, smkPlayer->data->width, smkPlayer->data->height);
		SDL_Delay(50);
		smkPlayer->ptrSmackWait(smkPlayer->data);
		smkPlayer->ptrSmackNextFrame(smkPlayer->data);
	}
	else //end of video
	{
		return false;
	}
	return true;
}

#else

#include "../client/SDL_Extensions.h"

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

static URLProtocol lod_protocol = {
	protocol_name,
	lod_open,
	lod_read,
	NULL,						// no write
	NULL,						// no seek
	lod_close
};

CVideoPlayer::CVideoPlayer()
{
	format = NULL;
	frame = NULL;
	codec = NULL;
	sws = NULL;
	overlay = NULL;

	// Register codecs. TODO: May be overkill. Should call a
	// combination of av_register_input_format() /
	// av_register_output_format() / av_register_protocol() instead.
	av_register_all();

	// Register our protocol 'lod' so we can directly read from mmaped memory
	av_register_protocol(&lod_protocol);

	vidh = new CVidHandler(std::string(DATA_DIR "Data" PATHSEPARATOR "VIDEO.VID"));
}

bool CVideoPlayer::open(std::string fname, int x, int y)
{
	char url[100];

	close();

	data = vidh->extract(fname, length);

	if (data == NULL)
		return false;

	offset = 0;

	// Create our URL name with the 'lod' protocol as a prefix and a
	// back pointer to our object. Should be 32 and 64 bits compatible.
	sprintf(url, "%s:0x%016llx", protocol_name, (unsigned long long)(uintptr_t)this);

	if (av_open_input_file(&format, url, NULL, 0, NULL)!=0)
		return false;

	// Retrieve stream information
	if (av_find_stream_info(format) < 0)
		return false;
  
#if 0
	// Dump information about file onto standard error
	dump_format(format, 0, url, 0);
#endif

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
	overlay = SDL_CreateYUVOverlay(codecContext->width, codecContext->height,
								   SDL_YV12_OVERLAY, screen);

	// Convert the image into YUV format that SDL uses
	sws = sws_getContext(codecContext->width, codecContext->height, 
						 codecContext->pix_fmt, codecContext->width, codecContext->height, 
						 PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	if (sws == NULL)
		return false;

	pos.x = x;
	pos.y = y;
	pos.w = codecContext->width;
	pos.h = codecContext->height;

	return true;
}

// Display the next frame. Return false on error/end of file.
bool CVideoPlayer::nextFrame()
{
	AVPacket packet;
	int frameFinished = 0;

	while(!frameFinished && av_read_frame(format, &packet)>=0) {

		// Is this a packet from the video stream?
		if (packet.stream_index == stream) {
			// Decode video frame
			avcodec_decode_video(codecContext, frame, &frameFinished, 
								 packet.data, packet.size);
      
			// Did we get a video frame?
			if (frameFinished) {
				SDL_LockYUVOverlay(overlay);

				AVPicture pict;
				pict.data[0] = overlay->pixels[0];
				pict.data[1] = overlay->pixels[2];
				pict.data[2] = overlay->pixels[1];

				pict.linesize[0] = overlay->pitches[0];
				pict.linesize[1] = overlay->pitches[2];
				pict.linesize[2] = overlay->pitches[1];

				sws_scale(sws, frame->data, frame->linesize,
						  0, codecContext->height, pict.data, pict.linesize);

				SDL_UnlockYUVOverlay(overlay);
	
				SDL_DisplayYUVOverlay(overlay, &pos);
			}
		}

		av_free_packet(&packet);
	}

	return frameFinished != 0;
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
	
CVideoPlayer::~CVideoPlayer()
{
	close();
}

#endif

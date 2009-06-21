#include "../stdafx.h"
#include <iostream>
#include "CSndHandler.h"
#include "CVideoHandler.h"
#include <SDL.h>

#ifdef _WIN32

void DLLHandler::Instantiate(const char *filename)
{
#ifdef _WIN32
	dll = LoadLibraryA(filename);
#else
	dll = dlopen(filename,RTLD_LOCAL | RTLD_LAZY);
#endif
}
const char *DLLHandler::GetLibExtension()
{
#ifdef _WIN32
	return "dll";
#elif defined(__APPLE__)
	return "dylib";
#else
	return "so";
#endif
}



void *DLLHandler::FindAddress234(const char *symbol)
{
#ifdef _WIN32
	if ((int)symbol == 0x00001758)
		return NULL;
	std::cout<<"co ja tu robie"<<std::endl;
	return (void*) GetProcAddress(dll,symbol);
#else
	return (void *)dlsym(dll, symbol);
#endif
}
DLLHandler::~DLLHandler()
{
#ifdef _WIN32
	FreeLibrary(dll);
#else
	dlclose(dll);
#endif
}


CBIKHandler::CBIKHandler()
{
	ourLib.Instantiate("BINKW32.DLL");
	newmode=-1;
	waveOutOpen=0;
	///waveOutOpen = ourLib.FindAddress("_BinkOpenWaveOut@4");
}
int readNormalNr2 (unsigned char* bufor, int &iter, int bytCon)
{
	int ret=0;
	int amp=1;
	for (int i=iter; i<iter+bytCon; i++)
	{
		ret+=bufor[i]*amp;
		amp<<=8;
	}
	iter+=bytCon;
	return ret;
}
void RaiseLastOSErrorAt(char * offset)
{
#ifdef _WIN32
	int * lastError = new int;
	std::exception * error;
	*lastError = GetLastError();
	if (*lastError)
		throw lastError;
#else
	throw new std::exception();
#endif
}
//var
//  LastError: Integer;
//  Error: EOSError;
//begin
//  LastError := GetLastError;
//  if LastError <> 0 then
//    Error := EOSError.CreateResFmt(@SOSError, [LastError,
//      SysErrorMessage(LastError)])
//  else
//    Error := EOSError.CreateRes(@SUnkOSError);
//  Error.ErrorCode := LastError;
//  raise Error at Offset;
//end;
//void RSRaiseLastOSError()
//{
//	__asm
//	{
//		mov eax, [esp]
//		sub eax, 5
//		jmp RaiseLastOSErrorAt
//	}
//}
//int RSWin32Check(int CheckForZero)
//{
//	__asm
//	{
//		test eax, eax
//		jz RSRaiseLastOSError
//	}
//}
void CBIKHandler::open(std::string name)
{
#ifdef _WIN32
	hBinkFile = CreateFile
	(
		L"CSECRET.BIK",				// file name
		GENERIC_READ,						// access mode
		FILE_SHARE_READ,	// share mode
		NULL,								// Security Descriptor
		OPEN_EXISTING,						// how to create
		FILE_ATTRIBUTE_NORMAL,//FILE_FLAG_SEQUENTIAL_SCAN,			// file attributes
		0								// handle to template file
	);
	//RSWin32Check(hBinkFile!=INVALID_HANDLE_VALUE);
	if(hBinkFile == INVALID_HANDLE_VALUE)
	{
		printf("failed to open \"%s\"\n", name.c_str());
		return ;
	}


	try
	{
		BinkGetError = ourLib.FindAddress234("_BinkGetError@0");
		BinkOpen = ourLib.FindAddress234("_BinkOpen@8");
		if (!waveOutOpen)
		{
			BinkSetSoundSystem = ourLib.FindAddress234("_BinkSetSoundSystem@8");
			((void(*)(void*,void*))BinkSetSoundSystem)(waveOutOpen,NULL);
		}
		std::cout<<"punkt kulminacyjny... "<<std::flush;
		hBink = ((HBINK(*)(HANDLE)) BinkOpen)(hBinkFile);
		width = hBink->width;
		height = hBink->height;
		BITMAP gg;
		gg.bmWidth=width;
		gg.bmHeight=height;
		gg.bmBitsPixel=24;
		gg.bmPlanes=1;
		gg.bmWidthBytes=3*width;
		gg.bmBits = new unsigned char[width*height*(gg.bmBitsPixel/8)];

		//HBITMAP bitmapa = CreateBitmap(width, height,1,24,NULL);
		std::cout<<"przeszlo!"<<std::endl;
	}
	catch(...)
	{
		printf("cos nie tak");
	}
#endif
}

//void CBIKHandler::close()
//{
//	void *binkClose;
//	binkClose = ourLib.FindAddress234("_BinkClose@4");
//	(( void(*)() ) binkClose )();
//
//}
//void CBIKHandler::preparePic()
//procedure TRSBinkPlayer.PreparePic(b: TBitmap);
//var j:int; Pal:array[0..256] of int;
//begin
//  inherited;
//  case RSGetPixelFormat(b) of
//    pf24bit, pf32bit, pf15bit, pf16bit:;
//
//    pf8bit:
//    begin
//      if @BinkGetPalette=nil then
//        @BinkGetPalette:=GetProcAddress(FLib, '_BinkGetPalette@4');
//      if @BinkGetPalette<>nil then
//      begin
//        with PLogPalette(@Pal)^ do
//        begin
//          palVersion:=$300;
//          palNumEntries:=BinkGetPalette(@palPalEntry);
//          for j:=0 to palNumEntries-1 do
//            int(palPalEntry[j]):=RSSwapColor(int(palPalEntry[j]));
//        end;
//        b.Palette:=CreatePalette(PLogPalette(@Pal)^);
//      end else
//        b.PixelFormat:=pf24bit;
//    end;
//
//    else
//      b.PixelFormat:=pf24bit;
//  end
//
//end;

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




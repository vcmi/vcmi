#ifndef __CVIDEOHANDLER_H__
#define __CVIDEOHANDLER_H__
#include "../global.h"

#ifdef _WIN32

#include <windows.h>

struct SDL_Surface;

#pragma pack(push,1)
struct BINK_STRUCT
{
	si32 width;
	si32 height;
	si32 frameCount;
	si32 currentFrame;
	si32 lastFrame;
	si32 FPSMul;
	si32 FPSDiv;
	si32 unknown0;
	ui8 flags;
	ui8 unknown1[260];
	si32 CurPlane;		// current plane
	void *plane0;		// posi32er to plane 0
	void *plane1;		// posi32er to plane 1
	si32 unknown2;
	si32 unknown3;
	si32 yWidth;			// Y plane width
	si32 yHeight;		// Y plane height
	si32 uvWidth;		// U&V plane width
	si32 uvHeight;		// U&V plane height
};
#pragma pack(pop)

typedef BINK_STRUCT* HBINK;

class DLLHandler
{
public:
	std::string name;
	HINSTANCE dll;
	void Instantiate(const char *filename);
	const char *GetLibExtension();
	void *FindAddress(const char *symbol);

	DLLHandler();
	virtual ~DLLHandler(); //d-tor
};

typedef void*(__stdcall*  BinkSetSoundSystem)(void * soundfun, void*);
typedef HBINK(__stdcall*  BinkOpen)(HANDLE bikfile, int flags);
typedef si32(__stdcall*  BinkGetPalette)(HBINK);
typedef void(__stdcall*  BinkNextFrame)(HBINK);
typedef void(__stdcall*  BinkDoFrame)(HBINK);
typedef ui8(__stdcall*  BinkWait)(HBINK);
typedef si32(__stdcall*  BinkCopyToBuffer)(HBINK, void* buffer, int stride, int height, int x, int y, int mode);


class CBIKHandler : public DLLHandler
{
public:
	int newmode;
	HANDLE hBinkFile;
	HBINK hBink;
	char * buffer;
	BinkSetSoundSystem binkSetSoundSystem;
	BinkOpen binkOpen;
	BinkGetPalette getPalette;
	BinkNextFrame binkNextFrame;
	BinkDoFrame binkDoFrame;
	BinkCopyToBuffer binkCopyToBuffer;
	BinkWait binkWait;

	void * waveOutOpen, * binkGetError;

	int width, height;

	CBIKHandler();
	void open(std::string name);
	void close();
	void nextFrame();
	void show(int x, int y, SDL_Surface *dst);
	bool wait();
};

//////////SMK Player ///////////////////////////////////////////////////////

typedef enum { bmDIB, bmDDB} BitmapHandleType;
typedef enum { pfDevice, pf1bit, pf4bit, pf8bit, pf15bit, pf16bit, pf24bit, pf32bit, pfCustom} PixelFormat;
typedef enum {tmAuto, tmFixed} TransparentMode;

class TBitmap
{
public:
	ui32	width;
	ui32 height;
	PixelFormat pixelFormat;
	BitmapHandleType handleType;
	char* buffer;

};

struct SmackStruct
{
    si32 version;
    si32 width;
    si32 height;
    si32 frameCount;
    si32 mspf;
    ui8 unk1[88];
    ui8 palette[776];
    si32 currentFrame;	// Starting with 0
    ui8 unk[56];
    ui32 fileHandle;  // exact type is HANDLE in windows.h
};

// defines function pointer types
typedef SmackStruct* (__stdcall*  SmackOpen)(void* , ui32, si32 );
typedef int (__stdcall* SmackDoFrame)( SmackStruct * );
typedef void (__stdcall * SmackGoto )(SmackStruct *, int frameNumber);
typedef void (__stdcall* SmackNextFrame)(SmackStruct*);
typedef void (__stdcall* SmackClose)(SmackStruct*);
typedef void (__stdcall* SmackToBuffer) (SmackStruct*, int, int, int, int, char *, ui32);
typedef bool (__stdcall* SmackWait)(SmackStruct*);
typedef void (__stdcall* SmackSoundOnOff) (SmackStruct*, bool);



class CSmackPlayer: public DLLHandler
{
public:
	SmackOpen ptrSmackOpen;
	SmackDoFrame ptrSmackDoFrame;
	SmackToBuffer ptrSmackToBuffer;
	SmackNextFrame ptrSmackNextFrame;
	SmackWait ptrSmackWait;
	SmackSoundOnOff ptrSmackSoundOnOff;
	SmackStruct* data;

	void init();
	void preparePic(TBitmap &b);
	TBitmap extractFrame(TBitmap &b);
	void nextFrame();
	bool wait();
};

class CVidHandler;

class CVideoPlayer
{
private:
	CVidHandler * vidh; //.vid file handling
	CSmackPlayer * smkPlayer;

	int frame;
	int xPos, yPos;
	char * buffer;
	char * buf;


	std::string fname; //name of current video file (empty if idle)

public:
	CVideoPlayer(); //c-tor
	~CVideoPlayer(); //d-tor

	bool init();
	bool open(std::string fname, int x, int y); //x, y -> position where animation should be displayed on the screen
	void close();
	bool nextFrame(); // display next frame
};

#else

#include <SDL_video.h>

typedef struct AVFormatContext AVFormatContext;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVCodec AVCodec;
typedef struct AVFrame AVFrame;
struct SwsContext;
class CVidHandler;

class CVideoPlayer
{
private:
	int stream;					// stream index in video
	AVFormatContext *format;
	AVCodecContext *codecContext; // codec context for stream
	AVCodec *codec;
	AVFrame *frame; 
	struct SwsContext *sws;

	SDL_Overlay *overlay;
	SDL_Rect pos;				// overlay position

	CVidHandler *vidh;

public:
	CVideoPlayer();
	~CVideoPlayer();

	bool init();
	bool open(std::string fname, int x, int y);
	void close();
	bool nextFrame();			// display next frame

	const char *data;			// video buffer
	int length;					// video size
	unsigned int offset;		// current data offset
};

#endif

#endif // __CVIDEOHANDLER_H__

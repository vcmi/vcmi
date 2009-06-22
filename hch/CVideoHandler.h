#ifndef __CVIDEOHANDLER_H__
#define __CVIDEOHANDLER_H__
#include "../global.h"

#ifdef _WIN32

#include <stdio.h>
#include <windows.h>
#include <SDL.h>

//
#define BINKNOTHREADEDIO 0x00800000
//
//  protected
//    FLib: HINST;
//    FLibName: string;
//    FFileHandle: HFile;
//    function GetCurrentFrame: int; virtual; abstract;
//    function GetFramesCount: int; virtual; abstract;
//    procedure SetCurrentFrame(v: int); virtual; abstract;
//    procedure DoOpen(FileHandle: hFile); virtual; abstract;
//    function NormalizeFrame(i:int):int;
//    procedure SetPause(v:Boolean); virtual; abstract;
//
//    procedure LoadProc(var Proc:Pointer; const ProcName:string);
//  public
//    Width:pint;
//    Height:pint;
//    constructor Create(const LibName:string);
//    destructor Destroy; override;
//    procedure Open(FileHandle:hFile); overload;
//    procedure Open(FileName:string); overload;
////    procedure Open(FileData:TRSByteArray); overload;
//    procedure SetVolume(i: int); virtual;
//    procedure Close; virtual;
//    procedure NextFrame; virtual; abstract;
//    procedure PreparePic(b:TBitmap); virtual;
//    procedure GotoFrame(Index:int; b:TBitmap); virtual;
//    function ExtractFrame(b:TBitmap = nil):TBitmap; virtual; abstract;
//    function Wait:Boolean; virtual; abstract;
//      // Workaround for Bink and Smack thread synchronization bug
//    property Frame: int read GetCurrentFrame write SetCurrentFrame;
//    property FramesCount: int read GetFramesCount;
//    property LibInstance: HINST read FLib;
//    property Pause: Boolean write SetPause;

  //TRSSmkStruct = packed record
  //  Version: int;
  //  Width: int;
  //  Height: int;
  //  FrameCount: int;
  //  mspf: int;
  //  Unk1: array[0..87] of byte;
  //  Palette: array[0..775] of byte;
  //  CurrentFrame: int; // Starting with 0
  //   // 72 - ุ่๏
  //   // 1060 - interesting
  //   // 1100 - Mute:Bool
  //end;

  //TRSBinkStruct = packed record
  //  Width: int;
  //  Height: int;
  //  FrameCount: int;
  //  CurrentFrame: int; // Starting with 1
  //  LastFrame: int;
  //  FPSMul: int; // frames/second multiplier
  //  FPSDiv: int; // frames/second divisor
  //  Unk1: int;
  //  Flags: int;
  //  Unk2: array[0..259] of byte;
  //  CurrentPlane: int;
  //  Plane1: ptr;
  //  Plane2: ptr;
  //  Unk3: array[0..1] of int;
  //  YPlaneWidth: int;
  //  YPlaneHeight: int;
  //  UVPlaneWidth: int;
  //  UVPlaneHeight: int;
  //end;
typedef struct
{
	int width;
	int height;
	int frameCount;
	int currentFrame;
	int lastFrame;
	int FPSMul;
	int FPSDiv;
	int unknown0;
	unsigned char flags;
	unsigned char unknown1[260];
	int CurPlane;		// current plane
	void *plane0;		// pointer to plane 0
	void *plane1;		// pointer to plane 1
	int unknown2;
	int unknown3;
	int yWidth;			// Y plane width
	int yHeight;		// Y plane height
	int uvWidth;		// U&V plane width
	int uvHeight;		// U&V plane height
	int d,e,f,g,h,i;
} BINK_STRUCT, *HBINK;

struct SMKStruct
{
	int version, width, height, frameCount, mspf, currentFrame;
	unsigned char unk1[88], palette[776];
};




class DLLHandler
{
public:
#if !defined(__amigaos4__) && !defined(__unix__) && !defined(__APPLE__)
	HINSTANCE dll;
#else
	void *dll;
#endif
	void Instantiate(const char *filename);
	const char *GetLibExtension();
	void *FindAddress234(const char *symbol);

	virtual ~DLLHandler(); //d-tor
};

class CBIKHandler
{
public:
	DLLHandler ourLib;
	int newmode;
#if !defined(__amigaos4__) && !defined(__unix__) && !defined(__APPLE__)
	HANDLE hBinkFile;
#else
	void *hBinkFile;
#endif
	HBINK hBink;
	BINK_STRUCT data;
	unsigned char * buffer;
	void * waveOutOpen, * BinkGetError, *BinkOpen, *BinkSetSoundSystem ;

	int width, height;

	CBIKHandler();
	void open(std::string name);
	void close();
};

//////////SMK Player ///////////////////////////////////////////////////////


struct SmackStruct
{
    Sint32 version;		//
    Sint32 width;
    Sint32 height;
    Sint32 frameCount;
    Sint32 mspf;
    unsigned char unk1[88];
    unsigned char palette[776];
    Sint32 currentFrame;	// Starting with 0

     // 72 - ุ่?
     // 1060 - interesting
     // 1100 - Mute:Bool

    unsigned char unk[56];
    Uint32 fileHandle;  // exact type is HANDLE in windows.h
};

// defines function pointer type
typedef SmackStruct* (__stdcall*  SmackOpen)(void* , Uint32, Sint32 );
// todo default value
typedef int (__stdcall* SmackDoFrame)( SmackStruct * );
typedef void (__stdcall * SmackGoto )(SmackStruct *, int frameNumber);
typedef void (__stdcall* SmackNextFrame)(SmackStruct*);
typedef void (__stdcall* SmackClose)(SmackStruct*);
typedef void (__stdcall* SmackToBuffer) (SmackStruct*, int, int, int, int, char *, Uint32);
typedef bool (__stdcall* SmackWait)(SmackStruct*);
typedef void (__stdcall* SmackSoundOnOff) (SmackStruct*, bool);


typedef enum { bmDIB, bmDDB} BitmapHandleType;
typedef enum { pfDevice, pf1bit, pf4bit, pf8bit, pf15bit, pf16bit, pf24bit, pf32bit, pfCustom} PixelFormat;
typedef enum {tmAuto, tmFixed} TransparentMode;

class TBitmap
{
public:
    Uint32	width;
    Uint32 height;
	PixelFormat pixelFormat;
	BitmapHandleType handleType;
	char* buffer;
	
};

class CRADPlayer
{
public:
	HINSTANCE hinstLib;
	void loadProc(char* ptrFunc,char* procName);
	PixelFormat getPixelFormat(TBitmap);
};

class CSmackPlayer: public CRADPlayer{
public:
	SmackOpen ptrSmackOpen;
	SmackDoFrame ptrSmackDoFrame;
	SmackToBuffer ptrSmackToBuffer;
	SmackNextFrame ptrSmackNextFrame;
	SmackWait ptrSmackWait;
	SmackSoundOnOff ptrSmackSoundOnOff;
	SmackStruct* data;

	void preparePic(TBitmap b);
	TBitmap extractFrame(TBitmap b);
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
public:
	CVideoPlayer(); //c-tor
	~CVideoPlayer(); //d-tor

	bool init();
	bool open(std::string fname, int x, int y); //x, y -> position where animation should be displayed on the screen
	void close();
	bool nextFrame(); // display next frame
};

#else

#ifdef _WIN32
#include <SDL_video.h>
#else
#include <SDL/SDL_video.h>
#endif

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

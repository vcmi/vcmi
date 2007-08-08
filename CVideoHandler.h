#ifndef CVIDEOHANDLEER_H
#define CVIDEOHANDLEER_H

#include "windows.h"
//
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
  //   // 72 - Øèï
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
struct BINKStruct
{
	int width, height, frameCount, lastFrame, currentFrame,
		FPSMul, // frames/second multiplier
		FPSDiv, // frames/second divisor
		unk1, flags, YPlaneWidth, YPlaneHeight, UVPlaneWidth, UVPlaneHeight;
	unsigned char unk2[260];
	int unk3[2];
	void *plane1, *plane2;
};

struct SMKStruct
{
	int version, width, height, frameCount, mspf, currentFrame;
	unsigned char unk1[88], palette[776];
};




class DLLHandler
{
public:
	HINSTANCE dll;

	void Instantiate(const char *filename);
	const char *GetLibExtension();
	void *FindAddress(const char *symbol);

	virtual ~DLLHandler();
};

class CBIKHandler
{
public:
	DLLHandler ourLib;
	std::ifstream str;
	int newmode;
	BINKStruct data;
	unsigned char * buffer;
	void * waveOutOpen, * BinkGetError, *BinkOpen, *BinkSetSoundSystem ;

	CBIKHandler();
	void open(std::string name);
	void close();
};
#endif //CVIDEOHANDLEER_H
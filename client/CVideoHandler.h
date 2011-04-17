#ifndef __CVIDEOHANDLER_H__
#define __CVIDEOHANDLER_H__
#include "../global.h"

struct SDL_Surface;

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN //excludes rarely used stuff from windows headers - delete this line if something is missing
#include <windows.h>

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
typedef void(__stdcall*  BinkClose)(HBINK);
//typedef si32(__stdcall*  BinkGetPalette)(HBINK);
typedef void(__stdcall*  BinkNextFrame)(HBINK);
typedef void(__stdcall*  BinkDoFrame)(HBINK);
typedef ui8(__stdcall*  BinkWait)(HBINK);
typedef si32(__stdcall*  BinkCopyToBuffer)(HBINK, void* buffer, int stride, int height, int x, int y, int mode);

class IVideoPlayer
{
public:
	virtual bool open(std::string name)=0; //true - succes
	virtual void close()=0;
	virtual void nextFrame()=0;
	virtual void show(int x, int y, SDL_Surface *dst, bool update = true)=0;
	virtual void redraw(int x, int y, SDL_Surface *dst, bool update = true)=0; //reblits buffer
	virtual bool wait()=0;
	virtual int curFrame() const =0;
	virtual int frameCount() const =0;
};

class CBIKHandler : public DLLHandler, public IVideoPlayer
{
	void allocBuffer(int Bpp = 0);
	void freeBuffer();
public:
	HANDLE hBinkFile;
	HBINK hBink;
	char * buffer;
	int bufferSize;
	BinkSetSoundSystem binkSetSoundSystem;
	BinkOpen binkOpen;
	//BinkGetPalette getPalette;
	BinkNextFrame binkNextFrame;
	BinkDoFrame binkDoFrame;
	BinkCopyToBuffer binkCopyToBuffer;
	BinkWait binkWait;
	BinkClose binkClose;

	CBIKHandler();
	bool open(std::string name);
	void close();
	void nextFrame();
	void show(int x, int y, SDL_Surface *dst, bool update = true);
	void redraw(int x, int y, SDL_Surface *dst, bool update = true); //reblits buffer
	bool wait();
	int curFrame() const;
	int frameCount() const;
};

//////////SMK Player ///////////////////////////////////////////////////////

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
typedef int (__stdcall* SmackVolumePan)(SmackStruct *, int SmackTrack, int volume, int pan);



class CSmackPlayer: public DLLHandler, public IVideoPlayer
{
public:
	SmackOpen ptrSmackOpen;
	SmackDoFrame ptrSmackDoFrame;
	SmackToBuffer ptrSmackToBuffer;
	SmackNextFrame ptrSmackNextFrame;
	SmackWait ptrSmackWait;
	SmackSoundOnOff ptrSmackSoundOnOff;
	SmackClose ptrSmackClose;
	SmackVolumePan ptrVolumePan;

	char *buffer, *buf;
	SmackStruct* data;

	CSmackPlayer();
	~CSmackPlayer();
	bool open(std::string name);
	void close();
	void nextFrame();
	void show(int x, int y, SDL_Surface *dst, bool update = true);
	void redraw(int x, int y, SDL_Surface *dst, bool update = true); //reblits buffer
	bool wait();
	int curFrame() const;
	int frameCount() const;
};

class CVidHandler;

class CVideoPlayer : public IVideoPlayer
{
private:
	CVidHandler * vidh; //.vid file handling
	CVidHandler *vidh_ab; // armageddon's blade video file handling

	CSmackPlayer smkPlayer; //for .SMK
	CBIKHandler bikPlayer; //for .BIK
	IVideoPlayer *current; //points to bik or smk player, appropriate to type of currently played video

	bool first; //are we about to display the first frame (blocks update)
public:
	std::string fname; //name of current video file (empty if idle)
	
	CVideoPlayer(); //c-tor
	~CVideoPlayer(); //d-tor


	bool open(std::string name);
	void close();
	void nextFrame(); //move animation to the next frame
	void show(int x, int y, SDL_Surface *dst, bool update = true); //blit current frame
	void redraw(int x, int y, SDL_Surface *dst, bool update = true); //reblits buffer
	void update(int x, int y, SDL_Surface *dst, bool forceRedraw, bool update = true); //moves to next frame if appropriate, and blits it or blits only if redraw paremeter is set true
	bool wait(); //true if we should wait before displaying next frame (for keeping FPS)
	int curFrame() const; //current frame number <1, framecount>
	int frameCount() const;

	bool openAndPlayVideo(std::string name, int x, int y, SDL_Surface *dst, bool stopOnKey = false); //opens video, calls playVideo, closes video; returns playVideo result (if whole video has been played)
	bool playVideo(int x, int y, SDL_Surface *dst, bool stopOnKey = false); //plays whole opened video; returns: true when whole video has been shown, false when it has been interrupted
};

#define VIDEO_WIN "WIN3.BIK"
#define VIDEO_LOSE_BATTLE_START "LBSTART.BIK"
#define VIDEO_LOSE_BATTLE_LOOP "LBLOOP.BIK"
#define VIDEO_RETREAT_START "RTSTART.BIK"
#define VIDEO_RETREAT_LOOP "RTLOOP.BIK"
#define VIDEO_SURRENDER "SURRENDER.BIK"

#else

#include <SDL_video.h>

typedef struct AVFormatContext AVFormatContext;
typedef struct AVCodecContext AVCodecContext;
typedef struct AVCodec AVCodec;
typedef struct AVFrame AVFrame;
struct SwsContext;
class CVidHandler;

class CVideoPlayer //: public IVideoPlayer
{
private:
	int stream;					// stream index in video
	AVFormatContext *format;
	AVCodecContext *codecContext; // codec context for stream
	AVCodec *codec;
	AVFrame *frame; 
	struct SwsContext *sws;

	// Destination. Either overlay or dest.
	SDL_Overlay *overlay;
	SDL_Surface *dest;
	SDL_Rect destRect;			// valid when dest is used
	SDL_Rect pos;				// destination on screen

	CVidHandler *vidh;

	int refreshWait; // Wait several refresh before updating the image
	int refreshCount;
	bool doLoop;				// loop through video

public:
	CVideoPlayer();
	~CVideoPlayer();

	bool init();
	bool open(std::string fname, bool loop = false, bool useOverlay = false);
	void close();
	bool nextFrame();			// display next frame

	void show(int x, int y, SDL_Surface *dst, bool update = true); //blit current frame
	void redraw(int x, int y, SDL_Surface *dst, bool update = true); //reblits buffer
	void update(int x, int y, SDL_Surface *dst, bool forceRedraw, bool update = true); //moves to next frame if appropriate, and blits it or blits only if redraw parameter is set true
	
	// Opens video, calls playVideo, closes video; returns playVideo result (if whole video has been played)
	bool playVideo(int x, int y, SDL_Surface *dst, bool stopOnKey);
	bool openAndPlayVideo(std::string name, int x, int y, SDL_Surface *dst, bool stopOnKey = false);

	const char *data;			// video buffer
	int length;					// video size
	unsigned int offset;		// current data offset
	std::string fname; //name of current video file (empty if idle)
};

#define VIDEO_WIN "win3.mjpg"
#define VIDEO_LOSE_BATTLE_START "lbstart.mjpg"
#define VIDEO_LOSE_BATTLE_LOOP "lbloop.mjpg"
#define VIDEO_RETREAT_START "rtstart.mjpg"
#define VIDEO_RETREAT_LOOP "rtloop.mjpg"
#define VIDEO_SURRENDER "surrender.mjpg"

#endif

#endif // __CVIDEOHANDLER_H__

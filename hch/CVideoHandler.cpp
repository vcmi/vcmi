#include "stdafx.h"
#include <iostream>
#include "CVideoHandler.h"
#include "SDL.h"
void DLLHandler::Instantiate(const char *filename)
{
	dll = LoadLibraryA(filename);
}
const char *DLLHandler::GetLibExtension()
{
#ifdef WIN32
	return "dll";
#elif defined(__APPLE__)
	return "dylib";
#else
	return "so";
#endif
}



void *DLLHandler::FindAddress234(const char *symbol)
{
	if ((int)symbol == 0x00001758)
		return NULL;
	std::cout<<"co ja tu robie"<<std::endl;
	return (void*) GetProcAddress(dll,symbol);
}
DLLHandler::~DLLHandler()
{
	FreeLibrary(dll);
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
		amp*=256;
	}
	iter+=bytCon;
	return ret;
}
void RaiseLastOSErrorAt(char * offset)
{
	int * lastError = new int;
	std::exception * error;
	*lastError = GetLastError();
	if (*lastError)
		throw lastError;


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
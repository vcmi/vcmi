#include "stdafx.h"
#include "CVideoHandler.h"

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
void *DLLHandler::FindAddress(const char *symbol)
{
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

void CBIKHandler::open(std::string name)
{
	unsigned char * fdata = new unsigned char[400];
	unsigned char * fdata2 = new unsigned char[400];
	for (int i=0;i<400;i++) fdata[i]=0;

	//str.open(name.c_str(),std::ios::binary);
	BinkGetError = ourLib.FindAddress("_BinkGetError@0");
	BinkOpen = ourLib.FindAddress("_BinkOpen@8");
	BinkSetSoundSystem = ourLib.FindAddress("_BinkSetSoundSystem@8");

	//((void(*)(void*,void*)) BinkSetSoundSystem)(waveOutOpen,NULL);

	while (!fdata)
		fdata = ((unsigned char *(*)(const char *)) BinkOpen)("CSECRET.BIK");

	
		fdata2 = ((unsigned char *(*)()) BinkGetError)();


	int it = 0;
	data.width = readNormalNr2(fdata,it,4);
	data.height =  readNormalNr2(fdata,it,4);
	data.frameCount =  readNormalNr2(fdata,it,4);
	data.currentFrame =   readNormalNr2(fdata,it,4);
	data.lastFrame =  readNormalNr2(fdata,it,4);

  //FData:= BinkOpen(FileHandle);
  //if FData = nil then
  //  raise ERSRADException.Create(BinkGetError);
  //Width:= @FData^.Width;
  //Height:= @FData^.Height;
}
void CBIKHandler::close()
{
	str.close();
	void *binkClose;
	binkClose = ourLib.FindAddress("_BinkClose@4");
	(( void(*)() ) binkClose )();

}
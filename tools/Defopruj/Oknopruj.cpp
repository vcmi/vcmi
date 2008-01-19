#include "Oknopruj.h"
#include "tchar.h"
#include "CDefHandler.h"
#include "Oknopruj.h"
#include <string>
#include <sstream>
using namespace System;
using namespace wyprujdef;
std::string Oknopruj::ToString(System::String^ src)
{
	std::string dest;
	using namespace System::Runtime::InteropServices;
	const char* chars = (const char*)(Marshal::StringToHGlobalAnsi(src)).ToPointer();
	dest = chars;
	Marshal::FreeHGlobal(IntPtr((void*)chars));
	return dest;
}
void Oknopruj::wyprujDefyZPlikow(array<String^> ^pliki)
{
	progressBar1->Maximum = pliki->Length;
	progressBar1->Value = 0;
	CDefHandler * defik;
	for each(String ^ plik in pliki)
	{
		progressBar1->Value++;
		if(!((plik->EndsWith(".def")||(plik->EndsWith(".DEF")))))
			continue;
		defik = new CDefHandler();
		defik->openDef(ToString(plik));

		int to=1;
		std::string bmpname;
		if (rall->Checked)
		{
			to = defik->ourImages.size();
		}

		for (int i=0;i<to;i++)
		{
			std::ostringstream oss;
			oss << ToString(plik->Substring(0,plik->Length-4)) << '_' << i << "_.bmp";
			SDL_SaveBMP(defik->ourImages[i].bitmap,oss.str().c_str());
		}


		delete defik;
	}
}
[STAThreadAttribute]
int WinMain()
{
	String^ folder = (System::IO::Directory::GetCurrentDirectory());
	array<String^>^ pliki = IO::Directory::GetFiles(folder);
	Oknopruj ^ okno = gcnew Oknopruj();
	Application::Run(okno);
	exit(0);
}

void Oknopruj::runSearch()
{
	wyprujDefyZPlikow(IO::Directory::GetFiles(pathBox->Text));
}// wyprujdef.cpp : main project file.

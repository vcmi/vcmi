// Wpasujbud.cpp : main project file.
//#include <msclr\marshal.h>
#include "SDL.h"
#include "Wpasujbud.h"
#include "tchar.h"
#include "obrazek.h"
#include "dataEditor.h"
using namespace Wpasuj;

int Inaccuracy=5;
int zgodnosc(Bitmap^ bg, Bitmap^ st, int x, int y);
int zgodnosc(SDL_Surface * bg, SDL_Surface * st, int x, int y);

std::string Wpasujbud::ToString(System::String^ src)
{
	std::string dest;
	using namespace System::Runtime::InteropServices;
	const char* chars = (const char*)(Marshal::StringToHGlobalAnsi(src)).ToPointer();
	dest = chars;
	Marshal::FreeHGlobal(IntPtr((void*)chars));
	return dest;
}

[STAThreadAttribute]
int __stdcall WinMain()
{
	// Enabling Windows XP visual effects before any controls are created
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false); 

	Application::Run(gcnew Wpasujbud());
	exit(0);
	return 0;
}

Wpasujbud::Wpasujbud(void)
{
	InitializeComponent();
	openFileDialog1->InitialDirectory = IO::Directory::GetCurrentDirectory();
	IO::StreamReader ^sr = gcnew IO::StreamReader(L"config/TOWNTYPE.TXT");
	Int32 i=0;
	while(!sr->EndOfStream)
	{
		String ^ n = sr->ReadLine();
		n = (i.ToString()) + L". " + n;
		townID->Items->Add(n);
		i++;
	}
	townID->SelectedIndex=0;
	sr->Close();
	i++;

	
	sr = gcnew IO::StreamReader(L"config/BNAMES.TXT");
	i=0;
	while(!sr->EndOfStream)
	{
		String ^ n = sr->ReadLine();
		//n = (i.ToString()) + L". " + n;
		buildingType->Items->Add(n);
		i++;
	}
	buildingType->SelectedIndex=0;
	sr->Close();
	i++;

	bitmapsFolder->Text = openFileDialog1->InitialDirectory;

}

System::Void Wpasujbud::searchBitmaps_Click(System::Object^  sender, System::EventArgs^  e)
{
	bitmapList->Items->Clear();
	array<String^>^ pliki = IO::Directory::GetFiles(bitmapsFolder->Text,bmpPattern->Text);
	for each(String ^ plik in pliki)
	{
		//if(plik->EndsWith(L".bmp") || plik->EndsWith(L".BMP"))
			bitmapList->Items->Add(plik->Substring(plik->LastIndexOf('\\')+1));
	}
	if(bitmapList->Items->Count>0)
		bitmapList->SelectedIndex=0;
}
System::Void Wpasujbud::startLocating_Click(System::Object^  sender, System::EventArgs^  e)
{
	searchPicturePos();
}
System::Void Wpasujbud::setBackground_Click(System::Object^  sender, System::EventArgs^  e)
{
	if(townBgPath->Text->Length==0)
		return;
	Bitmap ^ bg = gcnew Bitmap(townBgPath->Text);
	townBg->Image = dynamic_cast<Image^>(bg);
	sbg = SDL_LoadBMP(ToString(townBgPath->Text).c_str());
}
System::Void Wpasujbud::openFileDialog1_FileOk(System::Object^  sender, System::ComponentModel::CancelEventArgs^  e) 
{
	townBgPath->Text = (dynamic_cast<OpenFileDialog^>(sender))->FileName;
}
System::Void Wpasujbud::browseForBg_Click(System::Object^  sender, System::EventArgs^  e)
{
	openFileDialog1->ShowDialog();
}
System::Void Wpasujbud::browseForbmpfol_Click(System::Object^  sender, System::EventArgs^  e)
{
	folderBrowserDialog1->ShowDialog();
	bitmapsFolder->Text = folderBrowserDialog1->SelectedPath;
}
System::Void Wpasujbud::bitmapList_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e)
{
	int nsel = bitmapList->SelectedIndex;
	if (nsel<0)
		return;
	String ^ bmpname = bitmapsFolder->Text + "\\" + dynamic_cast<String^>(bitmapList->SelectedItem);
	Bitmap ^building = gcnew Bitmap(bmpname);
	buildingImg->Image = dynamic_cast<Image^>(building);
}
System::Void Wpasujbud::townBg_Click(System::Object^  sender, System::EventArgs^  e)
{
	//Int32 tx, ty, ux, uy;
	//tx = townBg->Location.X;
	//ty = townBg->Location.Y;
	//ux = Location.X;
	//uy = Location.Y;
	mx = MousePosition.X - townBg->Location.X - Location.X - 4;
	my = MousePosition.Y - townBg->Location.Y - Location.Y - 30;
	koordy->Text = mx.ToString() + L", " + my.ToString();
}
System::Void Wpasujbud::button1_Click(System::Object^  sender, System::EventArgs^  e) //skip
{
	nextPicture();
}

Uint32 SDL_GetPixel(SDL_Surface *surface, int x, int y)
{
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * surface->format->BytesPerPixel;

    switch(surface->format->BytesPerPixel) 
	{
    case 1:
		{
			SDL_Color * color = surface->format->palette->colors+(*p);
			return color->r<<16 | color->g<<8 | color->b;
		}
    case 3:
        return p[0] | p[1] << 8 | p[2] << 16;
    }
}
int zgodnosc(SDL_Surface * bg, SDL_Surface * st, int x, int y)
{
	int ret=0;
	for(int i=0;i<st->w;i+=Inaccuracy)
	{
		for(int j=0;j<st->h;j+=Inaccuracy)
		{
			int c1 = SDL_GetPixel(bg,i+x,j+y);
			int c2 = SDL_GetPixel(st,i,j);
			if((i+x)>=bg->w)
				break;
			if((j+y)>=bg->h)
				break;
			if(SDL_GetPixel(bg,i+x,j+y) == SDL_GetPixel(st,i,j))
			{
				ret++;
			}
		}
	}
	return ret*(Inaccuracy*Inaccuracy);
}
int zgodnosc(Bitmap^ bg, Bitmap^ st, int x, int y)
{
	int ret=0;
	for(int i=0;i<st->Width;i++)
	{
		for(int j=0;j<st->Height;j++)
		{
			//Color c1 = bg->GetPixel(i+x,j+y);
			//Color c2 = st->GetPixel(i,j);
			if((i+x)>=bg->Width)
				break;
			if((j+y)>=bg->Height)
				break;
			if(bg->GetPixel(i+x,j+y) == st->GetPixel(i,j))
			{
				ret++;
			}
		}
	}
	return ret;
}
void Wpasujbud::searchPicturePos()
{
	//Bitmap^ b = gcnew Bitmap((townBg->Image)->Width,(townBg->Image)->Height,Imaging::PixelFormat::Format24bppRgb);
	//Bitmap^ s = dynamic_cast<Bitmap^>(buildingImg->Image);
	//Graphics ^ g = Graphics::FromImage(b);
	//g->DrawImage(townBg->Image,0,0);
	//townBg->Image = b;
	String^ sss = bitmapsFolder->Text;
	if(!(sss->EndsWith(L"/")||sss->EndsWith(L"\\")))
		sss+=L"\\";
	sss+=dynamic_cast<String^>(bitmapList->SelectedItem);
	SDL_Surface *str = SDL_LoadBMP(ToString(sss).c_str());
	Int32 colorPxs = 0;

	int aq = 0x00ffff;
	for (int x=0;x<str->w;x++)
	{
		for(int y=0;y<str->h;y++)
		{
			if(SDL_GetPixel(str,x,y) != aq)
				colorPxs++;
		}
	}
	int zgoda=0, retx, rety;
	int px=mx, py=my, maxr = Convert::ToInt32(radious->Text);
	if (radioButton2->Checked)
	{
		px-=str->w/2;
		py-=str->h/2;
	}
	else if (radioButton3->Checked)
	{
		px-=str->w;
		py-=str->h;
	}
	px-=maxr/2;
	py-=maxr/2;
	for(int i=0;i<maxr;i++)
	{
		for (int j=0;j<maxr;j++)
		{
			int pom = zgodnosc(sbg,str,px+i,py+j);
			if (pom>zgoda)
			{
				zgoda = pom;
				retx = px+i;
				rety = py+j;
			}
		}
	}
	//for(int r=0;r<maxr;r++)
	//{
	//	for(int i=0;i<=r;i++)
	//	{
	//		int pom, pom2;
	//		pom = zgodnosc(sbg,str,px+i,py);
	//		if (pom>zgoda)
	//		{
	//			zgoda = pom;
	//			retx = px+i;
	//			rety = py;
	//		}
	//		pom = zgodnosc(sbg,str,px+i,py+r);
	//		if (pom>zgoda)
	//		{
	//			zgoda = pom;
	//			retx = px+i;
	//			rety = py+r;
	//		}
	//		pom = zgodnosc(sbg,str,px,py+i);
	//		if (pom>zgoda)
	//		{
	//			zgoda = pom;
	//			retx = px;
	//			rety = py+i;
	//		}
	//		pom = zgodnosc(sbg,str,px+r,py+i);
	//		if (pom>zgoda)
	//		{
	//			zgoda = pom;
	//			retx = px+r;
	//			rety = py+i;
	//		}
	//		float per = zgoda/colorPxs;
	//		
	//	}
	//	px--;
	//	py--;
	//}
	accordanceBox->Text = zgoda.ToString() + L"/" + colorPxs.ToString();
	curx = retx;
	cury = rety;
	foundedCoords->Text = retx.ToString() + L", " + rety.ToString();
	//townBg->Refresh();
}

System::Void Wpasujbud::inaccuracy_TextChanged(System::Object^  sender, System::EventArgs^  e)
{
	int ni = Convert::ToUInt32(inaccuracy->Text);
	if(ni==0)
	{
		inaccuracy->Text == L"1";
		Inaccuracy = 1;
	}
	else
	{
		Inaccuracy = ni;
	}
}

void Wpasujbud::nextPicture()
{
	if(bitmapList->SelectedIndex<(bitmapList->Items->Count-1))
		bitmapList->SelectedIndex++;
}
void Wpasujbud::previousPicture()
{
	if(bitmapList->SelectedIndex>0)
		bitmapList->SelectedIndex--;
} 

System::Void Wpasujbud::confirm_Click(System::Object^  sender, System::EventArgs^  e)
{
	CBuildingData ^cbd = gcnew CBuildingData();
	//cbd->defname = bitmapsFolder->Text;
	//if(!(cbd->defname->EndsWith(L"/")||cbd->defname->EndsWith(L"\\")))
	//	cbd->defname+=L"\\";
	cbd->defname+=dynamic_cast<String^>(bitmapList->SelectedItem);
	cbd->defname = (cbd->defname)->Substring(0,cbd->defname->Length-7)+".def";
	cbd->ID = buildingType->SelectedIndex;
	cbd->townID = townID->SelectedIndex;
	cbd->x = curx;
	cbd->y = cury;

	//int curind = bitmapList->SelectedIndex;
	bitmapList->Items->RemoveAt(bitmapList->SelectedIndex);

	nextPicture();
	nextPicture();
	data.push_back(cbd);

}
	//System::String^ defname;
	//System::Int32 ID, x, y;
	//System::Int32 townID;
System::Void Wpasujbud::save_Click(System::Object^  sender, System::EventArgs^  e)
{
	String ^n = DateTime::Now.ToString()+".txt";
	n = n->Replace(L" ",L"___")->Replace(':','_');
	IO::StreamWriter sr(IO::Directory::GetCurrentDirectory() + L"\\" + n);
	for each (CBuildingData ^Data in data)
	{
		String ^temp = Data->ToString();
		sr.Write(temp);
	}
	sr.Close();
}

System::Void Wpasujbud::buildingImg_Click(System::Object^  sender, System::EventArgs^  e)
{
	if (!buildingImg->Image)
		return;
	obrazek ^ob = gcnew obrazek(buildingImg->Image);
	ob->Show();
}
System::Void Wpasujbud::dataview_Click(System::Object^  sender, System::EventArgs^  e)
{
	if (data.size()<=0)
		return;
	dataEditor ^ de = gcnew dataEditor(%data);
	de->Show();
}
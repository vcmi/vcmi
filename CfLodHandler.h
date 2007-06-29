//TSearchRec
#include <iostream>
#include <fstream>
#include <vector>
const int dmHelp=0, dmNoExtractingMask=1;
std::string P1,P2,CurDir;
struct TSearchRec 
{
}sr;
struct Entry
{
	unsigned char name[12], //filename
		hlam_1[4], //
		hlam_2[4]; //
	int offset, //from beginning
		realSize, //size without compression
		size;	//and with
} ;
std::vector<Entry> Entries;

long TotalFiles;
int readNormalNr (unsigned char* bufor, int bytCon, bool cyclic=false)
{
	int ret=0;
	int amp=1;
	for (int i=0; i<bytCon; i++)
	{
		ret+=bufor[i]*amp;
		amp*=256;
	}
	if(cyclic && bytCon<4 && ret>=amp/2)
	{
		ret = ret-amp;
	}
	return ret;
}
int Decompress (unsigned char * source, int size, int realSize, std::ofstream & dest);
void Extract(std::string FName)
{
	std::ifstream FLOD;
	std::ofstream FOut;
	long i;

	std::string Ts;
	std::cout<<"*** Loading FAT ... \n";
	FLOD.open(FName.c_str(),std::ios::binary);
	std::cout<<"*** Archive: "+FName+" loaded\n";
	FLOD.seekg(8,std::ios_base::beg);
	unsigned char temp[4];
	FLOD.read((char*)temp,4);
	TotalFiles = readNormalNr(temp,4);
	FLOD.seekg(0x5c,std::ios_base::beg);
	Entries.reserve(TotalFiles);
	std::cout<<"*** Loading FAT ...\n";
	for (int i=0;i<TotalFiles;i++)
	{
		Entries.push_back(Entry());
		FLOD.read((char*)Entries[i].name,12);
		FLOD.read((char*)Entries[i].hlam_1,4);
		FLOD.read((char*)temp,4);
		Entries[i].offset=readNormalNr(temp,4);
		FLOD.read((char*)temp,4);
		Entries[i].realSize=readNormalNr(temp,4);
		FLOD.read((char*)Entries[i].hlam_2,4);
		FLOD.read((char*)temp,4);
		Entries[i].size=readNormalNr(temp,4);
	}
	std::cout<<" done\n";
	if (TotalFiles>6) TotalFiles=6;
	for (int i=0;i<TotalFiles;i++)
	{
		FLOD.seekg(Entries[i].offset,std::ios_base::beg);
		FOut.open((char*)Entries[i].name,std::ios_base::binary);
		unsigned char* outp ;
		if (Entries[i].size==0)
		{
			outp = new unsigned char[Entries[i].realSize];
			FLOD.read((char*)outp,Entries[i].realSize);
		}
		else 
		{
			outp = new unsigned char[Entries[i].size];
			//unsigned char * outd = new unsigned char[Entries[i].realSize];
			FLOD.read((char*)outp,Entries[i].size);
			FLOD.seekg(0,std::ios_base::beg);
			//Decompress(outp,Entries[i].size,Entries[i].realSize,FOut);
		}
		for (int j=0;j<Entries[i].size;j++)
			FOut << outp[j];
		FOut.flush();
		delete outp;
		FOut.close();
		std::cout<<"*** done\n";
	}
	FLOD.close();
	std::cout<<"*** Archive: "+FName+" closed\n";
}
int Decompress (unsigned char * source, int size, int realSize, std::ofstream & dest)
{
	return -1;
} 

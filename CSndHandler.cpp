#include "stdafx.h"
#include "CSndHandler.h"


CSndHandler::~CSndHandler()
{
	entries.clear();
	file.close();
}
CSndHandler::CSndHandler(std::string fname):CHUNK(65535)
{
	file.open(fname.c_str(),std::ios::binary);
	if (!file.is_open())
		throw new std::exception((std::string("Cannot open ")+fname).c_str());
	int nr = readNormalNr(0,4);
	char tempc;
	for (int i=0;i<nr;i++)
	{
		Entry entry;
		while(true)
		{
			file.read(&tempc,1);
			if (tempc)
				entry.name+=tempc;
			else break;
		}
		entry.name+='.';
		while(true)
		{
			file.read(&tempc,1);
			if (tempc)
				entry.name+=tempc;
			else break;
		}
		file.seekg(40-entry.name.length()-1,std::ios_base::cur);
		entry.offset = readNormalNr(-1,4);
		entry.size = readNormalNr(-1,4);
		entries.push_back(entry);
	}
}
int CSndHandler::readNormalNr (int pos, int bytCon)
{
	if (pos>=0)
		file.seekg(pos,std::ios_base::beg);
	int ret=0;
	int amp=1;
	unsigned char zcz=0;
	for (int i=0; i<bytCon; i++)
	{
		file.read((char*)(&zcz),1);
		ret+=zcz*amp;
		amp*=256;
	}
	return ret;
}
void CSndHandler::extract(int index, std::string dstfile) //saves selected file
{
	std::ofstream out(dstfile.c_str(),std::ios_base::binary);
	file.seekg(entries[index].offset,std::ios_base::beg);
	int toRead=entries[index].size;
	char * buffer = new char[std::min(CHUNK,entries[index].size)];
	while (toRead>CHUNK)
	{
		file.read(buffer,CHUNK);
		out.write(buffer,CHUNK);
		toRead-=CHUNK;
	}
	file.read(buffer,toRead);
	out.write(buffer,toRead);
	out.close();
}
void CSndHandler::extract(std::string srcfile, std::string dstfile, bool caseSens) //saves selected file
{
	if (caseSens)
	{
		for (int i=0;i<entries.size();i++)
		{
			if (entries[i].name==srcfile)
				extract(i,dstfile);
		}
	}
	else
	{
		std::transform(srcfile.begin(),srcfile.end(),srcfile.begin(),tolower);
		for (int i=0;i<entries.size();i++)
		{
			if (entries[i].name==srcfile)
			{
				std::string por = entries[i].name;
				std::transform(por.begin(),por.end(),por.begin(),tolower);
				if (por==srcfile)
					extract(i,dstfile);
			}
		}
	}
}
unsigned char * CSndHandler::extract (int index, int & size)
{
	size = entries[index].size;
	unsigned char * ret = new unsigned char[size];
	file.seekg(entries[index].offset,std::ios_base::beg);
	file.read((char*)ret,entries[index].size);
	return ret;
}
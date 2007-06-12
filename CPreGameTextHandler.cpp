#include "CPreGameTextHandler.h"
#include "stdafx.h"

void CPreGameTextHandler::loadTexts()
{
	std::ifstream inp("H3bitmap.lod\\ZELP.TXT", std::ios::in|std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i=0; //buf iterator
	int hmcr=0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==3)
			break;
	}
	i+=3;

	int befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainNewGame = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainLoadGame = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainHighScores = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainCredits = buf.substr(befi, i-befi);
	++i;

	hmcr = 0;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			++hmcr;
		if(hmcr==1)
			break;
	}
	i+=3;

	befi=i;
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\t')
			break;
	}
	mainQuit = buf.substr(befi, i-befi);
	++i;
}
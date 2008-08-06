#include "../stdafx.h"
#include "CGeneralTextHandler.h"
#include "../CGameInfo.h"
#include "CLodHandler.h"
#include <fstream>

void CGeneralTextHandler::load()
{
	std::string buf = CGI->bitmaph->getTextFile("GENRLTXT.TXT");
	int andame = buf.size();
	int i=0; //buf iterator
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}

	i+=2;
	for(int jj=0; jj<764; ++jj)
	{
		std::string buflet;
		loadToIt(buflet, buf, i, 2);
		allTexts.push_back(buflet);
	}

	std::string  strs = CGI->bitmaph->getTextFile("ARRAYTXT.TXT");

	int itr=0;
	while(itr<strs.length()-1)
	{
		std::string tmp;
		loadToIt(tmp, strs, itr, 3);
		arraytxt.push_back(tmp);
	}

	itr = 0;
	std::string strin = CGI->bitmaph->getTextFile("PRISKILL.TXT");
	for(int hh=0; hh<4; ++hh)
	{
		std::string tmp;
		loadToIt(tmp, strin, itr, 3);
		primarySkillNames.push_back(tmp);
	}

	itr = 0;
	std::string strin2 = CGI->bitmaph->getTextFile("JKTEXT.TXT");
	for(int hh=0; hh<45; ++hh)
	{
		std::string tmp;
		loadToIt(tmp, strin2, itr, 3);
		jktexts.push_back(tmp);
	}

	itr = 0;
	std::string strin3 = CGI->bitmaph->getTextFile("HEROSCRN.TXT");
	for(int hh=0; hh<33; ++hh)
	{
		std::string tmp;
		loadToIt(tmp, strin3, itr, 3);
		heroscrn.push_back(tmp);
	}
}
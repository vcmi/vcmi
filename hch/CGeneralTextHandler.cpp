#include "../stdafx.h"
#include "CGeneralTextHandler.h"
#include "../CGameInfo.h"
#include "CLodHandler.h"
#include <fstream>

void CGeneralTextHandler::load()
{
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("GENRLTXT.TXT");
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
		CGeneralTextHandler::loadToIt(tmp, strs, itr, 3);
		arraytxt.push_back(tmp);
	}

	itr = 0;
	std::string strin = CGI->bitmaph->getTextFile("PRISKILL.TXT");
	for(int hh=0; hh<4; ++hh)
	{
		std::string tmp;
		CGeneralTextHandler::loadToIt(tmp, strin, itr, 3);
		primarySkillNames.push_back(tmp);
	}

	itr = 0;
	std::string strin2 = CGI->bitmaph->getTextFile("JKTEXT.TXT");
	for(int hh=0; hh<45; ++hh)
	{
		std::string tmp;
		CGeneralTextHandler::loadToIt(tmp, strin2, itr, 3);
		jktexts.push_back(tmp);
	}
}


void CGeneralTextHandler::loadToIt(std::string &dest, std::string &src, int &iter, int mode)
{
	switch(mode)
	{
	case 0:
		{
			int hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					++hmcr;
				if(hmcr==1)
					break;
			}
			++iter;

			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					break;
			}
			dest = src.substr(befi, iter-befi);
			++iter;

			hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\r')
					++hmcr;
				if(hmcr==1)
					break;
			}
			iter+=2;
			break;
		}
	case 1:
		{
			int hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					++hmcr;
				if(hmcr==1)
					break;
			}
			++iter;

			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\r')
					break;
			}
			dest = src.substr(befi, iter-befi);
			iter+=2;
			break;
		}
	case 2:
		{
			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					break;
			}
			dest = src.substr(befi, iter-befi);
			++iter;

			int hmcr = 0;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\r')
					++hmcr;
				if(hmcr==1)
					break;
			}
			iter+=2;
			break;
		}
	case 3:
		{
			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\r')
					break;
			}
			dest = src.substr(befi, iter-befi);
			iter+=2;
			break;
		}
	case 4:
		{
			int befi=iter;
			for(iter; iter<src.size(); ++iter)
			{
				if(src[iter]=='\t')
					break;
			}
			dest = src.substr(befi, iter-befi);
			iter++;
			break;
		}
	}
}


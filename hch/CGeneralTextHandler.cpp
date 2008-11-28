#define VCMI_DLL
#include "../stdafx.h"
#include "../lib/VCMI_Lib.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>

void CGeneralTextHandler::load()
{
	std::string buf = bitmaph->getTextFile("GENRLTXT.TXT"), tmp;
	int andame = buf.size();
	int i=0; //buf iterator
	for(i; i<andame; ++i)
	{
		if(buf[i]=='\r')
			break;
	}

	i+=2;
	std::string buflet;
	for(int jj=0; jj<764; ++jj)
	{
		loadToIt(buflet, buf, i, 2);
		if(buflet[0] == '"'  &&  buflet[buflet.size()-1] == '"')
			buflet = buflet.substr(1,buflet.size()-2);
		allTexts.push_back(buflet);
	}

	std::string  strs = bitmaph->getTextFile("ARRAYTXT.TXT");

	int itr=0;
	while(itr<strs.length()-1)
	{
		loadToIt(tmp, strs, itr, 3);
		arraytxt.push_back(tmp);
	}

	itr = 0;
	std::string strin = bitmaph->getTextFile("PRISKILL.TXT");
	for(int hh=0; hh<4; ++hh)
	{
		loadToIt(tmp, strin, itr, 3);
		primarySkillNames.push_back(tmp);
	}

	itr = 0;
	std::string strin2 = bitmaph->getTextFile("JKTEXT.TXT");
	for(int hh=0; hh<45; ++hh)
	{
		loadToIt(tmp, strin2, itr, 3);
		jktexts.push_back(tmp);
	}

	itr = 0;
	std::string strin3 = bitmaph->getTextFile("HEROSCRN.TXT");
	for(int hh=0; hh<33; ++hh)
	{
		loadToIt(tmp, strin3, itr, 3);
		heroscrn.push_back(tmp);
	}

	strin3 = bitmaph->getTextFile("ARTEVENT.TXT");
	for(itr = 0; itr<strin3.size();itr++)
	{
		loadToIt(tmp, strin3, itr, 3);
		artifEvents.push_back(tmp);
	}
}


std::string CGeneralTextHandler::getTitle(std::string text)
{
	std::string ret;
	int i=0;
	while ((text[i++]!='{'));
	while ((text[i]!='}') && (i<text.length()))
		ret+=text[i++];
	return ret;
}
std::string CGeneralTextHandler::getDescr(std::string text)
{
	std::string ret;
	int i=0;
	while ((text[i++]!='}'));
	i+=2;
	while ((text[i]!='"') && (i<text.length()))
		ret+=text[i++];
	return ret;
}
void CGeneralTextHandler::loadTexts()
{
	std::string buf1 = bitmaph->getTextFile("ZELP.TXT");
	int itr=0, eol=-1, eolnext=-1, pom;
	eolnext = buf1.find_first_of('\r',itr);
	while(itr<buf1.size())
	{
		eol = eolnext; //end of this line
		eolnext = buf1.find_first_of('\r',eol+1); //end of the next line
		pom=buf1.find_first_of('\t',itr); //upcoming tab
		if(eol<0 || pom<0)
			break;
		if(pom>eol) //in current line there is not tab
			zelp.push_back(std::pair<std::string,std::string>());
		else
		{
			zelp.push_back
				(std::pair<std::string,std::string>
				(buf1.substr(itr,pom-itr),
				buf1.substr(pom+1,eol-pom-1)));
			boost::algorithm::replace_all(zelp[zelp.size()-1].first,"\t","");
			boost::algorithm::replace_all(zelp[zelp.size()-1].second,"\t","");
		}
		itr=eol+2;
	}
	std::string buf = bitmaph->getTextFile("VCDESC.TXT");
	int andame = buf.size();
	int i=0; //buf iterator
	for(int gg=0; gg<14; ++gg)
	{
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		victoryConditions[gg] = buf.substr(befi, i-befi);
		i+=2;
	}
	buf = bitmaph->getTextFile("LCDESC.TXT");
	andame = buf.size();
	i=0; //buf iterator
	for(int gg=0; gg<4; ++gg)
	{
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		lossCondtions[gg] = buf.substr(befi, i-befi);
		i+=2;
	}

	hTxts.resize(HEROES_QUANTITY);

	buf = bitmaph->getTextFile("HEROSPEC.TXT");
	i=0;
	std::string dump;
	for(int iii=0; iii<2; ++iii)
	{
		loadToIt(dump,buf,i,3);
	}
	for (int iii=0;iii<hTxts.size();iii++)
	{
		loadToIt(hTxts[iii].bonusName,buf,i,4);
		loadToIt(hTxts[iii].shortBonus,buf,i,4);
		loadToIt(hTxts[iii].longBonus,buf,i,3);
	}

	buf = bitmaph->getTextFile("HEROBIOS.TXT");
	i=0;
	for (int iii=0;iii<hTxts.size();iii++)
	{
		loadToIt(hTxts[iii].biography,buf,i,3);
	}
}
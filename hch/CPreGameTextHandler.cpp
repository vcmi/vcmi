#include "../stdafx.h"
#include "CPreGameTextHandler.h"
#include "../CGameInfo.h"
#include "CLodHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
std::string CPreGameTextHandler::getTitle(std::string text)
{
	std::string ret;
	int i=0;
	while ((text[i++]!='{'));
	while ((text[i]!='}') && (i<text.length()))
		ret+=text[i++];
	return ret;
}
std::string CPreGameTextHandler::getDescr(std::string text)
{
	std::string ret;
	int i=0;
	while ((text[i++]!='}'));
	i+=2;
	while ((text[i]!='"') && (i<text.length()))
		ret+=text[i++];
	return ret;
}
void CPreGameTextHandler::loadTexts()
{
	std::string buf1 = CGameInfo::mainObj->bitmaph->getTextFile("ZELP.TXT");
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
	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("VCDESC.TXT");
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
	buf = CGameInfo::mainObj->bitmaph->getTextFile("LCDESC.TXT");
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
}
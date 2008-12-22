#define VCMI_DLL
#include "../stdafx.h"
#include "../lib/VCMI_Lib.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <fstream>
#include <sstream>

std::string readTo(std::string &in, unsigned int &it, char end)
{
	int pom = it;
	int last = in.find_first_of(end,it);
	it+=(1+last-it);
	return in.substr(pom,last-pom);
}
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

	unsigned it;
	buf = bitmaph->getTextFile("BLDGNEUT.TXT");
	andame = buf.size(), it=0;

	for(int b=0;b<15;b++)
	{
		std::string name = readTo(buf,it,'\t'),
			description = readTo(buf,it,'\n');
		for(int fi=0;fi<F_NUMBER;fi++)
		{
			buildings[fi][b].first = name;
			buildings[fi][b].second = description;
		}
	}
	buf1 = readTo(buf,it,'\n');buf1 = readTo(buf,it,'\n');buf1 = readTo(buf,it,'\n');//silo,blacksmith,moat - useless???
	//shipyard with the ship
	std::string name = readTo(buf,it,'\t'),
		description = readTo(buf,it,'\n');
	for(int fi=0;fi<F_NUMBER;fi++)
	{
		buildings[fi][20].first = name;
		buildings[fi][20].second = description;
	}

	for(int fi=0;fi<F_NUMBER;fi++)
	{
		buildings[fi][16].first = readTo(buf,it,'\t'),
			buildings[fi][16].second = readTo(buf,it,'\n');
	}
	/////done reading "BLDGNEUT.TXT"******************************

	buf = bitmaph->getTextFile("BLDGSPEC.TXT");
	andame = buf.size(), it=0;
	for(int f=0;f<F_NUMBER;f++)
	{
		for(int b=0;b<9;b++)
		{
			buildings[f][17+b].first = readTo(buf,it,'\t');
			buildings[f][17+b].second = readTo(buf,it,'\n');
		}
		buildings[f][26].first = readTo(buf,it,'\t');
		buildings[f][26].second = readTo(buf,it,'\n');
		buildings[f][15].first = readTo(buf,it,'\t'); //resource silo
		buildings[f][15].second = readTo(buf,it,'\n');//resource silo
	}
	/////done reading BLDGSPEC.TXT*********************************

	buf = bitmaph->getTextFile("DWELLING.TXT");
	andame = buf.size(), it=0;
	for(int f=0;f<F_NUMBER;f++)
	{
		for(int b=0;b<14;b++)
		{
			buildings[f][30+b].first = readTo(buf,it,'\t');
			buildings[f][30+b].second = readTo(buf,it,'\n');
		}
	}

	buf = bitmaph->getTextFile("TCOMMAND.TXT");
	itr=0;
	while(itr<buf.length()-1)
	{
		std::string tmp;
		loadToIt(tmp, buf, itr, 3);
		tcommands.push_back(tmp);
	}

	buf = bitmaph->getTextFile("HALLINFO.TXT");
	itr=0;
	while(itr<buf.length()-1)
	{
		std::string tmp;
		loadToIt(tmp, buf, itr, 3);
		hcommands.push_back(tmp);
	}

	std::istringstream ins, names;
	ins.str(bitmaph->getTextFile("TOWNTYPE.TXT"));
	names.str(bitmaph->getTextFile("TOWNNAME.TXT"));
	int si=0;
	char bufname[75];
	while (!ins.eof())
	{
		ins.getline(bufname,50);
		townTypes.push_back(std::string(bufname).substr(0,strlen(bufname)-1));
		townNames.resize(si+1);

		for (int i=0; i<NAMES_PER_TOWN; i++)
		{
			names.getline(bufname,50);
			townNames[si].push_back(std::string(bufname).substr(0,strlen(bufname)-1));
		}
		si++;
	}
}
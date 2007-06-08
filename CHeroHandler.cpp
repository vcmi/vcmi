#include <fstream>
#include "CHeroHandler.h"

void CHeroHandler::loadHeroes()
{
	std::ifstream inp("HOTRAITS.TXT", std::ios::in);
	std::string dump;
	for(int i=0; i<25; ++i)
	{
		inp>>dump;
	}
	inp.ignore();
	while(!inp.eof())
	{
		CHero nher;
		std::string base;
		char * tab = new char[500];
		int iit = 0;
		inp.getline(tab, 500);
		base = std::string(tab);
		if(base.size()<2) //ended, but some rubbish could still stay end we have something useless
		{
			return;
		}
		while(base[iit]!='\t')
		{
			++iit;
		}
		nher.name = base.substr(0, iit);
		++iit;
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher.low1stack = atoi(base.substr(iit, i).c_str());
				iit=i+1;
				break;
			}
		}
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher.high1stack = atoi(base.substr(iit, i).c_str());
				iit=i+1;
				break;
			}
		}
		int ipom=iit;
		while(base[ipom]!='\t')
		{
			++ipom;
		}
		nher.refType1stack = base.substr(iit, ipom-iit);
		iit=ipom+1;
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher.low2stack = atoi(base.substr(iit, i-iit).c_str());
				iit=i+1;
				break;
			}
		}
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher.high2stack = atoi(base.substr(iit, i-iit).c_str());
				iit=i+1;
				break;
			}
		}
		ipom=iit;
		while(base[ipom]!='\t')
		{
			++ipom;
		}
		nher.refType2stack = base.substr(iit, ipom-iit);
		iit=ipom+1;
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher.low3stack = atoi(base.substr(iit, i-iit).c_str());
				iit=i+1;
				break;
			}
		}
		for(int i=iit; i<iit+100; ++i)
		{
			if(base[i]==(char)(10) || base[i]==(char)(9))
			{
				nher.high3stack = atoi(base.substr(iit, i-iit).c_str());
				iit=i+1;
				break;
			}
		}
		nher.refType3stack = base.substr(iit, base.size()-iit);
		heroes.push_back(nher);
		delete[500] tab;
	}
}
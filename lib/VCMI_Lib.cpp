#define VCMI_DLL
#include "../global.h"
#include "VCMI_Lib.h"
#include "../hch/CArtHandler.h"
#include "../hch/CCreatureHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#include "../hch/CHeroHandler.h"
#include "../hch/CObjectHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CBuildingHandler.h"
class CLodHandler;
LibClasses * VLC = NULL;
CLodHandler * bitmaph=NULL;

DLL_EXPORT void initDLL(CLodHandler *b)
{
	timeHandler pomtime;
	bitmaph=b;
	VLC = new LibClasses;

	CHeroHandler * heroh = new CHeroHandler;
	heroh->loadHeroes();
	heroh->loadPortraits();
	VLC->heroh = heroh;
	THC std::cout<<"\tHero handler: "<<pomtime.getDif()<<std::endl;

	CArtHandler * arth = new CArtHandler;
	arth->loadArtifacts();
	VLC->arth = arth;
	THC std::cout<<"\tArtifact handler: "<<pomtime.getDif()<<std::endl;

	CCreatureHandler * creh = new CCreatureHandler();
	creh->loadCreatures();
	VLC->creh = creh;
	THC std::cout<<"\tCreature handler: "<<pomtime.getDif()<<std::endl;

	VLC->townh = new CTownHandler;
	VLC->townh->loadNames();
	THC std::cout<<"\tTown handler: "<<pomtime.getDif()<<std::endl;

	CObjectHandler * objh = new CObjectHandler;
	objh->loadObjects();
	VLC->objh = objh;
	THC std::cout<<"\tObject handler: "<<pomtime.getDif()<<std::endl;

	VLC->dobjinfo = new CDefObjInfoHandler;
	VLC->dobjinfo->load();
	THC std::cout<<"\tDef information handler: "<<pomtime.getDif()<<std::endl;

	VLC->buildh = new CBuildingHandler;
	VLC->buildh->loadBuildings();
	THC std::cout<<"\tBuilding handler: "<<pomtime.getDif()<<std::endl;
}

DLL_EXPORT void loadToIt(std::string &dest, std::string &src, int &iter, int mode)
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


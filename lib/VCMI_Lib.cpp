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
#include "../hch/CSpellHandler.h"
class CLodHandler;
LibClasses * VLC = NULL;
CLodHandler * bitmaph=NULL;
CLogger<0> log0;
CLogger<1> log1;
CLogger<2> log2;
CLogger<3> log3;
CLogger<4> log4;
CLogger<5> log5;
CConsoleHandler *console = NULL;
std::ostream *logfile = NULL;
DLL_EXPORT void initDLL(CLodHandler *b, CConsoleHandler *Console, std::ostream *Logfile)
{
	console = Console;
	logfile = Logfile;
	timeHandler pomtime;
	bitmaph=b;
	VLC = new LibClasses;

	CHeroHandler * heroh = new CHeroHandler;
	heroh->loadHeroes();
	heroh->loadPortraits();
	VLC->heroh = heroh;
	log0 <<"\tHero handler: "<<pomtime.getDif()<<std::endl;

	CArtHandler * arth = new CArtHandler;
	arth->loadArtifacts();
	VLC->arth = arth;
	log0<<"\tArtifact handler: "<<pomtime.getDif()<<std::endl;

	CCreatureHandler * creh = new CCreatureHandler();
	creh->loadCreatures();
	VLC->creh = creh;
	log0<<"\tCreature handler: "<<pomtime.getDif()<<std::endl;

	VLC->townh = new CTownHandler;
	VLC->townh->loadNames();
	log0<<"\tTown handler: "<<pomtime.getDif()<<std::endl;

	CObjectHandler * objh = new CObjectHandler;
	objh->loadObjects();
	VLC->objh = objh;
	log0<<"\tObject handler: "<<pomtime.getDif()<<std::endl;

	VLC->dobjinfo = new CDefObjInfoHandler;
	VLC->dobjinfo->load();
	log0<<"\tDef information handler: "<<pomtime.getDif()<<std::endl;

	VLC->buildh = new CBuildingHandler;
	VLC->buildh->loadBuildings();
	log0<<"\tBuilding handler: "<<pomtime.getDif()<<std::endl;

	CSpellHandler * spellh = new CSpellHandler;
	spellh->loadSpells();
	VLC->spellh = spellh;		
	log0<<"\tSpell handler: "<<pomtime.getDif()<<std::endl;
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


DLL_EXPORT void loadToIt(si32 &dest, std::string &src, int &iter, int mode)
{
	std::string pom;
	loadToIt(pom,src,iter,mode);
	dest = atol(pom.c_str());
}
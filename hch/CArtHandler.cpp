#include "../stdafx.h"
#include "CArtHandler.h"
#include "../CGameInfo.h"
#include "CGeneralTextHandler.h"
#include "CLodHandler.h"

void CArtHandler::loadArtifacts()
{
	artDefs = CGI->spriteh->giveDef("ARTIFACT.DEF");

	std::string buf = CGameInfo::mainObj->bitmaph->getTextFile("ARTRAITS.TXT");
	int it=0;
	std::string dump;
	for(int i=0; i<2; ++i)
	{
		CGeneralTextHandler::loadToIt(dump,buf,it,3);
	}
	for (int i=0; i<ARTIFACTS_QUANTITY; i++)
	{
		CArtifact nart;
		std::string pom;
		CGeneralTextHandler::loadToIt(nart.name,buf,it,4);
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		nart.price=atoi(pom.c_str());

		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.spellBook=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.warMachine4=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.warMachine3=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.warMachine2=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.warMachine1=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.misc5=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.misc4=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.misc3=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.misc2=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.misc1=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.feet=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.lRing=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.rRing=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.torso=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.lHand=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.rHand=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.neck=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.shoulders=true;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom.length())
			nart.head=true;

		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		switch (pom[0])
		{
		case 'S':
			nart.aClass=SartClass;
			break;
		case 'T':
			nart.aClass=TartClass;
			break;
		case 'N':
			nart.aClass=NartClass;
			break;
		case 'J':
			nart.aClass=JartClass;
			break;
		case 'R':
			nart.aClass=RartClass;
			break;
		}
		CGeneralTextHandler::loadToIt(nart.description,buf,it,3);
		nart.id=artifacts.size();

		artifacts.push_back(nart);
	}
}

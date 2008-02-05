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
		if (pom[0]=='x')
			nart.spellBook=true;
		else
			nart.spellBook = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.warMachine4=true;
		else
			nart.warMachine4 = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.warMachine3=true;
		else
			nart.warMachine3 = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.warMachine2=true;
		else
			nart.warMachine2 = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.warMachine1=true;
		else
			nart.warMachine1 = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.misc5=true;
		else
			nart.misc5 = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.misc4=true;
		else
			nart.misc4 = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.misc3=true;
		else
			nart.misc3 = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.misc2=true;
		else
			nart.misc2 = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.misc1=true;
		else
			nart.misc1 = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.feet=true;
		else
			nart.feet = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.lRing=true;
		else
			nart.lRing = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.rRing=true;
		else
			nart.rRing = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.torso=true;
		else
			nart.torso = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.lHand=true;
		else
			nart.lHand = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.rHand=true;
		else
			nart.rHand = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.neck=true;
		else
			nart.neck = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.shoulders=true;
		else
			nart.shoulders = false;
		CGeneralTextHandler::loadToIt(pom,buf,it,4);
		if (pom[0]=='x')
			nart.head=true;
		else
			nart.head = false;

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
	for(int i=0;i<144;i++) //do 144, bo nie chcemy bzdurek
	{
		switch (artifacts[i].aClass)
		{
		case TartClass:
			treasures.push_back(&(artifacts[i]));
			break;
		case NartClass:
			minors.push_back(&(artifacts[i]));
			break;
		case JartClass:
			majors.push_back(&(artifacts[i]));
			break;
		case RartClass:
			relics.push_back(&(artifacts[i]));
			break;
		}
	}
}

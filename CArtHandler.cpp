#include "CArtHandler.h"
#include <fstream>
#include <iostream>

void CArtHandler::loadArtifacts()
{
	std::ifstream inp("ARTRAITS.TXT", std::ios::in);
	std::string dump;
	for(int i=0; i<44; ++i)
	{
		inp>>dump;
	}
	inp.ignore();
	int numberlet = 0; //numer of artifact
	while(!inp.eof())
	{
		CArtifact nart;
		nart.number=numberlet++;
		char * read = new char[10000]; //here we'll have currently read character
		inp.getline(read, 10000);
		int eol=0; //end of looking
		std::string ss = std::string(read);
		for(int i=0; i<200; ++i)
		{
			if(ss[i]=='\t')
			{
				nart.name = ss.substr(0, i);
				eol=i+1;
				break;
			}
		}
		for(int i=eol; i<eol+200; ++i)
		{
			if(ss[i]=='\t')
			{
				nart.price = atoi(ss.substr(eol, i).c_str());
				eol=i+1;
				break;
			}
		}
		if(ss[eol]=='x')
			nart.spellBook = true;
		else
			nart.spellBook = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.warMachine4 = true;
		else
			nart.warMachine4 = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.warMachine3 = true;
		else
			nart.warMachine3 = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.warMachine2 = true;
		else
			nart.warMachine2 = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.warMachine1 = true;
		else
			nart.warMachine1 = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.misc5 = true;
		else
			nart.misc5 = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.misc4 = true;
		else
			nart.misc4 = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.misc3 = true;
		else
			nart.misc3 = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.misc2 = true;
		else
			nart.misc2 = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.misc1 = true;
		else
			nart.misc1 = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.feet = true;
		else
			nart.feet = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.lRing = true;
		else
			nart.lRing = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.rRing = true;
		else
			nart.rRing = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.torso = true;
		else
			nart.torso = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.lHand = true;
		else
			nart.lHand = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.rHand = true;
		else
			nart.rHand = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.neck = true;
		else
			nart.neck = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.shoulders = true;
		else
			nart.shoulders = false;
		eol+=2;
		if(ss[eol]=='x')
			nart.head = true;
		else
			nart.head = false;
		eol+=2;
		switch(ss[eol])
		{
		case 'J':
			nart.aClass = EartClass::JartClass;
			break;
			
		case 'N':
			nart.aClass = EartClass::NartClass;
			break;
			
		case 'R':
			nart.aClass = EartClass::RartClass;
			break;
			
		case 'S':
			nart.aClass = EartClass::SartClass;
			break;
			
		case 'T':
			nart.aClass = EartClass::TartClass;
			break;
		}
		eol+=2;
		nart.description = ss.substr(eol, ss.size());
		inp.getline(read, 10000);
		inp.getline(read, 10000);
		ss = std::string(read);
		nart.desc2 = ss;
	}
}
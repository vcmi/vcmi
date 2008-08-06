//#define VCMI_DLL
//#include "../stdafx.h"
//#include "CAmbarCendamo.h"
//#include "CObjectHandler.h"
//#include "CDefObjInfoHandler.h"
//#include <set>
//#include <sstream>
//#include <fstream>
//#include "../lib/VCMI_Lib.h"
//std::string nameFromType (EterrainType typ);
//int readInt(unsigned char * bufor, int bytCon)
//{
//	int ret=0;
//	int amp=1;
//	for (int i=0; i<bytCon; i++)
//	{
//		ret+=bufor[i]*amp;
//		amp*=256;
//	}
//	return ret;
//}
//CAmbarCendamo::CAmbarCendamo (unsigned char * data)
////:map(data)
//{}
////CAmbarCendamo::CAmbarCendamo (const char * tie)
////{
////	std::ifstream * is = new std::ifstream();
////	is -> open(tie,std::ios::binary);
////	is->seekg(0,std::ios::end); // na koniec
////	int andame = is->tellg();  // read length
////	is->seekg(0,std::ios::beg); // wracamy na poczatek
////	bufor = new unsigned char[andame]; // allocate memory 
////	is->read((char*)bufor, andame); // read map file to buffer
////	is->close();
////	delete is;
////}
//CAmbarCendamo::~CAmbarCendamo () 
//{// free memory
//	for (int ii=0;ii<map.width;ii++)
//		delete map.terrain[ii] ; 
//	delete map.terrain;
//	delete[] bufor;
//}

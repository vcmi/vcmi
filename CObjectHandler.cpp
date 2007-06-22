#include "stdafx.h"
#include "CObjectHandler.h"

void CObjectHandler::loadObjects()
{
	std::ifstream inp("H3bitmap.lod\\OBJNAMES.TXT", std::ios::in | std::ios::binary);
	inp.seekg(0,std::ios::end); // na koniec
	int andame = inp.tellg();  // read length
	inp.seekg(0,std::ios::beg); // wracamy na poczatek
	char * bufor = new char[andame]; // allocate memory 
	inp.read((char*)bufor, andame); // read map file to buffer
	inp.close();
	std::string buf = std::string(bufor);
	delete [andame] bufor;
	int i = 0; //buf iterator
	while(!inp.eof())
	{
		if(objects.size()>200 && buf.substr(i, buf.size()-i).find('\r')==std::string::npos)
			break;
		CObject nobj;
		int befi=i;
		for(i; i<andame; ++i)
		{
			if(buf[i]=='\r')
				break;
		}
		nobj.name = buf.substr(befi, i-befi);
		i+=2;
		objects.push_back(nobj);
	}
}
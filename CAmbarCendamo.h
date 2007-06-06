#ifndef AMBARCENDD
#define AMBARCENDD
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "global.h"
#include "SDL.h"
#include "map.h"
#include "CSemiDefHandler.h"

class CAmbarCendamo 
{
public:
/////////////////zmienne skladowe
	Mapa map;
	std::ifstream * is; // stream used to read map file
	int andame; // length of map file
	unsigned char * bufor; // here we store map bytecode
	std::vector<CSemiDefHandler*> defs;
/////////////////funkcje skladowe
	CAmbarCendamo (const char * tie); // c-tor; tie is the path of the map file
	~CAmbarCendamo (); // d-tor
	int readNormalNr (int pos, int bytCon=4); //read number from bytCon bytes starting from pos position in buffer 
	void teceDef (); // create files with info about defs
	void deh3m(); // decode file, results are stored in map
	void loadDefs();
};
#endif //AMBARCENDD
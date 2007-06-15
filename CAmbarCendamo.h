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
#include "CCreatureHandler.h"

enum EDefType {TOWN_DEF, HERO_DEF, CREATURES_DEF, SEERHUT_DEF, RESOURCE_DEF, TERRAINOBJ_DEF, EVENTOBJ_DEF, SIGN_DEF, GARRISON_DEF, ARTIFACT_DEF, WITCHHUT_DEF, SCHOLAR_DEF};

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
	int readNormalNr (int pos, int bytCon=4, bool cyclic = false); //read number from bytCon bytes starting from pos position in buffer ; if cyclic is true, number is treated as it were signed number with bytCon bytes
	void teceDef (); // create files with info about defs
	void deh3m(); // decode file, results are stored in map
	void loadDefs();
	EDefType getDefType(DefInfo& a); //returns type of object in def
	CCreatureSet readCreatureSet(int pos, int number = 7); //reads creature set in most recently encountered format; reades number units (default is 7)
};
#endif //AMBARCENDD
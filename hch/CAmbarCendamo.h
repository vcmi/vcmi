#ifndef CAMBARCENDAMO_H
#define CAMBARCENDAMO_H
#include <iostream>
#include <string>
#include <vector>
#include "../global.h"
#include "SDL.h"
#include "../map.h"
#include "CDefHandler.h"
#include "CCreatureHandler.h"

enum EDefType {TOWN_DEF, HERO_DEF, CREATURES_DEF, SEERHUT_DEF, RESOURCE_DEF, TERRAINOBJ_DEF, EVENTOBJ_DEF, SIGN_DEF, GARRISON_DEF, ARTIFACT_DEF, WITCHHUT_DEF, SCHOLAR_DEF, PLAYERONLY_DEF, SHRINE_DEF, SPELLSCROLL_DEF, PANDORA_DEF, GRAIL_DEF, CREGEN_DEF, CREGEN2_DEF, CREGEN3_DEF, BORDERGUARD_DEF, HEROPLACEHOLDER_DEF};

class CAmbarCendamo 
{
public:
/////////////////member variables
	Mapa map;

	//map file 
	unsigned char * bufor; // here we store map bytecode
	int i; //our pos in the file

	CAmbarCendamo (const char * tie); // c-tor; tie is the path of the map file
	CAmbarCendamo (unsigned char * map); // c-tor; map is pointer to array containing map; it is not copied, so don't delete
	~CAmbarCendamo (); // d-tor
	int readNormalNr (int pos, int bytCon=4, bool cyclic = false); //read number from bytCon bytes starting from pos position in buffer ; if cyclic is true, number is treated as it were signed number with bytCon bytes
	void deh3m(); // decode file, results are stored in map
	EDefType getDefType(CGDefInfo * a); //returns type of object in def
	CCreatureSet readCreatureSet(int number = 7); //reads creature set in most recently encountered format; reades number units (default is 7)
	char readChar();
	std::string readString();
};
#endif //CAMBARCENDAMO_H
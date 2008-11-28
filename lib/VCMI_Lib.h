#ifndef VCMI_LIB_H
#define VCMI_LIB_H
#include "../global.h"

class CLodHandler;
class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
//class CAbilityHandler;
class CSpellHandler;
//class CPreGameTextHandler;
class CBuildingHandler;
class CObjectHandler;
//class CMusicHandler;
//class CSemiLodHandler;
class CDefObjInfoHandler;
class CTownHandler;
class CGeneralTextHandler;
//class CConsoleHandler;
//class CPathfinder;
//class CGameState;

class LibClasses
{
public:
	CArtHandler * arth;
	CHeroHandler * heroh;
	CCreatureHandler * creh;
	CSpellHandler * spellh;
	CBuildingHandler * buildh;
	CObjectHandler * objh;
	CDefObjInfoHandler * dobjinfo;
	CTownHandler * townh;
	CGeneralTextHandler * generaltexth;
	//CPathfinder * pathf;
};

extern DLL_EXPORT LibClasses * VLC;
extern CLodHandler * bitmaph;

DLL_EXPORT void loadToIt(std::string &dest, std::string &src, int &iter, int mode);
DLL_EXPORT void loadToIt(si32 &dest, std::string &src, int &iter, int mode);
DLL_EXPORT void initDLL(CLodHandler *b, CConsoleHandler *Console, std::ostream *Logfile);

#endif //VCMI_LIB_H

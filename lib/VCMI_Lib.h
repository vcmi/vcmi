#ifndef __VCMI_LIB_H__
#define __VCMI_LIB_H__
#include "../global.h"
#ifndef _MSC_VER
#include "../hch/CArtHandler.h"
#include "../hch/CGeneralTextHandler.h"
#endif

/*
 * VCMI_Lib.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CLodHandler;
class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
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

class DLL_EXPORT LibClasses
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

	void init(); //uses standard config file
	void clear(); //deletes all handlers and its data

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroh & arth & creh & townh & objh & dobjinfo & buildh & spellh;
		if(!h.saving)
		{
			generaltexth = new CGeneralTextHandler;
			generaltexth->load();
			arth->loadArtifacts(true);
		}
	}
};

extern DLL_EXPORT LibClasses * VLC;
extern CLodHandler * bitmaph;

DLL_EXPORT void loadToIt(std::string &dest, std::string &src, int &iter, int mode);
DLL_EXPORT void loadToIt(si32 &dest, std::string &src, int &iter, int mode);
DLL_EXPORT void initDLL(CLodHandler *b, CConsoleHandler *Console, std::ostream *Logfile);


#endif // __VCMI_LIB_H__

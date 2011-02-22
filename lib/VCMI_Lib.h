#ifndef __VCMI_LIB_H__
#define __VCMI_LIB_H__
#include "../global.h"

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
class CBuildingHandler;
class CObjectHandler;
class CDefObjInfoHandler;
class CTownHandler;
class CGeneralTextHandler;

/// Loads and constructs several handlers
class DLL_EXPORT LibClasses
{
public:
	bool IS_AI_ENABLED; //VLC is teh only object visible from both CMT and GeniusAI
	CArtHandler * arth;
	CHeroHandler * heroh;
	CCreatureHandler * creh;
	CSpellHandler * spellh;
	CBuildingHandler * buildh;
	CObjectHandler * objh;
	CDefObjInfoHandler * dobjinfo;
	CTownHandler * townh;
	CGeneralTextHandler * generaltexth;

	LibClasses(); //c-tor, loads .lods and NULLs handlers
	void init(); //uses standard config file
	void clear(); //deletes all handlers and its data
	void makeNull(); //sets all handler (except of lodhs) pointers to null
	

	void callWhenDeserializing(); //should be called only by serialize !!!
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroh & arth & creh & townh & objh & dobjinfo & buildh & spellh & IS_AI_ENABLED;;
		if(!h.saving)
		{
			callWhenDeserializing();
		}
	}
};

extern DLL_EXPORT LibClasses * VLC;
extern DLL_EXPORT CLodHandler * bitmaph, *spriteh;

DLL_EXPORT void loadToIt(std::string &dest, const std::string &src, int &iter, int mode);
DLL_EXPORT void loadToIt(si32 &dest, const std::string &src, int &iter, int mode);
DLL_EXPORT void initDLL(CConsoleHandler *Console, std::ostream *Logfile);


#endif // __VCMI_LIB_H__

#pragma once

/*
 * VCMI_Lib.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
class CSpellHandler;
class CBuildingHandler;
class CObjectHandler;
class CDefObjInfoHandler;
class CTownHandler;
class CGeneralTextHandler;
class CModHandler;

/// Loads and constructs several handlers
class DLL_LINKAGE LibClasses
{
	void callWhenDeserializing(); //should be called only by serialize !!!
	void makeNull(); //sets all handler pointers to null
public:
	bool IS_AI_ENABLED; //VLC is the only object visible from both CMT and GeniusAI
	CArtHandler * arth;
	CHeroHandler * heroh;
	CCreatureHandler * creh;
	CSpellHandler * spellh;
	CObjectHandler * objh;
	CDefObjInfoHandler * dobjinfo;
	CTownHandler * townh;
	CGeneralTextHandler * generaltexth;
	CModHandler * modh;

	LibClasses(); //c-tor, loads .lods and NULLs handlers
	~LibClasses();
	void init(); //uses standard config file
	void clear(); //deletes all handlers and its data


	void loadFilesystem();// basic initialization. should be called before init()


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroh & arth & creh & townh & objh & dobjinfo & spellh & modh & IS_AI_ENABLED;;
		if(!h.saving)
		{
			callWhenDeserializing();
		}
	}
};

extern DLL_LINKAGE LibClasses * VLC;

DLL_LINKAGE void preinitDLL(CConsoleHandler *Console, std::ostream *Logfile);
DLL_LINKAGE void loadDLLClasses();


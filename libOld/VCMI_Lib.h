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
class CObjectClassesHandler;
class CTownHandler;
class CGeneralTextHandler;
class CModHandler;
class IBonusTypeHandler;
class CBonusTypeHandler;
class CTerrainViewPatternConfig;
class CRmgTemplateStorage;

/// Loads and constructs several handlers
class DLL_LINKAGE LibClasses
{
	CBonusTypeHandler * bth;

	void callWhenDeserializing(); //should be called only by serialize !!!
	void makeNull(); //sets all handler pointers to null
public:
	bool IS_AI_ENABLED; //VLC is the only object visible from both CMT and GeniusAI
	
	const IBonusTypeHandler * getBth() const;

	CArtHandler * arth;
	CHeroHandler * heroh;
	CCreatureHandler * creh;
	CSpellHandler * spellh;
	CObjectHandler * objh;
	CObjectClassesHandler * objtypeh;
	CTownHandler * townh;
	CGeneralTextHandler * generaltexth;
	CModHandler * modh;
	CTerrainViewPatternConfig * terviewh;
	CRmgTemplateStorage * tplh;

	LibClasses(); //c-tor, loads .lods and NULLs handlers
	~LibClasses();
	void init(); //uses standard config file
	void clear(); //deletes all handlers and its data


	void loadFilesystem();// basic initialization. should be called before init()


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroh & arth & creh & townh & objh & objtypeh & spellh & modh & IS_AI_ENABLED;
		h & bth;
		if(!h.saving)
		{
			callWhenDeserializing();
		}
	}
};

extern DLL_LINKAGE LibClasses * VLC;

DLL_LINKAGE void preinitDLL(CConsoleHandler *Console);
DLL_LINKAGE void loadDLLClasses();


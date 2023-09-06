/*
 * VCMI_Lib.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Services.h>

VCMI_LIB_NAMESPACE_BEGIN

class CConsoleHandler;
class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
class CSpellHandler;
class CSkillHandler;
class CBuildingHandler;
class CObjectHandler;
class CObjectClassesHandler;
class CTownHandler;
class CGeneralTextHandler;
class CModHandler;
class CContentHandler;
class BattleFieldHandler;
class IBonusTypeHandler;
class CBonusTypeHandler;
class TerrainTypeHandler;
class RoadTypeHandler;
class RiverTypeHandler;
class ObstacleHandler;
class CTerrainViewPatternConfig;
class CRmgTemplateStorage;
class IHandlerBase;
class IGameSettings;
class GameSettings;
class CIdentifierStorage;

#if SCRIPTING_ENABLED
namespace scripting
{
	class ScriptHandler;
}
#endif


/// Loads and constructs several handlers
class DLL_LINKAGE LibClasses : public Services
{
	CBonusTypeHandler * bth;

	void callWhenDeserializing(); //should be called only by serialize !!!
	void makeNull(); //sets all handler pointers to null
	std::shared_ptr<CContentHandler> getContent() const;
	void setContent(std::shared_ptr<CContentHandler> content);

public:
	bool IS_AI_ENABLED = false; //unused?

	const ArtifactService * artifacts() const override;
	const CreatureService * creatures() const override;
	const FactionService * factions() const override;
	const HeroClassService * heroClasses() const override;
	const HeroTypeService * heroTypes() const override;
#if SCRIPTING_ENABLED
	const scripting::Service * scripts() const override;
#endif
	const spells::Service * spells() const override;
	const SkillService * skills() const override;
	const BattleFieldService * battlefields() const override;
	const ObstacleService * obstacles() const override;
	const IGameSettings * settings() const override;

	void updateEntity(Metatype metatype, int32_t index, const JsonNode & data) override;

	const spells::effects::Registry * spellEffects() const override;
	spells::effects::Registry * spellEffects() override;

	const IBonusTypeHandler * getBth() const; //deprecated
	const CIdentifierStorage * identifiers() const;

	CArtHandler * arth;
	CHeroHandler * heroh;
	CCreatureHandler * creh;
	CSpellHandler * spellh;
	CSkillHandler * skillh;
	CObjectHandler * objh;
	CObjectClassesHandler * objtypeh;
	CTownHandler * townh;
	CGeneralTextHandler * generaltexth;
	CModHandler * modh;

	TerrainTypeHandler * terrainTypeHandler;
	RoadTypeHandler * roadTypeHandler;
	RiverTypeHandler * riverTypeHandler;
	CIdentifierStorage * identifiersHandler;

	CTerrainViewPatternConfig * terviewh;
	CRmgTemplateStorage * tplh;
	BattleFieldHandler * battlefieldsHandler;
	ObstacleHandler * obstacleHandler;
	GameSettings * settingsHandler;
#if SCRIPTING_ENABLED
	scripting::ScriptHandler * scriptHandler;
#endif

	LibClasses(); //c-tor, loads .lods and NULLs handlers
	~LibClasses();
	void init(bool onlyEssential); //uses standard config file
	void clear(); //deletes all handlers and its data

	// basic initialization. should be called before init(). Can also extract original H3 archives
	void loadFilesystem(bool extractArchives);
	void loadModFilesystem(bool onlyEssential);

#if SCRIPTING_ENABLED
	void scriptsLoaded();
#endif

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & identifiersHandler; // must be first - identifiers registry is used for handlers loading
#if SCRIPTING_ENABLED
		h & scriptHandler;//must be first (or second after identifiers), it can modify factories other handlers depends on
		if(!h.saving)
		{
			scriptsLoaded();
		}
#endif

		h & settingsHandler;
		h & heroh;
		h & arth;
		h & creh;
		h & townh;
		h & objh;
		h & objtypeh;
		h & spellh;
		h & skillh;
		h & battlefieldsHandler;
		h & obstacleHandler;
		h & roadTypeHandler;
		h & riverTypeHandler;
		h & terrainTypeHandler;

		if(!h.saving)
		{
			//modh will be changed and modh->content will be empty after deserialization
			auto content = getContent();
			h & modh;
			setContent(content);
		}
		else
			h & modh;

		h & IS_AI_ENABLED;
		h & bth;

		if(!h.saving)
		{
			callWhenDeserializing();
		}
	}
};

extern DLL_LINKAGE LibClasses * VLC;

DLL_LINKAGE void preinitDLL(CConsoleHandler * Console, bool onlyEssential = false, bool extractArchives = false);
DLL_LINKAGE void loadDLLClasses(bool onlyEssential = false);


VCMI_LIB_NAMESPACE_END

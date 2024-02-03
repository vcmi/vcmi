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
class CHeroClassHandler;
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
class DLL_LINKAGE LibClasses final : public Services
{
	std::shared_ptr<CBonusTypeHandler> bth;

	std::shared_ptr<CContentHandler> getContent() const;
	void setContent(std::shared_ptr<CContentHandler> content);

public:
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

	std::shared_ptr<CArtHandler> arth;
	std::shared_ptr<CHeroHandler> heroh;
	std::shared_ptr<CHeroClassHandler> heroclassesh;
	std::shared_ptr<CCreatureHandler> creh;
	std::shared_ptr<CSpellHandler> spellh;
	std::shared_ptr<CSkillHandler> skillh;
	std::shared_ptr<CObjectHandler> objh;
	std::shared_ptr<CObjectClassesHandler> objtypeh;
	std::shared_ptr<CTownHandler> townh;
	std::shared_ptr<CGeneralTextHandler> generaltexth;
	std::shared_ptr<CModHandler> modh;
	std::shared_ptr<TerrainTypeHandler> terrainTypeHandler;
	std::shared_ptr<RoadTypeHandler> roadTypeHandler;
	std::shared_ptr<RiverTypeHandler> riverTypeHandler;
	std::shared_ptr<CIdentifierStorage> identifiersHandler;
	std::shared_ptr<CTerrainViewPatternConfig> terviewh;
	std::shared_ptr<CRmgTemplateStorage> tplh;
	std::shared_ptr<BattleFieldHandler> battlefieldsHandler;
	std::shared_ptr<ObstacleHandler> obstacleHandler;
	std::shared_ptr<GameSettings> settingsHandler;

#if SCRIPTING_ENABLED
	std::shared_ptr<scripting::ScriptHandler> scriptHandler;
#endif

	LibClasses(); //c-tor, loads .lods and NULLs handlers
	~LibClasses();
	void init(bool onlyEssential); //uses standard config file

	// basic initialization. should be called before init(). Can also extract original H3 archives
	void loadFilesystem(bool extractArchives);
	void loadModFilesystem();

#if SCRIPTING_ENABLED
	void scriptsLoaded();
#endif
};

extern DLL_LINKAGE LibClasses * VLC;

DLL_LINKAGE void preinitDLL(CConsoleHandler * Console, bool extractArchives);
DLL_LINKAGE void loadDLLClasses(bool onlyEssential = false);


VCMI_LIB_NAMESPACE_END

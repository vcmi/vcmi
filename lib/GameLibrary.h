/*
 * GameLibrary.h, part of VCMI engine
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
class CObjectClassesHandler;
class ObstacleSetHandler;
class CTownHandler;
class CGeneralTextHandler;
class CModHandler;
class CContentHandler;
class BattleFieldHandler;
class IBonusTypeHandler;
class CBonusTypeHandler;
class TerrainTypeHandler;
class ResourceTypeHandler;
class RoadTypeHandler;
class RiverTypeHandler;
class ObstacleHandler;
class CTerrainViewPatternConfig;
class CRmgTemplateStorage;
class IHandlerBase;
class IGameSettings;
class GameSettings;
class CIdentifierStorage;
class SpellSchoolHandler;
class MapFormatSettings;
class CampaignRegionsHandler;

#if SCRIPTING_ENABLED
namespace scripting
{
	class ScriptHandler;
}
#endif

/// Loads and constructs several handlers
class DLL_LINKAGE GameLibrary final : public Services
{
public:
	const ArtifactService * artifacts() const override;
	const CreatureService * creatures() const override;
	const FactionService * factions() const override;
	const HeroClassService * heroClasses() const override;
	const HeroTypeService * heroTypes() const override;
	const ResourceTypeService * resources() const override;
#if SCRIPTING_ENABLED
	const scripting::Service * scripts() const override;
#endif
	const spells::Service * spells() const override;
	const SkillService * skills() const override;
	const BattleFieldService * battlefields() const override;
	const ObstacleService * obstacles() const override;
	const IGameSettings * engineSettings() const override;

	const spells::effects::Registry * spellEffects() const override;
	spells::effects::Registry * spellEffects() override;

	const IBonusTypeHandler * getBth() const; //deprecated
	const CIdentifierStorage * identifiers() const;

	std::unique_ptr<CArtHandler> arth;
	std::unique_ptr<CBonusTypeHandler> bth;
	std::unique_ptr<CHeroHandler> heroh;
	std::unique_ptr<CHeroClassHandler> heroclassesh;
	std::unique_ptr<CCreatureHandler> creh;
	std::unique_ptr<CSpellHandler> spellh;
	std::unique_ptr<SpellSchoolHandler> spellSchoolHandler;
	std::unique_ptr<CSkillHandler> skillh;
	std::unique_ptr<CObjectClassesHandler> objtypeh;
	std::unique_ptr<CTownHandler> townh;
	std::unique_ptr<CGeneralTextHandler> generaltexth;
	std::unique_ptr<CModHandler> modh;
	std::unique_ptr<TerrainTypeHandler> terrainTypeHandler;
	std::unique_ptr<ResourceTypeHandler> resourceTypeHandler;
	std::unique_ptr<RoadTypeHandler> roadTypeHandler;
	std::unique_ptr<RiverTypeHandler> riverTypeHandler;
	std::unique_ptr<CIdentifierStorage> identifiersHandler;
	std::unique_ptr<CTerrainViewPatternConfig> terviewh;
	std::unique_ptr<CRmgTemplateStorage> tplh;
	std::unique_ptr<BattleFieldHandler> battlefieldsHandler;
	std::unique_ptr<ObstacleHandler> obstacleHandler;
	std::unique_ptr<GameSettings> settingsHandler;
	std::unique_ptr<ObstacleSetHandler> biomeHandler;
	std::unique_ptr<MapFormatSettings> mapFormat;
	std::unique_ptr<CampaignRegionsHandler> campaignRegions;

#if SCRIPTING_ENABLED
	std::unique_ptr<scripting::ScriptHandler> scriptHandler;
#endif

	GameLibrary();
	~GameLibrary();

	/// initializes settings and filesystem
	void initializeFilesystem(bool extractArchives);

	/// Loads all game entities
	void initializeLibrary();

private:
	// basic initialization. should be called before init(). Can also extract original H3 archives
	void loadFilesystem(bool extractArchives);
	void loadModFilesystem();

#if SCRIPTING_ENABLED
	void scriptsLoaded();
#endif
};

extern DLL_LINKAGE GameLibrary * LIBRARY;

VCMI_LIB_NAMESPACE_END

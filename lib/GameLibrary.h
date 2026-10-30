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

class CConsoleHandler;
class ScriptHandler;
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
class ITranslator;
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
class LibraryLoadContext;
class SpellSchoolHandler;
class MapFormatSettings;
class CampaignRegionsHandler;
class MapLayerTypeHandler;
class RumorTypeHandler;

/// Owning handler pointer that gives only const access through a const library
template<typename T>
class LibraryHandlerPtr
{
	std::unique_ptr<T> ptr;
public:
	LibraryHandlerPtr() = default;
	LibraryHandlerPtr & operator=(std::unique_ptr<T> && other) { ptr = std::move(other); return *this; }
	T * operator->() { return ptr.get(); }
	const T * operator->() const { return ptr.get(); }
	T & operator*() { return *ptr; }
	const T & operator*() const { return *ptr; }
	T * get() { return ptr.get(); }
	const T * get() const { return ptr.get(); }
};

/// Loads and constructs several handlers
class DLL_LINKAGE GameLibrary final : public Services
{
public:
	enum class GameDataMode
	{
		SOD,
		ROE,
		DEMO_SOD,
		DEMO_ROE
	};

	const ArtifactService * artifacts() const override;
	const CreatureService * creatures() const override;
	const FactionService * factions() const override;
	const HeroClassService * heroClasses() const override;
	const HeroTypeService * heroTypes() const override;
	const ResourceTypeService * resources() const override;
	const scripting::Service * scripts() const override;
	const spells::Service * spells() const override;
	const SkillService * skills() const override;
	const BattleFieldService * battlefields() const override;
	const ObstacleService * obstacles() const override;
	const IGameSettings * engineSettings() const override;
	const spells::SchoolService * spellSchools() const override;
	const ScriptService * scriptTypes() const override;
	/// Resolver over the static text store: mod-defined text that every side shares.
	/// It can not see map or campaign texts - anything a specific player reads must be
	/// rendered with that player's translator instead.
	const ITranslator * staticTexts() const;

	const IBonusTypeHandler * getBth() const;
	const CIdentifierStorage * identifiers() const;

	LibraryHandlerPtr<CArtHandler> arth;
	LibraryHandlerPtr<CBonusTypeHandler> bth;
	LibraryHandlerPtr<CHeroHandler> heroh;
	LibraryHandlerPtr<CHeroClassHandler> heroclassesh;
	LibraryHandlerPtr<CCreatureHandler> creh;
	LibraryHandlerPtr<CSpellHandler> spellh;
	LibraryHandlerPtr<SpellSchoolHandler> spellSchoolHandler;
	LibraryHandlerPtr<ScriptHandler> scriptTypeHandler;
	LibraryHandlerPtr<CSkillHandler> skillh;
	LibraryHandlerPtr<CObjectClassesHandler> objtypeh;
	LibraryHandlerPtr<CTownHandler> townh;
	LibraryHandlerPtr<CGeneralTextHandler> generaltexth;
	LibraryHandlerPtr<CModHandler> modh;
	LibraryHandlerPtr<TerrainTypeHandler> terrainTypeHandler;
	LibraryHandlerPtr<ResourceTypeHandler> resourceTypeHandler;
	LibraryHandlerPtr<RoadTypeHandler> roadTypeHandler;
	LibraryHandlerPtr<RiverTypeHandler> riverTypeHandler;
	LibraryHandlerPtr<CIdentifierStorage> identifiersHandler;
	LibraryHandlerPtr<CTerrainViewPatternConfig> terviewh;
	LibraryHandlerPtr<CRmgTemplateStorage> tplh;
	LibraryHandlerPtr<BattleFieldHandler> battlefieldsHandler;
	LibraryHandlerPtr<ObstacleHandler> obstacleHandler;
	LibraryHandlerPtr<GameSettings> settingsHandler;
	LibraryHandlerPtr<ObstacleSetHandler> biomeHandler;
	LibraryHandlerPtr<MapFormatSettings> mapFormat;
	LibraryHandlerPtr<CampaignRegionsHandler> campaignRegions;
	LibraryHandlerPtr<MapLayerTypeHandler> mapLayerHandler;
	LibraryHandlerPtr<RumorTypeHandler> rumorHandler;
	LibraryHandlerPtr<scripting::Service> scriptHandler;

	GameLibrary();
	~GameLibrary();

	/// initializes settings and filesystem
	void initializeFilesystem(bool extractArchives, bool useTestPreset = false);

	/// Loads all game entities
	void initializeLibrary();

	/// Loads all game entities using provided context, which stays available to the caller after loading
	void initializeLibrary(LibraryLoadContext & context);

	/// Returns current game data mode
	GameDataMode getGameDataMode() const;

	/// Returns true if game is running with RoE data files (either full or demo)
	bool isRoeData() const;

	/// Returns true if game is running with demo data files
	bool isDemoData() const;

	GameDataMode gameDataMode = GameDataMode::SOD;
	// basic initialization. should be called before init(). Can also extract original H3 archives
	void loadFilesystem(bool extractArchives);

	// loads filesystems of all mods
	void loadModFilesystem(bool useTestPreset = false);
};

extern DLL_LINKAGE const GameLibrary * LIBRARY;

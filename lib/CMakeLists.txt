set(lib_SRCS
		StdInc.cpp
		${CMAKE_BINARY_DIR}/Version.cpp

		battle/AccessibilityInfo.cpp
		battle/BattleAction.cpp
		battle/BattleAttackInfo.cpp
		battle/BattleHex.cpp
		battle/BattleInfo.cpp
		battle/BattleProxy.cpp
		battle/CBattleInfoCallback.cpp
		battle/CBattleInfoEssentials.cpp
		battle/CCallbackBase.cpp
		battle/CObstacleInstance.cpp
		battle/CPlayerBattleCallback.cpp
		battle/CUnitState.cpp
		battle/Destination.cpp
		battle/IBattleState.cpp
		battle/ReachabilityInfo.cpp
		battle/SideInBattle.cpp
		battle/SiegeInfo.cpp
		battle/Unit.cpp

		events/ApplyDamage.cpp
		events/GameResumed.cpp
		events/ObjectVisitEnded.cpp
		events/ObjectVisitStarted.cpp
		events/PlayerGotTurn.cpp
		events/TurnStarted.cpp

		filesystem/AdapterLoaders.cpp
		filesystem/CArchiveLoader.cpp
		filesystem/CBinaryReader.cpp
		filesystem/CCompressedStream.cpp
		filesystem/CFileInputStream.cpp
		filesystem/CFilesystemLoader.cpp
		filesystem/CMemoryBuffer.cpp
		filesystem/CMemoryStream.cpp
		filesystem/CZipLoader.cpp
		filesystem/CZipSaver.cpp
		filesystem/FileInfo.cpp
		filesystem/FileStream.cpp
		filesystem/Filesystem.cpp
		filesystem/MinizipExtensions.cpp
		filesystem/ResourceID.cpp

		logging/CBasicLogConfigurator.cpp
		logging/CLogger.cpp

		mapObjects/CArmedInstance.cpp
		mapObjects/CBank.cpp
		mapObjects/CGHeroInstance.cpp
		mapObjects/CGMarket.cpp
		mapObjects/CGPandoraBox.cpp
		mapObjects/CGTownInstance.cpp
		mapObjects/CObjectClassesHandler.cpp
		mapObjects/CObjectHandler.cpp
		mapObjects/CommonConstructors.cpp
		mapObjects/CQuest.cpp
		mapObjects/CRewardableConstructor.cpp
		mapObjects/CRewardableObject.cpp
		mapObjects/JsonRandom.cpp
		mapObjects/MiscObjects.cpp
		mapObjects/ObjectTemplate.cpp

		mapping/CCampaignHandler.cpp
		mapping/CDrawRoadsOperation.cpp
		mapping/CMap.cpp
		mapping/CMapEditManager.cpp
		mapping/CMapInfo.cpp
		mapping/CMapOperation.cpp
		mapping/CMapService.cpp
		mapping/MapEditUtils.cpp
		mapping/MapFormatH3M.cpp
		mapping/MapFormatJson.cpp

		registerTypes/RegisterTypes.cpp
		registerTypes/TypesClientPacks1.cpp
		registerTypes/TypesClientPacks2.cpp
		registerTypes/TypesMapObjects1.cpp
		registerTypes/TypesMapObjects2.cpp
		registerTypes/TypesMapObjects3.cpp
		registerTypes/TypesLobbyPacks.cpp
		registerTypes/TypesServerPacks.cpp

		rmg/RmgArea.cpp
		rmg/RmgObject.cpp
		rmg/RmgPath.cpp
		rmg/CMapGenerator.cpp
		rmg/CMapGenOptions.cpp
		rmg/CRmgTemplate.cpp
		rmg/CRmgTemplateStorage.cpp
		rmg/CZonePlacer.cpp
		rmg/TileInfo.cpp
		rmg/Zone.cpp
		rmg/Functions.cpp
		rmg/ObjectManager.cpp
		rmg/RoadPlacer.cpp
		rmg/TreasurePlacer.cpp
		rmg/RmgMap.cpp
		rmg/ConnectionsPlacer.cpp
		rmg/WaterAdopter.cpp
		rmg/TownPlacer.cpp
		rmg/WaterProxy.cpp
		rmg/WaterRoutes.cpp
		rmg/RockPlacer.cpp
		rmg/ObstaclePlacer.cpp
		rmg/RiverPlacer.cpp
		rmg/TerrainPainter.cpp

		serializer/BinaryDeserializer.cpp
		serializer/BinarySerializer.cpp
		serializer/CLoadIntegrityValidator.cpp
		serializer/CMemorySerializer.cpp
		serializer/Connection.cpp
		serializer/CSerializer.cpp
		serializer/CTypeList.cpp
		serializer/JsonDeserializer.cpp
		serializer/JsonSerializeFormat.cpp
		serializer/JsonSerializer.cpp
		serializer/JsonUpdater.cpp

		spells/AbilityCaster.cpp
		spells/AdventureSpellMechanics.cpp
		spells/BattleSpellMechanics.cpp
		spells/BonusCaster.cpp
		spells/CSpellHandler.cpp
		spells/ISpellMechanics.cpp
		spells/Problem.cpp
		spells/ProxyCaster.cpp
		spells/TargetCondition.cpp
		spells/ViewSpellInt.cpp

		spells/effects/Catapult.cpp
		spells/effects/Clone.cpp
		spells/effects/Damage.cpp
		spells/effects/Dispel.cpp
		spells/effects/Effect.cpp
		spells/effects/Effects.cpp
		spells/effects/Heal.cpp
		spells/effects/LocationEffect.cpp
		spells/effects/Obstacle.cpp
		spells/effects/Registry.cpp
		spells/effects/UnitEffect.cpp
		spells/effects/Summon.cpp
		spells/effects/Teleport.cpp
		spells/effects/Timed.cpp
		spells/effects/RemoveObstacle.cpp
		spells/effects/Sacrifice.cpp

		vstd/StringUtils.cpp
		
		BattleFieldHandler.cpp
		CAndroidVMHelper.cpp
		CArtHandler.cpp
		CBonusTypeHandler.cpp
		CBuildingHandler.cpp
		CConfigHandler.cpp
		CConsoleHandler.cpp
		CCreatureHandler.cpp
		CCreatureSet.cpp
		CGameInfoCallback.cpp
		CGameInterface.cpp
		CGameState.cpp
		CGeneralTextHandler.cpp
		CHeroHandler.cpp
		CModHandler.cpp
		CPathfinder.cpp
		CPlayerState.cpp
		CRandomGenerator.cpp
		CScriptingModule.cpp
		CSkillHandler.cpp
		CStack.cpp
		CThreadHelper.cpp
		CTownHandler.cpp
		GameConstants.cpp
		HeroBonus.cpp
		IGameCallback.cpp
		IHandlerBase.cpp
		JsonDetail.cpp
		JsonNode.cpp
		LoadProgress.cpp
		LogicalExpression.cpp
		NetPacksLib.cpp
		ObstacleHandler.cpp
		StartInfo.cpp
		ResourceSet.cpp
		ScriptHandler.cpp
		Terrain.cpp
		VCMIDirs.cpp
		VCMI_Lib.cpp

		${VCMILIB_ADDITIONAL_SOURCES}
)

# Version.cpp is a generated file
set_source_files_properties(${CMAKE_BINARY_DIR}/Version.cpp
	PROPERTIES GENERATED TRUE
)

set(lib_HEADERS
		../include/vstd/CLoggerBase.h
		../Global.h
		StdInc.h

		../include/vstd/ContainerUtils.h
		../include/vstd/RNG.h
		../include/vstd/StringUtils.h

		../include/vcmi/events/ApplyDamage.h
		../include/vcmi/events/Event.h
		../include/vcmi/events/EventBus.h
		../include/vcmi/events/SubscriptionRegistry.h

		../include/vcmi/scripting/Service.h

		../include/vcmi/spells/Caster.h
		../include/vcmi/spells/Magic.h
		../include/vcmi/spells/Service.h
		../include/vcmi/spells/Spell.h

		../include/vcmi/Artifact.h
		../include/vcmi/ArtifactService.h
		../include/vcmi/Creature.h
		../include/vcmi/CreatureService.h
		../include/vcmi/Entity.h
		../include/vcmi/Environment.h
		../include/vcmi/Services.h

		abilities/Ability.h

		battle/AccessibilityInfo.h
		battle/BattleAction.h
		battle/BattleAttackInfo.h
		battle/BattleHex.h
		battle/BattleInfo.h
		battle/BattleProxy.h
		battle/CBattleInfoCallback.h
		battle/CBattleInfoEssentials.h
		battle/CCallbackBase.h
		battle/CObstacleInstance.h
		battle/CPlayerBattleCallback.h
		battle/CUnitState.h
		battle/Destination.h
		battle/IBattleInfoCallback.h
		battle/IBattleState.h
		battle/IUnitInfo.h
		battle/ReachabilityInfo.h
		battle/SideInBattle.h
		battle/SiegeInfo.h
		battle/Unit.h

		events/ApplyDamage.h
		events/GameResumed.h
		events/ObjectVisitEnded.h
		events/ObjectVisitStarted.h
		events/PlayerGotTurn.h
		events/TurnStarted.h

		filesystem/AdapterLoaders.h
		filesystem/CArchiveLoader.h
		filesystem/CBinaryReader.h
		filesystem/CCompressedStream.h
		filesystem/CFileInputStream.h
		filesystem/CFilesystemLoader.h
		filesystem/CInputOutputStream.h
		filesystem/CInputStream.h
		filesystem/CMemoryBuffer.h
		filesystem/CMemoryStream.h
		filesystem/COutputStream.h
		filesystem/CStream.h
		filesystem/CZipLoader.h
		filesystem/CZipSaver.h
		filesystem/FileInfo.h
		filesystem/FileStream.h
		filesystem/Filesystem.h
		filesystem/ISimpleResourceLoader.h
		filesystem/MinizipExtensions.h
		filesystem/ResourceID.h

		logging/CBasicLogConfigurator.h
		logging/CLogger.h

		mapObjects/CArmedInstance.h
		mapObjects/CBank.h
		mapObjects/CGHeroInstance.h
		mapObjects/CGMarket.h
		mapObjects/CGPandoraBox.h
		mapObjects/CGTownInstance.h
		mapObjects/CObjectClassesHandler.h
		mapObjects/CObjectHandler.h
		mapObjects/CommonConstructors.h
		mapObjects/CQuest.h
		mapObjects/CRewardableConstructor.h
		mapObjects/CRewardableObject.h
		mapObjects/JsonRandom.h
		mapObjects/MapObjects.h
		mapObjects/MiscObjects.h
		mapObjects/ObjectTemplate.h

		mapping/CCampaignHandler.h
		mapping/CDrawRoadsOperation.h
		mapping/CMapDefines.h
		mapping/CMapEditManager.h
		mapping/CMap.h
		mapping/CMapInfo.h
		mapping/CMapOperation.h
		mapping/CMapService.h
		mapping/MapEditUtils.h
		mapping/MapFormatH3M.h
		mapping/MapFormatJson.h

		registerTypes/RegisterTypes.h

		rmg/RmgArea.h
		rmg/RmgObject.h
		rmg/RmgPath.h
		rmg/CMapGenerator.h
		rmg/CMapGenOptions.h
		rmg/CRmgTemplate.h
		rmg/CRmgTemplateStorage.h
		rmg/CZonePlacer.h
		rmg/TileInfo.h
		rmg/Zone.h
		rmg/Functions.h
		rmg/ObjectManager.h
		rmg/RoadPlacer.h
		rmg/TreasurePlacer.h
		rmg/RmgMap.h
		rmg/ConnectionsPlacer.h
		rmg/WaterAdopter.h
		rmg/TownPlacer.h
		rmg/WaterProxy.h
		rmg/WaterRoutes.h
		rmg/RockPlacer.h
		rmg/ObstaclePlacer.h
		rmg/RiverPlacer.h
		rmg/TerrainPainter.h
		rmg/float3.h

		serializer/BinaryDeserializer.h
		serializer/BinarySerializer.h
		serializer/CLoadIntegrityValidator.h
		serializer/CMemorySerializer.h
		serializer/Connection.h
		serializer/CSerializer.h
		serializer/CTypeList.h
		serializer/JsonDeserializer.h
		serializer/JsonSerializeFormat.h
		serializer/JsonSerializer.h
		serializer/JsonUpdater.h
		serializer/Cast.h

		spells/AbilityCaster.h
		spells/AdventureSpellMechanics.h
		spells/BattleSpellMechanics.h
		spells/BonusCaster.h
		spells/CSpellHandler.h
		spells/ISpellMechanics.h
		spells/Problem.h
		spells/ProxyCaster.h
		spells/TargetCondition.h
		spells/ViewSpellInt.h

		spells/effects/Catapult.h
		spells/effects/Clone.h
		spells/effects/Damage.h
		spells/effects/Dispel.h
		spells/effects/Effect.h
		spells/effects/Effects.h
		spells/effects/EffectsFwd.h
		spells/effects/Heal.h
		spells/effects/LocationEffect.h
		spells/effects/Obstacle.h
		spells/effects/Registry.h
		spells/effects/UnitEffect.h
		spells/effects/Summon.h
		spells/effects/Teleport.h
		spells/effects/Timed.h
		spells/effects/RemoveObstacle.h
		spells/effects/Sacrifice.h

		AI_Base.h
		BattleFieldHandler.h
		CAndroidVMHelper.h
		CArtHandler.h
		CBonusTypeHandler.h
		CBuildingHandler.h
		CConfigHandler.h
		CConsoleHandler.h
		CCreatureHandler.h
		CCreatureSet.h
		CGameInfoCallback.h
		CGameInterface.h
		CGameStateFwd.h
		CGameState.h
		CGeneralTextHandler.h
		CHeroHandler.h
		CModHandler.h
		CondSh.h
		ConstTransitivePtr.h
		CPathfinder.h
		CPlayerState.h
		CRandomGenerator.h
		CScriptingModule.h
		CSkillHandler.h
		CSoundBase.h
		CStack.h
		CStopWatch.h
		CThreadHelper.h
		CTownHandler.h
		FunctionList.h
		GameConstants.h
		HeroBonus.h
		IBonusTypeHandler.h
		IGameCallback.h
		IGameEventsReceiver.h
		IHandlerBase.h
		int3.h
		Interprocess.h
		JsonDetail.h
		JsonNode.h
		LoadProgress.h
		LogicalExpression.h
		NetPacksBase.h
		NetPacks.h
		NetPacksLobby.h
		ObstacleHandler.h
		PathfinderUtil.h
		ResourceSet.h
		ScriptHandler.h
		ScopeGuard.h
		StartInfo.h
		StringConstants.h
		Terrain.h
		UnlockGuard.h
		VCMIDirs.h
		vcmi_endian.h
		VCMI_Lib.h
)

if(APPLE_IOS)
	set(lib_SRCS ${lib_SRCS} CIOSUtils.m)
	set(lib_HEADERS ${lib_HEADERS} CIOSUtils.h)
endif(APPLE_IOS)

assign_source_group(${lib_SRCS} ${lib_HEADERS})

add_library(vcmi SHARED ${lib_SRCS} ${lib_HEADERS})
set_target_properties(vcmi PROPERTIES COMPILE_DEFINITIONS "VCMI_DLL=1")
target_link_libraries(vcmi PUBLIC
		minizip::minizip ZLIB::ZLIB
		${SYSTEM_LIBS} Boost::boost Boost::thread Boost::filesystem Boost::program_options Boost::locale Boost::date_time
)

target_include_directories(vcmi
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PUBLIC ${CMAKE_HOME_DIRECTORY}
	PUBLIC ${CMAKE_HOME_DIRECTORY}/include
	PRIVATE ${SDL2_INCLUDE_DIR}
)

if(WIN32)
	set_target_properties(vcmi
		PROPERTIES
			OUTPUT_NAME "VCMI_lib"
			PROJECT_LABEL "VCMI_lib"
	)
endif()

if(ANDROID)
	return()
endif()

vcmi_set_output_dir(vcmi "")

enable_pch(vcmi)

# We want to deploy assets into build directory for easier debugging without install
if(NOT APPLE_IOS)
	add_custom_command(TARGET vcmi POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/config
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/Mods
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/config ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/config
		COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Mods ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/Mods
	)
endif()

# Update version before vcmi compiling
if(TARGET update_version)
	add_dependencies(vcmi update_version)
endif()

install(TARGETS vcmi RUNTIME DESTINATION ${LIB_DIR} LIBRARY DESTINATION ${LIB_DIR} COMPONENT core)
if(APPLE_IOS)
	get_target_property(LINKED_LIBS vcmi LINK_LIBRARIES)
	foreach(LINKED_LIB IN LISTS LINKED_LIBS)
		if(TARGET ${LINKED_LIB})
			get_target_property(LIB_TYPE ${LINKED_LIB} TYPE)
			if(LIB_TYPE STREQUAL "SHARED_LIBRARY")
				install(TARGETS ${LINKED_LIB} LIBRARY DESTINATION ${LIB_DIR} COMPONENT core)
			endif()
		else()
			if(LINKED_LIB MATCHES "\\${CMAKE_SHARED_LIBRARY_SUFFIX}$")
				install(FILES ${LINKED_LIB} DESTINATION ${LIB_DIR} COMPONENT core)
			endif()
		endif()
	endforeach()
endif()

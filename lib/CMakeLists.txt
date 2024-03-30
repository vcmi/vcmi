set(lib_SRCS
	StdInc.cpp

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
	filesystem/Filesystem.cpp
	filesystem/MinizipExtensions.cpp
	filesystem/ResourcePath.cpp

	json/JsonNode.cpp
	json/JsonParser.cpp
	json/JsonUtils.cpp
	json/JsonValidator.cpp
	json/JsonWriter.cpp

	logging/CBasicLogConfigurator.cpp
	logging/CLogger.cpp
	logging/VisualLogger.cpp

	network/NetworkConnection.cpp
	network/NetworkHandler.cpp
	network/NetworkServer.cpp

	vstd/DateUtils.cpp
	vstd/StringUtils.cpp

	CConfigHandler.cpp
	CConsoleHandler.cpp
	CThreadHelper.cpp
	TextOperations.cpp
	VCMIDirs.cpp
)

set(lib_MAIN_SRCS

	battle/AccessibilityInfo.cpp
	battle/BattleAction.cpp
	battle/BattleAttackInfo.cpp
	battle/BattleHex.cpp
	battle/BattleInfo.cpp
	battle/BattleProxy.cpp
	battle/BattleStateInfoForRetreat.cpp
	battle/CBattleInfoCallback.cpp
	battle/CBattleInfoEssentials.cpp
	battle/CObstacleInstance.cpp
	battle/CPlayerBattleCallback.cpp
	battle/CUnitState.cpp
	battle/DamageCalculator.cpp
	battle/Destination.cpp
	battle/IBattleState.cpp
	battle/ReachabilityInfo.cpp
	battle/SideInBattle.cpp
	battle/SiegeInfo.cpp
	battle/Unit.cpp

	bonuses/Bonus.cpp
	bonuses/BonusEnum.cpp
	bonuses/BonusList.cpp
	bonuses/BonusParams.cpp
	bonuses/BonusSelector.cpp
	bonuses/BonusCustomTypes.cpp
	bonuses/CBonusProxy.cpp
	bonuses/CBonusSystemNode.cpp
	bonuses/IBonusBearer.cpp
	bonuses/Limiters.cpp
	bonuses/Propagators.cpp
	bonuses/Updaters.cpp

	campaign/CampaignHandler.cpp
	campaign/CampaignState.cpp

	constants/EntityIdentifiers.cpp

	events/ApplyDamage.cpp
	events/GameResumed.cpp
	events/ObjectVisitEnded.cpp
	events/ObjectVisitStarted.cpp
	events/PlayerGotTurn.cpp
	events/TurnStarted.cpp

	json/JsonBonus.cpp
	json/JsonRandom.cpp

	gameState/CGameState.cpp
	gameState/CGameStateCampaign.cpp
	gameState/InfoAboutArmy.cpp
	gameState/TavernHeroesPool.cpp

	mapObjectConstructors/AObjectTypeHandler.cpp
	mapObjectConstructors/CBankInstanceConstructor.cpp
	mapObjectConstructors/CObjectClassesHandler.cpp
	mapObjectConstructors/CommonConstructors.cpp
	mapObjectConstructors/CRewardableConstructor.cpp
	mapObjectConstructors/DwellingInstanceConstructor.cpp
	mapObjectConstructors/HillFortInstanceConstructor.cpp
	mapObjectConstructors/ShipyardInstanceConstructor.cpp

	mapObjects/CArmedInstance.cpp
	mapObjects/CBank.cpp
	mapObjects/CGCreature.cpp
	mapObjects/CGDwelling.cpp
	mapObjects/CGHeroInstance.cpp
	mapObjects/CGMarket.cpp
	mapObjects/CGObjectInstance.cpp
	mapObjects/CGPandoraBox.cpp
	mapObjects/CGTownBuilding.cpp
	mapObjects/CGTownInstance.cpp
	mapObjects/CObjectHandler.cpp
	mapObjects/CQuest.cpp
	mapObjects/CRewardableObject.cpp
	mapObjects/IMarket.cpp
	mapObjects/IObjectInterface.cpp
	mapObjects/MiscObjects.cpp
	mapObjects/ObjectTemplate.cpp

	mapping/CDrawRoadsOperation.cpp
	mapping/CMap.cpp
	mapping/CMapHeader.cpp
	mapping/CMapEditManager.cpp
	mapping/CMapInfo.cpp
	mapping/CMapOperation.cpp
	mapping/CMapService.cpp
	mapping/MapEditUtils.cpp
	mapping/MapIdentifiersH3M.cpp
	mapping/MapFeaturesH3M.cpp
	mapping/MapFormatH3M.cpp
	mapping/MapReaderH3M.cpp
	mapping/MapFormatJson.cpp
	mapping/ObstacleProxy.cpp

	modding/ActiveModsInSaveList.cpp
	modding/CModHandler.cpp
	modding/CModInfo.cpp
	modding/CModVersion.cpp
	modding/ContentTypeHandler.cpp
	modding/IdentifierStorage.cpp
	modding/ModUtility.cpp

	networkPacks/NetPacksLib.cpp

	pathfinder/CGPathNode.cpp
	pathfinder/CPathfinder.cpp
	pathfinder/NodeStorage.cpp
	pathfinder/PathfinderOptions.cpp
	pathfinder/PathfindingRules.cpp
	pathfinder/TurnInfo.cpp

	rewardable/Configuration.cpp
	rewardable/Info.cpp
	rewardable/Interface.cpp
	rewardable/Limiter.cpp
	rewardable/Reward.cpp

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
	rmg/RmgMap.cpp
	rmg/PenroseTiling.cpp
	rmg/modificators/Modificator.cpp
	rmg/modificators/ObjectManager.cpp
	rmg/modificators/ObjectDistributor.cpp
	rmg/modificators/RoadPlacer.cpp
	rmg/modificators/TreasurePlacer.cpp
	rmg/modificators/PrisonHeroPlacer.cpp
	rmg/modificators/QuestArtifactPlacer.cpp
	rmg/modificators/ConnectionsPlacer.cpp
	rmg/modificators/WaterAdopter.cpp
	rmg/modificators/MinePlacer.cpp
	rmg/modificators/TownPlacer.cpp
	rmg/modificators/WaterProxy.cpp
	rmg/modificators/WaterRoutes.cpp
	rmg/modificators/RockPlacer.cpp
	rmg/modificators/RockFiller.cpp
	rmg/modificators/ObstaclePlacer.cpp
	rmg/modificators/RiverPlacer.cpp
	rmg/modificators/TerrainPainter.cpp
	rmg/threadpool/MapProxy.cpp

	serializer/BinaryDeserializer.cpp
	serializer/BinarySerializer.cpp
	serializer/CLoadFile.cpp
	serializer/CMemorySerializer.cpp
	serializer/Connection.cpp
	serializer/CSaveFile.cpp
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
	spells/ExternalCaster.cpp
	spells/ISpellMechanics.cpp
	spells/ObstacleCasterProxy.cpp
	spells/Problem.cpp
	spells/ProxyCaster.cpp
	spells/TargetCondition.cpp
	spells/ViewSpellInt.cpp

	spells/effects/Catapult.cpp
	spells/effects/Clone.cpp
	spells/effects/Damage.cpp
	spells/effects/DemonSummon.cpp
	spells/effects/Dispel.cpp
	spells/effects/Effect.cpp
	spells/effects/Effects.cpp
	spells/effects/Heal.cpp
	spells/effects/LocationEffect.cpp
	spells/effects/Moat.cpp
	spells/effects/Obstacle.cpp
	spells/effects/Registry.cpp
	spells/effects/UnitEffect.cpp
	spells/effects/Summon.cpp
	spells/effects/Teleport.cpp
	spells/effects/Timed.cpp
	spells/effects/RemoveObstacle.cpp
	spells/effects/Sacrifice.cpp

	ArtifactUtils.cpp
	BasicTypes.cpp
	BattleFieldHandler.cpp
	CAndroidVMHelper.cpp
	CArtHandler.cpp
	CArtifactInstance.cpp
	CBonusTypeHandler.cpp
	CBuildingHandler.cpp
	CCreatureHandler.cpp
	CCreatureSet.cpp
	CGameInfoCallback.cpp
	CGameInterface.cpp
	CGeneralTextHandler.cpp
	CHeroHandler.cpp
	CPlayerState.cpp
	CRandomGenerator.cpp
	CScriptingModule.cpp
	CSkillHandler.cpp
	CStack.cpp
	CTownHandler.cpp
	GameSettings.cpp
	IGameCallback.cpp
	IHandlerBase.cpp
	LoadProgress.cpp
	LogicalExpression.cpp
	MetaString.cpp
	ObstacleHandler.cpp
	StartInfo.cpp
	ResourceSet.cpp
	RiverHandler.cpp
	RoadHandler.cpp
	ScriptHandler.cpp
	TerrainHandler.cpp
	TurnTimerInfo.cpp
	VCMI_Lib.cpp
)

if (NOT ENABLE_MINIMAL_LIB)
	list(APPEND lib_SRCS ${lib_MAIN_SRCS})
endif()

# Version.cpp is a generated file
if(ENABLE_GITVERSION)
	list(APPEND lib_SRCS ${CMAKE_BINARY_DIR}/Version.cpp)
	set_source_files_properties(${CMAKE_BINARY_DIR}/Version.cpp
		PROPERTIES GENERATED TRUE
	)
endif()

set(lib_HEADERS
	../include/vstd/CLoggerBase.h
	../include/vstd/DateUtils.h
	../include/vstd/StringUtils.h
	../Global.h
	../AUTHORS.h
	StdInc.h

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
	filesystem/Filesystem.h
	filesystem/ISimpleResourceLoader.h
	filesystem/MinizipExtensions.h
	filesystem/ResourcePath.h

	json/JsonFormatException.h
	json/JsonNode.h
	json/JsonParser.h
	json/JsonUtils.h
	json/JsonValidator.h
	json/JsonWriter.h

	logging/CBasicLogConfigurator.h
	logging/CLogger.h
	logging/VisualLogger.h

	network/NetworkConnection.h
	network/NetworkDefines.h
	network/NetworkHandler.h
	network/NetworkInterface.h
	network/NetworkServer.h

	CConfigHandler.h
	CConsoleHandler.h
	CThreadHelper.h
	TextOperations.h
	VCMIDirs.h
)

set(lib_MAIN_HEADERS
	../include/vstd/ContainerUtils.h
	../include/vstd/RNG.h

	../include/vcmi/events/AdventureEvents.h
	../include/vcmi/events/ApplyDamage.h
	../include/vcmi/events/BattleEvents.h
	../include/vcmi/events/Event.h
	../include/vcmi/events/EventBus.h
	../include/vcmi/events/GameResumed.h
	../include/vcmi/events/GenericEvents.h
	../include/vcmi/events/ObjectVisitEnded.h
	../include/vcmi/events/ObjectVisitStarted.h
	../include/vcmi/events/PlayerGotTurn.h
	../include/vcmi/events/SubscriptionRegistry.h
	../include/vcmi/events/TurnStarted.h

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
	../include/vcmi/Faction.h
	../include/vcmi/FactionService.h
	../include/vcmi/HeroClass.h
	../include/vcmi/HeroClassService.h
	../include/vcmi/HeroType.h
	../include/vcmi/HeroTypeService.h
	../include/vcmi/Metatype.h
	../include/vcmi/Player.h
	../include/vcmi/ServerCallback.h
	../include/vcmi/Services.h
	../include/vcmi/Skill.h
	../include/vcmi/SkillService.h
	../include/vcmi/Team.h

	battle/AccessibilityInfo.h
	battle/AutocombatPreferences.h
	battle/BattleAction.h
	battle/BattleAttackInfo.h
	battle/BattleHex.h
	battle/BattleInfo.h
	battle/BattleStateInfoForRetreat.h
	battle/BattleProxy.h
	battle/CBattleInfoCallback.h
	battle/CBattleInfoEssentials.h
	battle/CObstacleInstance.h
	battle/CPlayerBattleCallback.h
	battle/CUnitState.h
	battle/DamageCalculator.h
	battle/Destination.h
	battle/IBattleInfoCallback.h
	battle/IBattleState.h
	battle/IUnitInfo.h
	battle/PossiblePlayerBattleAction.h
	battle/ReachabilityInfo.h
	battle/SideInBattle.h
	battle/SiegeInfo.h
	battle/Unit.h

	bonuses/Bonus.h
	bonuses/BonusEnum.h
	bonuses/BonusList.h
	bonuses/BonusParams.h
	bonuses/BonusSelector.h
	bonuses/BonusCustomTypes.h
	bonuses/CBonusProxy.h
	bonuses/CBonusSystemNode.h
	bonuses/IBonusBearer.h
	bonuses/Limiters.h
	bonuses/Propagators.h
	bonuses/Updaters.h

	campaign/CampaignConstants.h
	campaign/CampaignHandler.h
	campaign/CampaignScenarioPrologEpilog.h
	campaign/CampaignState.h

	constants/EntityIdentifiers.h
	constants/Enumerations.h
	constants/IdentifierBase.h
	constants/VariantIdentifier.h
	constants/NumericConstants.h
	constants/StringConstants.h

	events/ApplyDamage.h
	events/GameResumed.h
	events/ObjectVisitEnded.h
	events/ObjectVisitStarted.h
	events/PlayerGotTurn.h
	events/TurnStarted.h

	json/JsonBonus.h
	json/JsonRandom.h

	gameState/CGameState.h
	gameState/CGameStateCampaign.h
	gameState/EVictoryLossCheckResult.h
	gameState/InfoAboutArmy.h
	gameState/SThievesGuildInfo.h
	gameState/TavernHeroesPool.h
	gameState/TavernSlot.h
	gameState/QuestInfo.h

	mapObjectConstructors/AObjectTypeHandler.h
	mapObjectConstructors/CBankInstanceConstructor.h
	mapObjectConstructors/CDefaultObjectTypeHandler.h
	mapObjectConstructors/CObjectClassesHandler.h
	mapObjectConstructors/CommonConstructors.h
	mapObjectConstructors/CRewardableConstructor.h
	mapObjectConstructors/DwellingInstanceConstructor.h
	mapObjectConstructors/HillFortInstanceConstructor.h
	mapObjectConstructors/IObjectInfo.h
	mapObjectConstructors/RandomMapInfo.h
	mapObjectConstructors/ShipyardInstanceConstructor.h
	mapObjectConstructors/SObjectSounds.h

	mapObjects/CArmedInstance.h
	mapObjects/CBank.h
	mapObjects/CGCreature.h
	mapObjects/CGDwelling.h
	mapObjects/CGHeroInstance.h
	mapObjects/CGMarket.h
	mapObjects/CGObjectInstance.h
	mapObjects/CGPandoraBox.h
	mapObjects/CGTownBuilding.h
	mapObjects/CGTownInstance.h
	mapObjects/CObjectHandler.h
	mapObjects/CQuest.h
	mapObjects/CRewardableObject.h
	mapObjects/IMarket.h
	mapObjects/IObjectInterface.h
	mapObjects/MapObjects.h
	mapObjects/MiscObjects.h
	mapObjects/ObjectTemplate.h

	mapping/CDrawRoadsOperation.h
	mapping/CMapDefines.h
	mapping/CMapEditManager.h
	mapping/CMapHeader.h
	mapping/CMap.h
	mapping/CMapInfo.h
	mapping/CMapOperation.h
	mapping/CMapService.h
	mapping/MapEditUtils.h
	mapping/MapIdentifiersH3M.h
	mapping/MapFeaturesH3M.h
	mapping/MapFormatH3M.h
	mapping/MapFormat.h
	mapping/MapReaderH3M.h
	mapping/MapFormatJson.h
	mapping/ObstacleProxy.h

	modding/ActiveModsInSaveList.h
	modding/CModHandler.h
	modding/CModInfo.h
	modding/CModVersion.h
	modding/ContentTypeHandler.h
	modding/IdentifierStorage.h
	modding/ModIncompatibility.h
	modding/ModScope.h
	modding/ModUtility.h
	modding/ModVerificationInfo.h

	networkPacks/ArtifactLocation.h
	networkPacks/BattleChanges.h
	networkPacks/Component.h
	networkPacks/EInfoWindowMode.h
	networkPacks/EntityChanges.h
	networkPacks/EOpenWindowMode.h
	networkPacks/NetPacksBase.h
	networkPacks/NetPackVisitor.h
	networkPacks/ObjProperty.h
	networkPacks/PacksForClient.h
	networkPacks/PacksForClientBattle.h
	networkPacks/PacksForLobby.h
	networkPacks/PacksForServer.h
	networkPacks/SetStackEffect.h
	networkPacks/StackLocation.h
	networkPacks/TradeItem.h

	pathfinder/INodeStorage.h
	pathfinder/CGPathNode.h
	pathfinder/CPathfinder.h
	pathfinder/NodeStorage.h
	pathfinder/PathfinderOptions.h
	pathfinder/PathfinderUtil.h
	pathfinder/PathfindingRules.h
	pathfinder/TurnInfo.h

	registerTypes/RegisterTypes.h
	registerTypes/RegisterTypesClientPacks.h
	registerTypes/RegisterTypesLobbyPacks.h
	registerTypes/RegisterTypesMapObjects.h
	registerTypes/RegisterTypesServerPacks.h

	rewardable/Configuration.h
	rewardable/Info.h
	rewardable/Interface.h
	rewardable/Limiter.h
	rewardable/Reward.h

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
	rmg/RmgMap.h
	rmg/float3.h
	rmg/Functions.h
	rmg/PenroseTiling.h
	rmg/modificators/Modificator.h
	rmg/modificators/ObjectManager.h
	rmg/modificators/ObjectDistributor.h
	rmg/modificators/RoadPlacer.h
	rmg/modificators/TreasurePlacer.h
	rmg/modificators/PrisonHeroPlacer.h
	rmg/modificators/QuestArtifactPlacer.h
	rmg/modificators/ConnectionsPlacer.h
	rmg/modificators/WaterAdopter.h
	rmg/modificators/MinePlacer.h
	rmg/modificators/TownPlacer.h
	rmg/modificators/WaterProxy.h
	rmg/modificators/WaterRoutes.h
	rmg/modificators/RockPlacer.h
	rmg/modificators/RockFiller.h
	rmg/modificators/ObstaclePlacer.h
	rmg/modificators/RiverPlacer.h
	rmg/modificators/TerrainPainter.h
	rmg/threadpool/BlockingQueue.h
	rmg/threadpool/ThreadPool.h
	rmg/threadpool/MapProxy.h

	serializer/BinaryDeserializer.h
	serializer/BinarySerializer.h
	serializer/CLoadFile.h
	serializer/CMemorySerializer.h
	serializer/Connection.h
	serializer/CSaveFile.h
	serializer/CSerializer.h
	serializer/CTypeList.h
	serializer/JsonDeserializer.h
	serializer/JsonSerializeFormat.h
	serializer/JsonSerializer.h
	serializer/JsonUpdater.h
	serializer/Cast.h
	serializer/ESerializationVersion.h

	spells/AbilityCaster.h
	spells/AdventureSpellMechanics.h
	spells/BattleSpellMechanics.h
	spells/BonusCaster.h
	spells/CSpellHandler.h
	spells/ExternalCaster.h
	spells/ISpellMechanics.h
	spells/ObstacleCasterProxy.h
	spells/Problem.h
	spells/ProxyCaster.h
	spells/TargetCondition.h
	spells/ViewSpellInt.h

	spells/effects/Catapult.h
	spells/effects/Clone.h
	spells/effects/Damage.h
	spells/effects/DemonSummon.h
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
	ArtifactUtils.h
	BattleFieldHandler.h
	CAndroidVMHelper.h
	CArtHandler.h
	CArtifactInstance.h
	CBonusTypeHandler.h
	CBuildingHandler.h
	CCreatureHandler.h
	CCreatureSet.h
	CGameInfoCallback.h
	CGameInterface.h
	CGeneralTextHandler.h
	CHeroHandler.h
	CondSh.h
	ConstTransitivePtr.h
	Color.h
	CPlayerState.h
	CRandomGenerator.h
	CScriptingModule.h
	CSkillHandler.h
	CSoundBase.h
	CStack.h
	CStopWatch.h
	CTownHandler.h
	ExtraOptionsInfo.h
	FunctionList.h
	GameCallbackHolder.h
	GameConstants.h
	GameSettings.h
	IBonusTypeHandler.h
	IGameCallback.h
	IGameEventsReceiver.h
	IHandlerBase.h
	int3.h
	Languages.h
	LoadProgress.h
	LogicalExpression.h
	MetaString.h
	ObstacleHandler.h
	Point.h
	Rect.h
	Rect.cpp
	ResourceSet.h
	RiverHandler.h
	RoadHandler.h
	ScriptHandler.h
	ScopeGuard.h
	StartInfo.h
	TerrainHandler.h
	TurnTimerInfo.h
	UnlockGuard.h
	vcmi_endian.h
	VCMI_Lib.h
)

if (NOT ENABLE_MINIMAL_LIB)
	list(APPEND lib_HEADERS ${lib_MAIN_HEADERS})
endif()

assign_source_group(${lib_SRCS} ${lib_HEADERS})

if(ENABLE_STATIC_LIBS)
	add_library(vcmi STATIC ${lib_SRCS} ${lib_HEADERS})
else()
	add_library(vcmi SHARED ${lib_SRCS} ${lib_HEADERS})
endif()

set_target_properties(vcmi PROPERTIES COMPILE_DEFINITIONS "VCMI_DLL=1")
target_link_libraries(vcmi PUBLIC
	minizip::minizip ZLIB::ZLIB
	${SYSTEM_LIBS} Boost::boost Boost::thread Boost::filesystem Boost::program_options Boost::locale Boost::date_time
)

if(ENABLE_STATIC_LIBS AND ENABLE_CLIENT)
	target_compile_definitions(vcmi PRIVATE STATIC_AI)
	target_link_libraries(vcmi PRIVATE
		BattleAI
		EmptyAI
		StupidAI
		VCAI
	)
	if(ENABLE_NULLKILLER_AI)
		target_link_libraries(vcmi PRIVATE Nullkiller)
	endif()
endif()

# no longer necessary, but might be useful to keep in future
# unfortunately at the moment tests do not support namespaced build, so enable only on some systems
if(APPLE_IOS OR ANDROID)
	target_compile_definitions(vcmi PUBLIC VCMI_LIB_NAMESPACE=VCMI)
endif()

if(APPLE_IOS)
	target_link_libraries(vcmi PUBLIC iOS_utils)
endif()

target_include_directories(vcmi
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PUBLIC ${CMAKE_SOURCE_DIR}
	PUBLIC ${CMAKE_SOURCE_DIR}/include
)

if(WIN32)
	set_target_properties(vcmi
		PROPERTIES
			OUTPUT_NAME "VCMI_lib"
			PROJECT_LABEL "VCMI_lib"
	)
endif()

vcmi_set_output_dir(vcmi "")

enable_pch(vcmi)

# We want to deploy assets into build directory for easier debugging without install
if(COPY_CONFIG_ON_BUILD)
	add_custom_command(TARGET vcmi POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/config
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/Mods
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake_modules/create_link.cmake ${CMAKE_SOURCE_DIR}/config ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/config
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake_modules/create_link.cmake ${CMAKE_SOURCE_DIR}/Mods ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/Mods
	)
endif()

# Update version before vcmi compiling
if(TARGET update_version)
	add_dependencies(vcmi update_version)
endif()

if(NOT ENABLE_STATIC_LIBS)
	install(TARGETS vcmi RUNTIME DESTINATION ${LIB_DIR} LIBRARY DESTINATION ${LIB_DIR})
endif()

if(APPLE_IOS AND NOT USING_CONAN)
	get_target_property(LINKED_LIBS vcmi LINK_LIBRARIES)
	foreach(LINKED_LIB IN LISTS LINKED_LIBS)
		if(NOT TARGET ${LINKED_LIB})
			if(LINKED_LIB MATCHES "\\${CMAKE_SHARED_LIBRARY_SUFFIX}$")
				install(FILES ${LINKED_LIB} DESTINATION ${LIB_DIR})
			endif()
			continue()
		endif()

		get_target_property(LIB_TYPE ${LINKED_LIB} TYPE)
		if(NOT LIB_TYPE STREQUAL "SHARED_LIBRARY")
			continue()
		endif()

		get_target_property(_aliased ${LINKED_LIB} ALIASED_TARGET)
		if(_aliased)
			set(LINKED_LIB_REAL ${_aliased})
		else()
			set(LINKED_LIB_REAL ${LINKED_LIB})
		endif()

		get_target_property(_imported ${LINKED_LIB_REAL} IMPORTED)
		if(_imported)
			set(INSTALL_TYPE IMPORTED_RUNTIME_ARTIFACTS)
			get_target_property(BOOST_DEPENDENCIES ${LINKED_LIB_REAL} INTERFACE_LINK_LIBRARIES)
			foreach(BOOST_DEPENDENCY IN LISTS BOOST_DEPENDENCIES)
				get_target_property(BOOST_DEPENDENCY_TYPE ${BOOST_DEPENDENCY} TYPE)
				if(BOOST_DEPENDENCY_TYPE STREQUAL "SHARED_LIBRARY")
					install(IMPORTED_RUNTIME_ARTIFACTS ${BOOST_DEPENDENCY} LIBRARY DESTINATION ${LIB_DIR})
				endif()
			endforeach()
		else()
			set(INSTALL_TYPE TARGETS)
		endif()
		install(${INSTALL_TYPE} ${LINKED_LIB_REAL} LIBRARY DESTINATION ${LIB_DIR})
	endforeach()
endif()

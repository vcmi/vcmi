project(libvcmi)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Boost_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

set(lib_SRCS
		StdInc.cpp
		${CMAKE_BINARY_DIR}/Version.cpp

		battle/AccessibilityInfo.cpp
		battle/BattleAction.cpp
		battle/BattleAttackInfo.cpp
		battle/BattleHex.cpp
		battle/BattleInfo.cpp
		battle/CBattleInfoCallback.cpp
		battle/CBattleInfoEssentials.cpp
		battle/CCallbackBase.cpp
		battle/CObstacleInstance.cpp
		battle/CPlayerBattleCallback.cpp
		battle/ReachabilityInfo.cpp
		battle/SideInBattle.cpp
		battle/SiegeInfo.cpp

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
		mapping/CMapService.cpp
		mapping/MapFormatH3M.cpp
		mapping/MapFormatJson.cpp

		registerTypes/RegisterTypes.cpp
		registerTypes/TypesClientPacks1.cpp
		registerTypes/TypesClientPacks2.cpp
		registerTypes/TypesMapObjects1.cpp
		registerTypes/TypesMapObjects2.cpp
		registerTypes/TypesMapObjects3.cpp
		registerTypes/TypesPregamePacks.cpp
		registerTypes/TypesServerPacks.cpp

		rmg/CMapGenerator.cpp
		rmg/CMapGenOptions.cpp
		rmg/CRmgTemplate.cpp
		rmg/CRmgTemplateStorage.cpp
		rmg/CRmgTemplateZone.cpp
		rmg/CZoneGraphGenerator.cpp
		rmg/CZonePlacer.cpp

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

		spells/AdventureSpellMechanics.cpp
		spells/BattleSpellMechanics.cpp
		spells/CDefaultSpellMechanics.cpp
		spells/CreatureSpellMechanics.cpp
		spells/CSpellHandler.cpp
		spells/ISpellMechanics.cpp
		spells/ViewSpellInt.cpp

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
		CRandomGenerator.cpp
		CStack.cpp
		CThreadHelper.cpp
		CTownHandler.cpp
		GameConstants.cpp
		HeroBonus.cpp
		IGameCallback.cpp
		IHandlerBase.cpp
		JsonDetail.cpp
		JsonNode.cpp
		LogicalExpression.cpp
		NetPacksLib.cpp
		ResourceSet.cpp
		VCMIDirs.cpp
		VCMI_Lib.cpp

		${VCMILIB_ADDITIONAL_SOURCES}
)

set(lib_HEADERS
		StdInc.h
		../Global.h

		battle/AccessibilityInfo.h
		battle/BattleAction.h
		battle/BattleAttackInfo.h
		battle/BattleHex.h
		battle/BattleInfo.h
		battle/CBattleInfoCallback.h
		battle/CBattleInfoEssentials.h
		battle/CCallbackBase.h
		battle/CObstacleInstance.h
		battle/CPlayerBattleCallback.h
		battle/ReachabilityInfo.h
		battle/SideInBattle.h
		battle/SiegeInfo.h

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
		mapping/CMapService.h
		mapping/MapFormatH3M.h
		mapping/MapFormatJson.h

		registerTypes/RegisterTypes.h

		rmg/CMapGenerator.h
		rmg/CMapGenOptions.h
		rmg/CRmgTemplate.h
		rmg/CRmgTemplateStorage.h
		rmg/CRmgTemplateZone.h
		rmg/CZoneGraphGenerator.h
		rmg/CZonePlacer.h
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

		spells/AdventureSpellMechanics.h
		spells/BattleSpellMechanics.h
		spells/CDefaultSpellMechanics.h
		spells/CreatureSpellMechanics.h
		spells/CSpellHandler.h
		spells/ISpellMechanics.h
		spells/Magic.h
		spells/SpellMechanics.h
		spells/ViewSpellInt.h

		AI_Base.h
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
		LogicalExpression.h
		NetPacksBase.h
		NetPacks.h
		ResourceSet.h
		ScopeGuard.h
		StartInfo.h
		StringConstants.h
		UnlockGuard.h
		VCMIDirs.h
		vcmi_endian.h
		VCMI_Lib.h
)

add_library(vcmi SHARED ${lib_SRCS} ${lib_HEADERS})
set_target_properties(vcmi PROPERTIES XCODE_ATTRIBUTE_LD_DYLIB_INSTALL_NAME "@rpath/libvcmi.dylib")
set_target_properties(vcmi PROPERTIES COMPILE_DEFINITIONS "VCMI_DLL=1")
target_link_libraries(vcmi ${MINIZIP_LIBRARIES} ${Boost_LIBRARIES} ${SDL_LIBRARY} ${ZLIB_LIBRARIES} ${SYSTEM_LIBS})

if(WIN32)
	set_target_properties(vcmi PROPERTIES OUTPUT_NAME VCMI_lib)
endif()

if (ANDROID)
	return()
endif()

vcmi_set_output_dir(vcmi "")

set_target_properties(vcmi PROPERTIES ${PCH_PROPERTIES})
cotire(vcmi)

if (NOT APPLE) # Already inside vcmiclient bundle
	install(TARGETS vcmi RUNTIME DESTINATION ${LIB_DIR} LIBRARY DESTINATION ${LIB_DIR})
endif()

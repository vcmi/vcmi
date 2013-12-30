project(libvcmi)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Boost_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

set(lib_SRCS
		RegisterTypes.cpp
		IGameCallback.cpp
		CGameState.cpp
		CObjectHandler.cpp
		Connection.cpp
		NetPacksLib.cpp

		filesystem/AdapterLoaders.cpp
		filesystem/CCompressedStream.cpp
		filesystem/CFilesystemLoader.cpp
		filesystem/CArchiveLoader.cpp
		filesystem/CFileInfo.cpp
		filesystem/CMemoryStream.cpp
		filesystem/CBinaryReader.cpp
		filesystem/CFileInputStream.cpp
		filesystem/CZipLoader.cpp
		filesystem/Filesystem.cpp
		filesystem/ResourceID.cpp

		logging/CBasicLogConfigurator.cpp
		logging/CLogger.cpp

		mapping/CCampaignHandler.cpp
		mapping/CMap.cpp
		mapping/CMapEditManager.cpp
		mapping/CMapInfo.cpp
		mapping/CMapService.cpp
		mapping/MapFormatH3M.cpp
		mapping/MapFormatJson.cpp

		rmg/CMapGenerator.cpp
		rmg/CMapGenOptions.cpp
		rmg/CRmgTemplate.cpp
		rmg/CRmgTemplateZone.cpp
		rmg/CRmgTemplateStorage.cpp
		rmg/CZoneGraphGenerator.cpp
		rmg/CZonePlacer.cpp

		BattleAction.cpp
		BattleHex.cpp
		BattleState.cpp
		CArtHandler.cpp
		CBattleCallback.cpp
		CBonusTypeHandler.cpp
		CBuildingHandler.cpp
		CConfigHandler.cpp
		CConsoleHandler.cpp
		CCreatureHandler.cpp
		CCreatureSet.cpp
		CDefObjInfoHandler.cpp
		CGameInterface.cpp
		CGeneralTextHandler.cpp
		CHeroHandler.cpp
		CModHandler.cpp
		CObstacleInstance.cpp
		CSpellHandler.cpp
		CThreadHelper.cpp
		CTownHandler.cpp
		GameConstants.cpp
		HeroBonus.cpp
		JsonDetail.cpp
		JsonNode.cpp
		LogicalExpression.cpp
		ResourceSet.cpp
		VCMI_Lib.cpp
		VCMIDirs.cpp
)

set(lib_HEADERS
		../Global.h

		filesystem/CInputStream.h
		filesystem/ISimpleResourceLoader.h

		AI_Base.h
		CondSh.h
		ConstTransitivePtr.h
		CBonusTypeHandler.h
		CRandomGenerator.h
		CScriptingModule.h
		CStopWatch.h
		FunctionList.h
		GameConstants.h
		StringConstants.h
		IGameEventsReceiver.h
		IHandlerBase.h
		int3.h
		Interprocess.h
		NetPacks.h
		RegisterTypes.h
		StartInfo.h
		UnlockGuard.h
		VCMIDirs.h
		vcmi_endian.h
)

add_library(vcmi SHARED ${lib_SRCS} ${lib_HEADERS})
set_target_properties(vcmi PROPERTIES XCODE_ATTRIBUTE_LD_DYLIB_INSTALL_NAME "@executable_path/libvcmi.dylib")
target_link_libraries(vcmi minizip ${Boost_LIBRARIES} ${SDL_LIBRARY} ${ZLIB_LIBRARIES} ${RT_LIB} ${DL_LIB})

if (NOT APPLE) # Already inside vcmiclient bundle
    install(TARGETS vcmi DESTINATION ${LIB_DIR})
endif()

project(libvcmi)
cmake_minimum_required(VERSION 2.6)

include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/lib)
include_directories(${Boost_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})

set(lib_SRCS
		Filesystem/CBinaryReader.cpp
		Filesystem/CFilesystemLoader.cpp
		Filesystem/CMemoryStream.cpp
		Filesystem/CFileInfo.cpp
		Filesystem/CLodArchiveLoader.cpp
		Filesystem/CResourceLoader.cpp
		Filesystem/CFileInputStream.cpp
		Filesystem/CCompressedStream.cpp
		Logging/CBasicLogConfigurator.cpp
		Logging/CLogConsoleTarget.cpp
		Logging/CLogFileTarget.cpp
		Logging/CLogFormatter.cpp
		Logging/CLogger.cpp
		Logging/CLogManager.cpp
		Mapping/CCampaignHandler.cpp
		Mapping/CMap.cpp
		Mapping/CMapEditManager.cpp
		Mapping/CMapInfo.cpp
		Mapping/CMapService.cpp
		Mapping/MapFormatH3M.cpp
		RMG/CMapGenOptions.cpp
		RMG/CMapGenerator.cpp
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
		CGameState.cpp
		CGeneralTextHandler.cpp
		CHeroHandler.cpp
		CLogger.cpp
		CModHandler.cpp
		CObjectHandler.cpp
		CObstacleInstance.cpp
		Connection.cpp
		CSpellHandler.cpp
		CThreadHelper.cpp
		CTownHandler.cpp
		GameConstants.cpp
		HeroBonus.cpp
		IGameCallback.cpp
		JsonNode.cpp
		NetPacksLib.cpp
		ResourceSet.cpp
		RegisterTypes.cpp
		VCMI_Lib.cpp
		VCMIDirs.cpp
)

set(lib_HEADERS
		Filesystem/CInputStream.h
		Filesystem/ISimpleResourceLoader.h
		Logging/CBasicLogConfigurator.h
		Logging/CLogConsoleTarget.h
		Logging/CLogFileTarget.h
		Logging/CLogFormatter.h
		Logging/CLogger.h
		Logging/CLogManager.h
		Logging/ILogTarget.h
		Logging/LogRecord.h
		Mapping/CCampaignHandler.h
		Mapping/CMap.h
		Mapping/CMapEditManager.h
		Mapping/CMapInfo.h
		Mapping/CMapService.h
		Mapping/MapFormatH3M.h
		AI_Base.h
		CondSh.h
		ConstTransitivePtr.h
		CBonusTypeHandler.h
		CRandomGenerator.h
		CScriptingModule.h
		CStopWatch.h
		GameConstants.h
		StringConstants.h
		IGameEventsReceiver.h
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
target_link_libraries(vcmi ${Boost_LIBRARIES} ${SDL_LIBRARY} ${ZLIB_LIBRARIES})

if (NOT APPLE) # Already inside vcmiclient bundle
    install(TARGETS vcmi DESTINATION ${LIB_DIR})
endif()

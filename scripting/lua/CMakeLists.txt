include_directories(${Boost_INCLUDE_DIRS} ${LUA_INCLUDE_DIR} ${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(lib_SRCS
		StdInc.cpp
		LuaReference.cpp
		LuaScriptModule.cpp
		LuaScriptingContext.cpp
		LuaSpellEffect.cpp
		LuaStack.cpp

		api/battle/UnitProxy.cpp

		api/events/BattleEvents.cpp
		api/events/EventBusProxy.cpp
		api/events/GenericEvents.cpp
		api/events/SubscriptionRegistryProxy.cpp

		api/netpacks/BattleLogMessage.cpp
		api/netpacks/BattleStackMoved.cpp
		api/netpacks/BattleUnitsChanged.cpp
		api/netpacks/EntitiesChanged.cpp
		api/netpacks/InfoWindow.cpp
		api/netpacks/SetResources.cpp

		api/Artifact.cpp
		api/BattleCb.cpp
		api/BonusSystem.cpp
		api/Creature.cpp
		api/Faction.cpp
		api/GameCb.cpp
		api/HeroClass.cpp
		api/HeroInstance.cpp
		api/HeroType.cpp
		api/Registry.cpp
		api/ServerCb.cpp
		api/Services.cpp
		api/Skill.cpp
		api/Spell.cpp
		api/StackInstance.cpp
)

add_library(vcmiLua SHARED ${lib_SRCS})
target_link_libraries(vcmiLua Boost::boost luajit::luajit vcmi)

vcmi_set_output_dir(vcmiLua "scripting")
enable_pch(vcmiLua)

install(TARGETS vcmiLua LIBRARY DESTINATION ${SCRIPTING_LIB_DIR} OPTIONAL)

#manually copy lua dll from vcpkg folder to build directory on windows since vcpkg deps copy feature has flaws, using hardcoded paths based on vcmi windows deps package 1.1 from github
if(MSVC)
	if(EXISTS ${LUA_INCLUDE_DIR}/../../bin/lua51.dll)
		file(COPY ${LUA_INCLUDE_DIR}/../../bin/lua51.dll DESTINATION ${CMAKE_BINARY_DIR}/bin)
	endif()
endif()

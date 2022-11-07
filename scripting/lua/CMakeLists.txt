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

set(lib_HDRS
		StdInc.h
		LuaCallWrapper.h
		LuaFunctor.h
		LuaReference.h
		LuaScriptModule.h
		LuaScriptingContext.h
		LuaSpellEffect.h
		LuaStack.h
		LuaWrapper.h

		api/battle/UnitProxy.h

		api/events/BattleEvents.h
		api/events/EventBusProxy.h
		api/events/GenericEvents.h
		api/events/SubscriptionRegistryProxy.h

		api/netpacks/BattleLogMessage.h
		api/netpacks/BattleStackMoved.h
		api/netpacks/BattleUnitsChanged.h
		api/netpacks/EntitiesChanged.h
		api/netpacks/InfoWindow.h
		api/netpacks/PackForClient.h
		api/netpacks/SetResources.h

		api/Artifact.h
		api/BattleCb.h
		api/BonusSystem.h
		api/Creature.h
		api/Faction.h
		api/GameCb.h
		api/HeroClass.h
		api/HeroInstance.h
		api/HeroType.h
		api/Registry.h
		api/ServerCb.h
		api/Services.h
		api/Skill.h
		api/Spell.h
		api/StackInstance.h
)

add_library(vcmiLua SHARED ${lib_SRCS} ${lib_HDRS})
target_link_libraries(vcmiLua Boost::boost luajit::luajit ${VCMI_LIB_TARGET})

vcmi_set_output_dir(vcmiLua "scripting")
enable_pch(vcmiLua)

if(APPLE_IOS)
	install(TARGETS vcmiLua LIBRARY DESTINATION ${SCRIPTING_LIB_DIR})
else()
	install(TARGETS vcmiLua DESTINATION ${SCRIPTING_LIB_DIR})
endif()

#manually copy lua dll from vcpkg folder to build directory on windows since vcpkg deps copy feature has flaws, using hardcoded paths based on vcmi windows deps package 1.1 from github
if(MSVC)
	if(EXISTS ${LUA_INCLUDE_DIR}/../../bin/lua51.dll)
		file(COPY ${LUA_INCLUDE_DIR}/../../bin/lua51.dll DESTINATION ${CMAKE_BINARY_DIR}/bin)
	endif()
endif()

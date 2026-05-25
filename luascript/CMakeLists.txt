include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(lualib_SRCS
		StdInc.cpp
		LuaReference.cpp
		LuaModule.cpp
		LuaScriptInstance.cpp
		LuaContext.cpp
		LuaScriptPool.cpp
		LuaSpellEffect.cpp
		LuaStack.cpp

		api/battle/UnitProxy.cpp
		api/battle/BattleHexProxy.cpp

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

		api/spells/Mechanics.cpp
		api/spells/Problem.cpp

		api/texts/MetaString.cpp

		api/Artifact.cpp
		api/BattleCb.cpp
		api/BonusSystem.cpp
		api/Creature.cpp
		api/Enums.cpp
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

set(lualib_HDRS
		StdInc.h
		LuaCallWrapper.h
		LuaReference.h
		LuaModule.h
		LuaScriptInstance.h
		LuaContext.h
		LuaSpellEffect.h
		LuaScriptPool.h
		LuaStack.h
		LuaWrapper.h

		api/battle/UnitProxy.h
		api/battle/BattleHexProxy.h

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

		api/spells/Mechanics.h
		api/spells/Problem.h

		api/texts/MetaString.h

		api/Artifact.h
		api/BattleCb.h
		api/BonusSystem.h
		api/Creature.h
		api/Enums.h
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

assign_source_group(${lualib_SRCS} ${lualib_HDRS})

if(ENABLE_STATIC_LIBS)
	add_library(vcmiLua STATIC ${lualib_SRCS} ${lualib_HDRS})
else()
	add_library(vcmiLua SHARED ${lualib_SRCS} ${lualib_HDRS})
	install(TARGETS vcmiLua RUNTIME DESTINATION ${SCRIPTING_LIB_DIR} LIBRARY DESTINATION ${SCRIPTING_LIB_DIR})
endif()

if(TARGET luajit::luajit)
	target_link_libraries(vcmiLua Boost::boost luajit::luajit vcmi)
elseif(TARGET lua::lua)
	target_link_libraries(vcmiLua Boost::boost lua::lua vcmi)
else()
	target_link_libraries(vcmiLua Boost::boost Lua::Lua vcmi)
endif()

vcmi_set_output_dir(vcmiLua "scripting")
enable_pch(vcmiLua)

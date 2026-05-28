include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_HOME_DIRECTORY}/include ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/lib)

set(lualib_SRCS
		StdInc.cpp
		LuaReference.cpp
		LuaModule.cpp
		LuaScriptInstance.cpp
		LuaContext.cpp
		LuaScriptPool.cpp
		LuaSpellEffect.cpp
		LuaUnitEffect.cpp
		LuaStack.cpp

		api/adventure/HeroInstance.cpp
		api/adventure/StackInstance.cpp

		api/battle/UnitProxy.cpp
		api/battle/BattleHexArrayProxy.cpp
		api/battle/BattleHexProxy.cpp

		api/events/BattleEvents.cpp
		api/events/EventBusProxy.cpp
		api/events/GenericEvents.cpp
		api/events/SubscriptionRegistryProxy.cpp

		api/spells/Mechanics.cpp
		api/spells/Problem.cpp

		api/library/Artifact.cpp
		api/library/Creature.cpp
		api/library/Faction.cpp
		api/library/HeroClass.cpp
		api/library/HeroType.cpp
		api/library/Services.cpp
		api/library/Skill.cpp
		api/library/Spell.cpp

		api/BattleCb.cpp
		api/Enums.cpp
		api/GameCb.cpp
		api/Registry.cpp
		api/ServerCb.cpp
)

set(lualib_HDRS
		StdInc.h
		LuaCallWrapper.h
		LuaReference.h
		LuaModule.h
		LuaScriptInstance.h
		LuaContext.h
		LuaSpellEffect.h
		LuaUnitEffect.h
		LuaScriptPool.h
		LuaStack.h
		LuaWrapper.h

		api/adventure/HeroInstance.h
		api/adventure/StackInstance.h

		api/battle/UnitProxy.h
		api/battle/BattleHexArrayProxy.h
		api/battle/BattleHexProxy.h

		api/events/BattleEvents.h
		api/events/EventBusProxy.h
		api/events/GenericEvents.h
		api/events/SubscriptionRegistryProxy.h

		api/library/Artifact.h
		api/library/Creature.h
		api/library/Faction.h
		api/library/HeroClass.h
		api/library/HeroType.h
		api/library/Services.h
		api/library/Skill.h
		api/library/Spell.h

		api/spells/Mechanics.h
		api/spells/Problem.h

		api/BattleCb.h
		api/Enums.h
		api/GameCb.h
		api/Registry.h
		api/ServerCb.h
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

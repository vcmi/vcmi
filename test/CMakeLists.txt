include(GoogleTest)
include(CheckCXXCompilerFlag)

set(googleTest_Dir ${CMAKE_CURRENT_SOURCE_DIR}/googletest)
if(EXISTS ${googleTest_Dir})
	set(GTestSrc ${googleTest_Dir}/googletest)
	set(GMockSrc ${googleTest_Dir}/googlemock)
else()
	message(FATAL_ERROR "No googletest src dir found!")
endif()

set(test_SRCS
 		StdInc.cpp
 		main.cpp
 		CMemoryBufferTest.cpp
 		CVcmiTestConfig.cpp
 		JsonComparer.cpp

 		battle/BattleHexTest.cpp
 		battle/CBattleInfoCallbackTest.cpp
 		battle/CHealthTest.cpp
		battle/CUnitStateTest.cpp
		battle/CUnitStateMagicTest.cpp
		battle/battle_UnitTest.cpp

		entity/CArtifactTest.cpp
		entity/CCreatureTest.cpp
		entity/CFactionTest.cpp
		entity/CHeroClassTest.cpp
		entity/CHeroTest.cpp
		entity/CSkillTest.cpp

		erm/ERM_BM.cpp
		erm/ERM_BU.cpp
		erm/ERM_FU.cpp
		erm/ERM_GM_T.cpp
		erm/ERM_MA.cpp
		erm/ERM_MC.cpp
		erm/ERM_MF.cpp
		erm/ERM_TM_T.cpp
		erm/ERM_VR.cpp
		erm/ERM_UN.cpp
		erm/ERMPersistenceTest.cpp
		erm/ExamplesTest.cpp
		erm/interpretter/ERM_VR.cpp
		erm/interpretter/ERM_UN.cpp
		erm/interpretter/ErmRunner.cpp

		events/ApplyDamageTest.cpp
		events/EventBusTest.cpp

		game/CGameStateTest.cpp

		map/CMapEditManagerTest.cpp
		map/CMapFormatTest.cpp
		map/MapComparer.cpp

		netpacks/EntitiesChangedTest.cpp
		netpacks/NetPackFixture.cpp

		scripting/LuaSandboxTest.cpp
		scripting/LuaSpellEffectTest.cpp
		scripting/LuaSpellEffectAPITest.cpp
		scripting/PoolTest.cpp
		scripting/ScriptFixture.cpp

		spells/AbilityCasterTest.cpp
		spells/CSpellTest.cpp
 		spells/TargetConditionTest.cpp

		spells/effects/EffectFixture.cpp
 		spells/effects/CatapultTest.cpp
 		spells/effects/CloneTest.cpp
 		spells/effects/DamageTest.cpp
 		spells/effects/DispelTest.cpp
 		spells/effects/HealTest.cpp
 		spells/effects/SacrificeTest.cpp
 		spells/effects/SummonTest.cpp
 		spells/effects/TeleportTest.cpp
 		spells/effects/TimedTest.cpp

 		spells/targetConditions/AbsoluteSpellConditionTest.cpp
 		spells/targetConditions/AbsoluteLevelConditionTest.cpp
 		spells/targetConditions/BonusConditionTest.cpp
 		spells/targetConditions/CreatureConditionTest.cpp
 		spells/targetConditions/ElementalConditionTest.cpp
 		spells/targetConditions/HealthValueConditionTest.cpp
 		spells/targetConditions/ImmunityNegationConditionTest.cpp
 		spells/targetConditions/NormalLevelConditionTest.cpp
 		spells/targetConditions/NormalSpellConditionTest.cpp
 		spells/targetConditions/ReceptiveFeatureConditionTest.cpp
 		spells/targetConditions/SpellEffectConditionTest.cpp
 		spells/targetConditions/TargetConditionItemFixture.cpp
		
		mock/BattleFake.cpp
 		mock/mock_IGameCallback.cpp
 		mock/mock_MapService.cpp
 		mock/mock_BonusBearer.cpp
		mock/mock_CPSICallback.cpp
)

set(test_HEADERS
 		StdInc.h

 		CVcmiTestConfig.h
		JsonComparer.h

		scripting/ScriptFixture.h
		erm/interpretter/ErmRunner.h

 		map/MapComparer.h

 		netpacks/NetPackFixture.h

		spells/effects/EffectFixture.h

		spells/targetConditions/TargetConditionItemFixture.h

		vcai/ResourceManagerTest.h

		mock/BattleFake.h
		mock/mock_BonusBearer.h
 		mock/mock_IGameCallback.h
 		mock/mock_MapService.h
		mock/mock_BonusBearer.h

)

assign_source_group(${test_SRCS} ${test_HEADERS})

set(mock_HEADERS
		mock/mock_battle_IBattleState.h
		mock/mock_battle_Unit.h
		mock/mock_Creature.h
		mock/mock_CreatureService.h
		mock/mock_IBattleInfoCallback.h
		mock/mock_scripting_Context.h
		mock/mock_scripting_Script.h
		mock/mock_scripting_Service.h
		mock/mock_spells_Mechanics.h
		mock/mock_spells_Problem.h
		mock/mock_spells_Spell.h
		mock/mock_UnitEnvironment.h
		mock/mock_UnitInfo.h
		mock/mock_vstd_RNG.h
		mock/mock_CPSICallback.h
)

if(MSVC)
	set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib." FORCE)
endif()
check_cxx_compiler_flag(-Wimplicit-int-float-conversion CONV)
if(CONV)
	add_compile_options(-Wno-error=implicit-int-float-conversion) #Used in googletest
endif()
check_cxx_compiler_flag(-Wdeprecated-copy-with-user-provided-copy COPY)
if(COPY)
	add_compile_options(-Wno-deprecated-copy-with-user-provided-copy) #Used in googletest
endif()
add_subdirectory_with_folder("3rdparty" googletest EXCLUDE_FROM_ALL)

add_executable(vcmitest ${test_SRCS} ${test_HEADERS} ${mock_HEADERS})
target_link_libraries(vcmitest PRIVATE gtest gmock vcmi ${SYSTEM_LIBS})

target_include_directories(vcmitest
		PUBLIC	${CMAKE_CURRENT_SOURCE_DIR}
		PRIVATE	${GTestSrc}
		PRIVATE	${GTestSrc}/include
		PRIVATE	${GMockSrc}
		PRIVATE	${GMockSrc}/include
)

if(FALSE AND NOT ${CMAKE_VERSION} VERSION_LESS "3.10.0")
	# Running tests one by one using ctest not recommended due to vcmi having
	# slow global initialization.
	gtest_discover_tests(vcmitest
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
else()
	add_test(NAME tests
		COMMAND vcmitest
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
endif()


vcmi_set_output_dir(vcmitest "")

enable_pch(vcmitest)

file (GLOB_RECURSE testdata "testdata/*.*")
foreach(resource ${testdata})
	get_filename_component(filename ${resource} NAME)
	get_filename_component(dir ${resource} DIRECTORY)
	get_filename_component(dirname ${dir} NAME)
	set (output "")
	while(NOT ${dirname} STREQUAL testdata)
		get_filename_component(path_component ${dir} NAME)
		set (output "${path_component}/${output}")
		get_filename_component(dir ${dir} DIRECTORY)
		get_filename_component(dirname ${dir} NAME)
	endwhile()
	set(output "${CMAKE_BINARY_DIR}/bin/test/testdata/${output}/${filename}")
	configure_file(${resource} ${output} COPYONLY)
endforeach()

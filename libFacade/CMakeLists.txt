#######################################
#        vcmiFacade                   #
#######################################
#
# Aggregator library that bundles vcmiMain plus every AI implementation
# and the Lua scripting backend into a single shipped artifact called
# 'vcmi' (libvcmi.so / VCMI_lib.dll). All consumers link this target
# instead of the individual static libraries.
#
# Built as SHARED on desktop and STATIC on mobile (controlled by
# ENABLE_STATIC_LIBS).

set(facade_SRCS
	Facade.cpp
)

assign_source_group(${facade_SRCS})

if(ENABLE_STATIC_LIBS)
	add_library(vcmi STATIC ${facade_SRCS})
else()
	add_library(vcmi SHARED ${facade_SRCS})
endif()

# The constituent libraries are CMake OBJECT libraries: their object files
# are unconditionally bundled into the facade (no --whole-archive needed),
# and PRIVATE-linking them keeps CMake from re-propagating their objects
# onto consumer link lines (which would duplicate code).
target_link_libraries(vcmi PRIVATE vcmiMain)

if(NOT ENABLE_MINIMAL_LIB)
	target_link_libraries(vcmi PRIVATE vcmiLua)

	if(ENABLE_CLIENT)
		target_link_libraries(vcmi PRIVATE EmptyAI)

		if(ENABLE_NULLKILLER2_AI)
			target_link_libraries(vcmi PRIVATE Nullkiller2)
		endif()
		if(ENABLE_STUPID_AI)
			target_link_libraries(vcmi PRIVATE StupidAI)
		endif()
		if(ENABLE_BATTLE_AI)
			target_link_libraries(vcmi PRIVATE BattleAI)
		endif()
		if(ENABLE_MMAI)
			target_link_libraries(vcmi PRIVATE MMAI)
		endif()
	endif()
endif()

# Forward vcmiMain's public interface (include dirs, external runtime deps
# such as Boost/TBB/ZLIB/Iconv, and compile definitions like VCMI_LIB_NAMESPACE
# on mobile single-binary builds) to consumers of the facade. The OBJECT
# library itself is PRIVATE so its objects are absorbed only once, but
# downstream code still needs to see vcmi headers, link the same runtime
# libraries vcmiMain depends on, and observe the same lib-side macros so
# namespaced symbols resolve correctly at link time.
target_include_directories(vcmi PUBLIC
	$<TARGET_PROPERTY:vcmiMain,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(vcmi PUBLIC
	$<TARGET_PROPERTY:vcmiMain,INTERFACE_LINK_LIBRARIES>)
target_compile_definitions(vcmi PUBLIC
	$<TARGET_PROPERTY:vcmiMain,INTERFACE_COMPILE_DEFINITIONS>)

if(WIN32 AND NOT ENABLE_STATIC_LIBS)
	set(VCMI_FILE_DESCRIPTION "VCMI Core Library")
	set(VCMI_ORIGINAL_FILENAME "VCMI_lib.dll")
	set(VCMI_ICON_RESOURCE "")
	set(vcmi_ICON "${CMAKE_CURRENT_BINARY_DIR}/VCMI_lib_versioninfo.rc")
	configure_file("${CMAKE_SOURCE_DIR}/win/WindowsFileInfo.rc.in" "${vcmi_ICON}" @ONLY)
	target_sources(vcmi PRIVATE ${vcmi_ICON})
	set_target_properties(vcmi
		PROPERTIES
			OUTPUT_NAME "VCMI_lib"
			PROJECT_LABEL "VCMI_lib"
	)
endif()

vcmi_set_output_dir(vcmi "")

# Deploy game assets next to the build output for in-place execution.
if(COPY_CONFIG_ON_BUILD)
	add_custom_command(TARGET vcmi POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/config
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/Mods
		COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/scripts
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake_modules/create_link.cmake ${CMAKE_SOURCE_DIR}/config ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/config
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake_modules/create_link.cmake ${CMAKE_SOURCE_DIR}/Mods ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/Mods
		COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake_modules/create_link.cmake ${CMAKE_SOURCE_DIR}/scripts ${CMAKE_BINARY_DIR}/bin/${CMAKE_CFG_INTDIR}/scripts
	)
endif()

if(NOT ENABLE_STATIC_LIBS)
	install(TARGETS vcmi RUNTIME DESTINATION ${LIB_DIR} LIBRARY DESTINATION ${LIB_DIR})
endif()

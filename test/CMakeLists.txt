cmake_minimum_required(VERSION 2.6)

enable_testing()
include_directories(${CMAKE_HOME_DIRECTORY} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_HOME_DIRECTORY}/test)
include_directories(${Boost_INCLUDE_DIRS})

set(test_SRCS
		StdInc.cpp
		CVcmiTestConfig.cpp
		CMapEditManagerTest.cpp
                MapComparer.cpp
                CMapFormatTest.cpp
)

add_executable(vcmitest ${test_SRCS})
target_link_libraries(vcmitest vcmi ${Boost_LIBRARIES} ${RT_LIB} ${DL_LIB})
add_test(vcmitest vcmitest)

set_target_properties(vcmitest PROPERTIES ${PCH_PROPERTIES})
cotire(vcmitest)

# Files to copy to the build directory
add_custom_target(vcmitestFiles ALL)
set(vcmitest_FILES
		TerrainViewTest.h3m
		terrainViewMappings.json
)

foreach(file ${vcmitest_FILES})
		add_custom_command(TARGET vcmitestFiles POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/${file}" ${CMAKE_CURRENT_BINARY_DIR}
	)
endforeach()

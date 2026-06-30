set(mapconverter_SRCS
		StdInc.cpp
		EntryPoint.cpp
)

set(mapconverter_HEADERS
		StdInc.h
)

assign_source_group(${mapconverter_SRCS} ${mapconverter_HEADERS})
add_executable(vcmimapconverter ${mapconverter_SRCS} ${mapconverter_HEADERS})
if(USING_CONAN)
	vcmi_copy_conan_runtime_deps(vcmimapconverter)
endif()

target_link_libraries(vcmimapconverter PRIVATE vcmi Boost::program_options)

target_include_directories(vcmimapconverter
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
	set(VCMI_FILE_DESCRIPTION "VCMI Map Converter")
	set(VCMI_ORIGINAL_FILENAME "VCMI_mapconverter.exe")
	set(VCMI_ICON_RESOURCE "")
	set(mapconverter_VERSIONINFO_RC "${CMAKE_CURRENT_BINARY_DIR}/VCMI_mapconverter_versioninfo.rc")
	configure_file("${CMAKE_SOURCE_DIR}/win/WindowsFileInfo.rc.in" "${mapconverter_VERSIONINFO_RC}" @ONLY)
	target_sources(vcmimapconverter PRIVATE "${mapconverter_VERSIONINFO_RC}")
	set_target_properties(vcmimapconverter
		PROPERTIES
			OUTPUT_NAME "VCMI_mapconverter"
			PROJECT_LABEL "VCMI_mapconverter"
	)
endif()

vcmi_set_output_dir(vcmimapconverter "")
enable_pch(vcmimapconverter)

install(TARGETS vcmimapconverter DESTINATION ${BIN_DIR})

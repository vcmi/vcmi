set(vcmiqt_SRCS
		StdInc.cpp

		jsonutils.cpp
		launcherdirs.cpp
)

set(vcmiqt_HEADERS
		StdInc.h

		MessageBox.h
		jsonutils.h
		launcherdirs.h
		convpathqstring.h
		vcmiqt.h
)

assign_source_group(${vcmiqt_SRCS} ${vcmiqt_HEADERS})

if(ENABLE_STATIC_LIBS OR NOT (ENABLE_EDITOR AND ENABLE_LAUNCHER))
	add_library(vcmiqt STATIC ${vcmiqt_SRCS} ${vcmiqt_HEADERS})
	target_compile_definitions(vcmiqt PRIVATE VCMIQT_STATIC)
	set(TARGET_IS_SHARED OFF)
else()
	add_library(vcmiqt SHARED ${vcmiqt_SRCS} ${vcmiqt_HEADERS})
	target_compile_definitions(vcmiqt PRIVATE VCMIQT_SHARED)
	set(TARGET_IS_SHARED ON)
endif()

if(WIN32)
	set(VCMI_FILE_DESCRIPTION "VCMI Qt Library")
	set(VCMI_ORIGINAL_FILENAME "VCMI_vcmiqt.dll")
	set(VCMI_ICON_RESOURCE "")
	set(vcmiqt_ICON "${CMAKE_CURRENT_BINARY_DIR}/VCMI_vcmiqt_versioninfo.rc")
	configure_file("${CMAKE_SOURCE_DIR}/win/WindowsFileInfo.rc.in" "${vcmiqt_ICON}" @ONLY)
	target_sources(vcmiqt PRIVATE ${vcmiqt_ICON})
	set_target_properties(vcmiqt
		PROPERTIES
			OUTPUT_NAME "VCMI_vcmiqt"
			PROJECT_LABEL "VCMI_vcmiqt"
	)
endif()

target_link_libraries(vcmiqt vcmi Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

if(IOS AND TARGET_IS_SHARED)
	target_link_libraries(vcmiqt
		"-framework CoreFoundation"
		"-framework Foundation"
		"-framework UIKit"
		objc
	)
endif()

target_include_directories(vcmiqt PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

if(TARGET_IS_SHARED)
	install(TARGETS vcmiqt RUNTIME DESTINATION ${LIB_DIR} LIBRARY DESTINATION ${LIB_DIR})
endif()

vcmi_set_output_dir(vcmiqt "")
enable_pch(vcmiqt)

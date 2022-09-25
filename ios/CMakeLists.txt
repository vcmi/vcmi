add_library(iOS_utils SHARED
	iOS_utils.h
	iOS_utils.mm
)
target_link_libraries(iOS_utils PRIVATE
	"-framework Foundation"
)
target_include_directories(iOS_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

vcmi_set_output_dir(iOS_utils "")

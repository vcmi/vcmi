if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	message(FATAL_ERROR "ENABLE_FUZZING requires Clang or AppleClang.")
endif()

set(fuzz_common_sources
	FuzzEnvironment.cpp
)

add_library(vcmi-fuzz-common STATIC ${fuzz_common_sources})
target_link_libraries(vcmi-fuzz-common PUBLIC vcmi ${SYSTEM_LIBS})
target_include_directories(vcmi-fuzz-common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
vcmi_set_output_dir(vcmi-fuzz-common "")

function(vcmi_add_fuzzer target)
	add_executable(${target} ${ARGN})
	target_link_libraries(${target} PRIVATE vcmi-fuzz-common vcmi ${SYSTEM_LIBS})
	target_compile_options(${target} PRIVATE
		-fsanitize=fuzzer,address,undefined
		-fno-omit-frame-pointer
		-g
		-O1
	)
	target_link_options(${target} PRIVATE -fsanitize=fuzzer,address,undefined)
	target_compile_definitions(${target} PRIVATE FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION=1)
	vcmi_set_output_dir(${target} "")
endfunction()

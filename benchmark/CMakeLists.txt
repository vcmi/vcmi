set(benchmark_SRCS
	RmgBenchmarkMain.cpp
)

add_executable(vcmi-rmg-bench ${benchmark_SRCS})
target_link_libraries(vcmi-rmg-bench PRIVATE vcmi ${SYSTEM_LIBS})
target_include_directories(vcmi-rmg-bench PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

vcmi_set_output_dir(vcmi-rmg-bench "")
enable_pch(vcmi-rmg-bench)

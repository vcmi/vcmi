add_main_lib(vcmi SHARED)
if(BUILD_SINGLE_APP)
	target_compile_definitions(vcmi PUBLIC VCMI_LIB_NAMESPACE=LIB_CLIENT)
endif()

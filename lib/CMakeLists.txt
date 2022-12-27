add_main_lib(vcmi SHARED)
if(ENABLE_SINGLE_APP_BUILD)
	target_compile_definitions(vcmi PUBLIC VCMI_LIB_NAMESPACE=LIB_CLIENT)
endif()

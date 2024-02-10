if(ENABLE_STATIC_AI_LIBS)
	add_main_lib(${VCMI_LIB_TARGET} STATIC)
	target_compile_definitions(${VCMI_LIB_TARGET} PRIVATE STATIC_AI)
	target_link_libraries(${VCMI_LIB_TARGET} PRIVATE
		BattleAI
		EmptyAI
		StupidAI
		VCAI
	)
	if(ENABLE_NULLKILLER_AI)
		target_link_libraries(${VCMI_LIB_TARGET} PRIVATE Nullkiller)
	endif()
else()
	add_main_lib(${VCMI_LIB_TARGET} SHARED)
endif()

# no longer necessary, but might be useful to keep in future
# unfortunately at the moment tests do not support namespaced build, so enable only on some systems
if(APPLE_IOS OR ANDROID)
	target_compile_definitions(${VCMI_LIB_TARGET} PUBLIC VCMI_LIB_NAMESPACE=VCMI)
endif()


# We need to keep this code into separate directory so CMake will execute it after all other subdirectories install code
# Otherwise we can't fix win bundle dependencies since binaries wouldn't be there when this code executed
# This will likely only work for Vcpkg
if(WIN32)
	#there are some weird issues with variables used in path not evaluated properly when trying to remove code duplication from below lines
	
	if(ENABLE_LAUNCHER)
		install(CODE "			
			file(WRITE \"\${CMAKE_INSTALL_PREFIX}/qt.conf\"
				\"[Paths]\nPlugins = .\"
			)
		")
	endif()

	
	#TODO: check if some equivalent of block below can be used for above block (easy qt dependencies copy)
	#LuaJIT will not be copied automatically by not meeting criteria for this block of code
	if(ENABLE_LUA)
		install_vcpkg_imported_tgt(luajit::luajit)
	endif()
	
	if(MSVC)
		set(gp_tool "dumpbin")
	endif()

	install(CODE "
		set(dirs \"${CMAKE_PREFIX_PATH}\")
		if(\"\${CMAKE_INSTALL_CONFIG_NAME}\" STREQUAL \"Debug\")
			list(TRANSFORM dirs APPEND \"/debug/bin\")
		else()
			list(TRANSFORM dirs APPEND \"/bin\")
			list(FILTER dirs EXCLUDE REGEX \".*debug.*\")
		endif()
		
		set(BU_CHMOD_BUNDLE_ITEMS ON)
		set(gp_tool \"${gp_tool}\")
		
		include(BundleUtilities)

		fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/VCMI_Client.exe\" \"\" \"\${dirs}\")

	" COMPONENT Runtime)
endif(WIN32)

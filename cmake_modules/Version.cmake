set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})
include(GetGitRevisionDescription)
include(VCMIUtils)
include(VersionDefinition)

if(ENABLE_GITVERSION)
	get_git_head_revision(GIT_REFSPEC GIT_SHA1)
endif()

if(GIT_SHA1)
	set(VCMI_VERSION_FULL "${VCMI_VERSION_STRING}.${GIT_SHA1}")
else()
	set(VCMI_VERSION_FULL "${VCMI_VERSION_STRING}")
endif()
set(VCMI_PROJECT_NAME_VERSIONED "${VCMI_PROJECT_NAME} ${VCMI_VERSION_FULL}")

configure_file("${CMAKE_CURRENT_LIST_DIR}/../Version.cpp.in" "Version.cpp" @ONLY)
vcmi_print_git_commit_hash()

#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran

# Macros
PLATFORM=GNU-Linux-x86

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=build/Debug/${PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/../CVCMIServer.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/../../hch/CLodHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/../CGameHandler.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L../../../../boost/lib -Wl,-rpath ../../lib/vcmi_lib/dist/Debug/GNU-Linux-x86 -L../../lib/vcmi_lib/dist/Debug/GNU-Linux-x86 -lvcmi_lib

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Debug.mk dist/Debug/${PLATFORM}/vcmi_server

dist/Debug/${PLATFORM}/vcmi_server: ../../lib/vcmi_lib/dist/Debug/GNU-Linux-x86/libvcmi_lib.so

dist/Debug/${PLATFORM}/vcmi_server: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/${PLATFORM}
	${LINK.cc} -lboost_system-mt -lboost_thread-mt -lboost_filesystem-mt -lz -o dist/Debug/${PLATFORM}/vcmi_server ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/../CVCMIServer.o: ../CVCMIServer.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/..
	$(COMPILE.cc) -g -I../../../../boost/include/boost-1_37 -I../.. -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/../CVCMIServer.o ../CVCMIServer.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/../../hch/CLodHandler.o: ../../hch/CLodHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/../../hch
	$(COMPILE.cc) -g -I../../../../boost/include/boost-1_37 -I../.. -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/../../hch/CLodHandler.o ../../hch/CLodHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/../CGameHandler.o: ../CGameHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/..
	$(COMPILE.cc) -g -I../../../../boost/include/boost-1_37 -I../.. -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/server/vcmi_server/../CGameHandler.o ../CGameHandler.cpp

# Subprojects
.build-subprojects:
	cd ../../lib/vcmi_lib && ${MAKE}  -f Makefile-nb CONF=Debug

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/Debug
	${RM} dist/Debug/${PLATFORM}/vcmi_server

# Subprojects
.clean-subprojects:
	cd ../../lib/vcmi_lib && ${MAKE}  -f Makefile-nb CONF=Debug clean

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
FC=

# Macros
PLATFORM=GNU-Linux-x86

# Include project Makefile
include Makefile-nb

# Object Directory
OBJECTDIR=build/Debug/${PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CHeroHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CTownHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../Connection.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../VCMI_Lib.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CSpellHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CCreatureHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../CGameState.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CDefObjInfoHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../CConsoleHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CLodHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CObjectHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../CGameInfo.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CGeneralTextHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../map.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CArtHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CBuildingHandler.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-L../../../../boost/lib

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Debug.mk dist/Debug/${PLATFORM}/libvcmi_lib.so

dist/Debug/${PLATFORM}/libvcmi_lib.so: ${OBJECTFILES}
	${MKDIR} -p dist/Debug/${PLATFORM}
	${LINK.cc} -shared -o dist/Debug/${PLATFORM}/libvcmi_lib.so -fPIC ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CHeroHandler.o: ../../hch/CHeroHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CHeroHandler.o ../../hch/CHeroHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CTownHandler.o: ../../hch/CTownHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CTownHandler.o ../../hch/CTownHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../Connection.o: ../Connection.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/..
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../Connection.o ../Connection.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../VCMI_Lib.o: ../VCMI_Lib.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/..
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../VCMI_Lib.o ../VCMI_Lib.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CSpellHandler.o: ../../hch/CSpellHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CSpellHandler.o ../../hch/CSpellHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CCreatureHandler.o: ../../hch/CCreatureHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CCreatureHandler.o ../../hch/CCreatureHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../CGameState.o: ../../CGameState.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../..
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../CGameState.o ../../CGameState.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CDefObjInfoHandler.o: ../../hch/CDefObjInfoHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CDefObjInfoHandler.o ../../hch/CDefObjInfoHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../CConsoleHandler.o: ../../CConsoleHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../..
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../CConsoleHandler.o ../../CConsoleHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CLodHandler.o: ../../hch/CLodHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CLodHandler.o ../../hch/CLodHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CObjectHandler.o: ../../hch/CObjectHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CObjectHandler.o ../../hch/CObjectHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../CGameInfo.o: ../../CGameInfo.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../..
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../CGameInfo.o ../../CGameInfo.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CGeneralTextHandler.o: ../../hch/CGeneralTextHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CGeneralTextHandler.o ../../hch/CGeneralTextHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../map.o: ../../map.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../..
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../map.o ../../map.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CArtHandler.o: ../../hch/CArtHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CArtHandler.o ../../hch/CArtHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CBuildingHandler.o: ../../hch/CBuildingHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch
	${RM} $@.d
	$(COMPILE.cc) -g -I../../hch -I../.. -I../../../../boost/include/boost-1_37 -I/usr/include/SDL -I../../../../boost/include/boost-1_37/boost -fPIC  -MMD -MP -MF $@.d -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/lib/vcmi_lib/../../hch/CBuildingHandler.o ../../hch/CBuildingHandler.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Debug
	${RM} dist/Debug/${PLATFORM}/libvcmi_lib.so

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc

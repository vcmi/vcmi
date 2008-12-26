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
OBJECTDIR=build/Release/${PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CSpellHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CLuaHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CSpellWindow.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CCastleInterface.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CCursorHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CObjectHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CCreatureAnimation.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../Client.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CGameInfo.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CHeroWindow.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CMessage.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CPlayerInterface.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CAbilityHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CSndHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CBattleInterface.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../mapHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CPathfinder.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CGameInterface.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CPreGame.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CBitmapHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../Graphics.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CDefHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../AdventureMapButton.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../SDL_Extensions.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CConfigHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CAdvmapInterface.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CThreadHelper.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CMusicHandler.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../SDL_framerate.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CCallback.o \
	${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CMT.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	${MAKE}  -f nbproject/Makefile-Release.mk dist/Release/${PLATFORM}/vcmi_client

dist/Release/${PLATFORM}/vcmi_client: ${OBJECTFILES}
	${MKDIR} -p dist/Release/${PLATFORM}
	${LINK.cc} -o dist/Release/${PLATFORM}/vcmi_client ${OBJECTFILES} ${LDLIBSOPTIONS} 

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CSpellHandler.o: ../../hch/CSpellHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CSpellHandler.o ../../hch/CSpellHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CLuaHandler.o: ../../CLuaHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CLuaHandler.o ../../CLuaHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CSpellWindow.o: ../CSpellWindow.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CSpellWindow.o ../CSpellWindow.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CCastleInterface.o: ../../CCastleInterface.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CCastleInterface.o ../../CCastleInterface.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CCursorHandler.o: ../../CCursorHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CCursorHandler.o ../../CCursorHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CObjectHandler.o: ../../hch/CObjectHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CObjectHandler.o ../../hch/CObjectHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CCreatureAnimation.o: ../CCreatureAnimation.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CCreatureAnimation.o ../CCreatureAnimation.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../Client.o: ../Client.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../Client.o ../Client.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CGameInfo.o: ../../CGameInfo.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CGameInfo.o ../../CGameInfo.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CHeroWindow.o: ../../CHeroWindow.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CHeroWindow.o ../../CHeroWindow.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CMessage.o: ../../CMessage.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CMessage.o ../../CMessage.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CPlayerInterface.o: ../../CPlayerInterface.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CPlayerInterface.o ../../CPlayerInterface.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CAbilityHandler.o: ../../hch/CAbilityHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CAbilityHandler.o ../../hch/CAbilityHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CSndHandler.o: ../../hch/CSndHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CSndHandler.o ../../hch/CSndHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CBattleInterface.o: ../../CBattleInterface.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CBattleInterface.o ../../CBattleInterface.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../mapHandler.o: ../../mapHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../mapHandler.o ../../mapHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CPathfinder.o: ../../CPathfinder.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CPathfinder.o ../../CPathfinder.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CGameInterface.o: ../../CGameInterface.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CGameInterface.o ../../CGameInterface.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CPreGame.o: ../../CPreGame.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CPreGame.o ../../CPreGame.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CBitmapHandler.o: ../CBitmapHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CBitmapHandler.o ../CBitmapHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../Graphics.o: ../Graphics.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../Graphics.o ../Graphics.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CDefHandler.o: ../../hch/CDefHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CDefHandler.o ../../hch/CDefHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../AdventureMapButton.o: ../../AdventureMapButton.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../AdventureMapButton.o ../../AdventureMapButton.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../SDL_Extensions.o: ../../SDL_Extensions.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../SDL_Extensions.o ../../SDL_Extensions.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CConfigHandler.o: ../CConfigHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../CConfigHandler.o ../CConfigHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CAdvmapInterface.o: ../../CAdvmapInterface.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CAdvmapInterface.o ../../CAdvmapInterface.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CThreadHelper.o: ../../CThreadHelper.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CThreadHelper.o ../../CThreadHelper.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CMusicHandler.o: ../../hch/CMusicHandler.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../hch/CMusicHandler.o ../../hch/CMusicHandler.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../SDL_framerate.o: ../../SDL_framerate.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../SDL_framerate.o ../../SDL_framerate.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CCallback.o: ../../CCallback.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CCallback.o ../../CCallback.cpp

${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CMT.o: ../../CMT.cpp 
	${MKDIR} -p ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../..
	$(COMPILE.cc) -O2 -o ${OBJECTDIR}/_ext/home/t0/vcmi/trunk/client/vcmi_client/../../CMT.o ../../CMT.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	${RM} -r build/Release
	${RM} dist/Release/${PLATFORM}/vcmi_client

# Subprojects
.clean-subprojects:

BUILT_SOURCES = StdInc.h.gch
StdInc.h.gch: StdInc.h
	$(CXXCOMPILE) -DVCMI_DLL -fPIC -c $<

pkglib_LTLIBRARIES = libvcmi.la
libvcmi_la_LIBADD = @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_IOSTREAMS_LIB@ -lz -lpthread
libvcmi_la_CXXFLAGS = @SDL_CFLAGS@ -DVCMI_DLL
libvcmi_la_SOURCES = \
	../Global.h \
	Filesystem/CBinaryReader.cpp \
	Filesystem/CFileInfo.cpp \
	Filesystem/CFileInputStream.cpp \
	Filesystem/CFilesystemLoader.cpp \
	Filesystem/CLodArchiveLoader.cpp \
	Filesystem/CLodStream.cpp \
	Filesystem/CMemoryStream.cpp \
	Filesystem/CResourceLoader.cpp \
	Filesystem/ISimpleResourceLoader.cpp \
	BattleAction.cpp \
	BattleAction.h \
	BattleState.cpp \
	BattleState.h \
	CArtHandler.cpp \
	CArtHandler.h \
	CBuildingHandler.cpp \
	CBuildingHandler.h \
	CCampaignHandler.cpp \
	CCampaignHandler.h \
	CConsoleHandler.cpp \
	CConsoleHandler.h \
	CCreatureHandler.cpp \
	CCreatureHandler.h \
	CCreatureSet.cpp \
	CCreatureSet.h \
	CDefObjInfoHandler.cpp \
	CDefObjInfoHandler.h \
	CFileUtility.cpp \
	CFileUtility.h \
	CGameInterface.cpp \
	CGameInterface.h \
	CGameState.cpp \
	CGameState.h \
	CGeneralTextHandler.cpp \
	CGeneralTextHandler.h \
	CHeroHandler.cpp \
	CHeroHandler.h \
	CLodHandler.cpp \
	CLodHandler.h \
	CLogger.cpp \
	CLogger.h \
	CMapInfo.cpp \
	CMapInfo.h \
	CObjectHandler.cpp \
	CObjectHandler.h \
	CObstacleInstance.cpp \
	CObstacleInstance.h \
	CSpellHandler.cpp \
	CSpellHandler.h \
	CScriptingModule.h \
	CTownHandler.cpp \
	CTownHandler.h \
	CThreadHelper.cpp \
	CThreadHelper.h \
	CondSh.h \
	Connection.cpp \
	Connection.h \
	ConstTransitivePtr.h \
	HeroBonus.cpp \
	HeroBonus.h \
	IGameCallback.cpp \
	IGameCallback.h \
	IGameEventsReceiver.h \
	Interprocess.h \
	JsonNode.cpp \
	JsonNode.h \
	NetPacks.h \
	NetPacksLib.cpp \
	ResourceSet.cpp \
	ResourceSet.h \
	RegisterTypes.h \
	BattleHex.cpp \
	BattleHex.h \
	VCMIDirs.h \
	VCMI_Lib.cpp \
	VCMI_Lib.h \
	map.cpp \
	map.h

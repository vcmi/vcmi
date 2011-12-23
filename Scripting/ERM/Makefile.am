vcmiscriptingdir = $(VCMI_SCRIPTING_LIBS_DIR)
BUILT_SOURCES = StdInc.h.gch
StdInc.h.gch: StdInc.h
	$(CXXCOMPILE) -DVCMI_DLL -fPIC -c $<

vcmiscripting_LTLIBRARIES = libvcmiERM.la
libvcmiERM_la_LIBADD = @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_IOSTREAMS_LIB@
libvcmiERM_la_CXXFLAGS = -DVCMI_DLL
libvcmiERM_la_SOURCES = \
	ERMInterpreter.cpp \
	ERMInterpreter.h \
	ERMParser.cpp \
	ERMParser.h \
	ERMScriptModule.cpp \
	ERMScriptModule.h

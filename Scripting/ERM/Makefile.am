vcmiscriptingdir = $(VCMI_SCRIPTING_LIBS_DIR)

vcmiscripting_LTLIBRARIES = libvcmiERM.la
libvcmiERM_la_LIBADD = @BOOST_SYSTEM_LIB@ @BOOST_FILESYSTEM_LIB@ @BOOST_THREAD_LIB@ @BOOST_IOSTREAMS_LIB@
libvcmiERM_la_CXXFLAGS =
libvcmiERM_la_SOURCES = \
	ERMInterpreter.cpp \
	ERMInterpreter.h \
	ERMParser.cpp \
	ERMParser.h \
	ERMScriptModule.cpp \
	ERMScriptModule.h

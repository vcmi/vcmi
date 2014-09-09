/*
 Author: Juan Rada-Vilela, Ph.D.
 Copyright (C) 2010-2014 FuzzyLite Limited
 All rights reserved

 This file is part of fuzzylite.

 fuzzylite is free software: you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation, either version 3 of the License, or (at your option)
 any later version.

 fuzzylite is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with fuzzylite.  If not, see <http://www.gnu.org/licenses/>.

 fuzzylite™ is a trademark of FuzzyLite Limited.

 */

#include "fl/Exception.h"


#ifdef FL_BACKTRACE_OFF
//do nothing

#elif defined FL_UNIX
#include <execinfo.h>

#elif defined FL_WINDOWS
#include <windows.h>
#include <winbase.h>
#include <dbghelp.h>
#endif


#include <signal.h>
#include <stdlib.h>
#include <string.h>

namespace fl {

    Exception::Exception(const std::string& what)
    : std::exception(), _what(what) {
        FL_DBG(this->what());
    }

    Exception::Exception(const std::string& what, const std::string& file, int line,
            const std::string& function)
    : std::exception(), _what(what) {
        append(file, line, function);
        FL_DBG(this->what());
    }

    Exception::~Exception() FL_INOEXCEPT {
    }

    void Exception::setWhat(const std::string& what) {
        this->_what = what;
    }

    std::string Exception::getWhat() const {
        return this->_what;
    }

    const char* Exception::what() const FL_INOEXCEPT {
        return this->_what.c_str();
    }

    void Exception::append(const std::string& whatElse) {
        this->_what += whatElse + "\n";
    }

    void Exception::append(const std::string& file, int line, const std::string& function) {
        std::ostringstream ss;
        ss << "\n{at " << file << "::" << function << "() [line:" << line << "]}";
        _what += ss.str();
    }

    void Exception::append(const std::string& whatElse,
            const std::string& file, int line, const std::string& function) {
        append(whatElse);
        append(file, line, function);
    }

    std::string Exception::btCallStack() {
#ifdef FL_BACKTRACE_OFF
        return "[backtrace disabled] fuzzylite was built with option -DFL_BACKTRACE_OFF";
#elif defined FL_UNIX
        std::ostringstream btStream;
        const int bufferSize = 30;
        void* buffer[bufferSize];
        int backtraceSize = backtrace(buffer, bufferSize);
        char **btSymbols = backtrace_symbols(buffer, backtraceSize);
        if (btSymbols == fl::null) {
            btStream << "[backtrace error] no symbols could be retrieved";
        } else {
            if (backtraceSize == 0) btStream << "[backtrace is empty]";
            for (int i = 0; i < backtraceSize; ++i) {
                btStream << btSymbols[i] << "\n";
            }
        }
        free(btSymbols);
        return btStream.str();


#elif defined FL_WINDOWS
        std::ostringstream btStream;
        const int bufferSize = 30;
        void* buffer[bufferSize];
        SymInitialize(GetCurrentProcess(), fl::null, TRUE);

        int backtraceSize = CaptureStackBackTrace(0, bufferSize, buffer, fl::null);
        SYMBOL_INFO* btSymbol = (SYMBOL_INFO *) calloc(sizeof ( SYMBOL_INFO) + 256 * sizeof ( char), 1);
        if (not btSymbol) {
            btStream << "[backtrace error] no symbols could be retrieved";
        } else {
            btSymbol->MaxNameLen = 255;
            btSymbol->SizeOfStruct = sizeof ( SYMBOL_INFO);
            if (backtraceSize == 0) btStream << "[backtrace is empty]";
            for (int i = 0; i < backtraceSize; ++i) {
                SymFromAddr(GetCurrentProcess(), (DWORD64) (buffer[ i ]), 0, btSymbol);
                btStream << (backtraceSize - i - 1) << ": " <<
                        btSymbol->Name << " at 0x" << btSymbol->Address << "\n";
            }
        }
        free(btSymbol);
        return btStream.str();
#else
        return "[backtrace missing] supported only in Unix and Windows platforms";
#endif
    }
    //execinfo

    void Exception::signalHandler(int signal) {
        std::ostringstream ex;
        ex << "[unexpected signal " << signal << "] ";
#ifdef FL_UNIX
        ex << strsignal(signal);
#endif
        ex << "\nBACKTRACE:\n" << btCallStack();
        fl::Exception::catchException(fl::Exception(ex.str(), FL_AT));
        exit(EXIT_FAILURE);
    }

    void Exception::convertToException(int signal) {
        std::string signalDescription;
#ifdef FL_UNIX
        //Unblock the signal
        sigset_t empty;
        sigemptyset(&empty);
        sigaddset(&empty, signal);
        sigprocmask(SIG_UNBLOCK, &empty, fl::null);
        signalDescription = strsignal(signal);
#endif
        std::ostringstream ex;
        ex << "[signal " << signal << "] " << signalDescription << "\n";
        ex << "BACKTRACE:\n" << btCallStack();
        throw fl::Exception(ex.str(), FL_AT);
    }

    void Exception::terminate() {
        fl::Exception::catchException(fl::Exception("[unexpected exception] BACKTRACE:\n" + btCallStack(), FL_AT));
        exit(EXIT_FAILURE);
    }

    void Exception::catchException(const std::exception& exception) {
        std::ostringstream ss;
        ss << exception.what();
        std::string backtrace = btCallStack();
        if (not backtrace.empty()) {
            ss << "\n\nBACKTRACE:\n" << backtrace;
        }
        FL_LOG(ss.str());
    }

}

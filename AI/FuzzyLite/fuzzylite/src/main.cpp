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

#include "fl/Headers.h"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <signal.h>
#include <typeinfo>

using namespace fl;

/*
#ifdef FL_WINDOWS
#include <windows.h>
BOOL WINAPI flSignalHandler(DWORD dwCtrlType)
{
  FL_LOG("Signal: " << dwCtrlType);
  if (CTRL_C_EVENT == dwCtrlType){
  }
  //return FALSE;
  return TRUE;
}
#endif
 */

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;
    std::set_terminate(fl::Exception::terminate);
    std::set_unexpected(fl::Exception::terminate);
    signal(SIGSEGV, fl::Exception::signalHandler);
    signal(SIGABRT, fl::Exception::signalHandler);
    signal(SIGILL, fl::Exception::signalHandler);
    signal(SIGSEGV, fl::Exception::signalHandler);
    signal(SIGFPE, fl::Exception::signalHandler);
#ifdef FL_UNIX
    signal(SIGBUS, fl::Exception::signalHandler);
    signal(SIGPIPE, fl::Exception::signalHandler);
#endif
#ifdef FL_WINDOWS
    //SetConsoleCtrlHandler(flSignalHandler, TRUE);
#endif
    fuzzylite::setDebug(false);
    return Console::main(argc, argv);
}




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

#ifndef FL_EXCEPTION_H
#define FL_EXCEPTION_H

#include "fl/fuzzylite.h"

#include <exception>
#include <string>
#include <vector>

namespace fl {

    class FL_API Exception : public std::exception {
    protected:
        std::string _what;
    public:
        Exception(const std::string& what);
        Exception(const std::string& what, const std::string& file, int line,
                const std::string& function);
        virtual ~Exception() FL_INOEXCEPT FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(Exception)

        virtual void setWhat(const std::string& what);
        virtual std::string getWhat() const;
        virtual const char* what() const FL_INOEXCEPT FL_IOVERRIDE;

        virtual void append(const std::string& whatElse);
        virtual void append(const std::string& file, int line, const std::string& function);
        virtual void append(const std::string& whatElse,
                const std::string& file, int line, const std::string& function);

        static std::string btCallStack();

        static void signalHandler(int signal);
        static void convertToException(int signal);
        static void terminate();
        static void catchException(const std::exception& exception);


    };

}
#endif /* FL_EXCEPTION_H */

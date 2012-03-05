/*   Copyright 2010 Juan Rada-Vilela

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "FuzzyExceptions.h"

namespace fl {

    OutOfRangeException::OutOfRangeException(const std::string& file, int line,
            const std::string& function, const std::string& message)
    : FuzzyException(file, line, function, message) {

    }

    OutOfRangeException::OutOfRangeException(const std::string& file, int line,
            const std::string& function, const std::string& message,
            const FuzzyException& previous_exception)
    : FuzzyException(file, line, function, message, previous_exception) {

    }

    OutOfRangeException::~OutOfRangeException() throw () {

    }

    std::string OutOfRangeException::name() const throw () {
        return "Out Of Range";
    }

    void OutOfRangeException::CheckArray(const std::string& file, int line, const std::string& function,
            int index, int max) {
        if (index < 0 || index >= max) {
            std::stringstream ss;
            ss << "index=" << index << " and maximum value is " << max - 1;
            throw OutOfRangeException(file, line, function, ss.str());
        }
    }
}

namespace fl {

    InvalidArgumentException::InvalidArgumentException(const std::string& file,
            int line, const std::string& function, const std::string& message)
    : FuzzyException(file, line, function, message) {

    }

    InvalidArgumentException::InvalidArgumentException(const std::string& file,
            int line, const std::string& function, const std::string& message,
            const FuzzyException& previous_exception)
    : FuzzyException(file, line, function, message, previous_exception) {

    }

    InvalidArgumentException::~InvalidArgumentException() throw () {

    }

    std::string InvalidArgumentException::name() const throw () {
        return "Invalid Argument";
    }

}

namespace fl {

    NullPointerException::NullPointerException(const std::string& file, int line,
            const std::string& function, const std::string& message)
    : FuzzyException(file, line, function, message) {

    }

    NullPointerException::NullPointerException(const std::string& file, int line,
            const std::string& function, const std::string& message,
            const FuzzyException& previous_exception)
    : FuzzyException(file, line, function, message, previous_exception) {

    }

    NullPointerException::~NullPointerException() throw () {

    }

    std::string NullPointerException::name() const throw () {
        return "Null Pointer";
    }
}


namespace fl {

    ParsingException::ParsingException(const std::string& file, int line,
            const std::string& function, const std::string& message)
    : FuzzyException(file, line, function, message) {

    }

    ParsingException::ParsingException(const std::string& file, int line,
            const std::string& function, const std::string& message,
            const FuzzyException& previous_exception)
    : FuzzyException(file, line, function, message, previous_exception) {

    }

    ParsingException::~ParsingException() throw () {

    }

    std::string ParsingException::name() const throw () {
        return "Rule Error";
    }

}


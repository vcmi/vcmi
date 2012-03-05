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
#include <sstream>

#include "FuzzyException.h"
#include "defs.h"

namespace fl {

    FuzzyException::FuzzyException(const std::string& file, int line,
            const std::string& function) :
    exception(), _message(""), _previous_exception(NULL) {
        setLocation(file, line, function);
    }

    FuzzyException::FuzzyException(const std::string& file, int line,
            const std::string& function, const std::string& message) :
    exception(), _message(message), _previous_exception(NULL) {
        setLocation(file, line, function);
    }

    FuzzyException::FuzzyException(const std::string& file, int line,
            const std::string& function, const std::string& message,
            const FuzzyException& previous_exception) :
    exception(), _message(message), _previous_exception(&previous_exception) {
        setLocation(file, line, function);
    }

    FuzzyException::~FuzzyException() throw () {
    }

    std::string FuzzyException::name() const throw () {
        return "General Error";
    }

    std::string FuzzyException::message() const throw () {
        return this->_message;
    }

    void FuzzyException::setMessage(const std::string& message) throw () {
        this->_message = message;
    }

    std::string FuzzyException::location() const throw () {
        return this->_location;
    }

    void FuzzyException::setLocation(const std::string& file, int line,
            const std::string& function) throw () {
        std::stringstream ss;
        ss << file + "[" << line << "]::" + function + "()";
        this->_location = ss.str();
    }

    const FuzzyException* FuzzyException::previousException() const throw () {
        return this->_previous_exception;
    }

    void FuzzyException::setPreviousException(const FuzzyException& exception) throw () {
        this->_previous_exception = &exception;
    }

    const char* FuzzyException::what() const throw () {
        return toString().c_str();
    }

    std::string FuzzyException::toString() const throw () {
        return name() + ": " + message() + "\n\tIN " + location();
    }

    std::string FuzzyException::stackTrace() const throw () {
        std::string result;
        const FuzzyException* stack = this;
        do {
            result += stack->toString() + "\n";
            stack = stack->previousException();
        } while (stack);
        return result;
    }

    void FuzzyException::Assert(const std::string& file, int line,
            const std::string& function, bool assert, const std::string message) {
        if (!assert) {
            throw FuzzyException(file, line, function, message);
        }
    }

}

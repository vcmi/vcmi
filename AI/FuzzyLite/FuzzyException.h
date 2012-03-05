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
/* 
 * File:   FuzzyException.h
 * Author: jcrada
 *
 * Created on November 11, 2009, 7:15 AM
 */

#ifndef FL_FUZZYEXCEPTION_H
#define	FL_FUZZYEXCEPTION_H

#include <string>
#include <exception>

namespace fl {

    class FuzzyException : public std::exception {
    private:
        std::string _message;
        std::string _location;
        const FuzzyException* _previous_exception;
    public:
        FuzzyException(const std::string& file, int line, const std::string& function);
        FuzzyException(const std::string& file, int line, const std::string& function,
                const std::string& message);
        FuzzyException(const std::string& file, int line, const std::string& function,
                const std::string& message, const FuzzyException& previous_exception);
        virtual ~FuzzyException() throw ();

        virtual std::string name() const throw ();

        virtual void setMessage(const std::string& message) throw ();
        virtual std::string message() const throw ();

        virtual void setLocation(const std::string& file, int line,
                const std::string& function) throw ();
        virtual std::string location() const throw ();

        virtual void setPreviousException(const FuzzyException& exception) throw ();
        virtual const FuzzyException* previousException() const throw ();

        virtual const char* what() const throw ();
        virtual std::string toString() const throw ();
        virtual std::string stackTrace() const throw ();

        static void Assert(const std::string& file, int line,
                const std::string& function, bool assert, const std::string message = "");
    };

}

#endif	/* FL_FUZZYEXCEPTION_H */


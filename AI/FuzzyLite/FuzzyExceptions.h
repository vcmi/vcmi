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
 * File:   FuzzyExceptions.h
 * Author: jcrada
 *
 * Created on November 13, 2009, 3:34 PM
 */

#ifndef FL_FUZZYEXCEPTIONS_H
#define	FL_FUZZYEXCEPTIONS_H

#include "FuzzyException.h"
#include <sstream>
namespace fl {

    class OutOfRangeException : public FuzzyException {
    public:
        OutOfRangeException(const std::string& file, int line, const std::string& function,
                const std::string& message);
        OutOfRangeException(const std::string& file, int line, const std::string& function,
                const std::string& message, const FuzzyException& previous_exception);
        virtual ~OutOfRangeException() throw ();

        virtual std::string name() const throw ();

        static void CheckArray(const std::string& file, int line, const std::string& function,
                int index, int max); 
    };

    class InvalidArgumentException : public FuzzyException {
    public:
        InvalidArgumentException(const std::string& file, int line, const std::string& function,
                const std::string& message);
        InvalidArgumentException(const std::string& file, int line, const std::string& function,
                const std::string& message, const FuzzyException& previous_exception);
        virtual ~InvalidArgumentException() throw ();

        virtual std::string name() const throw ();
    };

    class NullPointerException : public FuzzyException {
    public:
        NullPointerException(const std::string& file, int line, const std::string& function,
                const std::string& message);
        NullPointerException(const std::string& file, int line, const std::string& function,
                const std::string& message, const FuzzyException& previous_exception);
        virtual ~NullPointerException() throw ();

        virtual std::string name() const throw ();
    };

    class ParsingException : public FuzzyException {
    public:
        ParsingException(const std::string& file, int line, const std::string& function,
                const std::string& message);
        ParsingException(const std::string& file, int line, const std::string& function,
                const std::string& message, const FuzzyException& previous_exception);
        virtual ~ParsingException() throw ();

        virtual std::string name() const throw ();
    };

}

#endif	/* FL_FUZZYEXCEPTIONS_H */


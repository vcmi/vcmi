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
 * File:   StrOp.h
 * Author: jcrada
 *
 * Created on November 5, 2009, 12:23 PM
 */

#ifndef FL_STROP_H
#define	FL_STROP_H

#include "defs.h"
#include "flScalar.h"
#include "FuzzyExceptions.h"

#include <string>

#include <vector>
namespace fl {

    class StrOp {
    protected:
        static bool AscendantSorter(const std::string &left, const std::string &right);
        static bool DescendantSorter(const std::string &left, const std::string &right);
    public:
        static void LeftTrim(std::string& text);
        static void RightTrim(std::string& text);
        static void Trim(std::string& text);

//        static std::string Substring(const std::string& text,
//                const std::string substring) throw (OutOfRangeException);

        static std::string IntToString(int value) ;

        static std::string ScalarToString(flScalar value, int precision = 3);

        static void Sort(std::vector<std::string>& vector, bool asc = true);

        static std::vector<std::string> Tokenize(const std::string& str, const std::string delimiter = " ");

        static std::vector<std::string> SplitByWord(const std::string& str, const std::string word);

        static int FindReplace(std::string& str, const std::string& find, const std::string& replace, bool all = true);
        
        static void main(int args, char** argv);
    };
}

#endif	/* FL_STROP_H */


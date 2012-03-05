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
#include "StrOp.h"

#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iosfwd>
namespace fl {

    bool StrOp::AscendantSorter(const std::string &left, const std::string &right) {
        for (std::string::const_iterator lit = left.begin(), rit = right.begin();
                lit != left.end() && rit != right.end();
                ++lit, ++rit) {
            if (tolower(*lit) < tolower(*rit))
                return true;
            else if (tolower(*lit) > tolower(*rit))
                return false;
        }
        if (left.size() < right.size()) {
            return true;
        }
        return false;
    }

    bool StrOp::DescendantSorter(const std::string &left, const std::string &right) {
        for (std::string::const_iterator lit = left.begin(), rit = right.begin();
                lit != left.end() && rit != right.end();
                ++lit, ++rit) {
            if (tolower(*lit) < tolower(*rit))
                return false;
            else if (tolower(*lit) > tolower(*rit))
                return true;
        }
        if (left.size() < right.size()) {
            return false;
        }
        return true;
    }

    void StrOp::LeftTrim(std::string& text) {
        text.erase(text.begin(), text.begin() + text.find_first_not_of(" "));
    }

    void StrOp::RightTrim(std::string& text) {
        text.erase(text.find_last_not_of(" ") + 1);
    }

    void StrOp::Trim(std::string& text) {
        RightTrim(text);
        LeftTrim(text);
    }

    //    std::string StrOp::Substring(const std::string& text,
    //            const std::string substring) throw (OutOfRangeException) {
    //        int index = text.find_last_of(substring) - substring.size();
    //        OutOfRangeException::CheckArray(FL_AT, index, text.size());
    //        return text.substr(index);
    //    }

    std::string StrOp::IntToString(int value) {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    std::string StrOp::ScalarToString(flScalar value, int precision) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(precision) << value;
        return ss.str();
    }

    void StrOp::Sort(std::vector<std::string>& vector, bool asc) {
        std::sort(vector.begin(), vector.end(), (asc ? AscendantSorter : DescendantSorter));
    }

    std::vector<std::string> StrOp::Tokenize(const std::string& str, const std::string delimiters) {
        std::vector<std::string> result;
        std::string::size_type last_pos = str.find_first_not_of(delimiters, 0);
        // Find first "non-delimiter".
        std::string::size_type pos = str.find_first_of(delimiters, last_pos);

        while (std::string::npos != pos || std::string::npos != last_pos) {
            // Found a token, add it to the vector.
            result.push_back(str.substr(last_pos, pos - last_pos));
            // Skip delimiters.  Note the "not_of"
            last_pos = str.find_first_not_of(delimiters, pos);
            // Find next "non-delimiter"
            pos = str.find_first_of(delimiters, last_pos);
        }
        return result;
    }

    std::vector<std::string> StrOp::SplitByWord(const std::string& str, const std::string word) {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string accum_token;
        std::string token;
        while (ss >> token) {
            if (token == word) {
                if (accum_token != "") {
                    result.push_back(accum_token);
                }
                accum_token = "";
            } else {
                accum_token += " " + token;
            }
        }
        if (accum_token != "") {
            result.push_back(accum_token);
        }
        return result;
    }

    int StrOp::FindReplace(std::string& str, const std::string& find,
            const std::string& replace, bool all) {
        if (find.length() == 0) return 0;
        int result = 0;
        size_t index = -abs((int)(find.length() - replace.length()));
        do {
            index = str.find(find, index + abs((int)(find.length() - replace.length())));
            if (index != std::string::npos) {
                str.replace(index, find.length(), replace);
                ++result;
            }
        } while (all && index != std::string::npos);
        return result;
    }

    void StrOp::main(int argc, char** argv) {
        std::string x = "Esto es= que y=o diga t=d=o= sin ='s =D";
        FL_LOG(x);
        FindReplace(x,"="," = ");
        FL_LOG(x);

    }
}

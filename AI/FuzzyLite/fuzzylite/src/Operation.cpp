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

#include "fl/Operation.h"

#include "fl/defuzzifier/Defuzzifier.h"
#include "fl/norm/Norm.h"
#include "fl/norm/SNorm.h"
#include "fl/norm/TNorm.h"

#include <algorithm>
#include <iomanip>
#include <cstdarg>
#include <cctype>

namespace fl {

    template <typename T>
    T Operation::min(T a, T b) {
        if (isNaN(a)) return b;
        if (isNaN(b)) return a;
        return a < b ? a : b;
    }
    template FL_API scalar Operation::min(scalar a, scalar b);
    template FL_API int Operation::min(int a, int b);

    template <typename T>
    T Operation::max(T a, T b) {
        if (isNaN(a)) return b;
        if (isNaN(b)) return a;
        return a > b ? a : b;
    }
    template FL_API scalar Operation::max(scalar a, scalar b);
    template FL_API int Operation::max(int a, int b);

    template <typename T>
    T Operation::bound(T x, T min, T max) {
        if (isGt(x, max)) return max;
        if (isLt(x, min)) return min;
        return x;
    }
    template FL_API scalar Operation::bound(scalar x, scalar min, scalar max);
    template FL_API int Operation::bound(int x, int min, int max);

    template <typename T>
    bool Operation::in(T x, T min, T max, bool geq, bool leq) {
        bool left = geq ? isGE(x, min) : isGt(x, min);
        bool right = leq ? isLE(x, max) : isLt(x, max);
        return (left and right);
    }
    template FL_API bool Operation::in(scalar x, scalar min, scalar max, bool geq, bool leq);
    template FL_API bool Operation::in(int x, int min, int max, bool geq, bool leq);

    template <typename T>
    bool Operation::isInf(T x) {
        return std::abs(x) == fl::inf;
    }
    template FL_API bool Operation::isInf(int x);
    template FL_API bool Operation::isInf(scalar x);

    template <typename T>
    bool Operation::isNaN(T x) {
        return not (x == x);
    }
    template FL_API bool Operation::isNaN(int x);
    template FL_API bool Operation::isNaN(scalar x);

    template<typename T>
    bool Operation::isFinite(T x) {
        return not (isNaN(x) or isInf(x));
    }
    template FL_API bool Operation::isFinite(int x);
    template FL_API bool Operation::isFinite(scalar x);

    bool Operation::isLt(scalar a, scalar b, scalar macheps) {
        return not isEq(a, b, macheps) and a < b;
    }

    bool Operation::isLE(scalar a, scalar b, scalar macheps) {
        return isEq(a, b, macheps) or a < b;
    }

    bool Operation::isEq(scalar a, scalar b, scalar macheps) {
        return a == b or std::fabs(a - b) < macheps or (isNaN(a) and isNaN(b));
    }

    bool Operation::isGt(scalar a, scalar b, scalar macheps) {
        return not isEq(a, b, macheps) and a > b;
    }

    bool Operation::isGE(scalar a, scalar b, scalar macheps) {
        return isEq(a, b, macheps) or a > b;
    }

    scalar Operation::scale(scalar x, scalar fromMin, scalar fromMax, scalar toMin, scalar toMax, bool bounded) {
        scalar result = (toMax - toMin) / (fromMax - fromMin) * (x - fromMin) + toMin;
        return bounded ? fl::Op::bound(result, toMin, toMax) : result;
    }

    scalar Operation::add(scalar a, scalar b) {
        return a + b;
    }

    scalar Operation::subtract(scalar a, scalar b) {
        return a - b;
    }

    scalar Operation::multiply(scalar a, scalar b) {
        return a * b;
    }

    scalar Operation::divide(scalar a, scalar b) {
        return a / b;
    }

    scalar Operation::modulo(scalar a, scalar b) {
        return fmod(a, b);
    }

    scalar Operation::logicalAnd(scalar a, scalar b) {
        return (isEq(a, 1.0) and isEq(b, 1.0)) ? 1.0 : 0.0;
    }

    scalar Operation::logicalOr(scalar a, scalar b) {
        return (isEq(a, 1.0) or isEq(b, 1.0)) ? 1.0 : 0.0;
    }

    scalar Operation::negate(scalar a) {
        return -a;
    }

    scalar Operation::logicalNot(scalar a) {
        return isEq(a, 1.0) ? 0.0 : 1.0;
    }

    scalar Operation::round(scalar x) {
        return (x > 0.0) ? std::floor(x + 0.5) : std::ceil(x - 0.5);
    }

    scalar Operation::gt(scalar a, scalar b) {
        return isGt(a, b);
    }

    scalar Operation::ge(scalar a, scalar b) {
        return isGE(a, b);
    }

    scalar Operation::eq(scalar a, scalar b) {
        return isEq(a, b);
    }

    scalar Operation::neq(scalar a, scalar b) {
        return not isEq(a, b);
    }

    scalar Operation::le(scalar a, scalar b) {
        return isLE(a, b);
    }

    scalar Operation::lt(scalar a, scalar b) {
        return isLt(a, b);
    }

    bool Operation::increment(std::vector<int>& x, std::vector<int>& min, std::vector<int>& max) {
        return increment(x, (int) x.size() - 1, min, max);
    }

    bool Operation::increment(std::vector<int>& x, int position, std::vector<int>& min, std::vector<int>& max) {
        if (x.empty() or position < 0) return true;

        bool overflow = false;
        if (x.at(position) < max.at(position)) {
            ++x.at(position);
        } else {
            overflow = (position == 0);
            x.at(position) = min.at(position);
            --position;
            if (position >= 0) {
                overflow = increment(x, position, min, max);
            }
        }
        return overflow;
    }

    double Operation::mean(const std::vector<scalar>& x) {
        if (x.size() == 0) return fl::nan;
        scalar sum = 0.0;
        for (std::size_t i = 0; i < x.size(); ++i) sum += x.at(i);
        return sum / x.size();
    }

    double Operation::standardDeviation(const std::vector<scalar>& x) {
        if (x.size() <= 1) return 0.0;
        return standardDeviation(x, mean(x));
    }

    double Operation::standardDeviation(const std::vector<scalar>& x, scalar mean) {
        if (x.size() <= 1) return 0.0;
        return std::sqrt(variance(x, mean));
    }

    double Operation::variance(const std::vector<scalar>& x) {
        if (x.size() <= 1) return 0.0;
        return variance(x, mean(x));
    }

    double Operation::variance(const std::vector<scalar>& x, scalar mean) {
        if (x.size() <= 1) return 0.0;
        scalar result = 0;
        for (std::size_t i = 0; i < x.size(); ++i) {
            result += (x.at(i) - mean) * (x.at(i) - mean);
        }
        result /= -1 + x.size();
        return result;
    }



    //Text Operations:

    std::string Operation::validName(const std::string& name) {
        if (trim(name).empty()) return "unnamed";
        std::ostringstream ss;
        for (std::size_t i = 0; i < name.length(); ++i) {
            char c = name[i];
            if (c == '_' or c == '.' or isalnum(c)) {
                ss << c;
            }
        }
        return ss.str();
    }

    int Operation::isValidForName(int character) {
        return character == '_' or character == '.' or isalnum(character);
    }

    std::string Operation::findReplace(const std::string& str, const std::string& find,
            const std::string& replace, bool replaceAll) {
        std::ostringstream result;
        std::size_t fromIndex = 0, nextIndex;
        do {
            nextIndex = str.find(find, fromIndex);
            result << str.substr(fromIndex, nextIndex - fromIndex);
            if (nextIndex != std::string::npos)
                result << replace;
            fromIndex = nextIndex + find.size();
        } while (replaceAll and nextIndex != std::string::npos);
        return result.str();
    }

    std::vector<std::string> Operation::split(const std::string& str,
            const std::string& delimiter, bool ignoreEmpty) {
        std::vector<std::string> result;
        if (str.empty() or delimiter.empty()) {
            result.push_back(str);
            return result;
        }
        std::string::const_iterator position = str.begin(), next = str.begin();
        while (next != str.end()) {
            next = std::search(position, str.end(), delimiter.begin(), delimiter.end());
            std::string token(position, next);
            if (not (token.empty() and ignoreEmpty)) {
                result.push_back(token);
            }
            if (next != str.end()) {
                position = next + delimiter.size();
            }
        }
        return result;
    }

    std::string Operation::trim(const std::string& text) {
        if (text.empty()) return text;
        if (not (std::isspace(text.at(0)) or std::isspace(text.at(text.size() - 1))))
            return text;
        int start = 0, end = text.size() - 1;
        while (start <= end and std::isspace(text.at(start))) {
            ++start;
        }
        while (end >= start and std::isspace(text.at(end))) {
            --end;
        }
        int length = end - start + 1;
        if (length <= 0) return "";
        return text.substr(start, length);
    }

    std::string Operation::format(const std::string& text, int matchesChar(int),
            const std::string& replacement) {
        std::ostringstream ss;
        std::string::const_iterator it = text.begin();
        while (it != text.end()) {
            if (matchesChar(*it)) {
                ss << *it;
            } else {
                ss << replacement;
            }
            ++it;
        }
        return ss.str();
    }

    scalar Operation::toScalar(const std::string& x) {
        std::istringstream iss(x);
        scalar result;
        iss >> result;
        char strict;
        if (not (iss.fail() or iss.get(strict))) return result;

        std::ostringstream nan, pInf, nInf;
        nan << fl::nan;
        pInf << fl::inf;
        nInf << (-fl::inf);

        if (x == nan.str() or x == "nan")
            return fl::nan;
        if (x == pInf.str() or x == "inf")
            return fl::inf;
        if (x == nInf.str() or x == "-inf")
            return -fl::inf;

        std::ostringstream ex;
        ex << "[conversion error] from <" << x << "> to scalar";
        throw fl::Exception(ex.str(), FL_AT);
    }

    scalar Operation::toScalar(const std::string& x, scalar alternative) FL_INOEXCEPT {
        std::istringstream iss(x);
        scalar result;
        iss >> result;
        char strict;
        if (not (iss.fail() or iss.get(strict))) return result;

        std::ostringstream nan, pInf, nInf;
        nan << fl::nan;
        pInf << fl::inf;
        nInf << (-fl::inf);

        if (x == nan.str() or x == "nan")
            return fl::nan;
        if (x == pInf.str() or x == "inf")
            return fl::inf;
        if (x == nInf.str() or x == "-inf")
            return -fl::inf;

        return alternative;
    }

    bool Operation::isNumeric(const std::string& x) {
        try {
            fl::Op::toScalar(x);
            return true;
        } catch (std::exception& ex) {
            (void) ex;
            return false;
        }
    }

    template <typename T>
    std::string Operation::str(T x, int decimals) {
        std::ostringstream ss;
        ss << std::setprecision(decimals) << std::fixed;
        if (fl::Op::isNaN(x)) {
            ss << "nan";
        } else if (fl::Op::isInf(x)) {
            if (fl::Op::isLt(x, 0.0)) ss << "-";
            ss << "inf";
        } else if (fl::Op::isEq(x, 0.0)) {
            ss << std::fabs((x * 0.0)); //believe it or not, -1.33227e-15 * 0.0 = -0.0
        } else ss << x;
        return ss.str();
    }
    template FL_API std::string Operation::str(int x, int precision);
    template FL_API std::string Operation::str(scalar x, int precision);

    template <> FL_API std::string Operation::str(const std::string& x, int precision) {
        (void) precision;
        return x;
    }

    template <typename T>
    std::string Operation::join(const std::vector<T>& x,
            const std::string& separator) {
        std::ostringstream ss;
        for (std::size_t i = 0; i < x.size(); ++i) {
            ss << str(x.at(i));
            if (i + 1 < x.size()) ss << separator;
        }
        return ss.str();
    }
    template FL_API std::string Operation::join(const std::vector<int>& x,
            const std::string& separator);
    template FL_API std::string Operation::join(const std::vector<scalar>& x,
            const std::string& separator);

    template <> FL_API
    std::string Operation::join(const std::vector<std::string>& x,
            const std::string& separator) {
        std::ostringstream ss;
        for (std::size_t i = 0; i < x.size(); ++i) {
            ss << x.at(i);
            if (i + 1 < x.size()) ss << separator;
        }
        return ss.str();
    }

    template <typename T>
    std::string Operation::join(int items, const std::string& separator, T first, ...) {
        std::ostringstream ss;
        ss << str(first);
        if (items > 1) ss << separator;
        va_list args;
        va_start(args, first);
        for (int i = 0; i < items - 1; ++i) {
            ss << str(va_arg(args, T));
            if (i + 1 < items - 1) ss << separator;
        }
        va_end(args);
        return ss.str();
    }

    template FL_API std::string Operation::join(int items, const std::string& separator,
            int first, ...);

    template <> FL_API std::string Operation::join(int items, const std::string& separator,
            double first, ...) {
        std::ostringstream ss;
        ss << str(first);
        if (items > 1) ss << separator;
        va_list args;
        va_start(args, first);
        for (int i = 0; i < items - 1; ++i) {
            ss << str(va_arg(args, double)); //automatic promotion
            if (i + 1 < items - 1) ss << separator;
        }
        va_end(args);
        return ss.str();
    }

    template <> FL_API
    std::string Operation::join(int items, const std::string& separator, const char* first, ...) {
        std::ostringstream ss;
        ss << first;
        if (items > 1) ss << separator;
        va_list args;
        va_start(args, first);
        for (int i = 0; i < items - 1; ++i) {
            ss << va_arg(args, const char*);
            if (i + 1 < items - 1) ss << separator;
        }
        va_end(args);
        return ss.str();
    }

}

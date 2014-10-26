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

#ifndef FL_OPERATION_H
#define FL_OPERATION_H

#include "fl/fuzzylite.h"

#include "fl/Exception.h"

#include <string>
#include <vector>

namespace fl {

    class FL_API Operation {
    public:

        template <typename T>
        static T min(T a, T b);

        template <typename T>
        static T max(T a, T b);

        template <typename T>
        static T bound(T x, T min, T max);

        template <typename T>
        static bool in(T x, T min, T max, bool geq = true, bool leq = true);

        template <typename T>
        static bool isInf(T x);

        template <typename T>
        static bool isNaN(T x);

        template <typename T>
        static bool isFinite(T x);

        //Is less than

        static bool isLt(scalar a, scalar b, scalar macheps = fl::fuzzylite::macheps());
        static bool isLE(scalar a, scalar b, scalar macheps = fl::fuzzylite::macheps());
        static bool isEq(scalar a, scalar b, scalar macheps = fl::fuzzylite::macheps());
        static bool isGt(scalar a, scalar b, scalar macheps = fl::fuzzylite::macheps());
        static bool isGE(scalar a, scalar b, scalar macheps = fl::fuzzylite::macheps());

        static scalar scale(scalar x, scalar fromMin, scalar fromMax,
                scalar toMin, scalar toMax, bool bounded = false);

        static scalar add(scalar a, scalar b);
        static scalar subtract(scalar a, scalar b);
        static scalar multiply(scalar a, scalar b);
        static scalar divide(scalar a, scalar b);
        static scalar modulo(scalar a, scalar b);
        static scalar logicalAnd(scalar a, scalar b);
        static scalar logicalOr(scalar a, scalar b);
        static scalar logicalNot(scalar a);
        static scalar negate(scalar a);
        static scalar round(scalar x);

        //greater than
        static scalar gt(scalar a, scalar b);
        //greater than or equal to
        static scalar ge(scalar a, scalar b);
        //equal to
        static scalar eq(scalar a, scalar b);
        //not equal to
        static scalar neq(scalar a, scalar b);
        //less than or equal to
        static scalar le(scalar a, scalar b);
        //less than
        static scalar lt(scalar a, scalar b);

        static bool increment(std::vector<int>& x, std::vector<int>& min, std::vector<int>& max);
        static bool increment(std::vector<int>& x, int position, std::vector<int>& min, std::vector<int>& max);

        static double mean(const std::vector<scalar>& x);
        static double variance(const std::vector<scalar>& x);
        static double variance(const std::vector<scalar>& x, scalar mean);
        static double standardDeviation(const std::vector<scalar>& x);
        static double standardDeviation(const std::vector<scalar>& x, scalar mean);

        static std::string validName(const std::string& name);

        static int isValidForName(int character);

        static std::string findReplace(const std::string& str, const std::string& find,
                const std::string& replace, bool replaceAll = true);

        static std::vector<std::string> split(const std::string& str,
                const std::string& delimiter = " ", bool ignoreEmpty = true);

        static std::string trim(const std::string& text);

        static std::string format(const std::string& text, int matchesChar(int),
                const std::string& replacement = "");

        //Intentionally results in a compiler error in C++11, or linker error in C++98
        //in order to avoid the deprecated usage of this method from version 4.0
        static scalar toScalar(const std::string& x, bool quiet,
                scalar alternative = fl::nan) FL_IDELETE;

        static scalar toScalar(const std::string& x); //throws fl::Exception

        static scalar toScalar(const std::string& x, scalar alternative) FL_INOEXCEPT;

        static bool isNumeric(const std::string& x);

        template <typename T>
        static std::string str(T x, int decimals = fuzzylite::decimals());

        template <typename T>
        static std::string join(const std::vector<T>& x, const std::string& separator);

        template <typename T>
        static std::string join(int items, const std::string& separator, T first, ...);
    };

    typedef Operation Op;
}
#endif  /* FL_OPERATION_H */


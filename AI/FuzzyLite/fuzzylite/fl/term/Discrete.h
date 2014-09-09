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

#ifndef FL_DISCRETE_H
#define FL_DISCRETE_H

#include "fl/term/Term.h"

#include <vector>
#include <utility>

namespace fl {

    class FL_API Discrete : public Term {
    public:
        typedef std::pair<scalar, scalar> Pair;
    protected:
        std::vector<Pair> _xy;
    public:
        Discrete(const std::string& name = "",
                const std::vector<Pair>& xy = std::vector<Pair>(),
                scalar height = 1.0);
        virtual ~Discrete() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(Discrete)

        virtual std::string className() const FL_IOVERRIDE;
        virtual std::string parameters() const FL_IOVERRIDE;
        virtual void configure(const std::string& parameters) FL_IOVERRIDE;

        //Warning: this method is unsafe. Make sure you use it correctly.
        template <typename T>
        static Discrete* create(const std::string& name, int argc,
                T x1, T y1, ...); // throw (fl::Exception);

        virtual scalar membership(scalar x) const FL_IOVERRIDE;

        virtual void setXY(const std::vector<Pair>& pairs);
        virtual const std::vector<Pair>& xy() const;
        virtual std::vector<Pair>& xy();
        virtual const Pair& xy(int index) const;
        virtual Pair& xy(int index);


        static std::vector<scalar> toVector(const std::vector<Pair>& xy);
        static std::vector<Pair> toPairs(const std::vector<scalar>& xy);
        static std::vector<Pair> toPairs(const std::vector<scalar>& xy,
                scalar missingValue) FL_INOEXCEPT;

        static std::string formatXY(const std::vector<Pair>& xy,
                const std::string& prefix = "(", const std::string& innerSeparator = ",",
                const std::string& postfix = ")", const std::string& outerSeparator = " ");

        virtual Discrete* clone() const FL_IOVERRIDE;

        static Term* constructor();

    };

}
#endif /* FL_DISCRETE_H */
